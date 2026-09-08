# NVIDIA Nsight

NVIDIA **Nsight** 不是某一个软件，而是一组开发、调试和性能分析工具的总称。CUDA 性能分析最常用的是：

* **Nsight Systems**：从整个程序的时间线定位「时间花在哪里」。
* **Nsight Compute**：深入单个 CUDA kernel，解释「这个 kernel 为什么慢」。

此外还有 Nsight Graphics、Nsight Visual Studio Edition 等工具，但它们不是本笔记重点。

官方文档：

* [Nsight Systems Documentation](https://docs.nvidia.com/nsight-systems/)
* [Nsight Compute Documentation](https://docs.nvidia.com/nsight-compute/)
* [NVTX Documentation](https://nvidia.github.io/NVTX/)

## 工具定位

### Nsight Systems

Nsight Systems 是系统级 profiler，主要观察：

* CPU 线程在做什么，是否存在阻塞或长时间空闲。
* CUDA Runtime / Driver API 调用，例如 `cudaMemcpy`、kernel launch 和同步。
* GPU 上各 stream 的 kernel、内存复制和同步活动。
* CPU 与 GPU、数据传输与计算、不同 stream 之间是否重叠。
* GPU 是否存在大片空闲时间，kernel 启动间隔是否过大。
* NVTX 标记的业务阶段分别耗时多少。

它适合确定瓶颈位于 CPU、数据传输、同步、任务调度还是某个 GPU kernel，但通常不会深入解释 kernel 内部的指令和访存效率。

命令行程序是 `nsys`，图形界面是 `nsys-ui`，报告扩展名通常是 `.nsys-rep`。

### Nsight Compute

Nsight Compute 是 kernel 级 profiler，主要观察：

* kernel 启动配置、寄存器和 shared memory 用量。
* 理论与实际 occupancy。
* SM 和内存吞吐利用率。
* Warp 是否有足够多的 eligible warp。
* Warp 因访存、依赖、同步等原因停顿的情况。
* Global Memory 是否合并访问，L1/L2 缓存命中情况。
* 源码、PTX、SASS 与性能指标之间的对应关系。
* Roofline：kernel 更接近 memory-bound 还是 compute-bound。

命令行程序是 `ncu`，图形界面是 `ncu-ui`，报告扩展名通常是 `.ncu-rep`。

### 两者的关系

典型流程不是二选一，而是先粗后细：

```text
Nsight Systems
    │
    ├── CPU 或同步问题 ──> 检查线程、API、stream 和调用逻辑
    ├── 传输问题       ──> 减少传输或让传输与计算重叠
    └── 某个 kernel 慢 ──> 使用 Nsight Compute
                                │
                                ├── memory-bound
                                ├── compute-bound
                                ├── latency-bound
                                └── occupancy / launch 配置问题
```

一句话概括：

> Nsight Systems 判断整个程序哪里慢，Nsight Compute 判断某个 kernel 为什么慢。

## 使用前准备

### 检查工具

在 PowerShell 中执行：

```powershell
nsys --version
ncu --version
```

两个工具是独立产品，安装了 CUDA Toolkit 不代表当前环境一定同时安装了它们。如果命令不存在，应通过 NVIDIA 官方安装包或 CUDA Toolkit 的可选组件安装，并检查可执行文件是否加入 `PATH`。

### 编译用于分析的程序

性能分析应使用接近 Release 的优化版本，同时保留源码行号：

```powershell
nvcc -O3 -lineinfo main.cu -o main.exe
```

`-lineinfo` 让 Nsight Compute 能把指标关联到 CUDA 源码，通常不会像完整调试模式那样大幅改变生成代码。

不要用下面的配置测量最终性能：

```powershell
nvcc -G -g main.cu -o main.exe
```

`-G` 会生成设备调试代码并关闭或限制许多优化，适合调试正确性，不适合评估真实性能。

### 先保证程序正确

Profiler 不能替代错误检查。至少检查 kernel 启动错误和异步执行错误：

```cpp
kernel<<<grid, block>>>(/* arguments */);

cudaError_t err = cudaGetLastError();
if (err != cudaSuccess)
    std::fprintf(stderr, "launch failed: %s\n", cudaGetErrorString(err));

err = cudaDeviceSynchronize();
if (err != cudaSuccess)
    std::fprintf(stderr, "execution failed: %s\n", cudaGetErrorString(err));
```

越界访问、未初始化内存和数据竞争应优先使用 **Compute Sanitizer** 检查。Compute Sanitizer 是正确性检查工具，不等同于 Nsight Compute：

```powershell
compute-sanitizer --tool memcheck .\main.exe
compute-sanitizer --tool racecheck .\main.exe
```

## Nsight Systems

### 基本采集

采集 CUDA、NVTX 和操作系统运行时事件：

```powershell
nsys profile --trace=cuda,nvtx,osrt --sample=none -o report .\main.exe
```

常用参数含义：

* `profile`：运行程序并采集 trace。
* `--trace=cuda,nvtx,osrt`：选择要跟踪的事件。
* `--sample=none`：不做 CPU 指令采样；只分析 CUDA 时间线时可降低干扰。
* `-o report`：输出报告名。

打开图形报告：

```powershell
nsys-ui .\report.nsys-rep
```

生成或查看统计摘要：

```powershell
nsys stats .\report.nsys-rep
```

命令选项可能随版本调整，遇到差异时使用：

```powershell
nsys profile --help
nsys stats --help
```

### 时间线怎么看

建议按以下顺序阅读。

#### 1. 先看 NVTX 业务阶段

先确定初始化、预处理、推理、后处理等阶段的边界。没有 NVTX 时，复杂程序的 CUDA 调用很难对应到业务代码。

#### 2. 再看 CPU 线程

关注：

* CPU 是否长时间没有提交 GPU 工作。
* 是否有锁、I/O 或线程调度导致的间隙。
* 是否频繁调用高开销 API，例如反复分配和释放设备内存。
* 是否在每次 kernel 后立即同步。

#### 3. 看 CUDA API 行

CUDA API 行表示 CPU 在调用 CUDA Runtime 或 Driver API。API 调用结束并不一定表示 GPU 工作已经完成。

例如 kernel launch 通常对主机异步：

```text
CPU:  cudaLaunchKernel 很快返回
GPU:                   kernel 随后执行一段时间
```

`cudaDeviceSynchronize()` 在 CPU 时间线上很长，不一定表示同步函数本身很慢，而可能表示 CPU 正在等待此前提交的 GPU 工作结束。

#### 4. 看 GPU stream

关注：

* kernel 是否把 GPU 时间线填满。
* 不同 stream 是否真正重叠。
* H2D、D2H 复制是否与 kernel 重叠。
* 是否有大量很短的 kernel。
* kernel 之间是否存在明显空隙。

#### 5. 最后看统计摘要

统计摘要适合找出：

* 总耗时最高的 CUDA API。
* 总耗时最高、调用次数最多的 kernel。
* 内存复制的次数、方向和总耗时。

统计结果会聚合调用，必须结合时间线判断调用之间的依赖和空闲原因。

### 常见时间线现象

#### GPU 大片空闲

可能原因：

* CPU 准备数据或提交任务太慢。
* kernel 启动次数过多且每次工作量太小。
* 频繁同步。
* 串行的数据依赖。
* CPU 锁、I/O 或其他线程阻塞。

常见改进：

* 增大每次提交的工作量。
* 融合小 kernel。
* 使用 CUDA Graph 降低重复提交开销。
* 减少不必要的同步。
* 将 CPU 准备与 GPU 计算流水化。

#### H2D / D2H 占用大量时间

可能原因：

* CPU 与 GPU 之间往返复制过多。
* 每次只复制很小的数据。
* 使用 pageable host memory，难以获得理想的异步传输效果。
* 算法计算量很小，PCIe 传输成本超过 GPU 收益。

常见改进：

* 让数据尽可能长时间保留在 GPU。
* 合并小传输。
* 使用 pinned host memory。
* 使用 `cudaMemcpyAsync` 和不同 stream 构造流水线。
* 将相邻算子融合，避免中间结果返回 CPU。

异步 API 并不自动保证重叠。还要满足 pinned memory、独立 stream、正确依赖以及硬件并发能力等条件。

#### 每个 kernel 后都有同步

下面的写法会让 CPU 和 GPU 串行：

```cpp
for (int i = 0; i < count; ++i) {
    kernel<<<grid, block>>>(/* arguments */);
    cudaDeviceSynchronize();
}
```

如果算法没有逐次依赖，可以连续提交任务，只在真正需要结果的位置同步。

#### 大量极短 kernel

单个 kernel 的计算时间可能小于或接近提交开销。此时优化 kernel 内部一两条指令意义有限，应优先考虑：

* Kernel fusion。
* Batch。
* CUDA Graph。
* 减少框架和 CPU 调度开销。

#### 多 stream 没有并发

可能原因：

* 工作实际提交到了同一个 stream。
* stream 之间存在 event 或数据依赖。
* 默认 stream 的同步语义阻止并发。
* 单个 kernel 已占满关键硬件资源。
* 内存复制使用了 pageable memory。
* 设备不支持所需方向的复制与计算并发。

不要仅凭代码中创建了多个 stream 就认定发生了并发，应在时间线上验证。

### 使用 NVTX 标记业务阶段

NVTX 可以给时间线增加名称、范围和类别。最简单的 C API 用法：

```cpp
#include <nvtx3/nvToolsExt.h>

void inference() {
    nvtxRangePushA("preprocess");
    preprocess();
    nvtxRangePop();

    nvtxRangePushA("GPU inference");
    run_gpu_inference();
    nvtxRangePop();

    nvtxRangePushA("postprocess");
    postprocess();
    nvtxRangePop();
}
```

标记单个瞬时事件：

```cpp
nvtxMarkA("weights uploaded");
```

复杂项目应使用 NVTX domain 区分不同模块，例如 runtime、data loader 和 inference engine。NVTX 本身不优化性能，它的作用是把底层时间线和上层业务语义对应起来。

### 只采集关键区间

长时间运行的程序可以用 CUDA Profiler API 限制采集范围：

```cpp
#include <cuda_profiler_api.h>

int main() {
    initialize();
    warmup();

    cudaProfilerStart();
    benchmark();
    cudaProfilerStop();

    cleanup();
}
```

采集命令：

```powershell
nsys profile --trace=cuda,nvtx --capture-range=cudaProfilerApi --stop-on-range-end=true -o report .\main.exe
```

这样可以跳过初始化和 warm-up，减少报告大小及 profiler 干扰。

## Nsight Compute

### 基本采集

先运行较轻量的指标集合：

```powershell
ncu --set basic -o kernel_report .\main.exe
```

需要更完整的指标时：

```powershell
ncu --set full -o kernel_report .\main.exe
```

`full` 可能需要多次 replay kernel，采集时间和数据量会显著增加。因此不应一开始就对大型程序的所有 kernel 使用完整指标集。

打开图形报告：

```powershell
ncu-ui .\kernel_report.ncu-rep
```

查看当前版本支持的指标集和 section：

```powershell
ncu --list-sets
ncu --list-sections
```

### 只分析目标 kernel

实际程序通常有大量 kernel，应先在 Nsight Systems 中找到目标，再通过 kernel 名称和调用次数缩小采集范围：

```powershell
ncu --kernel-name regex:my_kernel --launch-skip 10 --launch-count 1 --set full -o my_kernel_report .\main.exe
```

含义：

* `--kernel-name`：按名称选择 kernel。
* `--launch-skip 10`：跳过前 10 次匹配，适合排除 warm-up。
* `--launch-count 1`：只采集一次匹配。
* `--set full`：采集完整指标集合。

C++ 模板生成的 kernel 名称可能很长，可以使用正则表达式。具体匹配语法以当前版本的 `ncu --help` 为准。

### 重要页面和指标

#### Summary / GPU Speed Of Light

先看：

* Kernel Duration。
* SM Compute Throughput。
* Memory Throughput。

初步判断：

* Memory Throughput 高、SM Throughput 低：可能偏 memory-bound。
* SM Throughput 高、Memory Throughput 低：可能偏 compute-bound。
* 两者都低：可能是并行度不足、依赖链过长、同步、访存延迟或启动配置问题。

吞吐百分比接近峰值不代表代码一定最优；必须结合算法有效工作量和数据规模判断。

#### Launch Statistics

关注：

* Grid 和 block 维度。
* 每个 block 的 thread 数。
* 每线程寄存器数量。
* 每 block 静态、动态 shared memory 用量。
* Waves per SM。

Grid 太小可能无法覆盖全部 SM；每个 block 使用过多寄存器或 shared memory，可能限制同一 SM 上的驻留 block 数量。

#### Occupancy

Occupancy 是驻留 active warp 数相对于硬件最大值的比例。

主要限制来源：

* 每线程寄存器数量。
* 每 block shared memory。
* 每个 block 的线程数量。
* 架构对 block、warp 和 thread 的硬件上限。

需要同时区分：

* **Theoretical Occupancy**：根据静态资源和启动配置推算的上限。
* **Achieved Occupancy**：实际运行期间测得的 active warp 比例。

高 occupancy 有助于隐藏延迟，但不是最终目标。计算密集型 kernel 可能在中等 occupancy 下已经达到很高吞吐；为提高 occupancy 强行降低寄存器数量，还可能引发 register spilling，反而变慢。

#### Scheduler Statistics

理解三个概念：

* **Active warp**：已驻留在 SM 上。
* **Eligible warp**：下一条指令已经满足发射条件。
* **Issued warp**：当前周期真正被 scheduler 选中并发射指令。

如果 active warp 很多，但每个 scheduler 的 eligible warp 长期很少，说明 warp 大量处于等待状态。应继续查看 Warp State Statistics，而不是继续盲目提高 occupancy。

#### Warp State Statistics

Warp Stall 描述 warp 为什么暂时不能发射下一条指令。常见类型包括：

* 等待内存访问完成。
* 等待指令操作数或执行依赖。
* 等待 barrier。
* 等待纹理、数学或其他执行流水线。
* 没有被选中，因为还有其他 eligible warp。

Stall 名称和分类会随 GPU 架构、Nsight Compute 版本变化。最高的 stall 指标不一定就是根因，必须结合源码、指令、内存和吞吐指标一起判断。

例如：

* Long scoreboard 类等待高，同时 global memory 延迟高：检查访存局部性和数据复用。
* Barrier 等待高：检查 block 内工作是否不均衡，以及 `__syncthreads()` 是否过多。
* Eligible warp 少：检查依赖链、访存延迟和 occupancy。
* Not selected 较高但吞吐也高：可能只是 scheduler 有足够多的候选 warp，不一定是问题。

#### Memory Workload Analysis

关注：

* DRAM、L2、L1/TEX 吞吐。
* Cache hit rate。
* Load / store requests。
* 每个 request 产生的 sector 数。
* 实际传输字节与算法所需字节。

同一个 warp 的相邻线程访问相邻、对齐地址，通常能形成良好的 coalesced access。跨步访问会产生更多内存事务：

```cpp
// 通常较好：相邻线程访问相邻元素
float value = data[global_tid];

// 通常较差：相邻线程访问相距 stride 的元素
float value = data[global_tid * stride];
```

缓存命中率不是越高越好。流式访问可能天然只有很低的 L1 命中率，但如果已经接近 DRAM 带宽上限，仍可能是合理实现。

#### Source / PTX / SASS

使用 `-lineinfo` 编译后，可以把高开销指标关联到源码行，并继续查看 PTX 和 SASS。

需要关注：

* 哪一行触发大量 global memory 事务。
* 循环是否展开。
* 是否生成了预期的向量化 load/store。
* 是否出现 local memory 访问，暗示 register spilling。
* 分支指令和活跃线程比例。

不能只根据 C++ 源码猜测最终指令，关键性能问题需要结合编译器实际生成的 SASS。

#### Roofline

Roofline 使用算术强度判断性能上限：

```text
算术强度 = 浮点运算次数 / 从内存传输的字节数
```

* 算术强度低，性能接近内存带宽上限：通常是 memory-bound。
* 算术强度高，性能接近计算峰值：通常是 compute-bound。
* 距离两种上限都很远：可能存在延迟、并行度、分支或指令效率问题。

提高算术强度的常见方法：

* 使用 shared memory 或寄存器复用数据。
* Kernel fusion，避免中间结果反复写回 global memory。
* 矩阵乘法采用 tiling。

### 常见瓶颈与优化方向

#### Memory-bound

证据通常包括：

* DRAM 或某级缓存吞吐已经很高。
* SM 计算吞吐相对较低。
* Roofline 位于带宽受限区域。

优化方向：

* 合并 global memory 访问。
* 减少重复读取和无效字节。
* 利用 shared memory、寄存器或缓存复用数据。
* 使用合适的向量化 load/store。
* 融合 kernel，减少中间数据读写。

#### Compute-bound

证据通常包括：

* SM 或特定计算流水线吞吐接近上限。
* Roofline 位于计算受限区域。

优化方向：

* 减少不必要的运算。
* 使用更合适的数据类型。
* 使用 Tensor Core 或成熟库。
* 改善指令级并行，减少长依赖链。
* 避免昂贵操作成为热点。

如果已经接近硬件计算峰值，继续做常规访存优化可能收益很小。

#### Latency-bound

典型表现是 SM 和内存吞吐都不高，但 eligible warp 很少。

可能原因：

* 访存延迟没有被足够多的 warp 隐藏。
* 指令依赖链过长。
* Grid 太小。
* 寄存器或 shared memory 限制驻留 warp。
* 频繁同步。

优化方向：

* 增加可并行工作量。
* 调整 block 大小。
* 增加线程或指令级并行。
* 减少依赖链。
* 在不引发 spilling 的前提下降低资源压力。

#### Warp divergence

分支只有在同一个 warp 内线程走不同路径时才造成 divergence：

```cpp
if ((threadIdx.x & 1) == 0)
    path_a();
else
    path_b();
```

优化方向：

* 调整数据布局，让同一 warp 处理行为相似的数据。
* 减少热点循环内部的数据相关分支。
* 如果两条路径很短，先测量再决定是否值得改写。

不要把「存在 `if`」直接等同于「存在严重分支发散」。

#### Shared Memory Bank Conflict

矩阵转置中常见的二维 shared memory 访问可能产生 bank conflict：

```cpp
__shared__ float tile[32][32];
```

增加一列 padding 经常可以打散列访问：

```cpp
__shared__ float tile[32][33];
```

是否有效应通过 shared memory 指标和实际耗时验证。

#### Register spilling

每线程寄存器使用过多时，编译器可能把变量放到 local memory。Local memory 通常位于设备显存地址空间，访问成本远高于寄存器。

可以结合以下信息判断：

* 编译器的寄存器与 spill 输出。
* Nsight Compute 的 local load/store。
* Source / SASS 中的 local memory 指令。

降低寄存器使用量有时能提高 occupancy，但使用 `--maxrregcount` 强行限制寄存器可能增加 spilling。必须用端到端耗时比较。

## 推荐分析流程

### 第一步：建立可靠基线

* 使用 Release 优化和 `-lineinfo`。
* 检查结果正确性。
* 做若干次 warm-up。
* 使用固定数据规模和输入。
* 重复测量，避免只相信单次结果。

### 第二步：用 Nsight Systems 定位

回答：

* 时间主要在 CPU、传输还是 GPU？
* GPU 是否持续忙碌？
* 是否存在频繁同步？
* 是否有大量小 kernel？
* 数据传输是否与计算重叠？
* 最耗时的 kernel 是哪个？

### 第三步：选择一个重要 kernel

优先选择总耗时贡献最大的 kernel，而不一定是单次调用最慢的 kernel：

```text
总贡献时间 = 单次耗时 × 调用次数
```

### 第四步：用 Nsight Compute 建立瓶颈证据

回答：

* Memory-bound、compute-bound 还是 latency-bound？
* Grid 是否足够大？
* Occupancy 被什么资源限制？
* Eligible warp 是否足够？
* Global Memory 是否合并访问？
* 是否存在 spilling、bank conflict、barrier 或 divergence？

### 第五步：一次只改一个关键因素

修改后重新测量：

* kernel 自身耗时是否下降。
* 整个应用耗时是否下降。
* 是否把瓶颈转移到其他阶段。
* 结果是否仍然正确。

局部 kernel 加速不一定能带来等比例的端到端加速，应结合 Amdahl 定律判断收益上限。

## 测量注意事项

### Profiler 会产生开销

Nsight Systems 的 tracing 会增加一定开销；Nsight Compute 为采集硬件计数器，可能 replay kernel 多次，开销更明显。因此 profiler 中的程序总运行时间不能直接等同于无采集时的运行时间。

最终耗时应使用 CUDA Event 或稳定的端到端计时重新验证。

### 需要 warm-up

首次执行可能包含：

* CUDA Context 初始化。
* Module 加载和 JIT。
* 内存池初始化。
* 缓存预热。
* GPU 频率爬升。

通常跳过前若干次调用，再分析稳定阶段。

### CUDA Event 与 CPU 计时不同

CUDA Event 可以测量同一设备上 stream 时间线中的 GPU 工作：

```cpp
cudaEvent_t begin, end;
cudaEventCreate(&begin);
cudaEventCreate(&end);

cudaEventRecord(begin);
kernel<<<grid, block>>>(/* arguments */);
cudaEventRecord(end);
cudaEventSynchronize(end);

float milliseconds = 0.0f;
cudaEventElapsedTime(&milliseconds, begin, end);
```

CPU 计时若没有在结束处同步，可能只测到异步 launch 的提交时间，而没有测到 kernel 的执行时间。

### 采集指标要有针对性

不要对整个大型应用直接使用 `ncu --set full`。更可靠的方法是：

1. Nsight Systems 找到热点。
2. 过滤到目标 kernel。
3. 跳过 warm-up。
4. 只采集需要的 section 或有限次数。

这既减少开销，也避免生成难以分析的超大报告。

## 常见误区

* **误区：Occupancy 越高，性能一定越好。**  
  Occupancy 只是隐藏延迟的条件之一，不是最终性能指标。

* **误区：使用多个 stream 就一定能够并发。**  
  依赖、默认 stream 语义、资源占用和 host memory 类型都可能阻止并发。

* **误区：`cudaDeviceSynchronize()` 本身耗时很长。**  
  它通常是在等待此前未完成的 GPU 工作。

* **误区：缓存命中率越高越好。**  
  必须结合访问模式、有效带宽和算法需求判断。

* **误区：最大 stall reason 就一定是根因。**  
  Stall 是症状分类，需要与吞吐、指令和源码一起分析。

* **误区：kernel 加速 50%，程序就加速 50%。**  
  还要看该 kernel 占端到端时间的比例。

* **误区：Profiler 报告中的耗时就是程序真实性能。**  
  指标采集和 replay 会改变执行时间，最终必须脱离 profiler 验证。

## 面试回答

如果面试官问「你如何分析 CUDA 性能」，可以回答：

> 我会先使用 Nsight Systems 查看 CPU/GPU 时间线，确认瓶颈是在 CPU 调度、数据传输、同步还是 GPU kernel，并找出总耗时贡献最大的 kernel。然后用 Nsight Compute 过滤并分析该 kernel，结合 SM 与内存吞吐、Roofline、occupancy、eligible warp、warp stall、访存事务和源码/SASS 判断它属于 memory-bound、compute-bound 还是 latency-bound。优化后会重新检查正确性，并分别比较 kernel 耗时和端到端耗时，而不会只追求更高的 occupancy。

## 速查

```powershell
# 查看版本
nsys --version
ncu --version

# 系统级时间线
nsys profile --trace=cuda,nvtx,osrt --sample=none -o report .\main.exe
nsys-ui .\report.nsys-rep
nsys stats .\report.nsys-rep

# Kernel 级分析
ncu --set basic -o kernel_report .\main.exe
ncu --kernel-name regex:my_kernel --launch-skip 10 --launch-count 1 --set full -o my_kernel_report .\main.exe
ncu-ui .\my_kernel_report.ncu-rep

# 正确性检查，不属于 profiler
compute-sanitizer --tool memcheck .\main.exe
compute-sanitizer --tool racecheck .\main.exe
```

