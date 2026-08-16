# QWen3.5-VL

本文讲解`QWen3.5-VL`模型架构，基于`llama.cpp`实现。

## 文本解码器

`QWen3.5`是原生多模态大模型，文本解码器统一处理文本和图像特征.

### 整体架构

```text
build_inp_embd(model.tok_embd)
  将输入 token 查表转换为 embedding
    |
    v
build_inp_mem_hybrid()
  创建混合缓存接口，同时管理：
  - get_recr(): DeltaNet recurrent state
  - get_attn(): Full Attention KV cache
    |
    +-------------------------------+
    |                               |
build_inp_pos()               build_inp_out_ids()
构造 MRoPE 位置索引          指定需要输出的 token 行
    |                               |
    +---------------+---------------+
                    |
                    v
            循环处理每个主干层
                    |
                    v
build_norm(...attn_norm...)
  对层输入执行 Attention 前 RMSNorm
                    |
                    v
hparams.is_recr(il)
  判断当前层使用哪种注意力机制
        /                           \
       / true                       \ false
      v                              v
build_layer_attn_linear()      build_layer_attn()
执行 Gated DeltaNet            执行 Full Attention
使用卷积状态和矩阵状态          使用 Q/K/V、MRoPE 和 KV cache
      \                              /
       +--------------+-------------+
                      |
                      v
ggml_get_rows() [可选]
  最后一层提前选取目标 token，减少后续计算
                      |
                      v
ggml_add(cur, inpSA)
  Attention 残差连接：
  attn_out = input + attention_output
                      |
                      v
build_norm(...attn_post_norm...)
  对 Attention 残差结果执行 FFN 前 RMSNorm
                      |
                      v
build_layer_ffn()
  执行 Dense SwiGLU FFN：
  down(SiLU(gate(x)) * up(x))
                      |
                      v
ggml_add(cur, ffn_residual)
  FFN 残差连接：
  layer_out = attn_out + ffn_output
                      |
                      v
build_cvec()
  应用可选的 control vector
                      |
                      v
inpL = cur
  将本层输出作为下一层输入
                      |
               重复直到最后一层
                      |
                      v
build_norm(...output_norm...)
  对主干最终 hidden state 执行 RMSNorm
                      |
          +-----------+-----------+
          |                       |
          v                       v
res->t_h_nextn = cur       ggml_get_rows() [可选]
保存给 MTP/NextN 使用       只保留需要生成 logits 的 token
                                  |
                                  v
                         res->t_embd = cur
                         保存最终输出 embedding
                                  |
                                  v
build_lora_mm(model.output, cur)
  执行支持 LoRA 的 LM Head：
  hidden state -> vocabulary logits
                                  |
                                  v
                         res->t_logits = cur
                         保存最终词表 logits
                                  |
                                  v
ggml_build_forward_expand(gf, cur)
  从 logits 开始展开依赖，加入 GGML 执行图
```

`Qwen3.5 Dense`架构的核心特点是`混合注意力 + 门控 + 稠密 FFN`：

* 混合注意力架构：每`4`层为一组，其中`3`层使用`Gated DeltaNet`，`1`层使用`Gated Full Attention`。
  * `Gated DeltaNet`层通过因果卷积提取局部信息，并用固定大小的`recurrent state`压缩历史信息；状态大小不随上下文长度增长，适合处理长序列。
  * `Gated Full Attention`层通过`KV Cache`保留历史`Key/Value`，允许当前`Query`直接访问任意历史`Token`，用于补偿压缩状态造成的信息损失。
  * 因此，只有约四分之一的层需要维护随上下文增长的`KV Cache`，其余层只维护卷积状态和矩阵状态。
* 多重门控机制：
  * `Gated DeltaNet`使用衰减门$\alpha$控制旧状态的保留程度，使用更新门$\beta$控制当前`Key/Value`的写入强度。
  * `DeltaNet`输出经过`RMSNorm(output) * SiLU(z)`门控，选择需要传递到输出投影的通道。
  * `Full Attention`将注意力结果乘以$\operatorname{sigmoid}(g)$后再执行输出投影，使每个注意力头能够动态控制信息输出。
* 稠密`FFN`：每层都使用标准`SwiGLU`前馈网络：

  $$
  \operatorname{FFN}(x)
  =
  W_{\mathrm{down}}
  \left(
  \operatorname{SiLU}(W_{\mathrm{gate}}x)
  \odot
  W_{\mathrm{up}}x
  \right)
  $$

  每个`Token`都会激活全部`FFN`参数，不使用路由器或稀疏专家，执行路径比`MoE`更直接。

#### build_inp_embd

##### 源码

```CPP
// input embeddings with optional lora
ggml_tensor * llm_graph_context::build_inp_embd(ggml_tensor * tok_embd) const {
    const int64_t n_embd_inp = hparams.n_embd_inp();
    const int64_t n_embd     = hparams.n_embd;

    assert(n_embd_inp >= n_embd);

    auto inp = std::make_unique<llm_graph_input_embd>(n_embd_inp);

    inp->tokens = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, ubatch.n_tokens);
    cb(inp->tokens, "inp_tokens", -1);
    ggml_set_input(inp->tokens);
    res->t_inp_tokens = inp->tokens;

    inp->embd = ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, n_embd_inp, ubatch.n_tokens);
    cb(inp->embd, "inp_embd", -1);
    ggml_set_input(inp->embd);

    // select one of the 2 inputs, based on the batch contents
    // ref: https://github.com/ggml-org/llama.cpp/pull/18550
    std::array<ggml_tensor *, 2> inps;

    // token embeddings path (ubatch.token != nullptr)
    {
        auto & cur = inps[0];

        cur = ggml_get_rows(ctx0, tok_embd, inp->tokens);

        // apply lora for embedding tokens if needed
        for (const auto & lora : *loras) {
            llama_adapter_lora_weight * lw = lora.first->get_weight(tok_embd);
            if (lw == nullptr) {
                continue;
            }

            const float adapter_scale = lora.second;
            const float scale = lw->get_scale(lora.first->alpha, adapter_scale);

            ggml_tensor * inpL_delta = ggml_scale(ctx0, ggml_mul_mat(
                        ctx0, lw->b, // non-transposed lora_b
                        ggml_get_rows(ctx0, lw->a, inp->tokens)
                        ), scale);

            cur = ggml_add(ctx0, cur, inpL_delta);
        }

        if (n_embd_inp != n_embd) {
            cur = ggml_pad(ctx0, cur, hparams.n_embd_inp() - n_embd, 0, 0, 0);
        }
    }

    // vector embeddings path (ubatch.embd != nullptr)
    {
        auto & cur = inps[1];

        cur = inp->embd;
    }

    assert(ggml_are_same_shape (inps[0], inps[1]));
    assert(ggml_are_same_stride(inps[0], inps[1]));

    ggml_tensor * cur = ggml_build_forward_select(gf, inps.data(), inps.size(), ubatch.token ? 0 : 1);

    if (n_embd_inp != n_embd) {
        cur = ggml_view_2d(ctx0, cur, n_embd, n_tokens, cur->nb[1], 0);
    }

    res->t_inp_embd = cur;

    // For Granite architecture
    // NOTE: For deepstack models, only apply scale to token inputs (ie text-only input).
    //  Raw embeddings are assumed to be multimodal inputs that should not be scaled.
    if (hparams.f_embedding_scale != 0.0f && (ubatch.token || hparams.n_deepstack_layers == 0)) {
        if (!ggml_is_contiguous(cur)) {
            cur = ggml_cont(ctx0, cur);
        }
        cur = ggml_scale(ctx0, cur, hparams.f_embedding_scale);
    }

    cb(cur, "embd", -1);

    res->add_input(std::move(inp));

    // make sure the produced embeddings are immediately materialized in the ggml graph
    // ref: https://github.com/ggml-org/llama.cpp/pull/18599
    ggml_build_forward_expand(gf, cur);

    return cur;
}
```

##### 解释

`build_inp_embd`将文本输入或者经过视觉编码器输出的输入转换为模型需要的`embedding`并处理`LoRA`、`DeepStack`和`Granite embedding scale`.

```CPP
const int64_t n_embd_inp = hparams.n_embd_inp();
const int64_t n_embd     = hparams.n_embd;
```

模型参数，`n_embd`表示模型的隐藏维度，`n_embd_inp`表示输入`token`所携带的完整`embedding`维度.

* 对于普通模型，`n_embd_inp = n_embd`.
* 对于DeepStack多模态模型，`n_embd_inp = n_embd * (1 + n_deepstack_layers)`，例如
  ```txt
  [主干 embedding][deepstack 0][deepstack 1][deepstack 2]
     4096            4096          4096          4096
  ```

`DeepStack`多模态模型指的是，多模态的输入，不只是传递给模型的输入端，还注入到了特定`transformer`层中.这只是打包，语言模型主干的隐藏维度仍然是`n_embd`.

```CPP
auto inp = std::make_unique<llm_graph_input_embd>(n_embd_inp);

inp->tokens = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, ubatch.n_tokens);
cb(inp->tokens, "inp_tokens", -1);
ggml_set_input(inp->tokens);
res->t_inp_tokens = inp->tokens;

inp->embd = ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, n_embd_inp, ubatch.n_tokens);
cb(inp->embd, "inp_embd", -1);
ggml_set_input(inp->embd);
```

这段代码创建了两套输入节点,张量的形状是

* `tokens`: `[n_tokens]`,保存着`token id`.
* `embd`: `[n_embd_inp, n_tokens]`,保存外部输入的`embedding`，通常是视觉编码器.

这里两套输入都会创建，但运行时只会填充使用的一套。`llm_graph_input_embd::set_input()`会根据 ubatch->token 或 `ubatch->embd`写入对应张量。

```CPP
// token embeddings path (ubatch.token != nullptr)
{
    auto & cur = inps[0];

    cur = ggml_get_rows(ctx0, tok_embd, inp->tokens);

    // apply lora for embedding tokens if needed
    for (const auto & lora : *loras) {
        llama_adapter_lora_weight * lw = lora.first->get_weight(tok_embd);
        if (lw == nullptr) {
            continue;
        }

        const float adapter_scale = lora.second;
        const float scale = lw->get_scale(lora.first->alpha, adapter_scale);

        ggml_tensor * inpL_delta = ggml_scale(ctx0, ggml_mul_mat(
                    ctx0, lw->b, // non-transposed lora_b
                    ggml_get_rows(ctx0, lw->a, inp->tokens)
                    ), scale);

        cur = ggml_add(ctx0, cur, inpL_delta);
    }

    if (n_embd_inp != n_embd) {
        cur = ggml_pad(ctx0, cur, hparams.n_embd_inp() - n_embd, 0, 0, 0);
    }
}
```

`tok_embd`词嵌入表，是一个训练得到的二维矩阵，每个`token id`对应一个长度为`n_embd`的向量.

从`tok_embd`词嵌入权重表中取出`token id`对应的行,如果有`lora`那么就加入进去。

进行`padding`,补齐空格到`n_embd_inp`长度.

```CPP
// vector embeddings path (ubatch.embd != nullptr)
{
    auto & cur = inps[1];

    cur = inp->embd;
}
```

直接输入的`embding`不需要词嵌入.

```CPP
ggml_tensor * cur = ggml_build_forward_select(gf, inps.data(), inps.size(), ubatch.token ? 0 : 1);

if (n_embd_inp != n_embd) {
    cur = ggml_view_2d(ctx0, cur, n_embd, n_tokens, cur->nb[1], 0);
}

res->t_inp_embd = cur;
```

选择是使用`token embdding`还是`vector embding`,按照`ubatch.token`是否有直接token输入作为选择.

如果`n_embd_inp`和`n_embd`不一致，那就需要将输入向量重新`view`一下，隐藏超过`n_embd`的部分.

```CPP
if (hparams.f_embedding_scale != 0.0f && (ubatch.token || hparams.n_deepstack_layers == 0)) {
    if (!ggml_is_contiguous(cur)) {
        cur = ggml_cont(ctx0, cur);
    }
    cur = ggml_scale(ctx0, cur, hparams.f_embedding_scale);
}

cb(cur, "embd", -1);

res->add_input(std::move(inp));

// make sure the produced embeddings are immediately materialized in the ggml graph
// ref: https://github.com/ggml-org/llama.cpp/pull/18599
ggml_build_forward_expand(gf, cur);
```

`f_embedding_scale`是模型参数，表示`embedding`缩放系数.

进行缩放并最终设置到输入，同时构建现有的计算图.

#### build_inp_mem_hybrid

##### 源码

```CPP
llm_graph_input_mem_hybrid * llm_graph_context::build_inp_mem_hybrid() const {
    const auto * mctx_cur = static_cast<const llama_memory_hybrid_context *>(mctx);

    auto inp_rs   = build_rs_inp_impl     (ctx0, ubatch, mctx_cur->get_recr());
    auto inp_attn = build_attn_inp_kv_impl(ctx0, ubatch, hparams, cparams, mctx_cur->get_attn());

    auto inp = std::make_unique<llm_graph_input_mem_hybrid>(cparams, std::move(inp_attn), std::move(inp_rs), mctx_cur);

    return (llm_graph_input_mem_hybrid *) res->add_input(std::move(inp));
}
```

##### 解释

`build_inp_mem_hybrid`这个函数为混合内存模型同时准备了两套推理状态输入,`Attention`层使用的`KV Cache`输入,`Recurrent/SSM/`线性注意力层使用的循环状态输入。

```CPP
const auto * mctx_cur = static_cast<const llama_memory_hybrid_context *>(mctx);
```

取混合内存上下文,`mctx`是通用的`memory context`指针。需要`down_cast`为混合内存上下文.

```CPP
static std::unique_ptr<llm_graph_input_rs> build_rs_inp_impl(
           ggml_context * ctx0,
     const llama_ubatch & ubatch,
    const llama_memory_recurrent_context * mctx_cur) {

    auto inp = std::make_unique<llm_graph_input_rs>(mctx_cur);

    const int64_t n_rs   = mctx_cur->get_n_rs();
    const int64_t n_seqs = ubatch.n_seqs;

    inp->s_copy = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, n_rs);
    ggml_set_input(inp->s_copy);

    inp->s_copy_main  = ggml_view_1d(ctx0, inp->s_copy, n_seqs, 0);
    inp->s_copy_extra = ggml_view_1d(ctx0, inp->s_copy, n_rs - n_seqs, n_seqs * inp->s_copy->nb[0]);

    inp->head = mctx_cur->get_head();
    inp->rs_z = mctx_cur->get_rs_z();

    return inp;
}
```

创建循环状态输入,目前只是创建了槽位，没有实际分配缓存空间.

`n_seqs`是ubatch中本轮要推进和更新的序列状态数，可以理解为`batch_size`.每个单独的序列需要维护自己的`recurrent-state`缓存。

例如：

```txt
2 个序列，每个序列处理 4 个 token
n_seqs       = 2  <- 类似 batch size
n_seq_tokens = 4  <- 每个序列的长度
n_tokens     = 8  <- 总 token 数
```

`n_rs`是本轮图计算需要处理的`recurrent-state`槽位数量，通常`n_rs >= n_seqs`

* `s_copy`描述循环状态槽位索引。
* `s_copy_main`：当前序列对应的状态复制映射。
* `s_copy_extra`:其余状态槽位的映射。
* `head`:当前`recurrent state`的写入位置。
* `rs_z`:槽位会先被清零，作为零状态模板

假设`recurrent cache`一共有8个状态槽位，本轮同时处理两个序列:

```txt
cache 槽位：0 1 2 3 4 5 6 7

n_seqs = 2
head    = 2
n_rs    = 4
rs_z    = 4
```

本轮要整理/使用的目标范围是：

```txt
head 到 head + n_rs - 1
即槽位 [2, 3, 4, 5]
```

整体概括为

```txt
s_copy       = [6, 4, 7,  3 ]
                |  |  └extra┘
                └main┘

main  -> 当前序列读取旧状态并继续计算
extra -> 不参与计算，只在缓存整理时搬运
head  -> 整理后的目标区域起点
rs_z  -> 临时提供零状态的缓存槽位
```

```CPP
static std::unique_ptr<llm_graph_input_attn_kv> build_attn_inp_kv_impl(
           ggml_context * ctx0,
     const llama_ubatch & ubatch,
    const llama_hparams & hparams,
    const llama_cparams & cparams,
    const llama_kv_cache_context * mctx_cur) {

    auto inp = std::make_unique<llm_graph_input_attn_kv>(hparams, cparams, mctx_cur);

    {
        GGML_ASSERT(hparams.swa_type == LLAMA_SWA_TYPE_NONE && "Use llama_kv_cache_iswa for SWA");

        inp->self_k_idxs = mctx_cur->build_input_k_idxs(ctx0, ubatch);
        inp->self_v_idxs = mctx_cur->build_input_v_idxs(ctx0, ubatch);

        inp->self_kq_mask = build_attn_inp_kq_mask(ctx0, mctx_cur, ubatch, cparams);
        inp->self_kq_mask_cnv = inp->self_kq_mask;
    }

    inp->self_k_rot = mctx_cur->build_input_k_rot(ctx0);
    inp->self_v_rot = mctx_cur->build_input_v_rot(ctx0);

    return inp;
}
```

为标准自注意力创建`KV Cache`相关的“计算图输入描述”。它不创建`K/V`内容，而是创建：

* 新`K/V`应写到哪些缓存槽位。
* 当前`Query`能看哪些缓存位置。
* 量化`KV Cache`是否需要`Hadamard`旋转。

```CPP
ggml_tensor * llama_kv_cache::build_input_k_idxs(ggml_context * ctx, const llama_ubatch & ubatch) const {
    const uint32_t n_tokens = ubatch.n_tokens;

    ggml_tensor * k_idxs = ggml_new_tensor_1d(ctx, GGML_TYPE_I64, n_tokens);

    ggml_set_input(k_idxs);

    return k_idxs;
}

ggml_tensor * llama_kv_cache::build_input_v_idxs(ggml_context * ctx, const llama_ubatch & ubatch) const {
    const uint32_t n_tokens = ubatch.n_tokens;

    ggml_tensor * v_idxs;

    if (!v_trans) {
        v_idxs = ggml_new_tensor_1d(ctx, GGML_TYPE_I64, n_tokens);
    } else {
        v_idxs = ggml_new_tensor_1d(ctx, GGML_TYPE_I64, n_tokens*hparams.n_embd_v_gqa_max());
    }

    ggml_set_input(v_idxs);

    return v_idxs;
}
```

创建`k_idxs`和`v_idxs`，当前`token`算出的`K`和`V`应写入哪些缓存索引.

```CPP
static ggml_tensor * build_attn_inp_kq_mask(
        ggml_context * ctx,
        const llama_kv_cache_context * mctx,
        const llama_ubatch & ubatch,
        const llama_cparams & cparams) {
    const auto n_kv     = mctx->get_n_kv();
    const auto n_tokens = ubatch.n_tokens;
    const auto n_stream = cparams.kv_unified ? 1 : ubatch.n_seqs_unq;

    // flash attention requires an f16 mask
    const auto type = cparams.flash_attn ? GGML_TYPE_F16 : GGML_TYPE_F32;

    ggml_tensor * res = ggml_new_tensor_4d(ctx, type, n_kv, n_tokens/n_stream, 1, n_stream);
    ggml_set_input(res);
    ggml_set_name(res, "attn_inp_kq_mask");

    return res;
}
```

创建`attention mask`，描述每个`Query`可以关注哪些`KV Cache`位置.

例如：

```txt
         K0 K1 K2 K3 K4 K5
Query 5   Y  Y  Y  Y  Y  Y
```

批量处理多个`token`,或者在`prefill`阶段并行处理`token`时。

```txt
         K0 K1 K2 K3 K4 K5 K6
Query 5   Y  Y  Y  Y  Y  Y  N
Query 6   Y  Y  Y  Y  Y  Y  Y
```

#### build_inp_pos，build_inp_out_ids

##### 代码

```CPP
ggml_tensor * llm_graph_context::build_inp_pos() const {
    auto inp = std::make_unique<llm_graph_input_pos>(hparams.n_pos_per_embd());

    auto & cur = inp->pos;

    cur = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, (int64_t)n_tokens*hparams.n_pos_per_embd());
    ggml_set_input(cur);

    res->add_input(std::move(inp));

    return cur;
}
ggml_tensor * llm_graph_context::build_inp_out_ids() const {
    // note: when all tokens are output, we could skip this optimization to spare the ggml_get_rows() calls,
    //       but this would make the graph topology depend on the number of output tokens, which can interfere with
    //       features that require constant topology such as pipeline parallelism
    //       ref: https://github.com/ggml-org/llama.cpp/pull/14275#issuecomment-2987424471
    //if (n_outputs < n_tokens) {
    //    return nullptr;
    //}

    auto inp = std::make_unique<llm_graph_input_out_ids>(hparams, cparams, n_outputs);

    auto & cur = inp->out_ids;

    cur = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, n_outputs);
    ggml_set_input(cur);

    res->add_input(std::move(inp));

    return cur;
}
```

##### 解释

`build_inp_pos`是每个输入`token`的位置索引，用于后续进行RoPE,QWen使用的`M-RoPE`每个`token`有`4`个位置维度.

`build_inp_out_ids`是每个需要输出结果的`Token`下标.

假设在`prefill`阶段，一次输入`4`个文本token，只需要最后一个`token`的`logits`：

```txt
token：          T0  T1  T2  T3
ubatch 内下标：   0   1   2   3
绝对位置：        0   1   2   3
需要输出：        否  否  否  是
```

`inp_pos`为

```txt
n_tokens        = 4
n_pos_per_embd  = 4
inp_pos 元素数   = 4 x 4 = 16
inp_pos 形状     = I32 [16]
T0 -> [0, 0, 0, 0]
T1 -> [1, 1, 1, 0]
T2 -> [2, 2, 2, 0]
T3 -> [3, 3, 3, 0]
```

`inp_pos_ids`为

```txt
n_outputs       = 1
inp_out_ids     = [3]
inp_out_ids形状 = I32 [1]
```

#### build_norm

##### 代码

```CPP
ggml_tensor * llm_graph_context::build_norm(
         ggml_tensor * cur,
         ggml_tensor * mw,
         ggml_tensor * mb,
       llm_norm_type   type,
                 int   il) const {
    switch (type) {
        case LLM_NORM:       cur = ggml_norm    (ctx0, cur, hparams.f_norm_eps);     break;
        case LLM_NORM_RMS:   cur = ggml_rms_norm(ctx0, cur, hparams.f_norm_rms_eps); break;
        case LLM_NORM_GROUP:
            {
                cur = ggml_reshape_3d(ctx0, cur, cur->ne[0], 1, cur->ne[1]);
                cur = ggml_group_norm(ctx0, cur, hparams.n_norm_groups, hparams.f_norm_group_eps);
                cur = ggml_reshape_2d(ctx0, cur, cur->ne[0],    cur->ne[2]);
            } break;
    }

    if (mw || mb) {
        cb(cur, "norm", il);
    }

    if (mw) {
        cur = ggml_mul(ctx0, cur, mw);
        if (mb) {
            cb(cur, "norm_w", il);
        }
    }

    if (mb) {
        cur = ggml_add(ctx0, cur, mb);
    }

    return cur;
}
```

##### 解释

构建`RMS_Norm`.

#### build_layer_attn_linear

##### 源码

```CPP

ggml_tensor * llama_model_qwen35::graph::build_layer_attn_linear(
        llm_graph_input_rs * inp,
        ggml_tensor *        cur,
        int                  il) {
    const auto * mctx_cur = inp->mctx;

    const int64_t d_inner      = hparams.ssm_d_inner;
    const int64_t n_seqs       = ubatch.n_seqs;
    const int64_t head_k_dim   = hparams.ssm_d_state;
    const int64_t num_k_heads  = hparams.ssm_n_group;
    const int64_t num_v_heads  = hparams.ssm_dt_rank;
    const int64_t head_v_dim   = d_inner / num_v_heads;
    const int64_t n_seq_tokens = ubatch.n_seq_tokens;

    GGML_ASSERT(n_seqs != 0);
    GGML_ASSERT(ubatch.equal_seqs());
    GGML_ASSERT(ubatch.n_tokens == n_seq_tokens * n_seqs);

    // Input projections
    auto qkvz = build_qkvz(cur, il);
    ggml_tensor * qkv_mixed = qkvz.first;
    ggml_tensor * z         = qkvz.second;

    ggml_tensor * beta = build_lora_mm(model.layers[il].ssm_beta, cur, model.layers[il].ssm_beta_s);
    beta = ggml_reshape_4d(ctx0, beta, 1, num_v_heads, n_seq_tokens, n_seqs);
    cb(beta, "beta", il);

    beta = ggml_sigmoid(ctx0, beta);
    cb(beta, "beta_sigmoid", il);

    ggml_tensor * alpha = build_lora_mm(model.layers[il].ssm_alpha, cur, model.layers[il].ssm_alpha_s);
    alpha = ggml_reshape_3d(ctx0, alpha, num_v_heads, n_seq_tokens, n_seqs);
    cb(alpha, "alpha", il);

    ggml_tensor * alpha_biased   = ggml_add(ctx0, alpha, model.layers[il].ssm_dt);
    ggml_tensor * alpha_softplus = ggml_softplus(ctx0, alpha_biased);
    cb(alpha_softplus, "a_softplus", il);

    ggml_tensor * gate = ggml_mul(ctx0, alpha_softplus, model.layers[il].ssm_a);  // -A_log.exp() * softplus
    cb(gate, "gate", il);

    gate = ggml_reshape_4d(ctx0, gate, 1, num_v_heads, n_seq_tokens, n_seqs);

    ggml_tensor * conv_states_all = mctx_cur->get_r_l(il);
    ggml_tensor * ssm_states_all  = mctx_cur->get_s_l(il);

    ggml_tensor * conv_kernel      = model.layers[il].ssm_conv1d;
    const int64_t conv_kernel_size = conv_kernel->ne[0];
    const int64_t conv_channels    = d_inner + 2 * hparams.ssm_n_group * hparams.ssm_d_state;

    ggml_tensor * conv_input = build_conv_state(inp, conv_states_all, qkv_mixed, conv_kernel_size, conv_channels, il);

    ggml_tensor * state = build_rs(inp, ssm_states_all, hparams.n_embd_s(), n_seqs);
    state = ggml_reshape_4d(ctx0, state, head_v_dim, head_v_dim, num_v_heads, n_seqs);
    cb(state, "state_predelta", il);

    ggml_tensor * conv_output_proper = ggml_ssm_conv(ctx0, conv_input, conv_kernel);
    cb(conv_output_proper, "conv_output_raw", il);

    ggml_tensor * conv_output_silu = ggml_silu(ctx0, conv_output_proper);
    cb(conv_output_silu, "conv_output_silu", il);

    ggml_tensor * conv_qkv_mix = conv_output_silu;

    // Calculate the total conv dimension
    int64_t qkv_dim = head_k_dim * num_k_heads * 2 + head_v_dim * num_v_heads;
    int64_t nb1_qkv = ggml_row_size(conv_qkv_mix->type, qkv_dim);

    // Extract the convolved Q, K, V from conv_output
    ggml_tensor * q_conv = ggml_view_4d(ctx0, conv_qkv_mix, head_k_dim, num_k_heads, n_seq_tokens, n_seqs,
            ggml_row_size(conv_qkv_mix->type, head_k_dim),
            nb1_qkv,
            nb1_qkv * n_seq_tokens,
            0);

    ggml_tensor * k_conv = ggml_view_4d(ctx0, conv_qkv_mix, head_k_dim, num_k_heads, n_seq_tokens, n_seqs,
            ggml_row_size(conv_qkv_mix->type, head_k_dim),
            nb1_qkv,
            nb1_qkv * n_seq_tokens,
            head_k_dim * num_k_heads * ggml_element_size(conv_qkv_mix));

    ggml_tensor * v_conv = ggml_view_4d(ctx0, conv_qkv_mix, head_v_dim, num_v_heads, n_seq_tokens, n_seqs,
            ggml_row_size(conv_qkv_mix->type, head_v_dim),
            nb1_qkv,
            nb1_qkv * n_seq_tokens,
            ggml_row_size(conv_qkv_mix->type, 2 * head_k_dim * num_k_heads));

    cb(q_conv, "q_conv", il);
    cb(k_conv, "k_conv", il);
    cb(v_conv, "v_conv", il);

    const float eps_norm = hparams.f_norm_rms_eps;

    q_conv = ggml_l2_norm(ctx0, q_conv, eps_norm);
    k_conv = ggml_l2_norm(ctx0, k_conv, eps_norm);

    //q_conv = ggml_cont_4d(ctx0, q_conv, head_k_dim, num_k_heads, n_seq_tokens, n_seqs);
    //k_conv = ggml_cont_4d(ctx0, k_conv, head_k_dim, num_k_heads, n_seq_tokens, n_seqs);
    //v_conv = ggml_cont_4d(ctx0, v_conv, head_v_dim, num_v_heads, n_seq_tokens, n_seqs);

    // if head keys and value keys are different, repeat to force tensors into matching shapes
    // note: need explicit repeat only if we are not using the fused GDN.
    if (num_k_heads != num_v_heads && (!cparams.fused_gdn_ar || !cparams.fused_gdn_ch)) {
        GGML_ASSERT(num_v_heads % num_k_heads == 0);
        q_conv = ggml_repeat_4d(ctx0, q_conv, head_k_dim, num_v_heads, n_seq_tokens, n_seqs);
        k_conv = ggml_repeat_4d(ctx0, k_conv, head_k_dim, num_v_heads, n_seq_tokens, n_seqs);
    }

    cb(q_conv, "q_conv_predelta", il);
    cb(k_conv, "k_conv_predelta", il);
    cb(v_conv, "v_conv_predelta", il);

    ggml_tensor * output = build_recurrent_attn(inp, ssm_states_all, q_conv, k_conv, v_conv, gate, beta, state, il);

    // z: [head_dim, n_heads, n_tokens, n_seqs] -> [n_heads * n_tokens * n_seqs, head_dim]
    ggml_tensor * z_2d = ggml_reshape_4d(ctx0, z, head_v_dim, num_v_heads, n_seq_tokens, n_seqs);

    // Apply gated normalization: self.norm(core_attn_out, z)
    ggml_tensor * attn_out_norm = build_norm_gated(output, model.layers[il].ssm_norm, z_2d, il);

    // Final reshape: [head_dim, n_heads, n_tokens, n_seqs] -> [n_tokens, n_seqs, n_heads * head_dim]
    ggml_tensor * final_output = ggml_reshape_3d(ctx0, attn_out_norm, head_v_dim * num_v_heads, n_seq_tokens, n_seqs);
    cb(final_output, "final_output", il);

    // Output projection
    cur = build_lora_mm(model.layers[il].ssm_out, final_output, model.layers[il].ssm_out_s);
    cb(cur, "linear_attn_out", il);

    // Reshape back to original dimensions
    cur = ggml_reshape_2d(ctx0, cur, n_embd, n_seq_tokens * n_seqs);

    return cur;
}
```

##### 解释

`build_layer_attn_linear`实现线性注意力层，具体算法是`Gated DeltaNet`,只用维护一个固定大小的卷积状态和矩阵状态，不需要完整KV Cache.

```CPP
const int64_t d_inner      = hparams.ssm_d_inner;
const int64_t n_seqs       = ubatch.n_seqs;
const int64_t head_k_dim   = hparams.ssm_d_state;
const int64_t num_k_heads  = hparams.ssm_n_group;
const int64_t num_v_heads  = hparams.ssm_dt_rank;
const int64_t head_v_dim   = d_inner / num_v_heads;
const int64_t n_seq_tokens = ubatch.n_seq_tokens;
```

读取参数.

* `n_seqs`表示当前`ubatch`中的批次$B$.
* `n_seq_tokens`表示当前`ubatch`中每条序列包含多少个`tokens`$T$.
* `d_inner`线性注意力内部的总`Value`特征维度,`d_inner = head_v_dim * num_v_heads`
* `head_k_dim`每个`Q/K head`的向量维度.`Q/K: [head_k_dim, num_k_heads,  n_seq_tokens, n_seqs]`
* `head_v_dim`每个`V head`的向量维度.`V: [head_v_dim, num_v_heads, n_seq_tokens, n_seqs]`
* `num_k_heads`,`Q`和`K`的`head`数量.
* `num_v_heads`,`V`的`head`数量.

```CPP
std::pair<ggml_tensor *, ggml_tensor *> llama_model_qwen35::graph::build_qkvz(
                ggml_tensor * input,
                        int   il) {
    const int64_t n_seqs       = ubatch.n_seqs;
    const int64_t n_seq_tokens = ubatch.n_seq_tokens;

    ggml_tensor * qkv_mixed = build_lora_mm(model.layers[il].wqkv, input, model.layers[il].wqkv_s);
    qkv_mixed = ggml_reshape_3d(ctx0, qkv_mixed, qkv_mixed->ne[0], n_seq_tokens, n_seqs);
    cb(qkv_mixed, "linear_attn_qkv_mixed", il);

    ggml_tensor * z = build_lora_mm(model.layers[il].wqkv_gate, input, model.layers[il].wqkv_gate_s);
    cb(z, "z", il);

    return { qkv_mixed, z };
}
```

把线性`Attention`层的输入投影成两部分：

* 混合在一起的`Q、K、V`。
* 最终输出门控`z`。

`build_lora_mm`统一处理普通矩阵乘法、权重缩放和LoRA.

进行线性投影,计算`qkv_mixed`

```txt
qkv_mixed = W_qkv * input
qkv_mixed = [Q | K | V]
```

随后`reshape`为:

```txt
[qkv_dim, n_seq_tokens, n_seqs]
```

进行线性投影，计算`z`门控

```txt
z = W_z * input
[value_dim, n_tokens]
```

```CPP
ggml_tensor * beta = build_lora_mm(model.layers[il].ssm_beta, cur, model.layers[il].ssm_beta_s);
beta = ggml_reshape_4d(ctx0, beta, 1, num_v_heads, n_seq_tokens, n_seqs);
cb(beta, "beta", il);

beta = ggml_sigmoid(ctx0, beta);
cb(beta, "beta_sigmoid", il);

ggml_tensor * alpha = build_lora_mm(model.layers[il].ssm_alpha, cur, model.layers[il].ssm_alpha_s);
alpha = ggml_reshape_3d(ctx0, alpha, num_v_heads, n_seq_tokens, n_seqs);
cb(alpha, "alpha", il);

ggml_tensor * alpha_biased   = ggml_add(ctx0, alpha, model.layers[il].ssm_dt);
ggml_tensor * alpha_softplus = ggml_softplus(ctx0, alpha_biased);
cb(alpha_softplus, "a_softplus", il);

ggml_tensor * gate = ggml_mul(ctx0, alpha_softplus, model.layers[il].ssm_a);  // -A_log.exp() * softplus
cb(gate, "gate", il);

gate = ggml_reshape_4d(ctx0, gate, 1, num_v_heads, n_seq_tokens, n_seqs);
```

计算`Gated Delta Rule`中的`gate`衰减门与`beta`，最终用到后面的变量是`beta`和`gate`.流程为

```txt
beta = sigmoid(ssm_beta * cur);
alpha = ssm_alpha * cur;
alpha_biased = alpha + ssm_dt;
alpha_softplus = softplus(alpha_biased);
gate = alpha_softplus * ssm_a; // ssm_a = -exp(A_log)
```

$$
M_t=
\alpha_t(I-\beta_tk_t^Tk_t)M_{t-1}
+\beta_tk_t^Tv_t
$$

```CPP
ggml_tensor * conv_states_all = mctx_cur->get_r_l(il);
ggml_tensor * ssm_states_all  = mctx_cur->get_s_l(il);

ggml_tensor * conv_kernel      = model.layers[il].ssm_conv1d;
const int64_t conv_kernel_size = conv_kernel->ne[0];
const int64_t conv_channels    = d_inner + 2 * hparams.ssm_n_group * hparams.ssm_d_state;
```

获取的卷积历史状态缓存`conv_states_all`,循环状态历史缓存`ssm_states_all`，卷积核`conv_kernel`,卷积核在`token`时间轴上的长度`conv_kernel_size`,要卷积的通道数`conv_channels`就是`conv_channels = Q宽度 + K宽度 + V宽度`.例如

卷积历史状态缓存`conv_states_all`的形状为`[state_size, mem_size * (1 + n_rs_seq)]`

循环状态历史缓存`ssm_states_all`的形状为`[head_v_dim, head_k_dim, num_v_heads, n_seqs]`

`conv_kernel_size = 4`表示计算当前`token`卷积时使用四个QKV位置`[x[t-3], x[t-2], x[t-1], x[t]]`，需要缓存`3`个历史QKV.

`conv_channels = 8192`表示需要计算`8192`个通道的卷积.

```CPP
ggml_tensor * llm_graph_context::build_rs(
        ggml_tensor * s,
        ggml_tensor * state_copy_main,
        ggml_tensor * state_copy_extra,
            int32_t   state_size,
            int32_t   n_seqs,
           uint32_t   n_rs,
           uint32_t   rs_head,
           uint32_t   rs_size,
            int32_t   rs_zero,
        const llm_graph_get_rows_fn & get_state_rows) const {

    GGML_UNUSED(rs_size);
    ggml_tensor * states = ggml_reshape_2d(ctx0, s, state_size, s->ne[1]);

    // Clear a single state which will then be copied to the other cleared states.
    // Note that this is a no-op when the view is zero-sized.
    ggml_tensor * state_zero = ggml_view_1d(ctx0, states, state_size*(rs_zero >= 0), rs_zero*states->nb[1]*(rs_zero >= 0));
    ggml_build_forward_expand(gf, ggml_scale_inplace(ctx0, state_zero, 0));

    // copy states
    // NOTE: assuming the copy destinations are ALL contained between rs_head and rs_head + n_rs
    // {state_size, rs_size} -> {state_size, n_seqs}
    ggml_tensor * output_states = get_state_rows(ctx0, states, state_copy_main);
    ggml_build_forward_expand(gf, output_states);

    // copy extra states which won't be changed further (between n_seqs and n_rs)
    ggml_tensor * states_extra = ggml_get_rows(ctx0, states, state_copy_extra);
    ggml_build_forward_expand(gf,
        ggml_cpy(ctx0,
            states_extra,
            ggml_view_2d(ctx0, s, state_size, (n_rs - n_seqs), s->nb[1], (rs_head + n_seqs)*s->nb[1])));

    return output_states;
}
```

`build_rs`负责从取出当前序列需要的状态，同时完成清零、复制和缓存槽位整理。它统一化短卷积状态`conv_states_all`和`DeltaNet`矩阵状态`ssm_states_all`.

核心输入输出

```txt
输入：
s                   所有缓存槽位的状态
state_copy_main     当前序列状态的来源槽位
state_copy_extra    额外状态的来源槽位

输出：
output_states: [state_size, n_seqs]  当前 n_seqs 条序列的状态
```

对于短卷积状态,`state_size = (conv_kernel_size - 1) * conv_channels`.

对于`DeltaNet`矩阵状态,`state_size = Dk * Dv * num_v_heads`

```CPP
ggml_tensor * llm_build_delta_net_base::build_conv_state(
        llm_graph_input_rs * inp,
        ggml_tensor *        conv_states_all,
        ggml_tensor *        qkv_mixed,
        int64_t              conv_kernel_size,
        int64_t              conv_channels,
        int                  il) {
    const auto * mctx_cur = inp->mctx;

    const auto kv_head  = mctx_cur->get_head();
    const auto mem_size = mctx_cur->get_size();

    const int64_t n_seqs = ubatch.n_seqs;

    ggml_tensor * conv_states = build_rs(inp, conv_states_all, hparams.n_embd_r(), n_seqs);
    cb(conv_states, "conv_states", il);

    conv_states = ggml_reshape_3d(ctx0, conv_states, conv_kernel_size - 1, conv_channels, n_seqs);
    cb(conv_states, "conv_states_reshaped", il);

    qkv_mixed = ggml_transpose(ctx0, qkv_mixed);
    cb(qkv_mixed, "qkv_mixed_transposed", il);

    ggml_tensor * conv_input = ggml_concat(ctx0, conv_states, qkv_mixed, 0);
    cb(conv_input, "conv_input", il);

    const int64_t row_count = (conv_kernel_size - 1) * conv_channels;

    const size_t row_size  = ggml_row_size(conv_states_all->type, row_count);

    if (cparams.n_rs_seq == 0) {
        const int64_t s_idx  = conv_input->ne[0] - conv_states->ne[0];
        const int64_t s_slot = 0;

        ggml_tensor * conv_state_last =
            ggml_view_3d(ctx0, conv_input,
                    conv_kernel_size - 1, conv_channels, n_seqs,
                    conv_input->nb[1], conv_input->nb[2],
                    ggml_row_size(conv_input->type, s_idx));
        cb(conv_state_last, "conv_state_last", il);

        ggml_tensor * conv_state_update =
            ggml_view_2d(ctx0, conv_states_all,
                    row_count, n_seqs, conv_states_all->nb[1],
                    (s_slot * mem_size + kv_head) * row_size);
        cb(conv_state_update, "conv_state_update", il);

        ggml_build_forward_expand(gf, ggml_cpy(ctx0, conv_state_last, conv_state_update));
    } else {
        // [TAG_RECURRENT_ROLLBACK_SPLITS]
        // this logic assumes that the last (n_rs_seq + 1) tokens of a sequence in a batch are inside
        //   the same ubatch, which `split_equal()` guarantees via its n_keep_tail argument

        const int64_t K = (int64_t) cparams.n_rs_seq + 1;

        for (int64_t t = 1; t <= K; ++t) {
            const int64_t s_idx  = std::max<int64_t>(0, conv_input->ne[0] - conv_states->ne[0] - K + t);
            const int64_t s_slot = K - t;

            ggml_tensor * conv_state_last =
                ggml_view_3d(ctx0, conv_input,
                        conv_kernel_size - 1, conv_channels, n_seqs,
                        conv_input->nb[1], conv_input->nb[2],
                        ggml_row_size(conv_input->type, s_idx));

            ggml_tensor * conv_state_update =
                ggml_view_2d(ctx0,
                        conv_states_all, row_count, n_seqs,
                        conv_states_all->nb[1],
                        (s_slot * mem_size + kv_head) * row_size);

            ggml_build_forward_expand(gf, ggml_cpy(ctx0, conv_state_last, conv_state_update));
        }
    }

    return conv_input;
}
```

`build_conv_state`函数管理短卷积的历史状态.

`kv_head`,当前序列状态要写入的缓存起点.`mem_size`,每个状态快照平面包含多少缓存槽位.`n_seqs`,本轮并行处理的序列数.

`build_rs`输出展平的循环状态缓存`conv_states: [state_size, n_seqs]`，先将它还原为`[conv_kernel_size - 1, conv_channels, n_seqs]`.将`qkv_mixed`转置后，变成`[n_seq_tokens, conv_channels, n_seqs]`，这样第`0`维就是时间维，与历史循环状态缓存`conv_states`对齐,进行拼接，得到`conv_input: [conv_kernel_size - 1 + n_seq_tokens, conv_channels, n_seqs]`.由于之后需要改变`conv_states_all`,所以`conv_input`在拼接时进行了拷贝。

不启用回滚时，`conv_state_last`就是从`n_seq_tokens`开始的`conv_input`.维度为`[conv_kernel_size - 1, conv_channels, n_seqs]`,它是当前序列的最新卷积状态.

`conv_state_update`的形状是`[(conv_kernel_size - 1) * conv_channels, n_seqs]`，表示需要更新的缓存。

```CPP
ggml_tensor * state = build_rs(inp, ssm_states_all, hparams.n_embd_s(), n_seqs);
state = ggml_reshape_4d(ctx0, state, head_v_dim, head_v_dim, num_v_heads, n_seqs);
```

先读取当前`n_seqs`条序列的状态，`state: [hparams.n_embd_s(), n_seqs]`其中，`hparams.n_embd_s() = ssm_d_state * ssm_d_inner = head_v_dim * head_v_dim * num_v_heads`.再将展平状态恢复为`[head_v_dim, head_v_dim, num_v_heads, n_seqs]`.

```CPP
ggml_tensor * conv_output_proper = ggml_ssm_conv(ctx0, conv_input, conv_kernel);
cb(conv_output_proper, "conv_output_raw", il);

ggml_tensor * conv_output_silu = ggml_silu(ctx0, conv_output_proper);
cb(conv_output_silu, "conv_output_silu", il);

ggml_tensor * conv_qkv_mix = conv_output_silu;
```

对拼接后的`QKV`做逐通道因果一维卷积，然后应用`SiLU`激活,之后改名准备拆分`Q/K/V`.

```CPP
// Calculate the total conv dimension
int64_t qkv_dim = head_k_dim * num_k_heads * 2 + head_v_dim * num_v_heads;
int64_t nb1_qkv = ggml_row_size(conv_qkv_mix->type, qkv_dim);

// Extract the convolved Q, K, V from conv_output
ggml_tensor * q_conv = ggml_view_4d(ctx0, conv_qkv_mix, head_k_dim, num_k_heads, n_seq_tokens, n_seqs,
        ggml_row_size(conv_qkv_mix->type, head_k_dim),
        nb1_qkv,
        nb1_qkv * n_seq_tokens,
        0);

ggml_tensor * k_conv = ggml_view_4d(ctx0, conv_qkv_mix, head_k_dim, num_k_heads, n_seq_tokens, n_seqs,
        ggml_row_size(conv_qkv_mix->type, head_k_dim),
        nb1_qkv,
        nb1_qkv * n_seq_tokens,
        head_k_dim * num_k_heads * ggml_element_size(conv_qkv_mix));

ggml_tensor * v_conv = ggml_view_4d(ctx0, conv_qkv_mix, head_v_dim, num_v_heads, n_seq_tokens, n_seqs,
        ggml_row_size(conv_qkv_mix->type, head_v_dim),
        nb1_qkv,
        nb1_qkv * n_seq_tokens,
        ggml_row_size(conv_qkv_mix->type, 2 * head_k_dim * num_k_heads));
```

把卷积后的混合张量`conv_qkv_mix`按内存区域拆成`Q、K、V`三个零拷贝`view`.

```CPP
q_conv = ggml_l2_norm(ctx0, q_conv, eps_norm);
k_conv = ggml_l2_norm(ctx0, k_conv, eps_norm);
```

对每个`Q head`和`K head`做`L2`归一化.

```CPP
if (num_k_heads != num_v_heads && (!cparams.fused_gdn_ar || !cparams.fused_gdn_ch)) {
    GGML_ASSERT(num_v_heads % num_k_heads == 0);
    q_conv = ggml_repeat_4d(ctx0, q_conv, head_k_dim, num_v_heads, n_seq_tokens, n_seqs);
    k_conv = ggml_repeat_4d(ctx0, k_conv, head_k_dim, num_v_heads, n_seq_tokens, n_seqs);
}
```

处理`Q/K head`数量与`V head`数量不同的情况.当不能完全依赖`fused GDN`的内部广播时，就显式重复`Q/K`

```CPP
ggml_tensor * llm_build_delta_net_base::build_recurrent_attn(
        llm_graph_input_rs * inp,
        ggml_tensor *        ssm_states_all,
        ggml_tensor *        q,
        ggml_tensor *        k,
        ggml_tensor *        v,
        ggml_tensor *        g,
        ggml_tensor *        b,
        ggml_tensor *        s,
        int                  il) {
    const auto * mctx_cur   = inp->mctx;
    const auto   kv_head    = mctx_cur->get_head();
    const uint32_t mem_size = mctx_cur->get_size();

    const int64_t S_v          = s->ne[0];
    const int64_t H_v          = s->ne[2];
    const int64_t n_seqs       = s->ne[3];
    const int64_t n_seq_tokens = q->ne[2];

    const bool keep = cparams.n_rs_seq > 0;

    if (!keep) {
        auto attn_out = build_delta_net(q, k, v, g, b, s, il);
        ggml_tensor * output    = attn_out.first;
        ggml_tensor * new_state = attn_out.second;
        cb(output, "attn_output", il);
        cb(new_state, "new_state", il);

        ggml_build_forward_expand(gf,
                ggml_cpy(ctx0, new_state,
                    ggml_view_2d(ctx0, ssm_states_all, hparams.n_embd_s(), n_seqs, ssm_states_all->nb[1],
                        kv_head * hparams.n_embd_s() * ggml_element_size(ssm_states_all))));

        return output;
    }

    const int64_t D = S_v * S_v * H_v;
    const int64_t K = cparams.n_rs_seq + 1;

    // state s is 4D [S_v, S_v, H_v, n_seqs]; K snapshot slots are written into the output.
    ggml_tensor * gdn_out = ggml_gated_delta_net(ctx0, q, k, v, g, b, s, K);
    if (n_seq_tokens > 1) {
        res->add_fused_node({LLM_FUSED_OP_GDN_CH, gdn_out, il});
    } else {
        res->add_fused_node({LLM_FUSED_OP_GDN_AR, gdn_out, il});
    }

    const int64_t attn_score_elems    = S_v * H_v * n_seq_tokens * n_seqs;
    const int64_t state_size_per_snap = S_v * S_v * H_v * n_seqs;

    ggml_tensor * output = ggml_view_4d(ctx0, gdn_out,
        S_v, H_v, n_seq_tokens, n_seqs,
        ggml_row_size(gdn_out->type, S_v),
        ggml_row_size(gdn_out->type, S_v * H_v),
        ggml_row_size(gdn_out->type, S_v * H_v * n_seq_tokens),
        0);
    cb(output, "attn_output", il);

    const size_t row_size = hparams.n_embd_s() * ggml_element_size(ssm_states_all);

    // op writes the last min(n_seq_tokens, K) snapshots; trailing slots are left unwritten
    const int64_t n_written = std::min<int64_t>(n_seq_tokens, K);

    // write the produced snapshots into the recurrent cache (snapshot slot i -> rollback group i)
    ggml_tensor * src = ggml_view_3d(ctx0, gdn_out,
        D, n_seqs, n_written,
        ggml_row_size(gdn_out->type, D),
        ggml_row_size(gdn_out->type, state_size_per_snap),
        ggml_row_size(gdn_out->type, attn_score_elems));

    ggml_tensor * dst = ggml_view_3d(ctx0, ssm_states_all,
        D, n_seqs, n_written,
        ssm_states_all->nb[1],
        (size_t) mem_size * row_size,
        (size_t) kv_head * row_size);

    ggml_build_forward_expand(gf, ggml_cpy(ctx0, src, dst));

    return output;
}
```

进行`Gated DeltaNet`递推注意力，并把新的`recurrent state`写回`ssm_states_all`.

`output:[head_v_dim, num_v_heads, n_seq_tokens, n_seqs]`是计算出的线性注意力,`new_state:[head_v_dim, head_k_dim, num_v_heads, n_seqs]`是要更新的线性注意力状态。

```CPP
std::pair<ggml_tensor *, ggml_tensor *> llm_build_delta_net_base::build_delta_net(
        ggml_tensor * q,
        ggml_tensor * k,
        ggml_tensor * v,
        ggml_tensor * g,
        ggml_tensor * b,
        ggml_tensor * s,
        int           il) {
    const int64_t n_seq_tokens = q->ne[2];

    if (n_seq_tokens == 1) {
        if (cparams.fused_gdn_ar) {
            return build_delta_net_fused(q, k, v, g, b, s, il);
        }
        return build_delta_net_autoregressive(q, k, v, g, b, s, il);
    }

    if (cparams.fused_gdn_ch) {
        return build_delta_net_fused(q, k, v, g, b, s, il);
    }

    return build_delta_net_chunking(q, k, v, g, b, s, il);
}
```

根据`token`数量和后端`fused`支持情况选择计算路径。

输入

```txt
q/k/v：当前 Q、K、V
g：状态衰减参数
b：beta，状态更新强度
s：初始递归状态
```

输出

```txt
std::pair<output, new_state>
output:    [S_v, H_v, n_seq_tokens, n_seqs]
new_state: [S_v, S_v, H_v, n_seqs]
```

```CPP
// z: [head_dim, n_heads, n_tokens, n_seqs] -> [n_heads * n_tokens * n_seqs, head_dim]
ggml_tensor * z_2d = ggml_reshape_4d(ctx0, z, head_v_dim, num_v_heads, n_seq_tokens, n_seqs);

// Apply gated normalization: self.norm(core_attn_out, z)
ggml_tensor * attn_out_norm = build_norm_gated(output, model.layers[il].ssm_norm, z_2d, il);

// Final reshape: [head_dim, n_heads, n_tokens, n_seqs] -> [n_tokens, n_seqs, n_heads * head_dim]
ggml_tensor * final_output = ggml_reshape_3d(ctx0, attn_out_norm, head_v_dim * num_v_heads, n_seq_tokens, n_seqs);
cb(final_output, "final_output", il);

// Output projection
cur = build_lora_mm(model.layers[il].ssm_out, final_output, model.layers[il].ssm_out_s);
cb(cur, "linear_attn_out", il);

// Reshape back to original dimensions
cur = ggml_reshape_2d(ctx0, cur, n_embd, n_seq_tokens * n_seqs);
```

对`DeltaNet`输出做门控归一化，合并`heads`，再投影回模型隐藏维度.

先将门控张量`z`恢复成多`head`形式,`z_2d: [head_v_dim, num_v_heads, n_seq_tokens, n_seqs]`.

计算`Gated RMSNorm`:

```txt
normalized = RMSNorm(output)
gate       = SiLU(z_2d)
attn_out_norm = normalized * gate
```

合并`value heads`，把`attn_out_norm: [head_v_dim, num_v_heads, n_seq_tokens, n_seqs]`合并为`final_output: [head_v_dim * num_v_heads, n_seq_tokens, n_seqs]`.

进行输出投影将`final_output`投影回模型隐藏维度`[n_embd, n_seq_tokens, n_seqs]`,最后合并`sequence`和`token`维度`[n_embd, n_seq_tokens * n_seqs]`和进入时保持一致。

#### build_layer_attn

##### 源码

```CPP

ggml_tensor * llama_model_qwen35::graph::build_layer_attn(
        llm_graph_input_attn_kv * inp,
        ggml_tensor *             cur,
        ggml_tensor *             inp_pos,
        int *                     sections,
        int                       il) {
    const int64_t n_embd_head = hparams.n_embd_head_v();
    GGML_ASSERT(n_embd_head == hparams.n_embd_head_k());

    // Order: joint QG projection, QG split, Q norm, KV projection, K norm, RoPE, attention

    // Qwen3Next uses a single Q projection that outputs query + gate
    ggml_tensor * Qcur_full = build_lora_mm(model.layers[il].wq, cur, model.layers[il].wq_s); // [ (n_embd_head * 2) * n_head, n_tokens ]
    cb(Qcur_full, "Qcur_full", il);

    ggml_tensor * Qcur = ggml_view_3d(ctx0, Qcur_full, n_embd_head, n_head, n_tokens,
        ggml_element_size(Qcur_full) * n_embd_head * 2,
        ggml_element_size(Qcur_full) * n_embd_head * 2 * n_head, 0);
    cb(Qcur, "Qcur_reshaped", il);

    // Apply Q normalization
    Qcur = build_norm(Qcur, model.layers[il].attn_q_norm, nullptr, LLM_NORM_RMS, il);
    cb(Qcur, "Qcur_normed", il);

    ggml_tensor * Kcur = build_lora_mm(model.layers[il].wk, cur, model.layers[il].wk_s);
    cb(Kcur, "Kcur", il);

    ggml_tensor * Vcur = build_lora_mm(model.layers[il].wv, cur, model.layers[il].wv_s);
    cb(Vcur, "Vcur", il);

    // Apply K normalization
    Kcur = ggml_reshape_3d(ctx0, Kcur, n_embd_head, n_head_kv, n_tokens);
    Kcur = build_norm(Kcur, model.layers[il].attn_k_norm, nullptr, LLM_NORM_RMS, il);
    cb(Kcur, "Kcur_normed", il);

    ggml_tensor * gate = ggml_view_3d(ctx0, Qcur_full, n_embd_head, n_head, n_tokens,
        ggml_element_size(Qcur_full) * n_embd_head * 2,
        ggml_element_size(Qcur_full) * n_embd_head * 2 * n_head,
        ggml_element_size(Qcur_full) * n_embd_head);
    gate = ggml_cont_2d(ctx0, gate, n_embd_head * n_head, n_tokens);
    cb(gate, "gate_reshaped", il);

    Vcur = ggml_reshape_3d(ctx0, Vcur, n_embd_head, n_head_kv, n_tokens);

    // Apply MRoPE
    Qcur = ggml_rope_multi(
            ctx0, Qcur, inp_pos, nullptr,
            n_rot, sections, rope_type, n_ctx_orig, freq_base, freq_scale,
            ext_factor, attn_factor, beta_fast, beta_slow
            );

    Kcur = ggml_rope_multi(
            ctx0, Kcur, inp_pos, nullptr,
            n_rot, sections, rope_type, n_ctx_orig, freq_base, freq_scale,
            ext_factor, attn_factor, beta_fast, beta_slow
            );

    cb(Qcur, "Qcur", il);
    cb(Kcur, "Kcur", il);
    cb(Vcur, "Vcur", il);

    // Attention computation
    const float kq_scale = hparams.f_attention_scale == 0.0f ? 1.0f / sqrtf(float(n_embd_head)) : hparams.f_attention_scale;

    cur = build_attn(inp,
                nullptr, nullptr, nullptr,
                Qcur, Kcur, Vcur, nullptr, nullptr, nullptr, kq_scale, il);
    cb(cur, "attn_pregate", il);

    ggml_tensor * gate_sigmoid = ggml_sigmoid(ctx0, gate);
    cb(gate_sigmoid, "gate_sigmoid", il);

    cur = ggml_mul(ctx0, cur, gate_sigmoid);
    cb(cur, "attn_gated", il);

    cur = build_lora_mm(model.layers[il].wo, cur, model.layers[il].wo_s);
    cb(cur, "attn_output", il);

    return cur;
}
```

##### 解释

这个函数计算全注意力层.

* `inp`这一层全注意力要用的`KV cache`句柄。
* `cur`本层注意力输入,维度是`[n_embd, n_tokens]`.
* `inp_pos`位置`id`,给RoPE用
* `sections`,MRoPE 四段划分（文本/高/宽/时间一类）。
* `il`层号

```CPP
const int64_t n_embd_head = hparams.n_embd_head_v();
GGML_ASSERT(n_embd_head == hparams.n_embd_head_k());
```

K/V head的维度相同.

```CPP
ggml_tensor * Qcur_full = build_lora_mm(model.layers[il].wq, cur, model.layers[il].wq_s); // [ (n_embd_head * 2) * n_head, n_tokens ]
cb(Qcur_full, "Qcur_full", il);

ggml_tensor * Qcur = ggml_view_3d(ctx0, Qcur_full, n_embd_head, n_head, n_tokens,
    ggml_element_size(Qcur_full) * n_embd_head * 2,
    ggml_element_size(Qcur_full) * n_embd_head * 2 * n_head, 0);
cb(Qcur, "Qcur_reshaped", il);
```

`wq`是$W_Q$和门控$G$的联合权重.`Qcur_full`维度为`[ (n_embd_head * 2) * n_head, n_tokens ]`,其中`Q`和`G`是按照head交错:

```text
token t:
  [ Q_h0 | G_h0 | Q_h1 | G_h1 | ... | Q_hN | G_hN ]
     ^      ^
  head_dim  head_dim
```

使用`view`获取`Qcur`.维度变成`[n_embd_head, n_head, n_tokens]`.

```CPP
// Apply Q normalization
Qcur = build_norm(Qcur, model.layers[il].attn_q_norm, nullptr, LLM_NORM_RMS, il);
cb(Qcur, "Qcur_normed", il);

ggml_tensor * Kcur = build_lora_mm(model.layers[il].wk, cur, model.layers[il].wk_s);
cb(Kcur, "Kcur", il);

ggml_tensor * Vcur = build_lora_mm(model.layers[il].wv, cur, model.layers[il].wv_s);
cb(Vcur, "Vcur", il);

// Apply K normalization
Kcur = ggml_reshape_3d(ctx0, Kcur, n_embd_head, n_head_kv, n_tokens);
Kcur = build_norm(Kcur, model.layers[il].attn_k_norm, nullptr, LLM_NORM_RMS, il);
cb(Kcur, "Kcur_normed", il);
```

获取QKV，同时对QK进行`RMS_NORM`,这个是为了稳定训练.

`K`的形状是`[n_embd_head, n_head_kv, n_tokens]`

```CPP
ggml_tensor * gate = ggml_view_3d(ctx0, Qcur_full, n_embd_head, n_head, n_tokens,
    ggml_element_size(Qcur_full) * n_embd_head * 2,
    ggml_element_size(Qcur_full) * n_embd_head * 2 * n_head,
    ggml_element_size(Qcur_full) * n_embd_head);
gate = ggml_cont_2d(ctx0, gate, n_embd_head * n_head, n_tokens);
cb(gate, "gate_reshaped", il);
```

将使用`view`获取`gate`,维度变成`[n_embd_head, n_head, n_tokens]`.之后连续化为`[n_embd_head * n_head, n_tokens]`.

```CPP
Vcur = ggml_reshape_3d(ctx0, Vcur, n_embd_head, n_head_kv, n_tokens);
```

将`Vcur`变为`[n_embd_head, n_head_kv, n_tokens]`.

```CPP
// Apply MRoPE
Qcur = ggml_rope_multi(
        ctx0, Qcur, inp_pos, nullptr,
        n_rot, sections, rope_type, n_ctx_orig, freq_base, freq_scale,
        ext_factor, attn_factor, beta_fast, beta_slow
        );

Kcur = ggml_rope_multi(
        ctx0, Kcur, inp_pos, nullptr,
        n_rot, sections, rope_type, n_ctx_orig, freq_base, freq_scale,
        ext_factor, attn_factor, beta_fast, beta_slow
        );
```

使用M-RoPE旋转。

```CPP
ggml_tensor * llm_graph_context::build_attn(
        llm_graph_input_attn_kv * inp,
        ggml_tensor * wo,
        ggml_tensor * wo_b,
        ggml_tensor * wo_s,
        ggml_tensor * q_cur,
        ggml_tensor * k_cur,
        ggml_tensor * v_cur,
        ggml_tensor * kq_b,
        ggml_tensor * sinks,
        ggml_tensor * v_mla, // TODO: remove
            float     kq_scale,
            int       il) const {
    GGML_ASSERT(v_mla == nullptr);

    if (inp->self_k_rot) {
        q_cur = llama_mul_mat_hadamard(ctx0, q_cur, inp->self_k_rot);
        k_cur = llama_mul_mat_hadamard(ctx0, k_cur, inp->self_k_rot);
    }

    if (inp->self_v_rot) {
        v_cur = llama_mul_mat_hadamard(ctx0, v_cur, inp->self_v_rot);
    }

    // these nodes are added to the graph together so that they are not reordered
    // by doing so, the number of splits in the graph is reduced
    // expand k later to enable rope fusion which directly writes into k-v cache
    ggml_build_forward_expand(gf, q_cur);
    ggml_build_forward_expand(gf, v_cur);
    ggml_build_forward_expand(gf, k_cur);

    const auto * mctx_cur = inp->mctx;

    // store to KV cache
    {
        const auto & k_idxs = inp->get_k_idxs();
        const auto & v_idxs = inp->get_v_idxs();

        ggml_build_forward_expand(gf, mctx_cur->cpy_k(ctx0, k_cur, k_idxs, il));
        ggml_build_forward_expand(gf, mctx_cur->cpy_v(ctx0, v_cur, v_idxs, il));
    }

    ggml_tensor * kq_mask = inp->get_kq_mask();

    ggml_tensor * q = q_cur;
    ggml_tensor * k = mctx_cur->get_k(ctx0, il);
    ggml_tensor * v = mctx_cur->get_v(ctx0, il);

    ggml_tensor * cur = build_attn_mha(q, k, v, kq_b, kq_mask, sinks, v_mla, kq_scale, il);
    cb(cur, "kqv_out", il);

    if (inp->self_v_rot) {
        cur = llama_mul_mat_hadamard(ctx0, cur, inp->self_v_rot);
    }

    if (wo) {
        if (arch == LLM_ARCH_GLM4 || arch == LLM_ARCH_GLM4_MOE || arch == LLM_ARCH_JAIS2) {
            // GLM4, GLM4_MOE, and JAIS2 seem to have numerical issues with half-precision accumulators
            cur = build_lora_mm(wo, cur);
            ggml_mul_mat_set_prec(cur, GGML_PREC_F32);
            if (wo_s) {
                cur = ggml_mul(ctx0, cur, wo_s);
            }
        } else {
            cur = build_lora_mm(wo, cur, wo_s);
        }
    }

    if (wo_b) {
        cur = ggml_add(ctx0, cur, wo_b);
    }

    return cur;
}
```

这个函数准备计算注意力.

* `inp`内存上下文.
* `wo`,`wo_s`,`wo_b`是注意力输出投影相关的权重。
* `q_cur`,`k_cur`,`v_cur`当前的`QKV`.
* `kq_scale`,`softmax`前的缩放，通常`1/sqrt(head_dim)`.
* `kq_b`,`sinks`,`v_mla`少量架构使用.

```CPP
if (inp->self_k_rot) {
    q_cur = llama_mul_mat_hadamard(ctx0, q_cur, inp->self_k_rot);
    k_cur = llama_mul_mat_hadamard(ctx0, k_cur, inp->self_k_rot);
}

if (inp->self_v_rot) {
    v_cur = llama_mul_mat_hadamard(ctx0, v_cur, inp->self_v_rot);
}
...
if (inp->self_v_rot) {
    cur = llama_mul_mat_hadamard(ctx0, cur, inp->self_v_rot);
}
```

`self_k_rot`,`self_v_rot`只在`K`或`V cache`被量化 且`head_dim % 64 == 0`时创建，只会创建一次。通常`self_k_rot`,`self_v_rot`是二维矩阵`[n,n]`,其中`n`就是把`QKV`沿着`head_dim`切分后的块元素个数.

`self_k_rot`,`self_v_rot`是正交对称阵，用来旋转量化的QK，`KV cache`量化通常按一组数共用一个`scale`（block / tensor）。真实`K/V`常有少数通道特别大:

```text
k ≈ [0.1, 0.2, 12.0, 0.1, ...]   // 第 3 维是 outlier
```

`scale`会被12放大，同时其余值量化挡位很少，信息消失。所以使用旋转，让每个坐标比较均匀。

```CPP
// store to KV cache
{
    const auto & k_idxs = inp->get_k_idxs();
    const auto & v_idxs = inp->get_v_idxs();

    ggml_build_forward_expand(gf, mctx_cur->cpy_k(ctx0, k_cur, k_idxs, il));
    ggml_build_forward_expand(gf, mctx_cur->cpy_v(ctx0, v_cur, v_idxs, il));
}
```

存储`KV Cache`.

```CPP
ggml_tensor * q = q_cur;
ggml_tensor * k = mctx_cur->get_k(ctx0, il);
ggml_tensor * v = mctx_cur->get_v(ctx0, il);
```

读出完整整段kv.形状是`[head_dim, n_head_kv, n_kv, n_stream]`.

```CPP
ggml_tensor * llm_graph_context::build_attn_mha(
         ggml_tensor * q,
         ggml_tensor * k,
         ggml_tensor * v,
         ggml_tensor * kq_b,
         ggml_tensor * kq_mask,
         ggml_tensor * sinks,
         ggml_tensor * v_mla,
               float   kq_scale,
                 int   il) const {
    const bool v_trans = v->nb[1] > v->nb[2];

    // split the batch into streams if needed
    const auto n_stream = k->ne[3];

    q = ggml_view_4d(ctx0, q, q->ne[0], q->ne[1], q->ne[2]/n_stream, n_stream, q->nb[1], q->nb[2], q->nb[3]/n_stream, 0);

    q = ggml_permute(ctx0, q, 0, 2, 1, 3);
    k = ggml_permute(ctx0, k, 0, 2, 1, 3);
    v = ggml_permute(ctx0, v, 0, 2, 1, 3);

    ggml_tensor * cur;

    const bool use_flash_attn = cparams.flash_attn && kq_b == nullptr;
    if (use_flash_attn) {
        GGML_ASSERT(kq_b == nullptr && "Flash attention does not support KQ bias yet");

        if (v_trans) {
            v = ggml_transpose(ctx0, v);
        }

        // this can happen when KV cache is not used (e.g. an embedding model with non-causal attn)
        if (k->type == GGML_TYPE_F32) {
            k = ggml_cast(ctx0, k, GGML_TYPE_F16);
        }

        if (v->type == GGML_TYPE_F32) {
            v = ggml_cast(ctx0, v, GGML_TYPE_F16);
        }

        cur = ggml_flash_attn_ext(ctx0, q, k, v, kq_mask, kq_scale, hparams.f_max_alibi_bias,
                                  hparams.attn_soft_cap ? hparams.f_attn_logit_softcapping : 0.0f);
        res->add_fused_node({LLM_FUSED_OP_FLASH_ATTN, cur, il});

        ggml_flash_attn_ext_add_sinks(cur, sinks);
        ggml_flash_attn_ext_set_prec (cur, GGML_PREC_F32);

        if (v_mla) {
#if 0
            // v_mla can be applied as a matrix-vector multiplication with broadcasting across dimension 3 == n_tokens.
            // However, the code is optimized for dimensions 0 and 1 being large, so this is inefficient.
            cur = ggml_reshape_4d(ctx0, cur, v_mla->ne[0], 1, n_head, n_tokens);
            cur = ggml_mul_mat(ctx0, v_mla, cur);
#else
            // It's preferable to do the calculation as a matrix-matrix multiplication with n_tokens in dimension 1.
            // The permutations are noops and only change how the tensor data is interpreted.
            cur = ggml_permute(ctx0, cur, 0, 2, 1, 3);
            cur = ggml_mul_mat(ctx0, v_mla, cur);
            cb(cur, "fattn_mla", il);
            cur = ggml_permute(ctx0, cur, 0, 2, 1, 3);
            cur = ggml_cont(ctx0, cur); // Needed because ggml_reshape_2d expects contiguous inputs.
#endif
        }

        cur = ggml_reshape_2d(ctx0, cur, cur->ne[0]*cur->ne[1], cur->ne[2]*cur->ne[3]);
    } else {
        ggml_tensor * kq = ggml_mul_mat(ctx0, k, q);
        cb(kq, "kq", il);

        // note: this op tends to require high floating point range
        //       while for some models F16 is enough, for others it is not, so we default to F32 here
        ggml_mul_mat_set_prec(kq, GGML_PREC_F32);

        if (arch == LLM_ARCH_GROK) {
            // need to do the following:
            // multiply by attn_output_multiplier
            // and then :
            // kq = 30 * tanh(kq / 30)
            // before the softmax below

            kq = ggml_tanh(ctx0, ggml_scale(ctx0, kq, hparams.f_attn_out_scale / hparams.f_attn_logit_softcapping));
            cb(kq, "kq_tanh", il);
            kq = ggml_scale(ctx0, kq, hparams.f_attn_logit_softcapping);
            cb(kq, "kq_scaled", il);
        }

        if (hparams.attn_soft_cap) {
            kq = ggml_scale(ctx0, kq, 1.0f / hparams.f_attn_logit_softcapping);
            cb(kq, "kq_scaled_1", il);
            kq = ggml_tanh (ctx0, kq);
            cb(kq, "kq_tanh", il);
            kq = ggml_scale(ctx0, kq, hparams.f_attn_logit_softcapping);
            cb(kq, "kq_scaled_2", il);
        }

        if (kq_b) {
            kq = ggml_add(ctx0, kq, kq_b);
            cb(kq, "kq_plus_kq_b", il);
        }

        kq = ggml_soft_max_ext(ctx0, kq, kq_mask, kq_scale, hparams.f_max_alibi_bias);
        ggml_soft_max_add_sinks(kq, sinks);
        cb(kq, "kq_soft_max", il);

        if (!v_trans) {
            // note: avoid this branch
            v = ggml_cont(ctx0, ggml_transpose(ctx0, v));
            cb(v, "v_cont", il);
        }

        ggml_tensor * kqv = ggml_mul_mat(ctx0, v, kq);
        cb(kqv, "kqv", il);

        // for MLA with the absorption optimization, we need to "decompress" from MQA back to MHA
        if (v_mla) {
            kqv = ggml_mul_mat(ctx0, v_mla, kqv);
            cb(kqv, "kqv_mla", il);
        }

        cur = ggml_permute(ctx0, kqv, 0, 2, 1, 3);

        // recombine streams
        cur = ggml_cont_2d(ctx0, cur, cur->ne[0]*cur->ne[1], cur->ne[2]*cur->ne[3]);

        if (!cparams.offload_kqv) {
            // all nodes between the KV store and the attention output are run on the CPU
            ggml_backend_sched_set_tensor_backend(sched, cur, backend_cpu);
        }
    }

    ggml_build_forward_expand(gf, cur);

    return cur;
}
```

实际进行多头因果注意力计算.

$$
\operatorname{Attn}(Q,K,V)=\operatorname{softmax}(\operatorname{scale} \cdot QK^T+ \operatorname{mask})V
$$

* `q`,`Query`,进入时`[head_dim, n_head, n_tokens]`.
* `k`,`Key`,维度是`[head_dim, n_head_kv, n_kv, n_stream]`
* `v`,`value`，布局取决与是否开启flash attention来决定是否转置存放.如果转置存放，就是`[n_kv, n_head_kv, head_dim, n_stream]`,未转置则是`[head_dim, n_head_kv, n_kv, n_stream]`
* `kq_b`，`QK`的偏置.
* `kq_mask`,掩码`mask`.
* `sinks`,`Attention Sink`.
* `v_mla`,`MLA`吸收矩阵`wv_b`
* `kq_scale`,通常`1/sqrt(head_dim)`
* `il`层号

```CPP
const bool v_trans = v->nb[1] > v->nb[2];

// split the batch into streams if needed
const auto n_stream = k->ne[3];

q = ggml_view_4d(ctx0, q, q->ne[0], q->ne[1], q->ne[2]/n_stream, n_stream, q->nb[1], q->nb[2], q->nb[3]/n_stream, 0);

q = ggml_permute(ctx0, q, 0, 2, 1, 3);
k = ggml_permute(ctx0, k, 0, 2, 1, 3);
v = ggml_permute(ctx0, v, 0, 2, 1, 3);
```

检查`V`是否转置.将`q`view`[head_dim, n_head, n_seq_tokens, n_stream]`，也就是将每个流拆分开来.

重新排列`qkv`，`q`变为`[head_dim, n_seq_tokens, n_head, n_stream]`,`k`变为`[head_dim, n_kv, n_head_kv, n_stream]`,`v`变为`[n_kv, head_dim, n_head_kv, n_stream]`(已转置)，`[head_dim, n_kv, n_head_kv, n_stream]`(未转置).

```CPP
GGML_ASSERT(kq_b == nullptr && "Flash attention does not support KQ bias yet");

if (v_trans) {
    v = ggml_transpose(ctx0, v);
}

// this can happen when KV cache is not used (e.g. an embedding model with non-causal attn)
if (k->type == GGML_TYPE_F32) {
    k = ggml_cast(ctx0, k, GGML_TYPE_F16);
}

if (v->type == GGML_TYPE_F32) {
    v = ggml_cast(ctx0, v, GGML_TYPE_F16);
}

cur = ggml_flash_attn_ext(ctx0, q, k, v, kq_mask, kq_scale, hparams.f_max_alibi_bias,
                            hparams.attn_soft_cap ? hparams.f_attn_logit_softcapping : 0.0f);
res->add_fused_node({LLM_FUSED_OP_FLASH_ATTN, cur, il});

ggml_flash_attn_ext_add_sinks(cur, sinks);
ggml_flash_attn_ext_set_prec (cur, GGML_PREC_F32);

if (v_mla) {
#if 0
    // v_mla can be applied as a matrix-vector multiplication with broadcasting across dimension 3 == n_tokens.
    // However, the code is optimized for dimensions 0 and 1 being large, so this is inefficient.
    cur = ggml_reshape_4d(ctx0, cur, v_mla->ne[0], 1, n_head, n_tokens);
    cur = ggml_mul_mat(ctx0, v_mla, cur);
#else
    // It's preferable to do the calculation as a matrix-matrix multiplication with n_tokens in dimension 1.
    // The permutations are noops and only change how the tensor data is interpreted.
    cur = ggml_permute(ctx0, cur, 0, 2, 1, 3);
    cur = ggml_mul_mat(ctx0, v_mla, cur);
    cb(cur, "fattn_mla", il);
    cur = ggml_permute(ctx0, cur, 0, 2, 1, 3);
    cur = ggml_cont(ctx0, cur); // Needed because ggml_reshape_2d expects contiguous inputs.
#endif
}

cur = ggml_reshape_2d(ctx0, cur, cur->ne[0]*cur->ne[1], cur->ne[2]*cur->ne[3]);
```

开启了Flash Attention的流程，如果`V`已转置，将其恢复`[head_dim, n_kv, n_head_kv, n_stream]`.

`K/V`若是`F32`（例如`embedding`模型、不用`KV cache`）-> `cast`成`F16`，`kernel`只吃半精度。

经过`flash attention`后，变为`[head_dim, n_head, n_seq_tokens, n_stream]`

最后将输出变换回二维`[head_dim*n_head,n_seq_tokens*n_stream]`

```CPP
ggml_tensor * kq = ggml_mul_mat(ctx0, k, q);
cb(kq, "kq", il);

// note: this op tends to require high floating point range
//       while for some models F16 is enough, for others it is not, so we default to F32 here
ggml_mul_mat_set_prec(kq, GGML_PREC_F32);

if (arch == LLM_ARCH_GROK) {
    // need to do the following:
    // multiply by attn_output_multiplier
    // and then :
    // kq = 30 * tanh(kq / 30)
    // before the softmax below

    kq = ggml_tanh(ctx0, ggml_scale(ctx0, kq, hparams.f_attn_out_scale / hparams.f_attn_logit_softcapping));
    cb(kq, "kq_tanh", il);
    kq = ggml_scale(ctx0, kq, hparams.f_attn_logit_softcapping);
    cb(kq, "kq_scaled", il);
}

if (hparams.attn_soft_cap) {
    kq = ggml_scale(ctx0, kq, 1.0f / hparams.f_attn_logit_softcapping);
    cb(kq, "kq_scaled_1", il);
    kq = ggml_tanh (ctx0, kq);
    cb(kq, "kq_tanh", il);
    kq = ggml_scale(ctx0, kq, hparams.f_attn_logit_softcapping);
    cb(kq, "kq_scaled_2", il);
}

if (kq_b) {
    kq = ggml_add(ctx0, kq, kq_b);
    cb(kq, "kq_plus_kq_b", il);
}

kq = ggml_soft_max_ext(ctx0, kq, kq_mask, kq_scale, hparams.f_max_alibi_bias);
ggml_soft_max_add_sinks(kq, sinks);
cb(kq, "kq_soft_max", il);

if (!v_trans) {
    // note: avoid this branch
    v = ggml_cont(ctx0, ggml_transpose(ctx0, v));
    cb(v, "v_cont", il);
}

ggml_tensor * kqv = ggml_mul_mat(ctx0, v, kq);
cb(kqv, "kqv", il);

// for MLA with the absorption optimization, we need to "decompress" from MQA back to MHA
if (v_mla) {
    kqv = ggml_mul_mat(ctx0, v_mla, kqv);
    cb(kqv, "kqv_mla", il);
}

cur = ggml_permute(ctx0, kqv, 0, 2, 1, 3);

// recombine streams
cur = ggml_cont_2d(ctx0, cur, cur->ne[0]*cur->ne[1], cur->ne[2]*cur->ne[3]);

if (!cparams.offload_kqv) {
    // all nodes between the KV store and the attention output are run on the CPU
    ggml_backend_sched_set_tensor_backend(sched, cur, backend_cpu);
}
```

标准路径的attention计算.

```CPP
ggml_tensor * kq = ggml_mul_mat(ctx0, k, q);
cb(kq, "kq", il);

// note: this op tends to require high floating point range
//       while for some models F16 is enough, for others it is not, so we default to F32 here
ggml_mul_mat_set_prec(kq, GGML_PREC_F32);
```

进行矩阵乘，`q`维度是`[head_dim, n_seq_tokens, n_head, n_stream]`,`k`维度是`[head_dim, n_kv, n_head_kv, n_stream]`,由于`GQA`设计，`n_head_kv`小于`n_head`,会自动进行广播，结果是`kq`的维度是`[n_kv, n_seq_tokens, n_head, n_stream]`，由于相对于标准公式$QK^T$，`k`,`q`是行优先的向量所以是$kq^T$。

使用`GGML_PREC_F32`提高精度，防止溢出.

```CPP
kq = ggml_soft_max_ext(ctx0, kq, kq_mask, kq_scale, hparams.f_max_alibi_bias);
ggml_soft_max_add_sinks(kq, sinks);
```

`ggml_soft_max_ext`进行`softmax`.加入`kq_mask`,`kq_scale`这些.

```CPP
ggml_tensor * kqv = ggml_mul_mat(ctx0, v, kq);
cb(kqv, "kqv", il);
```

通常这个流程中`v`已经是转置存储了,维度是`[n_kv, head_dim, n_head_kv, n_stream]`,计算`kqv`,维度是`[head_dim, n_seq_tokens, n_head_kv, n_stream]`,同样也是相对于标准公式$\operatorname{softmax}(QK^T)V$,这个是$v\operatorname{softmax}(kq^T)^T$.

```CPP
cur = ggml_permute(ctx0, kqv, 0, 2, 1, 3);

// recombine streams
cur = ggml_cont_2d(ctx0, cur, cur->ne[0]*cur->ne[1], cur->ne[2]*cur->ne[3]);
```

重排`kqv`为`[head_dim, n_head_kv, n_seq_tokens, n_stream]`,同时恢复为二维`[head_dim * n_head_kv, n_seq_tokens * n_stream]`并返回

```CPP
ggml_tensor * gate_sigmoid = ggml_sigmoid(ctx0, gate);
cb(gate_sigmoid, "gate_sigmoid", il);

cur = ggml_mul(ctx0, cur, gate_sigmoid);
cb(cur, "attn_gated", il);

cur = build_lora_mm(model.layers[il].wo, cur, model.layers[il].wo_s);
cb(cur, "attn_output", il);
```

计算`gate`激活函数，乘上输出，最后进行输出投影，`wo`的维度是`[n_head * head_dim, n_embd]`,返回`[n_embd,n_tokens]`,保证后续计算维度正确.

#### build_layer_ffn

##### 源码

```CPP
ggml_tensor * llama_model_qwen35::graph::build_layer_ffn(ggml_tensor * cur, const int il) {
    // Qwen3.5 does not use MoE FFN
    GGML_ASSERT(model.layers[il].ffn_gate_inp == nullptr);

    cur = build_ffn(cur,
        model.layers[il].ffn_up, NULL, model.layers[il].ffn_up_s,
        model.layers[il].ffn_gate, NULL, model.layers[il].ffn_gate_s,
        model.layers[il].ffn_down, NULL, model.layers[il].ffn_down_s,
        NULL,
        LLM_FFN_SILU, LLM_FFN_PAR, il);
    cb(cur, "ffn_out", il);

    return cur;
}
```

##### 分析

```CPP
ggml_tensor * llm_graph_context::build_ffn(
         ggml_tensor * cur,
         ggml_tensor * up,
         ggml_tensor * up_b,
         ggml_tensor * up_s,
         ggml_tensor * gate,
         ggml_tensor * gate_b,
         ggml_tensor * gate_s,
         ggml_tensor * down,
         ggml_tensor * down_b,
         ggml_tensor * down_s,
         ggml_tensor * act_scales,
     llm_ffn_op_type   type_op,
   llm_ffn_gate_type   type_gate,
                 int   il) const {
    // NVFP4 support is currently restricted to
    // 1) LORA absence (*_s would be applied after LORA residual, which is incorrect)
    // 2) bias absense (*_s would be applied after bias addition, which is incorrect)
    // TODO: disambiguate LLM-architectural scales (which use *_s) from NVFP4 scale_2 (which also uses *_s currently)
    auto has_lora = [this](ggml_tensor * w) {
        if (!w) {
            return false;
        }
        for (const auto & lora : *loras) {
            if (lora.first->get_weight(w) != nullptr) {
                return true;
            }
        }
        return false;
    };

    GGML_ASSERT(!up_s   || !up_b   || !up   || up->type   != GGML_TYPE_NVFP4);
    GGML_ASSERT(!gate_s || !gate_b || !gate || gate->type != GGML_TYPE_NVFP4);
    GGML_ASSERT(!down_s || !down_b || !down || down->type != GGML_TYPE_NVFP4);
    GGML_ASSERT(!up_s   || !up   || up->type   != GGML_TYPE_NVFP4 || !has_lora(up));
    GGML_ASSERT(!gate_s || !gate || gate->type != GGML_TYPE_NVFP4 || !has_lora(gate));
    GGML_ASSERT(!down_s || !down || down->type != GGML_TYPE_NVFP4 || !has_lora(down));

    ggml_tensor * tmp = up ? build_lora_mm(up, cur) : cur;
    cb(tmp, "ffn_up", il);

    if (up_b) {
        tmp = ggml_add(ctx0, tmp, up_b);
        cb(tmp, "ffn_up_b", il);
    }

    if (up_s) {
        tmp = ggml_mul(ctx0, tmp, up_s);
        cb(tmp, "ffn_up_s", il);
    }

    if (gate) {
        switch (type_gate) {
            case LLM_FFN_SEQ:
                {
                    cur = build_lora_mm(gate, tmp);
                    cb(cur, "ffn_gate", il);
                } break;
            case LLM_FFN_PAR:
                {
                    cur = build_lora_mm(gate, cur);
                    cb(cur, "ffn_gate", il);
                } break;
        }

        if (gate_b) {
            cur = ggml_add(ctx0, cur, gate_b);
            cb(cur, "ffn_gate_b", il);
        }

        if (gate_s) {
            cur = ggml_mul(ctx0, cur, gate_s);
            cb(cur, "ffn_gate_s", il);
        }

    } else {
        cur = tmp;
    }

    switch (type_op) {
        case LLM_FFN_SILU:
            if (gate && type_gate == LLM_FFN_PAR) {
                if (il >= 0) {
                    const float limit = hparams.swiglu_clamp_shexp[il];
                    constexpr float eps = 1e-6f;
                    if (limit > eps) {
                        tmp = ggml_clamp(ctx0, tmp, -limit, limit);
                        cb(tmp, "ffn_up_clamped", il);

                        if (arch == LLM_ARCH_DEEPSEEK4 || (arch == LLM_ARCH_DFLASH && hparams.dsv4_hc_mult > 0)) {
                            cur = ggml_clamp(ctx0, cur, -INFINITY, limit);
                            cb(cur, "ffn_gate_clamped", il);
                            cur = ggml_swiglu_split(ctx0, cur, tmp);
                        } else {
                            ggml_tensor * gate_act = ggml_silu(ctx0, cur);
                            cb(gate_act, "ffn_silu", il);
                            gate_act = ggml_clamp(ctx0, gate_act, -INFINITY, limit);
                            cb(gate_act, "ffn_silu_clamped", il);
                            cur = ggml_mul(ctx0, gate_act, tmp);
                        }
                        cb(cur, "ffn_swiglu_limited", il);
                        type_gate = LLM_FFN_SEQ;
                        break;
                    }
                }

                cur = ggml_swiglu_split(ctx0, cur, tmp);
                cb(cur, "ffn_swiglu", il);
                type_gate = LLM_FFN_SEQ;
            } else {
                cur = ggml_silu(ctx0, cur);
                cb(cur, "ffn_silu", il);
            } break;
        case LLM_FFN_GELU:
            if (gate && type_gate == LLM_FFN_PAR) {
                cur = ggml_geglu_split(ctx0, cur, tmp);
                cb(cur, "ffn_geglu", il);
                type_gate = LLM_FFN_SEQ;
            } else {
                cur = ggml_gelu(ctx0, cur);
                cb(cur, "ffn_gelu", il);
                if (act_scales != NULL) {
                    cur = ggml_div(ctx0, cur, act_scales);
                    cb(cur, "ffn_act", il);
                }
            } break;
        case LLM_FFN_RELU:
            if (gate && type_gate == LLM_FFN_PAR) {
                cur = ggml_reglu_split(ctx0, cur, tmp);
                cb(cur, "ffn_reglu", il);
                type_gate = LLM_FFN_SEQ;
            } else {
                cur = ggml_relu(ctx0, cur);
                cb(cur, "ffn_relu", il);
            } break;
        case LLM_FFN_RELU_SQR:
            {
                cur = ggml_relu(ctx0, cur);
                cb(cur, "ffn_relu", il);

                cur = ggml_sqr(ctx0, cur);
                cb(cur, "ffn_sqr(relu)", il);
            } break;
        case LLM_FFN_SWIGLU:
            {
                cur = ggml_swiglu(ctx0, cur);
                cb(cur, "ffn_swiglu", il);
            } break;
        case LLM_FFN_SWIGLU_OAI_MOE:
            if (gate && type_gate == LLM_FFN_PAR) {
                // same alpha/limit constants as gpt-oss
                const float alpha = 1.702f;
                const float limit = 7.0f;
                cur = ggml_swiglu_oai(ctx0, cur, tmp, alpha, limit);
                cb(cur, "ffn_swiglu_oai", il);
                type_gate = LLM_FFN_SEQ;
            } else {
                GGML_ABORT("LLM_FFN_SWIGLU_OAI_MOE requires a parallel gate");
            } break;
        case LLM_FFN_GEGLU:
            {
                cur = ggml_geglu(ctx0, cur);
                cb(cur, "ffn_geglu", il);
            } break;
        case LLM_FFN_REGLU:
            {
                cur = ggml_reglu(ctx0, cur);
                cb(cur, "ffn_reglu", il);
            } break;
        default:
            GGML_ABORT("fatal error");
    }

    if (gate && type_gate == LLM_FFN_PAR) {
        cur = ggml_mul(ctx0, cur, tmp);
        cb(cur, "ffn_gate_par", il);
    }

    if (down) {
        cur = build_lora_mm(down, cur);
        if (arch == LLM_ARCH_GLM4 || arch == LLM_ARCH_GLM4_MOE || arch == LLM_ARCH_JAIS2) {
            // GLM4, GLM4_MOE, and JAIS2 seem to have numerical issues with half-precision accumulators
            ggml_mul_mat_set_prec(cur, GGML_PREC_F32);
        }
    }

    if (down_b) {
        cb(cur, "ffn_down", il);
    }

    if (down_b) {
        cur = ggml_add(ctx0, cur, down_b);
    }

    if (down_s) {
        cur = ggml_mul(ctx0, cur, down_s);
        cb(cur, "ffn_down_s", il);
    }

    return cur;
}
```

* `cur`输入张量，维度是`[n_embd, n_tokens]`.
* `up`,$W_{up}$.维度是`[n_embd, n_ff]`
* `gate`,$W_{gate}$,维度是`[n_embd, n_ff]`.
* `down`,$W_{down}$,`[n_ff, n_embd]`

`build_ffn`实际进行多种`FFN`计算.对于当前的来说就是

$$
\operatorname{FFN}(x) = W_{down}(\operatorname{SiLU}(W_{gate}x) \odot W_{up}x)
$$

```CPP
ggml_tensor * tmp = up ? build_lora_mm(up, cur) : cur;
// ...
if (gate) {
    switch (type_gate) {
        case LLM_FFN_PAR:
            {
                cur = build_lora_mm(gate, cur);
            } break;
    }
    // gate_b / gate_s if present
}
switch (type_op) {
    case LLM_FFN_SILU:
        if (gate && type_gate == LLM_FFN_PAR) {
            cur = ggml_swiglu_split(ctx0, cur, tmp);
            type_gate = LLM_FFN_SEQ;
        }
        // ...
}
if (down) {
    cur = build_lora_mm(down, cur);
}
```

先计算$W_{up}x$，再计算$W_{gate}x$.

`ggml_swiglu_split`是一个融合算子，它计算$\operatorname{SiLU}(cur)\odot tmp$

## 视频解码器

## 音频解码器