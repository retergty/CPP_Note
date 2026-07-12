# llama.cpp 与 LLM 推理源码学习笔记

## 1. 学习目标

阅读 llama.cpp 的重点不是记住每个函数，而是回答下面几个问题：

1. 文本如何变成 token？
2. GGUF 文件中的模型参数如何加载为 GGML tensor？
3. 一个 Transformer 层如何被构造成计算图？
4. prompt 为什么可以一次处理，而生成阶段通常逐 token 处理？
5. KV cache 保存了什么，为什么它能显著减少计算？
6. logits 如何经过采样变成下一个 token？
7. 同一张计算图如何在 CPU、CUDA、Vulkan 等后端执行？
8. 量化权重如何参与矩阵乘法？

建议始终围绕一条最小推理链学习：

```text
输入文本
-> tokenizer
-> token ID
-> llama_decode()
-> 构造 Transformer 计算图
-> GGML backend 执行计算图
-> logits
-> sampler
-> 新 token ID
-> token_to_piece()
-> 输出文本
-> 将新 token 送入下一轮 decode
```

## 2. 阅读前需要掌握的知识

### 2.1 C/C++ 基础

至少应熟悉：

- 指针、数组和内存生命周期
- struct、class、继承和虚函数
- RAII、`std::unique_ptr`、`std::vector`
- 函数指针和回调
- 模板的基本用法
- CMake 的 target、include 和 link 概念
- 多线程的基本概念

不需要先成为 C++ 专家。遇到语言细节时单独查询即可，不要因此中断主调用链。

### 2.2 LLM 推理基础

建议先理解以下概念：

- tokenization
- embedding
- RMSNorm
- self-attention
- Query、Key、Value
- GQA 和 MQA
- RoPE
- residual connection
- SwiGLU FFN
- logits 和 softmax
- autoregressive generation
- KV cache
- weight quantization

对于 Llama 类模型，一个简化的 Transformer 层可以写成：

```text
x
-> RMSNorm
-> Q/K/V projection
-> RoPE(Q, K)
-> Attention(Q, K, V)
-> output projection
-> residual add
-> RMSNorm
-> SwiGLU FFN
-> residual add
```

注意力的核心公式是：

```math
Attention(Q,K,V)=softmax(QK^T/\sqrt{d})V
```

Llama 的 FFN 可以简化理解为：

```math
FFN(x)=W_{down}(SiLU(W_{gate}x)\odot W_{up}x)
```

阅读源码时，不必先推导全部数学细节。先知道每个 tensor 的作用，再回头补充公式。

## 3. 当前仓库的分层

当前项目大致分为三层。

### 3.1 应用层

主要目录：

- `examples/`
- `tools/`
- `common/`
- `app/`

这一层负责命令行参数、聊天模板、请求调度、输出和用户交互。

### 3.2 llama 推理层

主要文件：

- `include/llama.h`
- `src/llama.cpp`
- `src/llama-model.cpp`
- `src/llama-context.cpp`
- `src/llama-graph.cpp`
- `src/llama-vocab.cpp`
- `src/llama-sampler.cpp`
- `src/models/*.cpp`

这一层负责模型语义，例如模型结构、tokenizer、采样、KV cache 和 decode。

### 3.3 GGML 执行层

主要目录：

- `ggml/include/`
- `ggml/src/`

这一层负责 tensor、计算图、内存分配、backend 调度和具体算子执行。

可以把三层关系理解为：

```text
examples/tools
调用 llama API

llama
理解模型结构并构造计算图

GGML
分配并执行计算图
```

`src/CMakeLists.txt` 是观察 llama 库组成的好入口。当前模型架构实现通过 `src/models/*.cpp` 加入构建。

## 4. 第一阶段：跑通最小推理程序

### 4.1 首先阅读的文件

```text
examples/simple/simple.cpp
```

不要先从 `tools/cli/cli.cpp` 开始。当前 CLI 复用了 server 基础设施，包含大量与基础推理无关的逻辑。`examples/simple/simple.cpp` 才是最清晰的推理入口。

### 4.2 按下面的顺序阅读

#### 加载 backend

```cpp
ggml_backend_load_all();
```

它加载当前构建中可用的 CPU、CUDA、Vulkan 等 backend。没有 backend，模型即使能读入，也无法执行。

#### 加载模型

```cpp
llama_model_params model_params = llama_model_default_params();
model_params.n_gpu_layers = ngl;
llama_model * model = llama_model_load_from_file(model_path.c_str(), model_params);
```

`llama_model` 主要保存：

- 模型超参数
- vocabulary
- 权重 tensor
- backend buffer
- 模型架构类型

它不保存某个会话当前已经处理到哪个位置。

#### 获取 vocabulary 并分词

```cpp
const llama_vocab * vocab = llama_model_get_vocab(model);
llama_tokenize(...);
```

示例第一次调用 `llama_tokenize()` 时没有提供输出数组，用负返回值取得所需 token 数量。第二次分配数组并真正写入 token。

#### 创建 context

```cpp
llama_context_params ctx_params = llama_context_default_params();
llama_context * ctx = llama_init_from_model(model, ctx_params);
```

`llama_context` 表示一次推理上下文，主要拥有：

- backend scheduler
- graph 相关缓冲区
- logits 和 embedding 输出缓冲区
- KV cache 或其他模型 memory
- batch、sequence 和性能状态

同一个 `llama_model` 可以对应多个 `llama_context`。

#### 创建 sampler

```cpp
llama_sampler * smpl = llama_sampler_chain_init(...);
llama_sampler_chain_add(smpl, llama_sampler_init_greedy());
```

sampler 的输入是 logits，输出是 token ID。greedy sampler 总是选择 logit 最大的 token。

#### 处理 prompt 和生成 token

```cpp
llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

llama_decode(ctx, batch);
llama_token new_token = llama_sampler_sample(smpl, ctx, -1);
batch = llama_batch_get_one(&new_token, 1);
```

第一次 `llama_decode()` 通常处理完整 prompt。之后每次把刚采样出的一个 token 作为新输入。

### 4.3 最容易误解的地方

采样出的 token 来自当前 logits，可以立即转换为文本输出。然后这个 token 才被作为下一轮模型输入，用于预测再下一个 token。

```text
prompt 的最后位置产生 logits_1
-> 采样 token_1
-> 输出 token_1
-> decode(token_1)
-> 产生 logits_2
-> 采样 token_2
```

### 4.4 第一阶段练习

1. 使用 greedy sampler 连续运行两次，确认输出一致。
2. 打印 prompt 中每个 token 的 ID 和 piece。
3. 打印每轮 `batch.n_tokens`。
4. 将 `n_ctx` 设置得小于 prompt 长度，观察错误。
5. 在 `llama_decode()` 和 `llama_sampler_sample()` 上设置断点。

完成标准：能够不看源码解释 `simple.cpp` 的完整生命周期。

## 5. 第二阶段：理解公开 C API

### 5.1 阅读顺序

```text
include/llama.h
src/llama.cpp
src/llama-model.h
src/llama-context.h
```

`include/llama.h` 是公开 API。不要第一次就逐行阅读，应按功能分组。

### 5.2 重点对象

#### `llama_model`

不可见的内部实现对象，表示已经加载的模型。

#### `llama_context`

表示推理运行状态。模型权重相对静态，context 中的 memory 和输出状态会随推理变化。

#### `llama_vocab`

保存 token 与文本 piece 的映射，以及 tokenizer 所需规则。

#### `llama_batch`

描述一次送给模型的数据。它不仅可以包含 token，还可以包含：

- position
- sequence ID
- 哪些位置需要输出 logits
- embedding 输入

所以它比一个简单的 token 数组更通用。

#### `llama_sampler`

由接口和私有状态组成，可以单独使用，也可以组成 sampler chain。

### 5.3 API 分组

优先阅读：

1. 默认参数函数
2. 模型加载和释放
3. context 创建和释放
4. tokenizer 和 detokenizer
5. batch、encode 和 decode
6. logits 读取
7. sampler
8. memory 和 sequence 操作
9. state 保存与恢复

`src/llama.cpp` 主要实现公开 API 到内部 C++ 对象的转发。真正复杂的 decode 位于 `src/llama-context.cpp`。

### 5.4 第二阶段练习

画出以下对象的所有权关系：

```text
model
|- vocab
|- weights
`- backend buffers

context
|- reference to model
|- scheduler
|- model memory / KV cache
`- output buffers

sampler
`- sampler-specific state
```

完成标准：能够区分 model、context、batch 和 sampler 的职责。

## 6. 第三阶段：GGUF 与模型加载

### 6.1 阅读顺序

```text
ggml/include/gguf.h
ggml/src/gguf.cpp
src/llama-model-loader.h
src/llama-model-loader.cpp
src/llama-arch.h
src/llama-arch.cpp
src/llama-model.cpp
src/models/llama.cpp
```

### 6.2 GGUF 中包含什么

可以将 GGUF 简化理解为三部分：

```text
header
metadata key/value
tensor metadata + tensor data
```

metadata 记录：

- 模型架构
- 层数
- embedding 维度
- attention head 数
- context length
- tokenizer 类型和 vocabulary
- RoPE 参数
- chat template
- 量化相关信息

tensor metadata 记录：

- tensor 名称
- shape
- data type
- 数据偏移

真正的权重数据位于文件的数据区。

### 6.3 模型加载调用链

```text
llama_model_load_from_file()
-> llama_model_load_from_file_impl()
-> llama_model_load()
-> llama_model_loader
-> llama_model_create()
-> llama_model_base::load_hparams()
-> llama_model_base::load_vocab()
-> llama_model_base::load_tensors()
-> 各模型的 load_arch_tensors()
-> llama_model_loader::load_all_data()
```

### 6.4 三个容易混淆的层次

#### `gguf.cpp`

只负责理解 GGUF 文件本身，不理解 Transformer 的语义。

#### `llama_model_loader`

负责把 GGUF 中的 tensor 映射到 mmap、CPU buffer 或 GPU buffer。

其中 `create_tensor()` 很关键，它负责：

- 根据名称查找 tensor
- 检查维度
- 选择 buffer type
- 建立 GGML tensor 描述
- 处理可选 tensor 和重复 tensor

#### 模型架构类

模型类知道每一个权重在网络中的语义。例如 `src/models/llama.cpp` 中：

- `tok_embd` 是 token embedding
- `attn_norm` 是 attention 前的 RMSNorm
- Q/K/V 权重用于投影
- `wo` 是 attention output projection
- `ffn_gate`、`ffn_up`、`ffn_down` 构成 FFN
- `output` 是最终 lm head

### 6.5 `src/llama-arch.cpp` 应该如何阅读

该文件主要是：

- 架构枚举到名称的映射
- GGUF metadata key 模板
- tensor 枚举到 GGUF tensor 名称的映射

它是一份字典，不是一条执行流程。因此应在遇到某个 key 或 tensor 名称时查询，而不是从第一行顺序通读。

### 6.6 第三阶段练习

选择一个 Llama GGUF 文件，用项目中的 GGUF 工具查看 metadata，然后回答：

1. 模型有多少层？
2. `n_embd`、`n_head`、`n_head_kv` 分别是多少？
3. 模型使用什么 tokenizer？
4. 权重使用了哪些 GGML type？
5. `blk.0.attn_q.weight` 的 shape 是什么？

完成标准：能够从一个 GGUF tensor 名称找到对应的模型成员。

## 7. 第四阶段：tokenizer

### 7.1 阅读顺序

```text
src/llama-vocab.h
src/llama-vocab.cpp
src/unicode.h
src/unicode.cpp
```

### 7.2 重点函数

```text
llama_vocab::impl::load()
llama_vocab::impl::init_tokenizer()
llama_vocab::impl::tokenize()
llama_vocab::impl::token_to_piece()
llama_vocab::impl::detokenize()
```

### 7.3 当前支持的主要 tokenizer

源码中包含多种 tokenizer 路径，例如：

- SPM
- BPE
- WPM
- UGM
- RWKV
- PLAMO2

不要第一次就研究全部算法。先查看 `init_tokenizer()` 和 `tokenize()` 如何分派，然后只跟踪当前模型使用的实现。

### 7.4 重要参数

#### `add_special`

控制是否自动加入 BOS、EOS 等特殊 token。

#### `parse_special`

控制文本中的特殊 token 字符串是否应被识别为特殊 token。

#### `token_to_piece`

将单个 token 转为原始字节片段。一个 token 的 piece 不一定是完整 Unicode 字符。

#### `detokenize`

处理连续 token、特殊 token、空格和字节拼接规则。

### 7.5 第四阶段练习

对同一句中英文混合文本：

1. 打印 token ID。
2. 打印每个 token 的 piece 和字节长度。
3. 比较 `add_special=true/false`。
4. 将所有 piece 拼接，观察是否恢复原文。

完成标准：理解 token、piece、字符和字节并不是一一对应关系。

## 8. 第五阶段：sampling

### 8.1 阅读顺序

```text
include/llama.h 中的 Sampling API
src/llama-sampler.h
src/llama-sampler.cpp
common/sampling.cpp
```

先读底层 sampler，再读 `common/` 提供的应用层组合。

### 8.2 普通 sampling 调用链

```text
llama_sampler_sample()
-> llama_get_logits_ith()
-> 构造 llama_token_data_array
-> llama_sampler_chain_apply()
-> sampler chain 逐个修改候选
-> 最终 sampler 选择 selected token
-> llama_sampler_accept()
```

### 8.3 推荐阅读顺序

1. greedy
2. temperature
3. top-k
4. top-p
5. distribution sampling
6. repetition penalties
7. grammar
8. mirostat、DRY 等高级 sampler

### 8.4 sampler chain 的意义

不同 sampler 的职责不同：

```text
raw logits
-> repetition penalty
-> temperature
-> top-k
-> top-p
-> probability distribution sampling
-> selected token
```

greedy、distribution、mirostat 通常承担最终选择。temperature、top-k 和 top-p 通常只修改或裁剪候选。

### 8.5 第五阶段练习

固定 prompt 和随机种子，对比：

- greedy
- temperature + top-k + top-p + dist
- 不同 temperature
- 不同 repetition penalty

记录前几个候选 token 的 logit 和概率，观察各 sampler 如何改变分布。

完成标准：能够解释 temperature 不负责随机数生成，top-k 也不直接选择最终 token。

## 9. 第六阶段：GGML tensor 与计算图

### 9.1 阅读顺序

```text
ggml/include/ggml.h
ggml/src/ggml.c
```

第一次只关注：

- `ggml_context`
- `ggml_tensor`
- `ggml_cgraph`
- `ggml_init()`
- tensor 创建函数
- `ggml_mul_mat()`
- `ggml_add()`
- `ggml_build_forward_expand()`

### 9.2 `ggml_context`

它主要是 tensor 和 graph 元数据的 arena，不等同于 `llama_context`。

```text
llama_context
表示完整的模型推理会话

ggml_context
用于集中创建和管理 GGML 对象的元数据
```

### 9.3 `ggml_tensor`

一个 tensor 不只表示一块数据，还可以表示计算图中的一个节点。它包含：

- data type
- 维度和 shape
- strides
- data 指针
- op 类型
- source tensor
- backend buffer 信息

### 9.4 构图不等于立即执行

调用：

```cpp
ggml_mul_mat(ctx, a, b);
```

通常只是创建一个 `GGML_OP_MUL_MAT` 节点，并记录 `a`、`b` 是它的输入。此时矩阵乘可能尚未执行。

`ggml_build_forward_expand()` 从输出 tensor 递归访问依赖，建立拓扑有序的计算图。

真正执行由 backend scheduler 发起。

### 9.5 shape 阅读方法

GGML 的维度顺序和常见 Python 框架的直觉可能不同。阅读每个算子时都要检查：

- `ne[]` 表示什么
- `nb[]` 的 stride 是什么
- 哪个 tensor 被当作权重
- 是否存在 view、reshape、permute
- 量化类型是否只允许出现在特定位置

不要只根据变量名猜测 shape。

### 9.6 第六阶段练习

编写或查找一个最小 GGML 示例，构造：

```text
C = A x B
D = C + bias
```

在 graph 构建前后打印 tensor 的：

- name
- type
- shape
- op
- sources

完成标准：能够区分 tensor 元数据创建、数据分配和算子执行三个时刻。

## 10. 第七阶段：Llama 计算图

### 10.1 阅读顺序

```text
src/llama-graph.h
src/llama-graph.cpp
src/models/models.h
src/models/llama.cpp
```

### 10.2 模型类的三个核心职责

每个模型架构主要回答三个问题：

1. `load_arch_hparams()`：该模型需要哪些超参数？
2. `load_arch_tensors()`：该模型需要哪些权重和 shape？
3. `build_arch_graph()`：如何使用这些权重构建计算图？

### 10.3 `src/models/llama.cpp` 的图构建顺序

当前 Llama graph 的主线是：

```text
build_inp_embd()
build_inp_pos()
build_attn_inp_kv()

for each layer:
    RMSNorm
    build_qkv()
    RoPE(Q)
    RoPE(K)
    build_attn()
    residual add
    RMSNorm
    build_ffn() 或 build_moe_ffn()
    residual add

final RMSNorm
output projection
logits
ggml_build_forward_expand()
```

### 10.4 通用 graph helper

`src/llama-graph.cpp` 中的重要 helper 包括：

- `build_inp_embd`
- `build_inp_pos`
- `build_norm`
- `build_lora_mm`
- `build_qkv`
- `build_attn`
- `build_ffn`
- `build_moe_ffn`
- `build_out_ids`

模型文件描述网络结构，通用 helper 负责复用构图模式。

### 10.5 阅读 tensor shape 的建议

每经过一个步骤都在纸上记录：

```text
变量名
语义
GGML shape
数学 shape
产生它的 op
消费它的下一个 op
```

第一遍只跟踪单 batch、单 sequence、单 token，避免立即陷入广播和并行 sequence 的细节。

### 10.6 模型架构的后续顺序

理解 Llama 后再选择代表模型：

1. `src/models/qwen2.cpp`：另一个 dense decoder
2. `src/models/qwen3moe.cpp`：MoE
3. `src/models/deepseek2.cpp`：更复杂的 attention 和 MoE
4. `src/models/mamba.cpp`：recurrent/SSM
5. `src/models/jamba.cpp`：hybrid
6. `src/models/bert.cpp`：encoder-only
7. `src/models/t5.cpp`：encoder-decoder
8. `src/models/qwen2vl.cpp` 或 `qwen3vl.cpp`：多模态扩展

不要通读全部 `src/models/*.cpp`。应选择一种架构做对比学习。

完成标准：能够把 `src/models/llama.cpp` 的主要构图语句对应到 Transformer 结构图。

## 11. 第八阶段：decode、batch 与 KV cache

### 11.1 阅读顺序

```text
src/llama-batch.h
src/llama-batch.cpp
src/llama-memory.h
src/llama-context.h
src/llama-context.cpp
src/llama-kv-cache.h
src/llama-kv-cache.cpp
```

### 11.2 batch 与 micro-batch

`llama_batch` 是调用者提交的逻辑 batch。

内部可能将其拆成一个或多个 `llama_ubatch`，原因包括：

- 限制一次构图和执行的 token 数
- 不同 sequence 的组织方式
- memory slot 可用情况
- backend 资源限制

所以 `n_batch` 和 `n_ubatch` 不是同一个概念。

### 11.3 decode 主调用链

```text
llama_decode()
-> llama_context::decode()
-> llama_batch_allocr::init()
-> memory update
-> memory->init_batch()
-> batch 拆成 llama_ubatch
-> llama_context::process_ubatch()
-> memory context apply
-> model.build_graph()
-> graph allocation
-> graph compute
-> 复制 logits/embedding 到输出 buffer
```

建议在 `src/llama-context.cpp` 中先定位：

```text
llama_context::decode()
llama_context::process_ubatch()
llama_context::graph_compute()
```

不要从文件第一行开始通读。

### 11.4 为什么需要 KV cache

生成第一个 token 时，模型已经为 prompt 中所有位置计算过 Key 和 Value。

预测下一个 token 时：

- 新 token 只需要计算自己的 Q、K、V
- 旧 token 的 K、V 可以直接从 cache 读取
- 新 Q 与所有历史 K 做 attention
- attention 权重再与所有历史 V 相乘

如果没有 KV cache，每生成一个 token 都需要重新计算整个前缀。

### 11.5 KV cache 保存什么

对于每一层，主要保存历史 token 的：

- Key
- Value
- position 和 sequence 相关元数据

它通常不保存 Query，因为旧位置的 Query 不会在后续生成中再次使用。

### 11.6 当前 memory 抽象

当前项目不再假设所有模型都使用相同 KV cache。统一接口是 `llama_memory_i`，具体可能是：

- 普通 attention KV cache
- sliding-window attention cache
- recurrent memory
- hybrid memory
- 特殊架构专用 cache

普通 Llama 路径重点阅读：

```text
llama_kv_cache::init_batch()
llama_kv_cache::prepare()
llama_kv_cache::find_slot()
llama_kv_cache_context::apply()
llama_kv_cache_context::next()
```

### 11.7 prefill 与 decode

常见术语：

```text
prefill
一次处理 prompt 中的多个 token，主要受计算吞吐影响

decode
逐个生成新 token，频繁读取 KV cache，主要受内存带宽和延迟影响
```

这也是为什么 prompt processing 的 tokens/s 往往明显高于 generation 的 tokens/s。

### 11.8 第八阶段练习

1. 分别记录 prompt processing 和 generation 速度。
2. 打印每轮 batch 和 ubatch 大小。
3. 比较短 prompt 与长 prompt 的首 token 延迟。
4. 估算 KV cache 大小随层数、context 和 KV head 数如何增长。
5. 跟踪一个新 token 如何找到 cache slot。

完成标准：能够解释 KV cache 优化了什么，以及它没有优化什么。

## 12. 第九阶段：backend scheduler 与 CPU 执行

### 12.1 阅读顺序

```text
ggml/include/ggml-backend.h
ggml/src/ggml-backend.cpp
ggml/src/ggml-alloc.c
ggml/src/ggml-cpu/ggml-cpu.cpp
ggml/src/ggml-cpu/ggml-cpu.c
ggml/src/ggml-cpu/ops.cpp
ggml/src/ggml-cpu/traits.cpp
```

### 12.2 backend scheduler 的职责

一张图中的节点不一定都在同一个设备执行。scheduler 负责：

- 选择节点所在 backend
- 将图切分为多个 backend split
- 分配 tensor buffer
- 处理跨 backend 的 tensor 复制
- 按依赖顺序执行 split
- 支持异步执行和同步

主调用链可以概括为：

```text
llama_context::graph_compute()
-> ggml_backend_sched_graph_compute_async()
-> scheduler split and allocate
-> ggml_backend_graph_compute_async()
-> backend-specific graph compute
```

### 12.3 CPU 路径

```text
ggml_backend_cpu_graph_compute()
-> ggml_graph_plan()
-> ggml_graph_compute()
-> worker threads
-> 根据 GGML_OP_* 分派具体 kernel
```

先理解 CPU 路径，再阅读 CUDA、Vulkan 或其他 GPU backend。GPU backend 实现的是相同接口，而不是另一套 Transformer 逻辑。

### 12.4 第九阶段练习

1. 使用纯 CPU 运行最小示例。
2. 设置部分 GPU offload 后再次运行。
3. 比较加载日志中的 tensor 放置。
4. 跟踪一个 `GGML_OP_MUL_MAT` 最终进入哪个 backend kernel。

完成标准：能够解释 graph、scheduler、backend 和 kernel 的区别。

## 13. 第十阶段：量化

建议在完整推理链已经理解后再深入量化。

### 13.1 先理解的概念

- F32、F16、BF16
- block quantization
- scale、zero point
- dequantization
- quantized matrix multiplication
- importance matrix
- 精度、模型大小和速度的权衡

### 13.2 阅读方向

```text
include/llama.h 中的 quantization API
src/llama-quant.cpp
ggml/include/ggml.h 中的 ggml_type
ggml/src 中的 quantize 和 type traits
ggml/src/ggml-cpu/ 中的量化矩阵乘实现
```

量化不是简单地把每个浮点数单独压成整数。GGML 的许多量化类型以 block 为单位存储量化值和缩放信息。

### 13.3 需要回答的问题

1. 某种 GGML type 每个 block 包含多少个权重？
2. 每个 block 占多少字节？
3. 权重是否在矩阵乘前完整反量化？
4. 哪些 tensor 通常保持更高精度？
5. 为什么不同量化类型在相同模型大小下质量不同？

## 14. 第十一阶段：server 与高级功能

基础链路理解后再读：

```text
tools/server/README-dev.md
tools/server/main.cpp
tools/server/server.cpp
tools/server/server-context.h
tools/server/server-task.h
tools/server/server-queue.h
tools/server/server-context.cpp
```

server 的简化流程是：

```text
HTTP request
-> route
-> server_task
-> server_queue
-> server_context
-> server_slot
-> shared batch
-> llama_decode()
-> sampler
-> partial/final response
-> HTTP or SSE
```

第一次只研究普通 inference mode。以下功能放在最后：

- 多 slot 并行
- continuous batching
- context shift
- prompt cache
- speculative decoding
- multimodal
- router mode
- 可恢复 SSE

## 15. 一条 token 的完整源码路径

以下以普通 decoder-only Llama、普通 KV cache、CPU greedy sampler 为例。

### 15.1 文本变成 token

```text
prompt string
-> llama_tokenize()
-> llama_vocab::tokenize()
-> llama_vocab::impl::tokenize()
-> tokenizer-specific session
-> vector<llama_token>
```

### 15.2 token 进入 context

```text
llama_batch_get_one()
-> llama_decode()
-> llama_context::decode()
-> llama_batch_allocr::init()
-> memory->init_batch()
-> llama_kv_cache::prepare()
-> llama_kv_cache::find_slot()
-> llama_context::process_ubatch()
```

### 15.3 构造 Llama graph

```text
llama_model::build_graph()
-> llama_model_llama::build_arch_graph()
-> llama_model_llama::graph<false>
-> input embedding
-> 每层 RMSNorm
-> Q/K/V projection
-> RoPE
-> attention 与 KV cache
-> residual
-> FFN
-> residual
-> final RMSNorm
-> lm head
-> logits
```

### 15.4 执行 graph

```text
graph allocation
-> 设置 graph inputs
-> llama_context::graph_compute()
-> ggml_backend_sched_graph_compute_async()
-> backend graph compute
-> CPU/GPU kernels
-> logits 复制到 context 输出 buffer
```

### 15.5 logits 变成 token

```text
llama_sampler_sample()
-> llama_get_logits_ith()
-> llama_token_data_array
-> sampler chain
-> greedy 选择最大 logit
-> selected token ID
```

### 15.6 token 输出并进入下一轮

```text
token ID
-> llama_token_to_piece()
-> bytes
-> print

同一个 token ID
-> llama_batch_get_one()
-> 下一次 llama_decode()
-> 写入新 K/V
-> 预测下一个 token
```

## 16. 不建议一开始通读的文件

这些文件都很重要，但应按函数定位阅读：

- `src/llama-context.cpp`
- `src/llama-vocab.cpp`
- `src/llama-sampler.cpp`
- `src/llama-graph.cpp`
- `src/llama-kv-cache.cpp`
- `src/llama-model.cpp`
- `ggml/src/ggml.c`
- `ggml/src/ggml-backend.cpp`
- `ggml/src/ggml-cpu/ggml-cpu.c`
- `tools/server/server-context.cpp`
- 全部 `src/models/*.cpp`

`src/llama-arch.cpp` 也不适合顺序通读，它应作为名称和 metadata key 的查询表。

## 17. 推荐的六周学习计划

### 第 1 周：最小生成循环

- 跑通 `examples/simple`
- 阅读 `include/llama.h`
- 理解 model、context、batch、sampler
- 跟踪一次 prompt 和三次 token generation

成果：画出最小推理时序图。

### 第 2 周：GGUF 与 tokenizer

- 查看一个真实 GGUF 的 metadata 和 tensor
- 跟踪模型加载
- 跟踪一种 tokenizer
- 记录 token ID 与 piece

成果：画出 GGUF 到模型对象的加载图。

### 第 3 周：Transformer graph

- 学习 RMSNorm、RoPE、GQA、SwiGLU
- 阅读 `src/models/llama.cpp`
- 跟踪第一层的所有 tensor shape
- 理解构图与执行的区别

成果：将源码函数对应到 Transformer 结构图。

### 第 4 周：decode 与 KV cache

- 阅读 batch 和 ubatch
- 跟踪 `llama_context::decode()`
- 跟踪 cache slot 分配
- 对比 prefill 和 generation

成果：解释 KV cache 的内容、大小和性能作用。

### 第 5 周：GGML backend

- 理解 graph scheduler
- 跟踪 CPU graph compute
- 跟踪一个矩阵乘 kernel
- 比较 CPU 与 GPU offload

成果：画出从 GGML op 到 backend kernel 的调用链。

### 第 6 周：采样、量化与扩展

- 比较不同 sampler
- 研究一种量化格式
- 对比一个非 Llama 架构
- 根据兴趣进入 server 或 GPU backend

成果：独立解释从 GGUF 文件到文本输出的完整流程。

## 18. 调试和阅读技巧

### 18.1 使用函数断点

第一组推荐断点：

```text
llama_model_load_from_file
llama_tokenize
llama_context::decode
llama_context::process_ubatch
llama_model_llama::build_arch_graph
llama_context::graph_compute
llama_sampler_sample
llama_token_to_piece
```

### 18.2 每次只追踪一个 token

第一次调试时：

- 使用短 prompt
- 使用 greedy
- 只生成 2 到 3 个 token
- 使用单 sequence
- 优先使用 CPU

这样调用链最稳定，变量也最容易观察。

### 18.3 记录 tensor 时不要只写变量名

建议使用以下格式：

```text
name:
meaning:
type:
shape:
op:
sources:
backend:
lifetime:
```

### 18.4 区分四类问题

阅读中遇到问题时先分类：

1. 模型数学问题：为什么需要这个算子？
2. GGML 表示问题：这个算子如何表示为 graph node？
3. runtime 问题：tensor 在哪个 backend，何时分配和执行？
4. 工程问题：错误处理、日志、多设备和兼容性为何这样设计？

不要在同一次阅读中同时解决四类问题。

### 18.5 优先建立调用链，不追求逐行理解

对一个大函数第一遍只记录：

- 输入是什么
- 输出是什么
- 调用了哪些关键子函数
- 修改了哪些长期状态
- 失败时返回什么

第二遍再研究内部算法。

## 19. 最终自测问题

如果能够独立回答下面的问题，说明已经建立了较完整的 llama.cpp 推理框架：

1. `llama_model` 与 `llama_context` 为什么分开？
2. 第一次处理 prompt 与后续生成 token 有什么区别？
3. 为什么采样出的 token 可以先输出，再送入下一轮 decode？
4. `llama_batch` 为什么需要 position 和 sequence ID？
5. KV cache 为什么保存 K/V 而不保存 Q？
6. GQA 如何影响 KV cache 大小？
7. `ggml_mul_mat()` 为什么不一定立即执行矩阵乘？
8. graph scheduler 与具体 backend kernel 的职责分别是什么？
9. GGUF metadata、tensor metadata 和 tensor data 有什么区别？
10. `src/llama-arch.cpp` 与 `src/models/llama.cpp` 的职责有何不同？
11. top-k、top-p、temperature 和 distribution sampler 分别做什么？
12. 量化权重如何减少模型大小，它可能损失什么？
13. CPU 与 GPU backend 是否使用不同的 Transformer 模型逻辑？
14. 为什么 generation 经常受内存带宽限制？
15. 如何从某个输出 token 反向追踪到生成它的 logits 和计算图？

## 20. 最短推荐阅读清单

如果时间有限，只读下面这些：

```text
1. examples/simple/simple.cpp
2. include/llama.h
3. src/llama.cpp
4. src/llama-model-loader.cpp
5. src/llama-vocab.cpp 中的分派和当前 tokenizer
6. src/llama-sampler.cpp 中的 chain 与 greedy
7. ggml/include/ggml.h
8. src/models/llama.cpp
9. src/llama-context.cpp 中的 decode/process_ubatch/graph_compute
10. src/llama-kv-cache.cpp 中的 init_batch/prepare/find_slot
11. ggml/src/ggml-backend.cpp 中的 scheduler compute
12. ggml/src/ggml-cpu/ 中的一条算子执行路径
```

最有效的学习方式不是读完所有源码，而是选定一个小模型和一个短 prompt，把一个 token 从输入文本一直跟踪到输出文本。
