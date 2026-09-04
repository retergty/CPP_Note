# gpt2-onnx-cuda 路线图

用 C++/CUDA 解析 GPT-2 的 ONNX 计算图，在 GPU 上完成一次前向推理，并与官方运行时对齐数值。

独立实现，不要把代码写进 llama.cpp 源码树。本文件只作计划备忘。

---

## 1. 项目信息

| 项 | 内容 |
|----|------|
| 仓库 / 项目名 | `gpt2-onnx-cuda` |
| 中文名 | GPT-2 ONNX CUDA 迷你推理器 |
| 模型 | GPT-2 124M（[openai-community/gpt2](https://huggingface.co/openai-community/gpt2)） |
| 权重许可 | Modified MIT，保留官方版权与许可声明 |
| 硬件 | RTX 2070（约 8GB，CUDA 架构 sm_75） |

**一次推理：** 输入 `input_ids`，形状 `[1, T]`，`T` 先固定为 8（8 个 token）；输出 `logits`，形状 `[1, 8, 50257]`。

---

## 2. 技术栈

- 语言：C++、CUDA C++（v1 用 fp32）
- 模型格式：ONNX（protobuf `ModelProto`）
- 导出：Hugging Face Optimum，任务 `text-generation`（不要带 past KV）
- 构建：CMake + NVCC，`-DCMAKE_CUDA_ARCHITECTURES=75`
- 对照：ONNX Runtime CUDA、PyTorch（导出与对答案）
- 工具：Netron 或脚本列算子；可选 Nsight 记延迟

权重与工具只从官方源获取：

- [CUDA Toolkit](https://developer.nvidia.com/cuda-toolkit)
- [onnx/onnx](https://github.com/onnx/onnx)
- [ONNX Runtime Releases](https://github.com/microsoft/onnxruntime/releases)
- [Optimum 导出文档](https://huggingface.co/docs/optimum-onnx/onnx/usage_guides/export_a_model)
- [openai/gpt-2](https://github.com/openai/gpt-2)

---

## 3. 工作内容

1. 解析 GPT-2 的 ONNX 计算图，完成拓扑排序、张量分配和权重上 GPU，按节点调度执行一次前向推理。
2. 用 CUDA 实现图中所需算子（Gather、MatMul、Softmax、Where 因果 mask、LayerNorm、GELU 等），在 `batch=1`、序列长度 8 下输出 logits。
3. 用同一输入与 PyTorch、ONNX Runtime 对齐数值（fp32），并记录自研路径与 ORT CUDA 的推理延迟。

---

## 4. v1 范围

**做：**

- 无 past KV 的 `gpt2.onnx`
- 按 ONNX 节点解释执行（从 initializer 读权重，按图上的 op 调 kernel）
- `batch=1`，序列长度 `T=8`
- fp32，权重与中间结果都放 2070 显存
- logits 与参考实现对比，并记一笔延迟

**v1 不做：**

- 多轮生成、采样循环
- KV cache、动态序列长度
- fp16 / Tensor Core / 量化
- 完整 ONNX 算子集
- 把本项目并入 llama.cpp

**成功标准：**

- 同一 `input_ids` 下，自研 logits 相对 PyTorch / ORT 的 `max abs err < 1e-3`（fp32）
- 命令行可复现：加载 onnx、跑前向、打印最后一位的 top-5 token id

序列长度 8 的含义：一次前向送入 8 个 token，输入 `[1, 8]`，注意力是 `8 x 8`。先锁死方便对数值；对齐后再把 `T` 改成 32/64。

---

## 5. 阶段路线

### 阶段 A（2-3 天）：导出 + 算子清单

先锁图，再写 CUDA。

```bash
optimum-cli export onnx --model gpt2 --task text-generation --opset 14 gpt2_onnx/
```

必须用 `text-generation`。带 `-with-past` 的图输入输出会多出 `past_key_values`，v1 不采用。

脚本列出：

- 输入 / 输出名字与 shape
- 去重后的 `op_type` 及出现次数
- initializer 数量与总字节

**门禁：** 独特算子大约十几种到二十几种。常见包括：

`Gather` `Add` `Mul` `Sub` `Div` `MatMul` `Reshape` `Transpose` `Softmax` `Unsqueeze` `Squeeze` `Concat` `Where` `Cast`，以及 `LayerNormalization` 或拆开的 `ReduceMean`/`Pow`/`Sqrt`，GELU 相关的 `Erf` 或 `Tanh`。

若出现大量 `past_key_values_*`，重新导出。

### 阶段 B（1 天）：ORT / PyTorch 基线

同一组 `input_ids`：

- ONNX Runtime CUDA 跑一遍，存 `ref.npy`
- Hugging Face `GPT2LMHeadModel` 再存一份 `ref_pt.npy`

ORT 只作对照，推理路径走自研 CUDA。

### 阶段 C（约 1 周）：加载器 + 执行器

1. `onnx::ModelProto` 读文件；若有 external data，按官方格式把权重读全。
2. `name -> Tensor`（shape、dtype、device 指针）。
3. initializer 一次 `cudaMemcpy` 上 GPU。
4. 拓扑排序后按序执行节点。
5. 先用小图把 Add / Mul / MatMul 跑通，再挂整网。

124M 的 fp32 权重大约 0.5GB，`T=8` 时激活很小，8GB 显存足够，中间张量可以先不复用，方便对答案。

### 阶段 D（约 2 周）：按算子补 CUDA kernel

按 op 实现，同一 kernel 会被 12 层反复调用。建议顺序：

| 顺序 | 算子 | 作用 |
|------|------|------|
| 1 | Add / Mul / Sub / Div | 残差、缩放、LN 拆解 |
| 2 | Gather | token / 位置 embedding |
| 3 | MatMul | QKV、FFN、lm_head（先朴素实现，对齐后再做 tiled） |
| 4 | Reshape / Transpose / Squeeze / Unsqueeze | 注意力布局 |
| 5 | Softmax | 注意力 |
| 6 | Where（含广播） | 图里的因果 mask |
| 7 | LayerNorm 或 ReduceMean 套件 | 每层 LN |
| 8 | GELU（Erf 或 Tanh 公式） | FFN |
| 9 | Concat / Split / Slice / Cast | 图中剩余胶水节点 |

每完成 2-3 个 op，用子图测一次。一层注意力对不齐时，先查 Reshape / Transpose 和 mask，不要先优化 GEMM。

### 阶段 E（3-4 天）：整网对齐

1. 固定输入，例如 8 个 token id（不足则 pad）。
2. 自研 CUDA dump logits，与 `ref.npy` 比较。
3. 误差偏大时按层、按中间张量名字打点。
4. 命令行示例：

```text
tinyort --model gpt2.onnx --ids 15496,995 --T 8
```

打印 `logits[0, T-1, :]` 的 top-5 id。分词器可选，v1 不强制。

### 阶段 F（2-3 天）：收尾

- 表格：ORT CUDA vs 自研，`T=8` 延迟
- 写明支持的 op 列表（覆盖该导出图）
- README：导出命令、编译、如何对 `ref.npy`
- 保留 GPT-2 的许可声明

---

## 6. 建议目录（独立仓库）

```text
gpt2-onnx-cuda/
  scripts/export_gpt2.py
  scripts/list_ops.py
  scripts/ort_ref.py
  src/onnx_loader.cpp
  src/graph.cpp
  src/runtime.cu
  src/kernels.cu
  src/main.cpp
  README.md
```

---

## 7. 每周检查点

| 周 | 结果 |
|----|------|
| 1 | `gpt2.onnx` + 去重 op 表 + `ref.npy` |
| 2 | 权重上 GPU；小图 Add/MatMul 可跑 |
| 3 | Gather + 一层注意力数值对齐 |
| 4 | 12 层 + lm_head，整网 logits 达标 |
| 5 | 延迟表 + README，他人可复现 |

---

## 8. 显存与后续 KV

v1 没有 KV cache。一次前向把长度为 8 的序列整段算完。

以后若加 KV：

- 放在 2070 显存里即可
- 按最大上下文 `n_ctx` 一次分配，用已用长度 `t` 往里写
- GPT-2 124M、fp32、`n_ctx=1024` 时 KV 大约几十 MB，不必加卡
- 不要按 token 反复扩大显存块

公式（K 和 V）：

```text
bytes = 2 * n_layer * n_head * head_dim * n_ctx * sizeof(dtype)
```

GPT-2 small：`n_layer=12`，`n_head=12`，`head_dim=64`。

---

## 9. 简历可用表述

**技术栈：** C++、CUDA C++、ONNX、CMake、NVCC、ONNX Runtime（对照）、PyTorch / Optimum（导出）

**工作内容：** 见第 3 节。测出误差和延迟后，把数字填进第 3 条。
