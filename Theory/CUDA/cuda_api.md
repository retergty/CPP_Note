# cuda api

本文整理日常 CUDA 程序里最常用的 Runtime API 与设备端原语。概念、可见性与调用序见 [cuda 笔记](./cuda_note.md)。本笔记默认只用 Runtime（`cuda_runtime.h`），不混用 Driver API。

参考文档

* [CUDA Runtime API](https://docs.nvidia.com/cuda/cuda-runtime-api/index.html)
* [CUDA C++ Programming Guide](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html)

几乎所有 Runtime 调用返回 `cudaError_t`，成功为 `cudaSuccess`。每个可能失败的调用都应检查返回值。

## 类型

参考文档

* [Data types used by CUDA Runtime](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__TYPES.html)

```CPP
enum cudaError
typedef enum cudaError cudaError_t
```

  Runtime 错误码。`cudaSuccess` 表示该调用本身成功。kernel 体内的异步错误通常要等到同步之后才能在主机上看到。

```CPP
struct dim3 {
    unsigned int x, y, z;
    dim3(unsigned int vx = 1, unsigned int vy = 1, unsigned int vz = 1);
};
```

  启动配置用的三维尺寸。未写的分量默认为 `1`。`kernel<<<N, 256>>>` 等价于 `grid.x = N`，`block.x = 256`，其余为 `1`。

```CPP
struct uint3 { unsigned int x, y, z; };
```

  `threadIdx`、`blockIdx` 的类型。

```CPP
typedef struct CUstream_st* cudaStream_t;
typedef struct CUevent_st*  cudaEvent_t;
typedef struct CUgraph_st*   cudaGraph_t;
typedef struct CUgraphExec_st* cudaGraphExec_t;
```

  Stream、Event 与 CUDA Graph 的不透明句柄。`cudaGraph_t` 是可修改的图定义，`cudaGraphExec_t` 是实例化后可反复启动的可执行图。`0` / `cudaStreamDefault` 表示默认 stream。

```CPP
enum cudaMemcpyKind {
    cudaMemcpyHostToHost,
    cudaMemcpyHostToDevice,
    cudaMemcpyDeviceToHost,
    cudaMemcpyDeviceToDevice,
    cudaMemcpyDefault
};
```

  `cudaMemcpy*` 的方向。`cudaMemcpyDefault` 在统一寻址下按指针推断方向。

## 执行空间限定符

参考文档

* [Function Type Qualifiers](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#function-type-qualifiers)

`__host__`、`__device__`、`__global__` 不是 Runtime 函数，是 nvcc 的**执行空间**声明：标明这个函数**谁可以调用、在哪执行**。后文每个 API 签名前面的 `__host__` / `__device__` 都是这个含义。

`nvcc` 对 `.cu` 做两次编译：主机部分交给主机 C++ 编译器；设备部分生成 PTX 和/或 SASS。限定符决定函数进入哪一条路径，或两条都进。

| 限定符 | 从哪里调用 | 在哪里执行 | 说明 |
|--------|------------|------------|------|
| `__host__` | 主机 | 主机 | 未写限定符时的默认 |
| `__device__` | 设备 | 设备 | 供 kernel 或其他 `__device__` 调用 |
| `__host__ __device__` | 两者 | 对应侧 | nvcc 为主机和设备各生成一份 |
| `__global__` | 主机（`<<<>>>`）；计算能力足够时也可由设备 launch | 设备 | 必须 `void`，即 kernel |

```CPP
__host__ void f();
```

  在 CPU 上调用、在 CPU 上执行。`.cu` 里普通函数不写限定符时，默认就是 `__host__`。`cudaSetDevice`、`cudaMemcpy`（同步版）只标 `__host__`：只能从主机调用。

```CPP
__device__ float fma_dev(float a, float b, float c);
```

  在 GPU 线程里调用、在 GPU 上执行。不能用 `<<<>>>` 启动。主机普通函数不能直接调用它；只有 `__global__` / `__device__` 可以。

```CPP
__host__ __device__ cudaError_t cudaGetLastError(void);
```

  两个限定符可以叠写。nvcc 生成两份目标码：主机一份，设备一份。调用方在哪一侧，就链到哪一份。部分 Runtime API 标成这样，是因为设备端（动态并行等）也被允许调用它们；日常主机程序当成普通 `cuda*` 用即可。

  设备编译路径上定义 `__CUDA_ARCH__`（例如 sm_75 为 `750`），主机路径上不定义。同一份函数体若两侧行为必须不同，用它分支：

```CPP
__host__ __device__ int warp_size() {
#ifdef __CUDA_ARCH__
    return warpSize;
#else
    return 32;
#endif
}
```

```CPP
__global__ void saxpy(int n, float a, const float* x, float* y);
```

  kernel。主机用 `<<<grid, block>>>` 启动，在设备上执行。必须返回 `void`，参数按值传递。指针必须指向设备可访问的存储。它不是 `__host__`：主机侧看到的是启动配置，不是普通函数调用。

调用关系：

* 主机 → `__host__`（普通调用）或 `__global__`（`<<<>>>`）
* 设备 → `__device__` 或 `__global__`（计算能力足够时的动态并行）
* 主机不能直接调用 `__device__`；`__device__` 也不能 `<<<>>>`

写在变量上的 `__device__` / `__constant__` / `__shared__` 是**存储空间**，不是执行空间，见 [设备端内存限定符](#设备端内存限定符)。

## 设备管理

参考文档

* [Device Management](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__DEVICE.html)

```CPP
__host__ cudaError_t cudaGetDeviceCount(int* count)
```

  返回当前进程可见的 CUDA 设备个数。

```CPP
__host__ cudaError_t cudaSetDevice(int device)
```

  把当前主机线程绑定到设备 `device`。未调用时默认设备 `0`。之后该线程上的分配、拷贝、launch 都针对这张卡。

```CPP
__host__ cudaError_t cudaGetDevice(int* device)
```

  查询当前主机线程绑定的设备编号。

```CPP
__host__ cudaError_t cudaGetDeviceProperties(cudaDeviceProp* prop, int device)
```

  填入设备属性。笔记里常用字段：

  * `prop.major` / `prop.minor`：Compute Capability，例如 Turing RTX 2070 为 `7.5`
  * `prop.multiProcessorCount`：SM 数量
  * `prop.warpSize`：通常为 `32`
  * `prop.maxThreadsPerBlock`：每个 block 线程上限，当代卡一般为 `1024`
  * `prop.maxThreadsDim[3]`：`blockDim` 各维上限
  * `prop.maxGridSize[3]`：`gridDim` 各维上限
  * `prop.sharedMemPerBlock`：每 block 静态+动态 shared 上限
  * `prop.maxThreadsPerMultiProcessor`：每 SM 最大驻留线程
  * `prop.regsPerBlock` / `prop.regsPerMultiprocessor`：寄存器预算
  * `prop.totalGlobalMem`：设备 DRAM
  * `prop.concurrentKernels`：是否支持多 kernel 并发
  * `prop.asyncEngineCount`：拷贝引擎个数
  * `prop.managedMemory` / `prop.concurrentManagedAccess`：统一内存能力
  * `prop.cooperativeLaunch`：是否支持 `cudaLaunchCooperativeKernel`

```CPP
__host__ cudaError_t cudaDeviceSynchronize(void)
```

  等待当前设备上**全部** stream 完成。只等一条队列用 `cudaStreamSynchronize`。

```CPP
__host__ cudaError_t cudaDeviceReset(void)
```

  销毁当前设备上该进程的全部分配与上下文。一般只在退出或测试夹具里用。

```CPP
__host__ cudaError_t cudaDeviceGetStreamPriorityRange(int* leastPriority,
                                                      int* greatestPriority)
```

  查询 stream 优先级区间。数值**越小**优先级越高。`greatestPriority` 通常是负数或 `0`。

## 错误处理

参考文档

* [Error Handling](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__ERROR.html)

```CPP
__host__ __device__ cudaError_t cudaGetLastError(void)
```

  取出并**清除**当前主机线程记录的最近一次错误（含非法 launch 配置）。

```CPP
__host__ __device__ cudaError_t cudaPeekAtLastError(void)
```

  取出但不清除。

```CPP
__host__ __device__ const char* cudaGetErrorString(cudaError_t error)
```

  把错误码变成可读字符串。

```CPP
__host__ __device__ const char* cudaGetErrorName(cudaError_t error)
```

  返回枚举名，例如 `"cudaErrorIllegalAddress"`。

`<<<>>>` 启动本身是异步的。越界、非法指令、设备端 `assert` 通常要 `cudaDeviceSynchronize()` 或一次同步 `cudaMemcpy` 之后，再用 `cudaGetLastError()` 才能看到。只检查 launch 下一行、不同步设备，会漏掉 kernel 体内的失败。

```CPP
cudaError_t e = cudaDeviceSynchronize();
if (e != cudaSuccess)
    std::fprintf(stderr, "%s\n", cudaGetErrorString(e));
```

## 设备内存

参考文档

* [Memory Management](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__MEMORY.html)

存储空间含义见 [cuda 笔记 · 内存模型](./cuda_note.md#内存模型)。

```CPP
__host__ __device__ cudaError_t cudaMalloc(void** devPtr, size_t size)
```

  在设备 DRAM 上分配 `size` 字节，把设备指针写到 `*devPtr`。该指针可以传给 kernel，不能传给 `free` / `delete`，也不能在主机上解引用（除非统一寻址）。

```CPP
__host__ __device__ cudaError_t cudaFree(void* devPtr)
```

  释放 `cudaMalloc` / `cudaMallocManaged` 得到的指针。`devPtr == nullptr` 是合法空操作。

```CPP
__host__ __device__ cudaError_t cudaMallocManaged(void** devPtr, size_t size,
                                                  unsigned int flags = cudaMemAttachGlobal)
```

  统一寻址分配。Pascal 以后按页迁移或 page fault。`flags` 常见 `cudaMemAttachGlobal`（全设备可访问）或 `cudaMemAttachHost`。并发访问同一页时仍要遵守可见性规则，不是自动的全卡屏障。

```CPP
__host__ cudaError_t cudaMemPrefetchAsync(const void* devPtr, size_t count,
                                          int dstDevice, cudaStream_t stream = 0)
```

  把 managed 缓冲预取到 `dstDevice`。`cudaCpuDeviceId` 表示预取到主机。

```CPP
__host__ cudaError_t cudaMallocAsync(void** devPtr, size_t size, cudaStream_t hStream)
__host__ cudaError_t cudaFreeAsync(void* devPtr, cudaStream_t hStream)
```

  按 stream 排序的异步分配 / 释放，减少与其它入队操作的隐式同步。需要支持 memory pool 的驱动。

```CPP
__host__ cudaError_t cudaMemset(void* devPtr, int value, size_t count)
__host__ cudaError_t cudaMemsetAsync(void* devPtr, int value, size_t count,
                                     cudaStream_t stream = 0)
```

  按**字节**填充设备内存，类似 `memset`。`value` 只取低 8 位。要填 `float` 的 `0.f` 可以，填 `1.f` 不行。

## 主机页锁定内存

```CPP
__host__ cudaError_t cudaMallocHost(void** ptr, size_t size)
__host__ cudaError_t cudaFreeHost(void* ptr)
```

  分配 / 释放页锁定（pinned）主机内存。DMA 可以直接用，`cudaMemcpyAsync` 要与计算重叠时，主机端缓冲应是 pinned。用 `free` 释放会出错。

```CPP
__host__ cudaError_t cudaHostAlloc(void** pHost, size_t size, unsigned int flags)
```

  带标志的 pinned 分配。`cudaMallocHost` 相当于 `flags == cudaHostAllocDefault`。常见标志：

  * `cudaHostAllocDefault`：普通 pinned
  * `cudaHostAllocPortable`：对其它 CUDA 上下文也 pinned
  * `cudaHostAllocMapped`：映射到设备地址空间，可用 `cudaHostGetDevicePointer` 取设备指针
  * `cudaHostAllocWriteCombined`：写合并，主机读很慢，适合只写一次再 H2D

```CPP
__host__ cudaError_t cudaHostGetDevicePointer(void** pDevice, void* pHost,
                                              unsigned int flags)
```

  取出 `cudaHostAllocMapped` 内存对应的设备指针，可直接传给 kernel（零拷贝，走 PCIE，延迟高）。

`malloc` 得到的可分页内存上，`cudaMemcpyAsync` 往往无法与计算真正重叠：驱动可能先拷进内部 pinned 暂存。

## 拷贝

```CPP
__host__ cudaError_t cudaMemcpy(void* dst, const void* src, size_t count,
                                cudaMemcpyKind kind)
```

  同步拷贝 `count` 字节。调用返回时传输已完成。在默认 stream 上还会等待该 stream 中排在前面的工作，因此常被用来「顺便」同步刚才的 kernel。

```CPP
__host__ __device__ cudaError_t cudaMemcpyAsync(void* dst, const void* src,
                                                size_t count, cudaMemcpyKind kind,
                                                cudaStream_t stream = 0)
```

  把拷贝入队到 `stream`，调用本身通常立即返回。要与 kernel 重叠，主机端应是 pinned。设备到设备的异步拷贝不经过 PCIE，但仍占用拷贝引擎，并遵守所在 stream 的顺序。

```CPP
cudaMemcpyAsync(d, h, n * sizeof(float), cudaMemcpyHostToDevice, s);
kernel<<<grid, block, 0, s>>>(d, n);
cudaMemcpyAsync(h_out, d, n * sizeof(float), cudaMemcpyDeviceToHost, s);
```

```CPP
__host__ cudaError_t cudaMemcpy2D(void* dst, size_t dpitch,
                                  const void* src, size_t spitch,
                                  size_t width, size_t height,
                                  cudaMemcpyKind kind)
```

  二维拷贝。`width` 是每行要拷的**字节**数，`height` 是行数。`spitch` / `dpitch` 是源 / 目的相邻行之间的字节距，必须 `>= width`。矩阵带 padding、或 `cudaMallocPitch` 分配的缓冲用这个，不要用 `cudaMemcpy` 假设行连续。

  对应的异步版本是 `cudaMemcpy2DAsync(..., stream)`。

```CPP
__host__ cudaError_t cudaMemcpyToSymbol(const void* symbol, const void* src,
                                        size_t count, size_t offset = 0,
                                        cudaMemcpyKind kind = cudaMemcpyHostToDevice)
__host__ cudaError_t cudaMemcpyFromSymbol(void* dst, const void* symbol,
                                          size_t count, size_t offset = 0,
                                          cudaMemcpyKind kind = cudaMemcpyDeviceToHost)
```

  读写 `__constant__` 或 `__device__` 符号。`symbol` 写符号名即可，例如 `cudaMemcpyToSymbol(c_table, h, bytes)`。`offset` 是符号内字节偏移。异步版本带 `Async` 后缀并接受 `stream`。

同一 stream 里：kernel 与其后的拷贝有序。不同 stream 默认并发，要看见对方的写，用 `cudaEvent` 或 `cudaDeviceSynchronize`。

## Occupancy 查询

参考文档

* [Occupancy](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__OCCUPANCY.html)

```CPP
__host__ __device__ cudaError_t
cudaOccupancyMaxActiveBlocksPerMultiprocessor(int* numBlocks, const void* func,
                                              int blockSize, size_t dynamicSMemSize)
```

  给定 kernel `func`、每 block 线程数、动态 shared 字节数，返回每 SM 最多能驻留多少 block。用来估 occupancy，或反推 `grid` 取多大能铺满卡。

```CPP
int blocks_per_sm = 0;
cudaOccupancyMaxActiveBlocksPerMultiprocessor(&blocks_per_sm, saxpy, 256, 0);
int grid = blocks_per_sm * prop.multiProcessorCount;
```

occupancy 高不等于一定快，过低时访存等待期间可能没有其它 eligible warp。见 [cuda 笔记 · Occupancy](./cuda_note.md#occupancy)。

## Stream

参考文档

* [Stream Management](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__STREAM.html)

语义见 [cuda 笔记 · Stream 与 Event](./cuda_note.md#stream-与-event)。同一条 stream 内按入队顺序完成；不同 stream 之间没有默认先后关系。

```CPP
__host__ cudaError_t cudaStreamCreate(cudaStream_t* pStream)
```

  创建一条异步 stream，等价于 `cudaStreamCreateWithFlags(pStream, cudaStreamDefault)`。

```CPP
__host__ __device__ cudaError_t cudaStreamCreateWithFlags(cudaStream_t* pStream,
                                                          unsigned int flags)
```

  * `cudaStreamDefault`：可能与 legacy 默认 stream 隐式互等
  * `cudaStreamNonBlocking`：不与默认 stream 隐式互相等待，便于和默认队列上的工作并发

```CPP
__host__ cudaError_t cudaStreamCreateWithPriority(cudaStream_t* pStream,
                                                  unsigned int flags, int priority)
```

  带优先级的 stream。`priority` 落在 `cudaDeviceGetStreamPriorityRange` 给出的闭区间内，越小越高。不改变同一 stream 内的顺序。

```CPP
__host__ __device__ cudaError_t cudaStreamDestroy(cudaStream_t stream)
```

  销毁 stream。若其上仍有未完成工作，工作会做完再释放句柄；主机调用可以先返回。销毁前同步更清晰。

```CPP
__host__ cudaError_t cudaStreamSynchronize(cudaStream_t stream)
```

  阻塞主机，直到 `stream` 中已入队工作全部完成。

```CPP
__host__ cudaError_t cudaStreamQuery(cudaStream_t stream)
```

  非阻塞查询。已完成返回 `cudaSuccess`，尚未完成返回 `cudaErrorNotReady`。

```CPP
__host__ cudaError_t cudaStreamWaitEvent(cudaStream_t stream, cudaEvent_t event,
                                         unsigned int flags = 0)
```

  使 `stream` 在后续操作开始前等待 `event`。`flags` 一般传 `0`。这是跨 stream 表达依赖的标准手段。

`<<<grid, block, dyn_shared_bytes, stream>>>` 的第四个参数指定本次启动进入哪条队列，不改变 grid/block 尺寸。

### 默认 stream

第四个参数省略、传入 `0` 或 `cudaStreamDefault`，即默认 stream。

传统（legacy）默认 stream 还会与同一设备上其它非 `NonBlocking` stream 隐式同步。`nvcc --default-stream per-thread` 改为每主机线程一条自己的默认 stream，且不再做上述隐式同步。要重叠拷贝与计算，应创建显式 `cudaStream_t`，使用 `cudaMemcpyAsync`。

## Event

参考文档

* [Event Management](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__EVENT.html)

`cudaEvent_t` 标记某条 stream 时间线上的一点，用于跨 stream 依赖或测时。

```CPP
__host__ cudaError_t cudaEventCreate(cudaEvent_t* event)
```

  创建默认可计时 event。

```CPP
__host__ cudaError_t cudaEventCreateWithFlags(cudaEvent_t* event, unsigned int flags)
```

  常见标志：

  * `cudaEventDefault`：可计时
  * `cudaEventDisableTiming`：仅同步，开销更低
  * `cudaEventBlockingSync`：`cudaEventSynchronize` 时主机线程阻塞而非自旋
  * `cudaEventInterprocess`：可跨进程导出

```CPP
__host__ cudaError_t cudaEventDestroy(cudaEvent_t event)
```

  销毁 event。

```CPP
__host__ cudaError_t cudaEventRecord(cudaEvent_t event, cudaStream_t stream = 0)
```

  把「记录」入队到 `stream`：该 stream 上排在 record 之前的操作完成后，event 变为已触发。

```CPP
__host__ cudaError_t cudaEventSynchronize(cudaEvent_t event)
```

  阻塞主机，直到 `event` 已触发（即记录点之前的工作已完成）。

```CPP
__host__ cudaError_t cudaEventQuery(cudaEvent_t event)
```

  非阻塞查询是否已触发。完成返回 `cudaSuccess`，否则 `cudaErrorNotReady`。

```CPP
__host__ cudaError_t cudaEventElapsedTime(float* ms, cudaEvent_t start, cudaEvent_t end)
```

  两个已触发、且创建时未设 `cudaEventDisableTiming` 的 event 之间的毫秒数，分辨率约 `0.5 µs` 量级。主机侧 `std::chrono` 会把 launch 入队时间和 CPU 同步开销算进去，不能代替这对 event。第一次 launch 常含 JIT / 上下文初始化，测稳态应预热。

```CPP
cudaEventRecord(start, s);
kernel<<<grid, block, 0, s>>>(...);
cudaEventRecord(stop, s);
cudaEventSynchronize(stop);
float ms = 0.f;
cudaEventElapsedTime(&ms, start, stop);
```

有数据依赖的两项必须排在同一 stream，或用 event 显式等待；排进两条 stream 且不加 event，是数据竞争。

## CUDA Graph

参考文档

* [Graph Management](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__GRAPH.html)
* [Stream Capture](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__STREAM.html)

CUDA Graph 把 kernel、拷贝等操作以及它们的依赖记录成 DAG，再实例化为可反复提交的 `cudaGraphExec_t`。适合训练 step、推理或迭代算法这类**操作序列基本固定且重复很多次**的场景，主要减少 CPU 逐项 launch 和驱动调度开销；不会让单个 kernel 本身更快。

最简单的构图方式是捕获一条 stream 上的操作：

```CPP
__host__ cudaError_t cudaStreamBeginCapture(cudaStream_t stream,
                                            cudaStreamCaptureMode mode)
__host__ cudaError_t cudaStreamEndCapture(cudaStream_t stream,
                                          cudaGraph_t* pGraph)
```

`cudaStreamBeginCapture` 之后入队到捕获域中的操作暂不执行，而是成为图节点；必须在发起捕获的同一条 stream 上调用 `cudaStreamEndCapture`。捕获不能从 legacy 默认 stream 开始，通常使用显式创建的 `cudaStreamNonBlocking` stream。

常见捕获模式：

* `cudaStreamCaptureModeGlobal`：限制最严格，捕获期间会检查其它线程中的潜在不安全 CUDA 调用
* `cudaStreamCaptureModeThreadLocal`：只对发起捕获的主机线程做上述限制
* `cudaStreamCaptureModeRelaxed`：限制最少，由调用方保证捕获期间没有破坏依赖的操作

```CPP
__host__ cudaError_t cudaGraphInstantiate(cudaGraphExec_t* pGraphExec,
                                          cudaGraph_t graph,
                                          unsigned long long flags = 0)
```

把图定义检查并实例化为可执行图。实例化可能有明显开销，应在循环外完成。`flags` 没有特殊需求时传 `0`。

```CPP
__host__ __device__ cudaError_t cudaGraphLaunch(cudaGraphExec_t graphExec,
                                                cudaStream_t stream)
```

把整张可执行图异步提交到 `stream`。调用返回只表示已入队；主机要等结果仍需 `cudaStreamSynchronize`、Event 或其它同步手段。

```CPP
__host__ cudaError_t cudaGraphUpload(cudaGraphExec_t graphExec,
                                     cudaStream_t stream)
```

提前把执行图所需资源上传到设备，避免第一次 `cudaGraphLaunch` 才做准备。上传与随后在同一 stream 中的 launch 有序；普通场景可以不显式调用。

完整的 stream capture 调用序，其中 `h_x` / `h_y` 是 `cudaMallocHost` 等方式得到的 pinned 主机内存：

```CPP
cudaStream_t s;
cudaGraph_t graph;
cudaGraphExec_t graph_exec;

cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking);

cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal);
cudaMemcpyAsync(d_x, h_x, bytes, cudaMemcpyHostToDevice, s);
kernel<<<grid, block, 0, s>>>(d_x, d_y, n);
cudaMemcpyAsync(h_y, d_y, bytes, cudaMemcpyDeviceToHost, s);
cudaStreamEndCapture(s, &graph);

cudaGraphInstantiate(&graph_exec, graph, 0);

for (int iter = 0; iter < iterations; ++iter)
    cudaGraphLaunch(graph_exec, s);

cudaStreamSynchronize(s);
```

捕获期间不要调用 `cudaDeviceSynchronize`、`cudaStreamSynchronize` 或会引入隐式同步的同步 API。图会保存 kernel 参数值和指针地址；重复 launch 前必须保证对应内存仍然有效。需要改变参数或图定义时，可以重新捕获，或更新已有执行图：

```CPP
__host__ cudaError_t cudaGraphExecUpdate(
    cudaGraphExec_t graphExec,
    cudaGraph_t graph,
    cudaGraphExecUpdateResultInfo* resultInfo)
```

当新图与原执行图的拓扑和节点类型兼容时原地更新；不兼容时应重新 `cudaGraphInstantiate`。`resultInfo` 返回更新结果及失败位置。

不用后按依赖关系释放句柄：

```CPP
__host__ cudaError_t cudaGraphExecDestroy(cudaGraphExec_t graphExec)
__host__ cudaError_t cudaGraphDestroy(cudaGraph_t graph)
```

```CPP
cudaGraphExecDestroy(graph_exec);
cudaGraphDestroy(graph);
cudaStreamDestroy(s);
```

实例化完成后，`graphExec` 不依赖原 `graph` 的生命周期，因此也可以更早销毁 `graph`。同一个 Graph 对象不保证多主机线程并发访问安全，需要由调用方串行化。

除了 stream capture，也可以用 `cudaGraphCreate`、`cudaGraphAddKernelNode`、`cudaGraphAddMemcpyNode`、`cudaGraphAddDependencies` 显式构造复杂 DAG；日常固定流水线优先用 capture，代码更短且不需要手工填写节点参数结构体。

## Kernel 启动

参考文档

* [Execution Control](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__EXECUTION.html)

```CPP
kernel<<<grid, block>>>(args...);
kernel<<<grid, block, dyn_shared_bytes, stream>>>(args...);
```

  语言扩展，不是普通函数调用。四个配置项：grid 尺寸、block 尺寸、动态 shared memory 字节数（默认 `0`）、stream（默认 `0`）。

  * `grid` / `block` 可以是 `dim3` 或单个 `int`
  * 动态 shared 是每个 block 一块连续区域，kernel 里用 `extern __shared__ T smem[]` 接收
  * 启动本身只检查配置是否明显非法（例如 block 线程数超过 `1024`）并把 kernel 放入指定 stream
  * `<<<>>>` 在默认 stream 上是异步的：入队后即可返回

`__global__` 必须 `void`，参数按值传递。指针必须指向设备可访问的存储。参数总量有上限（当代工具链通常为数 KB 到 32KB 量级），大数组应放在 `cudaMalloc` 的缓冲里传指针。

```CPP
__host__ cudaError_t cudaLaunchCooperativeKernel(const void* func, dim3 gridDim,
                                                 dim3 blockDim, void** args,
                                                 size_t sharedMem, cudaStream_t stream)
```

  协作启动，使同一次 launch 的全部 block 同时驻留，从而可以在 kernel 里做 grid 级同步。block 数 × 每 block 资源超过整卡容量时 launch 会失败。用不上时，拆成两个 kernel 更简单。

```CPP
void* args[] = { &n, &d_x, &d_y };
cudaLaunchCooperativeKernel((void*)kernel, grid, block, args, 0, stream);
```

设备端：

```CPP
namespace cg = cooperative_groups;
cg::grid_group grid = cg::this_grid();
grid.sync();
```

## 核函数编写

参考文档

* [Built-in Variables](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#built-in-variables)
* [cuda 笔记 · 线程模型](./cuda_note.md#线程模型)

核函数是 `__global__` 函数：主机 `<<<>>>` 启动，设备上每个线程执行同一份函数体，靠内置索引区分自己该处理哪份数据。启动配置见上一节；同步、原子、shared 见后文各节。

### 声明

```CPP
__global__ void saxpy(int n, float a, const float* x, float* y);
```

  * 必须 `__global__`，必须返回 `void`
  * 参数按值传递；指针必须指向设备可访问的存储（`cudaMalloc` / managed），不能是 `malloc` 的主机指针
  * 参数总量有上限（当代工具链通常为数 KB 到 32KB 量级），大数组放设备缓冲里传指针
  * 只能调用 `__device__` / `__global__`（动态并行），不能调用普通 `__host__` 函数
  * 函数体里可用 `threadIdx` 等内置变量，主机函数里没有这些变量

```CPP
__device__ float fma_dev(float a, float b, float c) { return a * b + c; }

__global__ void saxpy(int n, float a, const float* x, float* y) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) y[i] = fma_dev(a, x[i], y[i]);
}
```

### 内置索引

kernel 里只读、由硬件填好：

| 变量 | 类型 | 含义 |
|------|------|------|
| `threadIdx` | `uint3` | 当前线程在本 block 内的坐标 |
| `blockIdx` | `uint3` | 当前 block 在 grid 内的坐标 |
| `blockDim` | `dim3` | 一个 block 的尺寸 |
| `gridDim` | `dim3` | grid 里有多少 block |
| `warpSize` | `int` | 通常为 `32` |

一维线性全局下标：

```CPP
int i = blockIdx.x * blockDim.x + threadIdx.x;
```

二维（图像 / 矩阵，`x` 走列、`y` 走行）：

```CPP
int col = blockIdx.x * blockDim.x + threadIdx.x;
int row = blockIdx.y * blockDim.y + threadIdx.y;
```

三维：

```CPP
int x = blockIdx.x * blockDim.x + threadIdx.x;
int y = blockIdx.y * blockDim.y + threadIdx.y;
int z = blockIdx.z * blockDim.z + threadIdx.z;
```

block 内三维压成线性 tid（shared memory 下标常用）：

```CPP
int tid = threadIdx.z * blockDim.x * blockDim.y
        + threadIdx.y * blockDim.x
        + threadIdx.x;
```

`threadIdx` 的排列是 `x` 最先变，然后 `y`，然后 `z`。warp 按这个线性顺序切：tid `0..31` 是 warp 0。要 coalesced 访问，让相邻 `threadIdx.x` 读相邻地址。

### 越界

`grid * block` 往往大于 `n`，多出来的线程必须自己挡住：

```CPP
__global__ void add_one(float* x, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) x[i] += 1.f;
}
```

主机侧常用：

```CPP
add_one<<<(n + 255) / 256, 256>>>(d, n);
```

  `(n + 255) / 256` 是向上取整的 block 数，保证线程总数 `>= n`。最后几个 block 仍可能有 `i >= n` 的线程，所以 `if (i < n)` 不能省。

block 大小常用 `128` / `256` / `512`，且为 `32` 的倍数。每 block 最多 `1024` 线程。

### grid-stride

元素数可能远大于一次能启动的线程数，或希望 grid 只铺满卡、每线程走多个元素：

```CPP
__global__ void saxpy(int n, float a, const float* x, float* y) {
    for (int i = blockIdx.x * blockDim.x + threadIdx.x;
         i < n;
         i += gridDim.x * blockDim.x) {
        y[i] = a * x[i] + y[i];
    }
}
```

  步长 `gridDim.x * blockDim.x` 是本次启动的线程总数。每个线程处理一组等间隔元素，越界仍用 `i < n` 挡住。这是最常见的 1D 映射。

二维同类写法：

```CPP
int stride_x = gridDim.x * blockDim.x;
int stride_y = gridDim.y * blockDim.y;
for (int row = blockIdx.y * blockDim.y + threadIdx.y; row < H; row += stride_y)
    for (int col = blockIdx.x * blockDim.x + threadIdx.x; col < W; col += stride_x)
        out[row * W + col] = ...;
```

铺满卡的 grid 可配合 [Occupancy 查询](#occupancy-查询)：

```CPP
int blocks_per_sm = 0;
cudaOccupancyMaxActiveBlocksPerMultiprocessor(&blocks_per_sm, saxpy, 256, 0);
int grid = blocks_per_sm * prop.multiProcessorCount;
saxpy<<<grid, 256>>>(n, a, d_x, d_y);
```

### 核函数里能做什么

可以：

* 读写设备指针、`__shared__` / `__constant__` / 寄存器
* 调用 `__device__`、设备数学库（`sinf`、`sqrtf` 等）、原子、warp 原语
* `printf`（输出经驱动缓冲，主机同步后才完整看到）
* `assert`（失败表现为异步设备错误，同步后才能在主机上看到）

不要：

* 解引用主机 `malloc` 指针
* 调用普通主机函数 / 大部分 C++ 标准库
* 在分歧路径里只有部分线程执行 `__syncthreads()`（死锁）
* 假设 `blockIdx.x == 0` 先于 `blockIdx.x == 1` 执行
* 用 kernel 返回值把结果传回主机（必须 `void`，结果写设备内存再 `cudaMemcpy`）

同 block 协作（tiling、归约）用 `__shared__` + `__syncthreads()`；跨 block 默认不同步，写 global 后结束本次 kernel 再 launch，或用原子 / cooperative grid sync。见 [设备端同步](#设备端同步)、[原子](#原子)。

## 设备端同步

参考文档

* [Synchronization Functions](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#synchronization-functions)

```CPP
void __syncthreads()
```

  block 内屏障：该 block **所有** 线程都到达，并且此前对本 block 可见的 shared / global 写，对块内其他线程可见。必须所有线程都执行到（或按文档在同一条件路径上），否则死锁。不管其他 block。

同族还有带谓词 / 计数的版本，仍要求全体线程执行到：

* `__syncthreads_count(int pred)`：到达后返回本 block 中 `pred` 为真的线程数
* `__syncthreads_and(int pred)`：全体 `pred` 为真则为非 0
* `__syncthreads_or(int pred)`：任一 `pred` 为真则为非 0

```CPP
void __syncwarp(unsigned mask = 0xffffffff)
```

  warp 内屏障：`mask` 里的 lane 都执行到这里，此前这些 lane 对 shared / global 的写对彼此可见，PC 重新对齐。`mask` 必须和实际活跃 lane 一致，否则未定义。Volta / Turing 及以后应显式使用带 mask 的 warp 原语。

## Named barrier 与 `cuda::barrier`

参考文档

* [PTX: bar / barrier](https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#parallel-synchronization-and-communication-instructions-bar)
* [Asynchronous Barriers](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/async-barriers.html)
* [libcu++ cuda::barrier](https://nvidia.github.io/cccl/libcudacxx/extended_api/synchronization_primitives/barrier.html)

C++ 对象在 shared memory 里。必须先由一个线程初始化，再用 `__syncthreads()`（或等价的全员同步）做启动同步，之后才能参加。

```CPP
#include <cuda/barrier>

__shared__ cuda::barrier<cuda::thread_scope_block> bar;

if (threadIdx.x == 0)
    init(&bar, expected_arrivals);
__syncthreads();
```

`expected_arrivals` 是本阶段期望的 `arrive` 次数，不是 `blockDim` 的默认值。

```CPP
cuda::barrier<cuda::thread_scope_block>::arrival_token
    token = bar.arrive();
bar.wait(cuda::std::move(token));
bar.arrive_and_wait();
bar.arrive_and_drop();
```

  `arrive` 不阻塞，返回当前 phase 的 token。`wait` 等到该 phase 完成（倒数到 0 并复位）。`arrive_and_wait` 合并两步。`arrive_and_drop` 完成本阶段义务，并降低后续 phase 的期望人数，供提前退出的线程使用。token 只能用于当前或紧邻的上一 phase，否则未定义。

全员到齐仍应优先 `__syncthreads()`。只约一部分 warp、或要在 arrive 和 wait 之间插入无关计算时，再用 named barrier / `cuda::barrier`。

## mbarrier 与 TMA

参考文档

* [Asynchronous Barriers：Tracking Asynchronous Memory Operations](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/async-barriers.html)
* [Asynchronous Data Copies / TMA](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/async-copies.html)
* [PTX: mbarrier](https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#parallel-synchronization-and-communication-instructions-mbarrier)
* [PTX: cp.async.bulk](https://docs.nvidia.com/cuda/parallel-thread-execution/index.html#data-movement-and-conversion-instructions-cp-async-bulk)

Hopper（CC 9.0）上的 transaction barrier（`mbarrier`）在 arrive 之外再跟踪 **事务计数**（通常是字节）。阶段完成 = 期望的 arrive 到齐 **且** 登记过的异步字节搬完。对象是 shared memory 中 8 字节对齐的硬件状态。概念见 [cuda 笔记](./cuda_note.md#mbarrier-与-tma)。

```CPP
#include <cuda/ptx>

__shared__ uint64_t bar;

if (threadIdx.x == 0)
    cuda::ptx::mbarrier_init(&bar, arrival_count);
__syncthreads();
```

```CPP
cuda::ptx::mbarrier_arrive(&bar);
cuda::ptx::mbarrier_expect_tx(&bar, nbytes);
cuda::ptx::mbarrier_arrive_expect_tx(/* sem, scope, space, */ &bar, /* arrive */, nbytes);

while (!cuda::ptx::mbarrier_try_wait(&bar, token)) {}
while (!cuda::ptx::mbarrier_try_wait_parity(&bar, phase_parity)) {}
```

  `arrive` 只减到达计数。`expect_tx` 只增加本阶段要等到的字节。`arrive_expect_tx` 两步合成一次。`try_wait` 按 token 等当前 phase；`try_wait_parity` 按 phase 奇偶（0/1）等，适合「一个线程 arrive+expect_tx，其余线程只 wait」。未完成时 `try_wait*` 可能返回 false，需要重试。

`cuda::barrier` 在 SM90 上可走同一硬件。高层登记事务：

```CPP
#include <cuda/barrier>

cuda::device::barrier_expect_tx(
    cuda::device::barrier_native_handle(bar), nbytes);
auto token = cuda::device::barrier_arrive_tx(bar, /* arrive */ 1, nbytes);
bar.wait(cuda::std::move(token));
```

TMA 把 global 上的整块（或 tensor tile）异步拷进 shared。完成时硬件对绑定的 mbarrier 做 `complete_tx`（按字节）。`cuda::memcpy_async(..., bar)` 会自动 `expect_tx` / `complete_tx`。`cuda::device::memcpy_async_tx` 与 `cuda::ptx::cp_async_bulk` 必须自己 `expect_tx`。

## Warp 原语

参考文档

* [Warp Shuffle Functions](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#warp-shuffle-functions)
* [Warp Vote Functions](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#warp-vote-functions)

第一个参数 `mask`（常见 `0xffffffff`）标明哪些 lane 参加这次握手。部分线程已经 `return`、或分歧导致没执行到这条指令时，`mask` 必须和实际活跃 lane 一致。

```CPP
T __shfl_sync(unsigned mask, T var, int srcLane, int width = warpSize)
```

  每个参加的 lane 读 `srcLane` 的 `var`。可做广播：`srcLane == 0` 时全 warp 拿到 lane 0 的值。

```CPP
T __shfl_up_sync(unsigned mask, T var, unsigned int delta, int width = warpSize)
```

  lane `k` 收到 lane `k - delta` 的值。没有更左侧源的 lane 保持原值。

```CPP
T __shfl_down_sync(unsigned mask, T var, unsigned int delta, int width = warpSize)
```

  lane `k` 收到 lane `k + delta` 的值。没有更右侧源的 lane 保持原值。warp 内归约常用：反复 `delta = 1, 2, 4, 8, 16`，最后 lane 0 拿到 32 个数的和。

```CPP
T __shfl_xor_sync(unsigned mask, T var, int laneMask, int width = warpSize)
```

  与 lane `k xor laneMask` 交换。归约也常用（butterfly）。

`width` 必须是 `2` 的幂且 `<= warpSize`，用来把 warp 切成更小的子组。

```CPP
unsigned int __ballot_sync(unsigned mask, int pred)
```

  每个参加的 lane 出一个布尔谓词，硬件收成一个 32-bit 掩码：bit `i` 为 1 表示 lane `i` 的谓词为真。全 warp 得到的 `mask` 相同。

```CPP
int __all_sync(unsigned mask, int pred)
int __any_sync(unsigned mask, int pred)
```

  参加的 lane 是否全部 / 任一 `pred` 为真。结果在参加的 lane 上相同。

```CPP
int __popc(unsigned int x)
```

  按位计数，返回 `x` 里有多少个 bit 为 1。每线程独立的整数指令，不是通信原语。常跟 `__ballot_sync` 连用：`__popc(__ballot_sync(0xffffffff, pred))` 得到本 warp 中谓词为真的个数。

同族还有 `__clz`（前导零个数）、`__ffs`（最低为 1 的 bit 位置，从 1 计；全 0 返回 0）。

## 内存栅栏

参考文档

* [Memory Fence Functions](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#memory-fence-functions)

内存栅栏约束当前线程栅栏前后的内存访问顺序及作用范围，不召唤其他线程，也不能当 barrier 用。只有与原子操作等同步机制配合建立 happens-before，另一个线程才能据此安全读取普通内存。

```CPP
void __threadfence_block()
```

  block 作用域的顺序一致栅栏。等价于：

```CPP
cuda::atomic_thread_fence(cuda::memory_order_seq_cst,
                          cuda::thread_scope_block);
```

```CPP
void __threadfence()
```

  device 作用域的顺序一致栅栏。等价于：

```CPP
cuda::atomic_thread_fence(cuda::memory_order_seq_cst,
                          cuda::thread_scope_device);
```

```CPP
void __threadfence_system()
```

  system 作用域的顺序一致栅栏，范围扩展到主机和 peer 设备。等价于：

```CPP
cuda::atomic_thread_fence(cuda::memory_order_seq_cst,
                          cuda::thread_scope_system);
```

使用 `<cuda/atomic>` 可以分别选择内存序和作用域：

* `cuda::memory_order_relaxed`：只保证原子性，不为周围的普通内存访问建立顺序
* `cuda::memory_order_acquire`：该操作之后的访问不能移动到它之前，常用于读取已发布的数据
* `cuda::memory_order_release`：该操作之前的访问不能移动到它之后，常用于发布数据
* `cuda::memory_order_acq_rel`：同时具有 acquire 和 release 语义
* `cuda::memory_order_seq_cst`：最强的顺序一致语义

`cuda::thread_scope_block`、`cuda::thread_scope_device`、`cuda::thread_scope_system` 分别限定保证覆盖同 block、同设备和整个系统。作用域应取能够覆盖通信双方的最小范围，范围越大通常同步代价越高。

典型错误：block 0 写 global，然后 `__threadfence()`，以为 block 1 一定已经能读。fence 不保证对方已经执行到读之前，也不保证对方已经启动。跨 block 通信还要原子标志或拆成两个 kernel。

## 原子

参考文档

* [Atomic Functions](https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#atomic-functions)

保证对该地址的读改写不被拆开。原子**不等于**「所有线程都执行到这里」，也不替代 `__syncthreads()`。传统 `atomicAdd`、`atomicCAS` 等接口默认是 device 作用域、relaxed 内存序；后缀 `_block` / `_system` 分别改变为 block / system 作用域。

```CPP
T atomicAdd(T* address, T val)
```

  `*address += val`，返回旧值。`T` 常见 `int`、`unsigned`、`unsigned long long`、`float`、`double`。

```CPP
T atomicSub(T* address, T val)
T atomicExch(T* address, T val)
T atomicMin(T* address, T val)
T atomicMax(T* address, T val)
T atomicInc(T* address, T val)
T atomicDec(T* address, T val)
T atomicAnd(T* address, T val)
T atomicOr(T* address, T val)
T atomicXor(T* address, T val)
```

  * `atomicExch`：交换并返回旧值；传统接口本身是 relaxed 内存序
  * `atomicInc`：`(*address >= val) ? 0 : (*address + 1)`，无符号
  * `atomicDec`：`((*address == 0) || (*address > val)) ? val : (*address - 1)`，无符号

```CPP
T atomicCAS(T* address, T compare, T val)
```

  比较并交换：若 `*address == compare` 则写成 `val`。**总是返回旧值**。用来实现锁或无锁更新：

```CPP
int old = *p, assumed;
do {
    assumed = old;
    old = atomicCAS(p, assumed, assumed + 1);
} while (assumed != old);
```

需要「先写数据、再发布标志」时，优先用 `cuda::atomic` / `cuda::atomic_ref` 直接表达 acquire-release 关系：

```CPP
#include <cuda/atomic>

cuda::atomic_ref<int, cuda::thread_scope_device> ready(*flag);

// 生产者
data[i] = v;
ready.store(1, cuda::memory_order_release);

// 消费者
if (ready.load(cuda::memory_order_acquire) == 1)
    use(data[i]);
```

当 acquire load 读到 release store 写入的值时，生产者在 release 之前对 `data[i]` 的写 happens-before 消费者之后的读取。`flag` 必须是原子对象或通过满足对齐要求的 `atomic_ref` 访问；普通 store/load 即使夹着 fence，也不能替代原子同步。

## 设备端内存限定符

```CPP
__shared__ T var;
extern __shared__ T smem[];
```

  每 block 一份，生命周期为 block 执行期间。静态版本编译期定大小；动态版本大小来自 launch 第三个参数，单位是**字节**。同 block 里 A 写 B 读必须 `__syncthreads()`。

```CPP
__constant__ T var;
```

  设备端只读符号，总量通常 64KB。同一 warp 读同一地址走广播，适合系数、小表。Host 用 `cudaMemcpyToSymbol` 写入。

```CPP
__device__ T var;
```

  模块级设备全局变量，位于 global memory。主机同样用 `cudaMemcpyToSymbol` / `cudaMemcpyFromSymbol` 访问。

```CPP
__restrict__
```

  告诉编译器该指针不与其它指针别名，有利于走只读缓存、向量化。只读 global 常写成 `const float* __restrict__ in`。

```CPP
T __ldg(const T* ptr)
```

  经只读路径加载。当代编译器对 `const` / `__restrict__` 指针常常自动生成等价指令。

## 常见调用序

```CPP
cudaSetDevice(0);

float *d_x = nullptr, *d_y = nullptr;
cudaMalloc(&d_x, n * sizeof(float));
cudaMalloc(&d_y, n * sizeof(float));
cudaMemcpy(d_x, h_x, n * sizeof(float), cudaMemcpyHostToDevice);
cudaMemcpy(d_y, h_y, n * sizeof(float), cudaMemcpyHostToDevice);

saxpy<<<(n + 255) / 256, 256>>>(n, a, d_x, d_y);

cudaMemcpy(h_y, d_y, n * sizeof(float), cudaMemcpyDeviceToHost);
cudaFree(d_x);
cudaFree(d_y);
```

异步流水线把同步拷贝换成 `cudaMemcpyAsync` + 显式 stream，主机缓冲用 `cudaMallocHost`。完整语义见笔记中的 [Runtime API 调用序](./cuda_note.md#runtime-api-调用序) 与 [多 stream 流水线](./cuda_note.md#多-stream-流水线)。
