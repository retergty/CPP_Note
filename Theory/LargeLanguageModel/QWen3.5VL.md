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
    |                               |
build_inp_pos()               build_inp_out_ids()
构造 MRoPE 位置索引          指定需要输出的 token 行
    |                               |
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
        /                           \
       / true                       \ false
      v                              v
build_layer_attn_linear()      build_layer_attn()
执行 Gated DeltaNet            执行 Full Attention
使用卷积状态和矩阵状态          使用 Q/K/V、MRoPE 和 KV cache
      \                              /
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
          |                       |
          v                       v
res->t_h_nextn = cur       ggml_get_rows() [可选]
保存给 MTP/NextN 使用       只保留需要生成 logits 的 token
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

`Qwen3.5 Dense`架构的核心特点是`混合注意力 + 门控 + 稠密 FFN`

* 混合注意力架构: 每`4`层一组,一组中`3`层`Gated DeltaNet`加`1`层`Full Attention`.
  * `DeltaNet`层用固定大小`recurrent state`压缩历史信息.
  * `Full Attention`层保留全局`token-to-token`交互

## 视频解码器

## 音频解码器