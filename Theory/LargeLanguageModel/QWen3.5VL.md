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

## 视频解码器

## 音频解码器
