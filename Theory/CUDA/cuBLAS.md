# cuBLAS

cuBLAS（CUDA Basic Linear Algebra Subprograms）是 NVIDIA 提供的 GPU 线性代数库。它实现 BLAS 的向量、矩阵向量和矩阵矩阵运算，并针对不同 GPU 架构、数据类型和 Tensor Core 做了优化。日常代码通常不应自己重写通用矩阵乘法 kernel，而应先使用 cuBLAS。

本文默认使用传统 cuBLAS API（`cublas_v2.h`）。CUDA Runtime、设备内存和 stream 的基础见 [cuda api](./cuda_api.md) 与 [cuda 笔记](./cuda_note.md)。

参考文档

* [cuBLAS Documentation](https://docs.nvidia.com/cuda/cublas/index.html)
* [cuBLAS API Reference](https://docs.nvidia.com/cuda/cublas/index.html#cublas-api)
* [cuBLASLt](https://docs.nvidia.com/cuda/cublas/index.html#using-the-cublaslt-api)
* [BLAS Quick Reference](https://www.netlib.org/blas/blasqr.pdf)

## BLAS 分级

| 级别 | 运算对象 | 典型运算 | 计算量 |
|------|----------|----------|--------|
| Level 1 | 向量—向量 | `AXPY`、点积、范数 | \(O(n)\) |
| Level 2 | 矩阵—向量 | `GEMV` | \(O(mn)\) |
| Level 3 | 矩阵—矩阵 | `GEMM`、`TRSM` | \(O(mnk)\) |

Level 3 的计算量相对访存量更大，更容易发挥 GPU 算力。很短的向量或很小的矩阵可能被 kernel launch、数据传输和同步开销主导，不一定比 CPU BLAS 快。

常见缩写：

| 缩写 | 含义 |
|------|------|
| `AXPY` | \(y = \alpha x + y\) |
| `DOT` | \(x^T y\) |
| `NRM2` | \(\sqrt{\sum_i x_i^2}\) |
| `GEMV` | \(y = \alpha op(A)x + \beta y\) |
| `GEMM` | \(C = \alpha op(A)op(B) + \beta C\) |
| `TRSM` | 解三角矩阵方程 |

## 头文件、链接与命名

```CPP
#include <cublas_v2.h>
```

用 nvcc 直接编译：

```shell
nvcc cublas_demo.cu -lcublas -o cublas_demo
```

CMake 推荐使用 CUDA Toolkit 导入目标：

```cmake
find_package(CUDAToolkit REQUIRED)
target_link_libraries(app PRIVATE CUDA::cublas)
```

传统 API 名称一般为：

```text
cublas + 数据类型前缀 + BLAS 名称
```

| 前缀 | 元素类型 |
|------|----------|
| `S` | `float` |
| `D` | `double` |
| `C` | `cuComplex`，单精度复数 |
| `Z` | `cuDoubleComplex`，双精度复数 |

例如 `cublasSaxpy`、`cublasDdot`、`cublasCgemm`。支持混合类型和显式计算精度的接口通常以 `Ex` 结尾，例如 `cublasGemmEx`。

## Handle 与错误处理

### 创建和销毁

```CPP
cublasStatus_t cublasCreate(cublasHandle_t* handle);
cublasStatus_t cublasDestroy(cublasHandle_t handle);
```

`cublasHandle_t` 保存设备、stream、pointer mode、math mode 等库状态。典型生命周期：

```CPP
cublasHandle_t handle = nullptr;
cublasCreate(&handle);

// 多次调用 cuBLAS

cublasDestroy(handle);
```

注意：

* `cublasCreate` 可能初始化 CUDA 上下文并分配内部资源，放在热循环外。
* handle 绑定创建它时的当前设备。切换设备后不要继续用原 handle 操作另一张卡。
* cuBLAS API 本身支持多主机线程，但共享 handle 时，stream、pointer mode 等可变配置也被共享。要并行使用不同配置，通常每个线程、每个设备各自创建 handle。
* 销毁 handle 前，应确保依赖它所提交工作的资源生命周期正确。

### 状态码

几乎所有 cuBLAS 调用返回 `cublasStatus_t`：

| 状态 | 含义 |
|------|------|
| `CUBLAS_STATUS_SUCCESS` | 成功 |
| `CUBLAS_STATUS_NOT_INITIALIZED` | 库未初始化 |
| `CUBLAS_STATUS_ALLOC_FAILED` | 内部资源分配失败 |
| `CUBLAS_STATUS_INVALID_VALUE` | 参数、尺寸或 leading dimension 非法 |
| `CUBLAS_STATUS_ARCH_MISMATCH` | 当前 GPU 不支持所需能力 |
| `CUBLAS_STATUS_MAPPING_ERROR` | 访问 GPU 内存失败 |
| `CUBLAS_STATUS_EXECUTION_FAILED` | GPU 上执行失败 |
| `CUBLAS_STATUS_NOT_SUPPORTED` | 参数组合或算法不受支持 |

cuBLAS 状态与 CUDA Runtime 错误是两套错误域，二者都要检查：

```CPP
#define CUDA_CHECK(call)                                                   \
    do {                                                                   \
        cudaError_t err__ = (call);                                        \
        if (err__ != cudaSuccess) {                                        \
            std::fprintf(stderr, "CUDA error %s:%d: %s\n",                 \
                         __FILE__, __LINE__, cudaGetErrorString(err__));    \
            std::exit(EXIT_FAILURE);                                       \
        }                                                                  \
    } while (0)

#define CUBLAS_CHECK(call)                                                 \
    do {                                                                   \
        cublasStatus_t status__ = (call);                                  \
        if (status__ != CUBLAS_STATUS_SUCCESS) {                           \
            std::fprintf(stderr, "cuBLAS error %s:%d: %d\n",               \
                         __FILE__, __LINE__, static_cast<int>(status__));   \
            std::exit(EXIT_FAILURE);                                       \
        }                                                                  \
    } while (0)
```

cuBLAS 调用通常只负责把工作提交到 stream。非法地址等设备端错误可能要到 `cudaStreamSynchronize`、`cudaDeviceSynchronize` 或同步拷贝时才暴露，因此只检查 `cublasStatus_t` 不够。

## 列主序与 leading dimension

### 列主序

传统 BLAS 源于 Fortran，cuBLAS 默认把矩阵解释为**列主序**。矩阵 \(A\) 有 `rows` 行，元素 `A(row, col)` 的线性下标是：

```CPP
index = row + col * lda;
```

`lda`（leading dimension）是相邻两列首元素之间的元素数，不一定等于逻辑行数。没有 padding、子矩阵等情况时：

```text
lda = rows
```

一个 2 行 3 列的列主序矩阵：

```text
逻辑矩阵             内存
[ a00 a01 a02 ]      a00, a10, a01, a11, a02, a12
[ a10 a11 a12 ]
```

C/C++ 的普通二维数组通常是行主序：

```CPP
index = row * cols + col;
```

因此不能把行主序矩阵直接按同样的 `m/n/k` 传给传统 cuBLAS，再期待得到相同布局的结果。

### `lda`、`ldb`、`ldc`

GEMM：

```text
C(m×n) = alpha * op(A)(m×k) * op(B)(k×n) + beta * C(m×n)
```

对列主序存储：

* `transa == CUBLAS_OP_N`：存储的 A 是 `m × k`，通常 `lda = m`
* `transa != CUBLAS_OP_N`：存储的 A 是 `k × m`，通常 `lda = k`
* `transb == CUBLAS_OP_N`：存储的 B 是 `k × n`，通常 `ldb = k`
* `transb != CUBLAS_OP_N`：存储的 B 是 `n × k`，通常 `ldb = n`
* C 总是存储为 `m × n`，通常 `ldc = m`

leading dimension 描述的是**实际存储矩阵的一列有多大跨度**，不是乘法中 `op(A)` 之后的逻辑列数。

### 行主序矩阵乘法

若已有紧凑行主序：

```text
C_row(m×n) = A_row(m×k) * B_row(k×n)
```

同一段内存按列主序看，分别是 \(A^T\)、\(B^T\)、\(C^T\)。利用：

```text
C^T = B^T * A^T
```

交换 A、B 及 `m`、`n`：

```CPP
// d_A、d_B、d_C 都是紧凑行主序
CUBLAS_CHECK(cublasSgemm(
    handle,
    CUBLAS_OP_N, CUBLAS_OP_N,
    n, m, k,
    &alpha,
    d_B, n,
    d_A, k,
    &beta,
    d_C, n));
```

这种技巧适合紧凑、普通的二维 GEMM。复杂转置、batch、padding 或需要融合 bias/activation 时，使用支持显式矩阵布局的 cuBLASLt 更不容易出错。

## 标量参数与 Pointer Mode

`alpha`、`beta` 等标量是指针参数。它们指向主机还是设备，由 handle 的 pointer mode 决定：

```CPP
cublasStatus_t cublasSetPointerMode(cublasHandle_t handle,
                                    cublasPointerMode_t mode);
cublasStatus_t cublasGetPointerMode(cublasHandle_t handle,
                                    cublasPointerMode_t* mode);
```

### Host pointer mode

默认是 `CUBLAS_POINTER_MODE_HOST`：

```CPP
float alpha = 1.0f;
float beta  = 0.0f;
cublasSetPointerMode(handle, CUBLAS_POINTER_MODE_HOST);
cublasSgemm(handle, /* ... */, &alpha, /* ... */, &beta, /* ... */);
```

标量放在普通主机内存里。调用返回后可以结束其生命周期。

### Device pointer mode

`CUBLAS_POINTER_MODE_DEVICE` 下标量必须在设备可访问内存中：

```CPP
cublasSetPointerMode(handle, CUBLAS_POINTER_MODE_DEVICE);
cublasSgemm(handle, /* ... */, d_alpha, /* ... */, d_beta, /* ... */);
```

适合让前一个 kernel 直接计算标量，并避免主机参与。设备标量在 cuBLAS 工作真正执行前必须保持有效。

Pointer mode 是 handle 的可变状态。修改后会影响后续所有使用该 handle 的调用。

## Stream 与异步执行

```CPP
cublasStatus_t cublasSetStream(cublasHandle_t handle, cudaStream_t stream);
cublasStatus_t cublasGetStream(cublasHandle_t handle, cudaStream_t* stream);
```

未设置时使用默认 stream。设置后，cuBLAS 操作会进入该 stream，并与同一 stream 中的拷贝、kernel 按提交顺序执行：

```CPP
cudaStream_t stream;
CUDA_CHECK(cudaStreamCreate(&stream));
CUBLAS_CHECK(cublasSetStream(handle, stream));

CUDA_CHECK(cudaMemcpyAsync(d_A, h_A, bytes_A,
                           cudaMemcpyHostToDevice, stream));
CUBLAS_CHECK(cublasSgemm(handle, /* ... */));
CUDA_CHECK(cudaMemcpyAsync(h_C, d_C, bytes_C,
                           cudaMemcpyDeviceToHost, stream));

CUDA_CHECK(cudaStreamSynchronize(stream));
```

使用 `cudaMemcpyAsync` 真正重叠主机和设备传输时，主机缓冲通常要由 `cudaMallocHost`、`cudaHostAlloc` 或 `cudaHostRegister` 提供。

多个 stream 并发时，常见做法是每条 stream 使用自己的 handle，减少反复修改 handle 状态，也避免多个主机线程共享 handle。不同 stream 之间的数据依赖用 event 显式表达。

## Level 1：向量运算

cuBLAS 向量接口用三个参数描述一个向量：

* `n`：元素个数
* `x`：第一个元素的地址
* `incx`：相邻逻辑元素的步长

`incx = 1` 表示连续元素；`incx = 2` 表示每隔一个元素取一个。`incy` 同理。

### AXPY

```CPP
cublasStatus_t cublasSaxpy(cublasHandle_t handle,
                           int n,
                           const float* alpha,
                           const float* x, int incx,
                           float* y, int incy);
```

计算：

```text
y = alpha * x + y
```

调用：

```CPP
float alpha = 2.0f;
CUBLAS_CHECK(cublasSaxpy(handle, n, &alpha, d_x, 1, d_y, 1));
```

### COPY、SCAL 与 SWAP

```CPP
cublasScopy(handle, n, x, incx, y, incy);         // y = x
cublasSscal(handle, n, &alpha, x, incx);          // x = alpha * x
cublasSswap(handle, n, x, incx, y, incy);         // 交换 x、y
```

这些是接口形状示意，实际调用仍要检查返回值。

### DOT 与 NRM2

```CPP
cublasStatus_t cublasSdot(cublasHandle_t handle,
                          int n,
                          const float* x, int incx,
                          const float* y, int incy,
                          float* result);

cublasStatus_t cublasSnrm2(cublasHandle_t handle,
                           int n,
                           const float* x, int incx,
                           float* result);
```

```CPP
float dot = 0.0f;
CUBLAS_CHECK(cublasSdot(handle, n, d_x, 1, d_y, 1, &dot));
```

在 host pointer mode 下，返回主机结果的归约操作可能使主机等待结果完成。若要保持整个流水线在 GPU 上，切换到 device pointer mode，把 `result` 放在设备内存。

### 最大值位置

```CPP
cublasStatus_t cublasIsamax(cublasHandle_t handle,
                            int n,
                            const float* x, int incx,
                            int* result);
```

返回绝对值最大元素的位置。BLAS 的索引是 **1-based**，不是 C/C++ 常用的 0-based：

```CPP
int one_based = 0;
CUBLAS_CHECK(cublasIsamax(handle, n, d_x, 1, &one_based));
int zero_based = one_based - 1;
```

## Level 2：矩阵向量乘法

```CPP
cublasStatus_t cublasSgemv(cublasHandle_t handle,
                           cublasOperation_t trans,
                           int m, int n,
                           const float* alpha,
                           const float* A, int lda,
                           const float* x, int incx,
                           const float* beta,
                           float* y, int incy);
```

当 `trans == CUBLAS_OP_N`：

```text
y(m) = alpha * A(m×n) * x(n) + beta * y(m)
```

列主序紧凑矩阵的 `lda = m`：

```CPP
CUBLAS_CHECK(cublasSgemv(
    handle, CUBLAS_OP_N,
    m, n,
    &alpha,
    d_A, m,
    d_x, 1,
    &beta,
    d_y, 1));
```

`CUBLAS_OP_T` 表示转置，`CUBLAS_OP_C` 表示共轭转置。对实数，两者结果相同。

## Level 3：矩阵乘法

### `cublasSgemm`

```CPP
cublasStatus_t cublasSgemm(cublasHandle_t handle,
                           cublasOperation_t transa,
                           cublasOperation_t transb,
                           int m, int n, int k,
                           const float* alpha,
                           const float* A, int lda,
                           const float* B, int ldb,
                           const float* beta,
                           float* C, int ldc);
```

计算：

```text
C(m×n) = alpha * op(A)(m×k) * op(B)(k×n) + beta * C(m×n)
```

紧凑列主序、都不转置：

```CPP
float alpha = 1.0f;
float beta  = 0.0f;

CUBLAS_CHECK(cublasSgemm(
    handle,
    CUBLAS_OP_N, CUBLAS_OP_N,
    m, n, k,
    &alpha,
    d_A, m,
    d_B, k,
    &beta,
    d_C, m));
```

参数顺序最好始终按以下问题检查：

1. 结果 C 是几行几列？得到 `m`、`n`。
2. 乘法内维是多少？得到 `k`。
3. A、B 的内存实际存了转置前还是转置后的形状？
4. 按实际存储矩阵的行数填写 `lda`、`ldb`。
5. C 的列跨度填写 `ldc`。

### `beta` 与 C 的初值

若 `beta != 0`，cuBLAS 会读取 C 的旧值：

```text
C = alpha * A * B + beta * C_old
```

此时 C 必须已经初始化。若 `beta == 0`，不要依赖“库一定不会读取 C”来掩盖无效指针或错误尺寸；仍应传入足够大的合法设备缓冲。

### `cublasGemmEx`

`cublasGemmEx` 把输入类型、输出类型和累加精度分开：

```CPP
cublasStatus_t cublasGemmEx(
    cublasHandle_t handle,
    cublasOperation_t transa,
    cublasOperation_t transb,
    int m, int n, int k,
    const void* alpha,
    const void* A, cudaDataType_t Atype, int lda,
    const void* B, cudaDataType_t Btype, int ldb,
    const void* beta,
    void* C, cudaDataType_t Ctype, int ldc,
    cublasComputeType_t computeType,
    cublasGemmAlgo_t algo);
```

例如 FP16 输入和输出、FP32 累加：

```CPP
float alpha = 1.0f;
float beta  = 0.0f;

CUBLAS_CHECK(cublasGemmEx(
    handle,
    CUBLAS_OP_N, CUBLAS_OP_N,
    m, n, k,
    &alpha,
    d_A, CUDA_R_16F, m,
    d_B, CUDA_R_16F, k,
    &beta,
    d_C, CUDA_R_16F, m,
    CUBLAS_COMPUTE_32F,
    CUBLAS_GEMM_DEFAULT_TENSOR_OP));
```

注意 `alpha`、`beta` 的类型通常跟计算类型/缩放类型有关，不应仅根据 C 的存储类型猜测。具体合法组合随 CUDA 版本变化，以当前版本的 `cublasGemmEx` 文档表格为准。

## Batch GEMM

大量小矩阵逐个调用 GEMM，launch 开销较高。Batch API 用一次调用处理多组矩阵。

### Pointer-array batched

```CPP
cublasStatus_t cublasSgemmBatched(
    cublasHandle_t handle,
    cublasOperation_t transa,
    cublasOperation_t transb,
    int m, int n, int k,
    const float* alpha,
    const float* const Aarray[], int lda,
    const float* const Barray[], int ldb,
    const float* beta,
    float* const Carray[], int ldc,
    int batchCount);
```

`Aarray`、`Barray`、`Carray` 是位于**设备内存**中的设备指针数组。各矩阵地址可以不连续，适合不规则分配，但需要额外维护指针数组。

### Strided batched

矩阵等间隔连续存放时，优先使用 strided batched：

```CPP
cublasStatus_t cublasSgemmStridedBatched(
    cublasHandle_t handle,
    cublasOperation_t transa,
    cublasOperation_t transb,
    int m, int n, int k,
    const float* alpha,
    const float* A, int lda, long long int strideA,
    const float* B, int ldb, long long int strideB,
    const float* beta,
    float* C, int ldc, long long int strideC,
    int batchCount);
```

stride 的单位是**元素**，不是字节。紧凑列主序且都不转置时：

```CPP
long long strideA = static_cast<long long>(m) * k;
long long strideB = static_cast<long long>(k) * n;
long long strideC = static_cast<long long>(m) * n;

CUBLAS_CHECK(cublasSgemmStridedBatched(
    handle,
    CUBLAS_OP_N, CUBLAS_OP_N,
    m, n, k,
    &alpha,
    d_A, m, strideA,
    d_B, k, strideB,
    &beta,
    d_C, m, strideC,
    batchCount));
```

某个 stride 为 `0` 可表达 batch 间复用同一矩阵，但是否支持、是否高效应查对应 CUDA 版本文档。

## 数据类型、Tensor Core 与数值精度

### 存储类型与计算类型

矩阵的存储类型不等于乘加的计算类型。例如：

* FP16 输入 + FP32 累加，通常比全 FP16 累加更稳定。
* BF16 指数范围接近 FP32，但尾数精度更低。
* TF32 使用 FP32 的指数范围和较短尾数，在 Ampere 及以后 Tensor Core 上加速 FP32 GEMM。
* FP64 精度高，但消费级 GPU 的 FP64 吞吐通常远低于 FP32。
* INT8 GEMM 通常使用 INT32 累加，并有对齐与维度约束。

`cublasGemmEx`、`cublasLtMatmul` 可以显式选择存储与计算类型。不要仅为了速度切换低精度；先确定误差容限，并对真实数据做数值验证。

### Math mode

```CPP
cublasStatus_t cublasSetMathMode(cublasHandle_t handle,
                                 cublasMath_t mode);
cublasStatus_t cublasGetMathMode(cublasHandle_t handle,
                                 cublasMath_t* mode);
```

常见模式包括默认数学模式、TF32 Tensor Core 模式和 pedantic 模式。不同 CUDA 版本的默认行为及枚举支持可能变化。新代码更适合通过 `cublasGemmEx` 的 `computeType` 或 cuBLASLt 的计算描述符明确表达精度要求。

浮点矩阵乘法不满足严格结合律。cuBLAS 可能按不同 tiling、归约顺序或 Tensor Core 路径计算，因此结果通常不能与 CPU 或另一算法逐 bit 比较，应使用绝对误差与相对误差：

```CPP
bool close(float got, float expected,
           float atol = 1e-5f, float rtol = 1e-4f) {
    return std::abs(got - expected)
        <= atol + rtol * std::abs(expected);
}
```

### 可复现性

同一 toolkit、GPU 架构和配置下，cuBLAS 通常提供文档规定范围内的确定性行为；但并发 stream、不同算法、workspace 配置、原子路径或 toolkit 版本变化可能改变舍入顺序。要求可复现时：

* 固定 CUDA/cuBLAS 版本、GPU 架构、math mode 和算法。
* 不要让存在写冲突的操作跨 stream 并发。
* 查阅当前版本文档的“Results Reproducibility”与环境变量要求。
* 不要把“数值接近”误当作“逐 bit 相同”。

## cuBLASLt

cuBLASLt 是面向矩阵乘法的轻量级、描述符式接口。相对传统 `cublasGemmEx`，它适合：

* 显式声明行主序或列主序布局。
* 从启发式结果中选择算法和 workspace。
* 融合 bias、ReLU、GELU 等 epilogue，减少额外 kernel 和显存往返。
* 处理更多低精度、量化和对齐组合。

典型调用过程：

```text
创建 cublasLt handle
    ↓
创建 matmul descriptor（计算类型、transpose、epilogue）
    ↓
创建 A/B/C/D layout（类型、行列、leading dimension、order）
    ↓
设置 preference 和最大 workspace
    ↓
查询 heuristic algorithm
    ↓
cublasLtMatmul
    ↓
销毁 descriptors 和 handle
```

cuBLASLt 配置项较多。普通 GEMM 先用传统 API；需要行主序、融合操作或算法调优时再使用 cuBLASLt。

## 性能要点

### 避免不必要的数据传输

一次 GEMM 很快，但每次计算前后都做 Host↔Device 拷贝可能抵消收益。尽量让数据长期驻留 GPU，把连续算子放入同一 stream，只在输入和最终输出处传输。

### 使用足够大的问题或 batch

单个很小的矩阵难以占满 GPU。可使用 batched/strided batched API，或通过 cuBLASLt 的融合功能减少 launch 数量。是否更快必须实测。

### 对齐与维度

Tensor Core 和向量化访存对地址、leading dimension、`m/n/k` 的对齐可能有偏好或限制。具体约束取决于数据类型、GPU 架构和算法。不要为了凑整越界访问；可以显式 padding，并让 `lda/ldb/ldc` 描述实际跨度。

### 不要用 CPU 墙钟直接测异步调用

错误测法：

```CPP
auto begin = std::chrono::high_resolution_clock::now();
cublasSgemm(handle, /* ... */);
auto end = std::chrono::high_resolution_clock::now();
```

这主要测到主机提交时间。使用 CUDA Event：

```CPP
cudaEvent_t begin, end;
CUDA_CHECK(cudaEventCreate(&begin));
CUDA_CHECK(cudaEventCreate(&end));

// 先 warm up，避免把首次上下文/算法初始化算进去。
CUBLAS_CHECK(cublasSgemm(handle, /* ... */));

CUDA_CHECK(cudaEventRecord(begin, stream));
for (int i = 0; i < repeats; ++i)
    CUBLAS_CHECK(cublasSgemm(handle, /* ... */));
CUDA_CHECK(cudaEventRecord(end, stream));
CUDA_CHECK(cudaEventSynchronize(end));

float total_ms = 0.0f;
CUDA_CHECK(cudaEventElapsedTime(&total_ms, begin, end));
float average_ms = total_ms / repeats;
```

GEMM 的近似浮点运算次数是：

```text
FLOPs = 2 * m * n * k
TFLOP/s = FLOPs / seconds / 1e12
```

### Workspace 与算法

部分算法需要临时 workspace。传统 cuBLAS 多数情况下自动管理；cuBLASLt 通常由调用者给出 workspace 上限并查询启发式算法。workspace 越大不保证越快，应针对固定形状、数据类型和目标 GPU benchmark。

### 先用 profiler 定位

优化前用 Nsight Systems 看数据传输、同步和 stream 空洞，再用 Nsight Compute 看实际 kernel 的 Tensor Core、访存和占用情况。不要仅根据 API 名称推断硬件路径。

## 完整示例：列主序 SGEMM

下面计算：

```text
A = [1 3]    B = [5 7]    C = A * B = [23 31]
    [2 4]        [6 8]                [34 46]
```

数组按列主序填写：

```CPP
#include <cublas_v2.h>
#include <cuda_runtime.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define CUDA_CHECK(call)                                                   \
    do {                                                                   \
        cudaError_t err__ = (call);                                        \
        if (err__ != cudaSuccess) {                                        \
            std::fprintf(stderr, "CUDA error %s:%d: %s\n",                 \
                         __FILE__, __LINE__, cudaGetErrorString(err__));    \
            std::exit(EXIT_FAILURE);                                       \
        }                                                                  \
    } while (0)

#define CUBLAS_CHECK(call)                                                 \
    do {                                                                   \
        cublasStatus_t status__ = (call);                                  \
        if (status__ != CUBLAS_STATUS_SUCCESS) {                           \
            std::fprintf(stderr, "cuBLAS error %s:%d: %d\n",               \
                         __FILE__, __LINE__, static_cast<int>(status__));   \
            std::exit(EXIT_FAILURE);                                       \
        }                                                                  \
    } while (0)

int main() {
    constexpr int m = 2;
    constexpr int n = 2;
    constexpr int k = 2;

    // 列主序：A=[[1,3],[2,4]]，B=[[5,7],[6,8]]
    const std::vector<float> h_A{1.0f, 2.0f, 3.0f, 4.0f};
    const std::vector<float> h_B{5.0f, 6.0f, 7.0f, 8.0f};
    std::vector<float> h_C(m * n, 0.0f);

    float* d_A = nullptr;
    float* d_B = nullptr;
    float* d_C = nullptr;
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_A),
                          h_A.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_B),
                          h_B.size() * sizeof(float)));
    CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_C),
                          h_C.size() * sizeof(float)));

    CUDA_CHECK(cudaMemcpy(d_A, h_A.data(), h_A.size() * sizeof(float),
                          cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_B, h_B.data(), h_B.size() * sizeof(float),
                          cudaMemcpyHostToDevice));

    cublasHandle_t handle = nullptr;
    CUBLAS_CHECK(cublasCreate(&handle));

    const float alpha = 1.0f;
    const float beta = 0.0f;
    CUBLAS_CHECK(cublasSgemm(
        handle,
        CUBLAS_OP_N, CUBLAS_OP_N,
        m, n, k,
        &alpha,
        d_A, m,
        d_B, k,
        &beta,
        d_C, m));

    // 同步 D2H 拷贝会等待默认 stream 中前面的 SGEMM。
    CUDA_CHECK(cudaMemcpy(h_C.data(), d_C, h_C.size() * sizeof(float),
                          cudaMemcpyDeviceToHost));

    // 列主序打印。
    for (int row = 0; row < m; ++row) {
        for (int col = 0; col < n; ++col)
            std::printf("%6.1f ", h_C[row + col * m]);
        std::printf("\n");
    }

    const float expected[]{23.0f, 34.0f, 31.0f, 46.0f};
    for (int i = 0; i < m * n; ++i) {
        if (std::fabs(h_C[i] - expected[i]) > 1e-5f) {
            std::fprintf(stderr, "wrong result at %d\n", i);
            CUBLAS_CHECK(cublasDestroy(handle));
            CUDA_CHECK(cudaFree(d_C));
            CUDA_CHECK(cudaFree(d_B));
            CUDA_CHECK(cudaFree(d_A));
            return EXIT_FAILURE;
        }
    }

    CUBLAS_CHECK(cublasDestroy(handle));
    CUDA_CHECK(cudaFree(d_C));
    CUDA_CHECK(cudaFree(d_B));
    CUDA_CHECK(cudaFree(d_A));
    return EXIT_SUCCESS;
}
```

输出：

```text
  23.0   31.0
  34.0   46.0
```

## 常见错误

### 把行主序当列主序

表现为结果像转置、A/B 顺序颠倒或维度不匹配。先在纸上写出 `C = op(A)op(B)` 的形状，再检查实际内存布局和 leading dimension。

### 把 `lda` 当作矩阵列数

列主序里 `lda` 是列之间的元素跨度，紧凑矩阵通常等于实际存储的**行数**。有转置时，应看传入内存中矩阵的形状，而不是只看 `op(A)` 后的形状。

### 忘记初始化 C

`beta != 0` 时会读取 C 的旧值。若 C 未初始化，结果包含未定义数据。

### 标量地址与 pointer mode 不匹配

默认 mode 下 `alpha`/`beta` 应是主机指针；device mode 下应是设备指针。把 `&alpha` 传给 device mode 会导致非法访问。

### 只检查 cuBLAS 返回值

API 返回成功可能只代表提交成功。设备端非法访问要在 stream/device 同步或后续同步拷贝时检查 CUDA 错误。

### 每次运算都创建 handle

频繁 `cublasCreate` / `cublasDestroy` 增加初始化和资源管理开销。handle 应在一段计算期间复用。

### 用错误单位填写 stride

`cublas*gemmStridedBatched` 的 stride 是元素数；`cudaMemcpy` 和 `cudaMalloc` 的大小是字节数。

### 小矩阵逐个调用 GEMM

大量小调用往往受 launch 开销限制。使用 batched API、cuBLASLt 融合或更高层库，并用实际输入尺寸 benchmark。

## 常见调用序

```CPP
cudaSetDevice(device);
cudaMalloc(&d_A, bytes_A);
cudaMalloc(&d_B, bytes_B);
cudaMalloc(&d_C, bytes_C);
cudaMemcpy(d_A, h_A, bytes_A, cudaMemcpyHostToDevice);
cudaMemcpy(d_B, h_B, bytes_B, cudaMemcpyHostToDevice);

cublasCreate(&handle);
cublasSetStream(handle, stream);       // 使用默认 stream 时可省略
cublasSetPointerMode(
    handle, CUBLAS_POINTER_MODE_HOST); // 默认值，可省略

cublasSgemm(handle, /* m, n, k, A, B, C ... */);

cudaMemcpy(h_C, d_C, bytes_C, cudaMemcpyDeviceToHost);

cublasDestroy(handle);
cudaFree(d_C);
cudaFree(d_B);
cudaFree(d_A);
```

真实程序应检查每个 CUDA 与 cuBLAS 返回值。异步流水线使用显式 stream、pinned 主机内存、`cudaMemcpyAsync` 和 event；矩阵布局、标量生命周期以及释放顺序必须与该 stream 上的工作保持一致。
