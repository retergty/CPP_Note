# cuda 笔记

## 线程模型

CUDA 把一次 kernel 启动看成一次大规模并行调用：主机用 `<<<grid, block>>>` 声明要跑多少线程，设备上这些线程按 **grid → block → warp → thread** 分层执行。程序员直接操作的是 grid / block / thread；warp 是硬件调度单位，写对性能几乎总绕不开它。

### 层次

```text
Grid                         一次 kernel launch 的全部线程
 └── Block                   可同步、可共享 shared memory 的一组线程
      └── Warp               硬件同时发射指令的 32 个线程（SIMT）
           └── Thread        最小执行单元，有独立寄存器与 program counter
```

| 层级 | 谁定义 | 谁调度 | 能做什么 |
|------|--------|--------|----------|
| Grid | 启动配置 `gridDim` | 整卡，block 分到各 SM | 覆盖全部数据；block 之间默认不同步 |
| Block | 启动配置 `blockDim` | 整个 block 驻留在同一个 SM | `__syncthreads()`、shared memory |
| Warp | 硬件固定，通常 32 | SM 上的 warp scheduler | 一次发射同一条指令；分歧会串行化 |
| Thread | `threadIdx` | 跟着 warp 走 | 自己的寄存器、local memory、独立控制流 |

CPU 线程是操作系统调度的重量级上下文。CUDA thread 极轻：几乎就是一组寄存器加一条逻辑 PC，切换 warp 的开销接近零。并行度主要来自「同时有很多 warp 在等内存或在算」，而不是来自抢占式调度。

### 启动配置

```cuda
__global__ void kernel(/* args */);

dim3 grid(Gx, Gy, Gz);    // 有多少个 block
dim3 block(Bx, By, Bz);   // 每个 block 有多少 thread
kernel<<<grid, block>>>(/* args */);
kernel<<<grid, block, dyn_shared_bytes, stream>>>(/* args */);
```

`dim3` 默认分量是 `1`。写成 `kernel<<<N, 256>>>` 等于 `grid.x = N`，`block.x = 256`，其余为 `1`。

总线程数：

```text
total_threads = gridDim.x * gridDim.y * gridDim.z
              * blockDim.x * blockDim.y * blockDim.z
```

大致约束

* 每个 block 最多 `1024` 个 thread（`Bx * By * Bz ≤ 1024`）
* `blockDim.x` 最大 `1024`，`y/z` 最大 `1024` / `64`
* `gridDim.x` 最大 `2^31 - 1`，`y/z` 最大 `65535`
* warp 大小 `32`，所以 block 大小最好是 `32` 的倍数
* 一个 SM 同时驻留的 block / warp / thread 数受寄存器、shared memory、硬件上限共同卡住

### 内置索引

kernel 里只读、由硬件填好：

| 变量 | 类型 | 含义 |
|------|------|------|
| `threadIdx` | `uint3` | 当前线程在 **本 block 内** 的坐标 |
| `blockIdx` | `uint3` | 当前 block 在 **grid 内** 的坐标 |
| `blockDim` | `dim3` | 一个 block 的尺寸 |
| `gridDim` | `dim3` | grid 里有多少 block |
| `warpSize` | `int` | 通常为 `32` |

线性全局下标（1D）：

```cuda
int i = blockIdx.x * blockDim.x + threadIdx.x;
```

2D 图像 / 矩阵：

```cuda
int col = blockIdx.x * blockDim.x + threadIdx.x;
int row = blockIdx.y * blockDim.y + threadIdx.y;
```

3D：

```cuda
int x = blockIdx.x * blockDim.x + threadIdx.x;
int y = blockIdx.y * blockDim.y + threadIdx.y;
int z = blockIdx.z * blockDim.z + threadIdx.z;
```

block 内部把三维坐标压成线性 `tid`（shared memory 下标常用）：

```cuda
int tid = threadIdx.z * blockDim.x * blockDim.y
        + threadIdx.y * blockDim.x
        + threadIdx.x;
```

`threadIdx` 的排列是 `x` 最先变，然后 `y`，然后 `z`。warp 按这个线性顺序切：tid `0..31` 是 warp 0，`32..63` 是 warp 1。要 coalesced 访问，让相邻 `threadIdx.x` 读相邻地址。

同样先把三维 `blockIdx` 压成线性 `bid`，再与 `tid` 组合，即可给本次启动的每个线程一个唯一的线性 `global_tid`：

```cuda
size_t bid = static_cast<size_t>(blockIdx.z) * gridDim.x * gridDim.y
           + static_cast<size_t>(blockIdx.y) * gridDim.x
           + blockIdx.x;

size_t threads_per_block =
    static_cast<size_t>(blockDim.x) * blockDim.y * blockDim.z;

size_t global_tid = bid * threads_per_block + tid;
```

若访问逻辑尺寸为 `width × height × depth` 的数组，应使用真实数据尺寸计算下标并检查边界：

```cuda
if (x < width && y < height && z < depth) {
    size_t index = (static_cast<size_t>(z) * height + y) * width + x;
    out[index] = /* ... */;
}
```

### grid-stride

元素数 `n` 往往不是 `grid * block` 的整数倍，也可能远大于一次能启动的线程数：

```cuda
__global__ void saxpy(int n, float a, const float* x, float* y) {
    for (int i = blockIdx.x * blockDim.x + threadIdx.x;
         i < n;
         i += gridDim.x * blockDim.x) {
        y[i] = a * x[i] + y[i];
    }
}
```

步长`gridDim.x * blockDim.x`是整次启动的线程总数。每个线程处理一组等间隔元素，越界用 `i < n` 挡住。这是 CUDA 里最常见的 1D 映射。

3D 同类写法：

```cuda
int stride_x = gridDim.x * blockDim.x;
int stride_y = gridDim.y * blockDim.y;
int stride_z = gridDim.z * blockDim.z;

for (int z = blockIdx.z * blockDim.z + threadIdx.z; z < D; z += stride_z)
    for (int y = blockIdx.y * blockDim.y + threadIdx.y; y < H; y += stride_y)
        for (int x = blockIdx.x * blockDim.x + threadIdx.x; x < W; x += stride_x)
            out[(z * H + y) * W + x] = ...;
```

### Block 与 SM

GPU 由多个 **SM（Streaming Multiprocessor）** 组成。启动后，runtime 把 block **整块** 分给某个 SM：一个 block 不会拆到两个 SM，一个 SM 通常同时驻留多个 block。

block 一旦驻留，占用的寄存器文件和 shared memory 直到该 block 结束才释放。SM 资源不够时，多出来的 block 排队，前一批跑完再上。

因此：

* **block 内** 可以 `__syncthreads()`，可以靠 shared memory 交换数据
* **block 间** 没有默认屏障。A block 写 global memory，B block 能否看见，取决于你是否用了另一轮 kernel、cooperative launch、原子，或有文档保证的 memory model
* 不要假设 `blockIdx.x == 0` 先于 `blockIdx.x == 1` 执行

独立、可任意重排的工作才适合切成多个 block。需要全卡同步时，结束当前 kernel 再 launch 下一个，或使用 cooperative groups 的 grid sync（有硬件/驱动条件）。

### Warp 与 SIMT

硬件一次真正「发射」的不是一个 thread，而是一个 **warp**：连续 32 个线程执行**同一条指令**，各自用自己的寄存器当操作数。这叫 **SIMT（Single Instruction, Multiple Threads）**。

和 SIMD 的差别：SIMD 是一条指令打在向量寄存器的多个 lane 上，lane 通常不能各走各的分支。SIMT 允许每个线程有自己的 PC 和分支，但同一 warp 里若走到不同路径，硬件会 **串行执行各条路径，把不参加的线程 mask 掉**，这就是 **warp divergence**。

```cuda
if (threadIdx.x % 2 == 0)
    even_path();
else
    odd_path();
```

这个 warp 里 16 个线程走 `even`，16 个走 `odd`：两条路径依次执行，吞吐近似腰斩。若整个 warp 条件相同，没有分歧，开销可忽略。

判断依据是 **同一 warp 内** 的控制流，不是整个 block：

* `if (threadIdx.x < 32)` 对只有 1 个 warp 的 block 会造成分歧；对 `blockDim.x == 256`，这只是「warp 0 全进、其余 warp 全不进」，warp 内部一致
* `if (data[i] > 0)` 这种数据相关分支，相邻线程条件不同就会分歧

warp 内线程号：

```cuda
int tid    = threadIdx.x;          // 一维 block 时
int warp_id = tid / warpSize;      // 0, 1, 2, ...
int lane_id = tid % warpSize;      // 0 .. 31
```

### SM 如何调度 warp

驻留（residency）指该 warp 的执行上下文已分配在某个 SM 上，因而可以被该 SM 的 warp scheduler 选中并发射指令。驻留 warp 数不等于同一时钟周期内正在发射指令的 warp 数。

warp 驻留须同时满足：所属 block 已分配到该 SM；该 warp 的 32 个线程已在该 SM 的寄存器文件中获得寄存器；若使用 shared memory，对应容量已从该 SM 的 shared memory 中分配。这些资源位于片上，已驻留 warp 之间的调度切换不需要把寄存器上下文换出到 DRAM。尚未获得 SM 分配的 block 不能被该 SM 的 scheduler 选中。

同一时钟周期内只有少数 warp 发射指令。其余已驻留 warp 因等待寄存器操作数、访存完成或 `__syncthreads()` 等，不处于 eligible（可发射）状态。

SM 划分为若干处理子分区，每个子分区有独立的 warp scheduler。以 Turing（RTX 2070）为例，每个 SM 通常有 4 个 scheduler，每 SM 最多约 32 个驻留 warp（1024 个线程）。确切上限以 `cudaDeviceProp` 为准。

每个时钟周期，每个 scheduler：

1. 在其负责的已驻留 warp 中确定 eligible 集合：下一条指令的寄存器操作数已就绪；未因访存或 barrier 停顿；发射端口与目标流水线可用。
2. 从该集合中选择一个 warp，向其发射一条指令（作用于该 warp 当前活跃的 lane）。

因此 4 个 scheduler 意味着一个 SM 每个时钟周期最多向 4 个 warp 各发射一条指令；其余驻留 warp 在该周期不发射。

warp 发射该指令后暂时离开 eligible 集合，scheduler 改为向其他已驻留且 eligible 的 warp 发射；数据返回后，原 warp 重新进入 eligible 集合。这是延迟隐藏（latency hiding）：用足够多的驻留 warp，使部分 warp 等待访存时仍可能存在可发射的 warp。若驻留过少，可能出现所有 warp 均非 eligible、该周期无指令可发射。

| 状态 | 是否驻留 | 是否 eligible |
|------|----------|----------------|
| 操作数就绪，可发射下一条指令 | 是 | 是 |
| 等待 global / local 访存完成 | 是 | 否，完成后恢复 |
| 等待 `__syncthreads()` | 是 | 否，同 block 全部到达后恢复 |
| block 尚未分配到该 SM | 否 | 否 |

已到达 `__syncthreads()` 的 warp 保持驻留，并继续占用已分配的寄存器与 shared memory，但在同 block 其余 warp 到达之前不是 eligible。若该 SM 当时只驻留一个 block，屏障等待期间可能没有任何 eligible warp。

warp scheduler 只能向已驻留的 warp 发射指令。可驻留数量由 occupancy 约束决定；每周期可发射的指令条数由 scheduler 数量和流水线结构决定。

### Occupancy

occupancy = 一个 SM **当前驻留的 warp 数** / 该 SM **硬件允许的最大 warp 数**。它给出 scheduler 可选择的驻留 warp 上限，不表示同一时钟周期实际发射了多少条指令。

限制来自三方面（取最紧的那个）：

1. 每线程寄存器数 × 每 block 线程数 × 驻留 block 数 ≤ SM 寄存器文件
2. 每 block shared memory × 驻留 block 数 ≤ SM shared memory
3. 硬件上限：每 SM 的 max warps、max blocks、max threads

occupancy 高不等于一定快，但过低时访存等待期间可能没有其他 eligible warp，延迟无法被覆盖。经验：

* block 大小常用 `128` / `256` / `512`，且为 `32` 倍数
* 先保证算法对（索引、sync、边界），再用 Nsight Compute 看 occupancy 是受寄存器还是 shared memory 限制
* `blockDim` 取到 `1024` 可能导致每个 SM 只能驻留 1 个 block，`__syncthreads()` 期间该 SM 可能没有其他 eligible warp

### 同步与协作

#### 单线程

只碰自己的寄存器和 local memory 时，不需要任何同步。同一线程内对同一地址的读写按程序顺序对那条依赖链成立。

#### Warp 级原语

同一 warp 内可以用硬件把寄存器送到别的 lane，或把 32 个谓词收成一份掩码，不必经过 shared memory，也不需要 `__syncthreads()`。这些是整 warp 一条 SIMT 指令，不会把 32 个 lane 改成串行执行。

第一个参数 `mask`（常见 `0xffffffff`）标明哪些 lane 参加这次握手。32 个 bit 全 1 表示全员参加。部分线程已经 `return`、或分歧导致没执行到这条指令时，`mask` 必须和实际活跃 lane 一致，否则未定义。

##### `__syncwarp`

warp 内屏障：`mask` 里的 lane 都执行到这里，此前这些 lane 对 shared/global 的写对彼此可见，PC 重新对齐。分歧之后、或接下来要用 shuffle 而当前活跃集不确定时，用它显式汇合。

```cuda
void __syncwarp(unsigned mask = 0xffffffff);
__syncwarp();
```

##### `__shfl_down_sync`

把每个 lane 的寄存器值沿 lane 号减小的方向平移，lane `k` 收到 lane `k + delta` 的值。

```cuda
T __shfl_down_sync(unsigned mask, T var, unsigned int delta, int width = warpSize);
int v = __shfl_down_sync(0xffffffff, v, 1);  // delta=1：lane k 收 lane k+1
```

`delta = 1` 时：

```text
lane:  0    1    2   ...  30   31
执行前: v0   v1   v2  ...  v30  v31
执行后: v1   v2   v3  ...  v31  v31
```

没有更右侧源的 lane（这里是 31）保持原值。典型用途是 warp 内归约：反复 `delta = 1, 2, 4, 8, 16`，最后 lane 0 拿到 32 个数的和。

同族还有：

| 原语 | 功能 |
|------|------|
| `__shfl_sync(mask, var, srcLane)` | 读指定 `srcLane` 的 `var`，可做广播 |
| `__shfl_up_sync(mask, var, delta)` | lane `k` 收 lane `k - delta` |
| `__shfl_xor_sync(mask, var, laneMask)` | 与 lane `k xor laneMask` 交换，归约常用 |

##### `__ballot_sync`

每个参加的 lane 出一个布尔谓词，硬件收成一个 32-bit 掩码：bit `i` 为 1 表示 lane `i` 的谓词为真。

```cuda
unsigned __ballot_sync(unsigned mask, int pred);
unsigned mask = __ballot_sync(0xffffffff, pred);
```

32 个线程执行同一条指令，得到的 `mask` 相同。用来做 warp 级投票、数有多少 lane 满足条件、或构造后续 shuffle 要用的活跃掩码。

##### `__popc`

按位计数（population count）：返回整数里有多少个 bit 为 1。它不是 warp 通信原语，是每线程独立的整数指令；常跟 `__ballot_sync` 连用，把投票结果变成个数。

```cuda
int __popc(unsigned int x);
int n = __popc(mask);  // 这个 warp 里有多少 lane 的 pred 为真
```

全 warp 对同一份 `mask` 做 `__popc`，每个 lane 得到的 `n` 相同。

#### `__syncthreads()`

block 内屏障：该 block **所有** 线程都到达，并且此前对本 block 可见的 shared/global 写，对块内其他线程可见。这是同 block 协作（shared tiling、block 归约）的默认手段。

```cuda
__syncthreads();
```

必须所有线程都执行到（或按文档在同一条件路径上），否则死锁。分歧路径里一边 sync、一边不 sync 是经典错误。它不管其他 block，也不能拿来等 `blockIdx.x == 1`。

调度上：已经到达的 warp 会暂时不再 eligible，但仍占着寄存器和 shared。

#### `atomic*`

`atomicAdd`、`atomicCAS` 等保证对该地址的读改写不被拆开，用于计数、唯一下标、CAS 锁。

原子 **不** 等于「所有线程都执行到这里」，也 **不** 替代 `__syncthreads()`。没有到达约定的话，只是对那个字的更新不会撕开。跨 block 发信号时通常还要配合 fence。

#### 下一个 kernel

最常见的全卡同步：当前 kernel 正常结束后，它对 global 的写对同一 stream 的后续操作可见，然后再 `<<<>>>` 下一次。block 之间不需要在同一个 kernel 里互相等。

同一 stream 里 kernel 与其后的 `cudaMemcpy` 也有序。不同 stream 默认并发，要用 `cudaEvent` 或 `cudaDeviceSynchronize`。

#### Cooperative Groups grid sync

要在 **同一个 kernel** 里做全 grid 屏障，需要 cooperative launch（`cudaLaunchCooperativeKernel`），并且这次 launch 的全部 block 必须同时驻留在 GPU 上。然后：

```cuda
namespace cg = cooperative_groups;
cg::grid_group grid = cg::this_grid();
grid.sync();
```

block 数 × 每 block 资源超过整卡容量时，launch 会失败：有的 block 永远上不来，grid sync 就会死等。用不上时，拆成两个 kernel 更简单。

## 内存模型

CUDA 内存模型包含两类规则。一类描述操作数所在的存储空间、缓存层次以及访问如何合并，它们决定延迟、带宽和 occupancy。另一类描述并发线程之间写操作的可见性与排序，由屏障、fence 和原子提供；缺少相应同步的冲突访问是数据竞争，结果未定义。

### 存储空间

```text
寄存器          每线程，SM 上的寄存器文件
local memory    每线程私有，物理上在 DRAM（可经 cache）
shared memory   每 block，SM 片上 SRAM
L1 / L2         硬件管理；L1 随 SM，L2 全卡
global memory   全设备 DRAM，host 用 cudaMalloc 等分配
constant        全 grid 只读，有独立小缓存，适合广播
```

| 空间 | 谁可见 | 生命周期 | 谁分配 |
|------|--------|----------|--------|
| 寄存器 | 当前线程 | 线程执行期间 | 编译器 |
| local | 当前线程 | 线程执行期间 | 编译器（溢出、大数组） |
| shared | 同 block | block 执行期间 | kernel 内 `__shared__` 或 launch 动态字节 |
| global | 全设备（跨 kernel 可保留） | `cudaMalloc` 到 `cudaFree` | host / 驱动 |
| constant | 全设备只读 | 模块/符号生命周期 | `__constant__` 或 `cudaMemcpyToSymbol` |

同一份 DRAM 上的地址，在 kernel 里用不同空间访问，走的缓存路径不同。`__shared__` 不在 DRAM；寄存器也不在。

### 寄存器

最快，延迟以周期计。每个 SM 有固定大小的寄存器文件，按驻留线程均分。每线程用得越多，能同时住的 warp 越少，occupancy 下降。

编译器给每个线程分配寄存器。超过硬件/占用限制就 **spill** 到 local memory，逻辑上还是「每线程私有变量」，速度掉到接近访存。

### Local memory

逻辑上每线程私有：线程局部数组太大、无法判定索引的数组、寄存器溢出，都会进这里。物理上在 global memory，可被 L1/L2 缓存。不要把它理解成一块独立的片上 SRAM。

### Shared memory

程序员管理的 SM 片上 SRAM，按 block 分配。同 block 线程共享，典型用途是协作加载、tiling、block 内归约。

```cuda
__shared__ float tile[32][33];          // 静态，编译期大小
extern __shared__ float smem[];         // 动态，大小来自 launch 第 3 个参数
```

动态大小在启动时给出：`kernel<<<grid, block, n * sizeof(float)>>>`。

同一 block 里，thread A 写入 shared，thread B 要读，必须 `__syncthreads()`。没有这条，即使「感觉上 A 应该先执行」也不成立。`__syncthreads()` 必须 **所有线程都执行到**（或按文档在同一条件路径上），否则死锁。分歧路径里一边 sync、一边不 sync 是经典错误。

#### Bank conflict

硬件把 shared memory 划成若干独立的 **bank**，使一个 warp 的多个线程可以并行访问。现代 GPU 通常有 32 个 bank，相邻 32-bit 字映射到相邻 bank，超过 bank 31 后循环回 bank 0。在这个常见配置下：

```text
bank = (byte_address / 4) % 32
```

例如

```text
smem[0]   → bank 0
smem[1]   → bank 1
...
smem[31]  → bank 31
smem[32]  → bank 0
```

bank conflict 只在**同一个 warp 的同一条 shared memory 指令**中判断：

* 各 lane 访问不同 bank：一次并行完成
* 多个 lane 访问同一 bank 的不同地址：请求被拆成多次，形成 bank conflict
* 多个 lane 读取同一地址：由 broadcast 完成，不冲突
* 不同 warp 之间不合并成一次 bank conflict；它们由 warp 调度器分别发射

若 lane `i` 访问 `base[i * stride]`，且元素是 32-bit，则可用 `gcd(stride, 32)` 快速判断冲突程度（`stride != 0` 且各 lane 地址不同）：

* `stride = 1`：连续访问 32 个 bank，无冲突
* `stride = 2`：只命中 16 个 bank，2-way conflict
* `stride = 4`：只命中 8 个 bank，4-way conflict
* `stride = 32`：全部命中同一个 bank，32-way conflict
* `stride = 33`：`33 % 32 == 1`，重新均匀分布，无冲突

对 64-bit 数据、向量类型或跨多个 32-bit 字的访问，应按一条指令实际覆盖的 32-bit 字和目标架构分析，不能直接套用上面的元素步长。

矩阵转置是最典型的例子：

```cuda
__shared__ float tile[32][32];

tile[threadIdx.y][threadIdx.x] = in[...];  // 按行写，通常无冲突
__syncthreads();
float v = tile[threadIdx.x][threadIdx.y];  // 按列读，步长为 32
```

假设 `blockDim.x == 32`，warp 内相邻 lane 对应相邻的 `threadIdx.x`。`float tile[32][32]` 的每行占 32 个 32-bit 字，因此这个 warp 按列读时，各 lane 的地址相差 32 个字，全部落到同一 bank。给每行增加一个 padding 元素即可改变行步长：

```cuda
__shared__ float tile[32][33];
```

此时按列访问的步长是 33，相邻 lane 会落到不同 bank。padding 会略微增加 shared memory 用量，可能影响 occupancy，因此仍应比较修改前后的 kernel 时间。

### Global memory

设备 DRAM，容量最大、延迟最高（通常数百周期）。`cudaMalloc` / `cudaFree` 分配，指针传给 kernel。跨 kernel、跨 block 都能用同一块，但 **看见彼此的写要遵守下面的可见性规则**。

访存以对齐的 32/64/128 字节事务为单位。一个 warp 的 32 个线程若读连续、对齐的地址，硬件合成很少几次事务，这叫 **coalesced access**。

对 `float` 数组，让 `lane i` 读 `base[i]`：

```cuda
int i = blockIdx.x * blockDim.x + threadIdx.x;
float v = in[i];   // warp 读 32*4=128B，一次事务打满
```

相邻线程跨很大 stride 时，一次 warp 可能拆成多次事务，带宽差一个数量级：

```cuda
float v = in[threadIdx.y * huge_stride + threadIdx.x]; // x 连续，仍可能合并
float v = in[threadIdx.x * huge_stride + threadIdx.y]; // 相邻 lane 跨度大，易非合并
```

要对齐：基址按事务大小对齐，每个线程访问的宽度一致。`float4` 向量加载有时能提高每指令字节数，仍要满足合并与对齐。

### Constant memory

`__constant__` 设备端只读符号，总量通常 64KB。有独立 constant cache，同一 warp 读同一地址时走广播，适合系数、小表。各 lane 读不同地址会串行化，不适合按线程索引的大表。

Host 侧用 `cudaMemcpyToSymbol` 写入。kernel 里当普通数组读即可。

### 缓存

```text
SM
 ├── L1（常与 shared memory 共用同一块片上 SRAM，容量可划分）
 ├── constant cache
 └── texture / 只读数据缓存（部分架构与 L1 合一）
设备
 └── L2（所有 SM 共享）
 └── DRAM（global / local 的物理位置）
```

#### L1

每个 SM 一份。主要缓存该 SM 上的 **local memory**（寄存器溢出、大数组）。global 是否进入 L1 随架构和访问类型而变。

L1 **不是** 全卡一致的。SM A 的 L1 中有某地址的副本，不表示 SM B 读同一 global 地址能看到对应写入。跨 SM 的可见性不建立在 L1 上。

#### L2

全设备一份，所有 SM 共用，是 global 访问的主缓存。一个 SM 对 global 的写要被其他 SM 上的线程观察到，通常须到达 L2（或 DRAM）这一级。`__threadfence()` 等待的是当前线程的写对设备上其他线程变为可见，实现上经过这条路径。kernel 在某 stream 中正常结束后，该 kernel 的 global 写对同 stream 后续操作可见，也不依赖程序员操作 L1。

L2 容量远大于 L1（消费级常见数 MB），但仍远小于设备 DRAM。大工作集反复扫过时仍会 miss，合并访问和 locality 仍然决定 DRAM 带宽。

#### Constant cache

服务于 `__constant__`。容量小，按地址缓存。同一 warp 的 32 个 lane 读 **同一地址** 时走广播，一次即可；各 lane 读不同地址时访问被串行化。因此 constant 适合全体线程共用的系数，不适合按 `threadIdx` 索引的大表。

#### Texture / 只读路径

纹理单元和 `__ldg` 一类只读加载可走独立或与 L1 合一的只读缓存，对二维空间局部性友好。计算 kernel 里更常见的是编译器把只读 global 指针标成 `__restrict__` / `const` 后走只读路径。它仍是缓存，不是存储空间，也不提供跨线程同步。

#### 与 shared memory 的关系

| | Shared memory | L1 / L2 |
|--|---------------|---------|
| 谁管理 | 程序员声明、按 block 分配 | 硬件填入与淘汰 |
| 地址空间 | 独立的 `__shared__` | 不占用 CUDA 指针空间，是 DRAM 数据的副本 |
| 可见范围 | 仅当前驻留 block | L1：该 SM；L2：全设备 |
| 同步 | 块内须 `__syncthreads()` | 不能当作 barrier |

部分架构上 L1 与 shared 占用同一块 SRAM，划分比例可配置（shared 增大则 L1 减小）。

#### 一致性

普通 `load`/`store` 可以停留在某级 cache 中，编译器也可以把值保存在寄存器里。因此：

* 不能用 cache 代替 `__syncthreads()`、`__threadfence*` 或 kernel 边界
* `volatile` 只阻止编译器省略访存，不刷新其他 SM 的 L1，也不构成 fence
* 数据竞争（多线程冲突的非原子读写）不因「可能在 L2 里」而变为已定义

### 主机与设备

CPU 虚拟地址和 GPU 的 device pointer 默认不是同一空间，不能把 `malloc` 的指针直接传给 kernel。

| API | 作用 |
|-----|------|
| `cudaMalloc` | 设备 DRAM，kernel 直接用 |
| `cudaMallocHost` / pinned | 页锁定主机内存，DMA 更快，`cudaMemcpyAsync` 常用 |
| `cudaMallocManaged` | 统一寻址，Pascal 以后按页迁移或 page fault |
| `cudaMemcpy` | 默认同步拷贝；`Kind` 为 `HostToDevice` 等 |
| `cudaMemcpyAsync` | 与 stream 排队，需 pinned 才能真正重叠 |

同一 stream 里：kernel 与其后的 `cudaMemcpy` 有序。不同 stream 默认并发，要看见对方的写，用 `cudaEvent` 或 `cudaDeviceSynchronize`。

kernel **正常结束** 后，该 kernel 对 global 的写，对同一 stream 的后续操作可见。这是最常见的「全卡同步」。

### 可见性

没有同步时，编译器和硬件都可以重排访问；多线程冲突的非原子读写是数据竞争，结果未定义（和 C++ 同一类问题）。

| 写的一方 | 读的一方 | 怎样保证看见 |
|----------|----------|----------------|
| 同一线程 | 自己 | 按程序顺序，对那条依赖链成立 |
| 同一 warp | 寄存器 | shuffle / ballot，不经内存 |
| 同一 block | shared / global | `__syncthreads()`（屏障 + 块内 fence） |
| 不同 block | global | 本 kernel 结束；或原子 + fence 的特定协议；或 cooperative grid sync |
| 设备 | 主机 | `cudaMemcpy` / 同 stream 后续操作 / `__threadfence_system` + 主机侧同步 |

`__syncthreads()` 做两件事：所有线程到达，并且此前对本 block 可见的 shared/global 写，对块内其他线程可见。只 fence、不等其他人，用下面的 `__threadfence*`。

### `__threadfence*`

这些只约束 **当前线程自己的写何时对别人可见**，不召唤其他线程，也不能当 barrier 用。

```cuda
__threadfence_block();   // 本线程对 shared/global 的写，对同 block 可见
__threadfence();         // 本线程对 global 的写，对设备上其他线程可见
__threadfence_system();  // 再扩展到主机、peer 设备
```

典型错误：block 0 写 global，然后 `__threadfence()`，以为 block 1 一定已经能读。fence 不保证 block 1 已经执行到读之前，也不保证 block 1 已经启动。跨 block 通信还要原子标志或拆成两个 kernel。

### 原子

`atomicAdd`、`atomicCAS` 等保证对该地址的读改写不被拆开，可用于计数、队列头、CAS 锁。

* 原子 **不** 等于「所有线程都执行到这里」
* 默认设备作用域；还有 `atomicAdd_block`、`atomicAdd_system` 等变体（计算能力足够时）
* 需要「先写数据、再发布标志」时，数据写 + fence + 原子 store 标志，读侧原子 load 看到标志后再读数据。Pascal 以后也可以用 `cuda::atomic` 的 acquire/release，语义接近 C++ `memory_order`

```cuda
data[i] = v;
__threadfence();                 // 让 data 对设备可见
atomicExch(flag, 1);             // 发布
// 另一线程：
if (atomicAdd(flag, 0) == 1)     // 或 atomicCAS 观察
    use(data[i]);
```

无 fence、只靠普通 store 再普通 load 一个 `int flag`，编译器或缓存都可能让你看到「flag 已置位、data 仍是旧值」。

### `volatile`

`volatile` 迫使每次都真正访存，禁止编译器把值藏在寄存器里。它：

* **不是** 原子
* **不是** fence
* **不** 保证其他 SM 的缓存何时更新

设备端自旋读标志时，人们常写 `volatile int *flag` 以免循环被优化成只读一次。跨 block 仍应优先用原子或拆 kernel，不要只靠 `volatile`。

### 和 C++ `memory_order` 的关系

PTX/CUDA 有接近 C++ 的一致性模型。`cuda::std::atomic`（libcu++）上的 `memory_order_relaxed / acquire / release / acq_rel / seq_cst` 含义与主机侧同类：relaxed 只管原子性，acquire/release 用来配对发布与观察。

日常 kernel 不必从 seq_cst 写起：

* 同 block 协作：shared + `__syncthreads()`
* 全卡一轮计算：写 global，结束 kernel，再 launch
* 计数、唯一下标：`atomicAdd`
* 真要在一个 kernel 里跨 block 发信号：原子 + fence，或 cooperative groups

## 编程模型

CUDA 程序在同一份源码里包含两类执行：主机（CPU）上的进程调用 Runtime API，设备（GPU）上的线程执行 kernel。线程层次见 [线程模型](#线程模型)，存储空间与可见性见 [内存模型](#内存模型)。本章描述二者如何衔接：函数在哪编译、kernel 如何启动、Runtime 的调用序、错误在哪出现、以及 nvcc 如何针对 Compute Capability 生成代码。

### 执行路径

一次典型路径：

1. 主机分配设备存储、把数据拷到设备（见 [主机与设备](#主机与设备)）。
2. 主机用 `<<<grid, block>>>` 启动 `__global__` 函数。启动配置把线程组织成 grid/block，见 [启动配置](#启动配置)。
3. 设备上各 SM 获得 block，按 warp 发射指令。
4. 主机在需要读回结果或确认 kernel 已完成时同步，再拷回或释放存储。

`<<<>>>` 在默认 stream 上是异步的：启动调用把工作入队后即可返回，不等待 kernel 结束。随后的同步 `cudaMemcpy`（默认同步版本）或 `cudaDeviceSynchronize()` 才会等待先入队的工作完成。kernel 内的执行错误也往往在这次同步时才回到主机。

主机代码使用 CPU 虚拟地址；传给 kernel 的指针必须是设备地址（或 Unified Memory），不能是 `malloc` 的指针。

### 函数类型限定符

| 限定符 | 从哪里调用 | 在哪里执行 | 说明 |
|--------|------------|------------|------|
| `__global__` | 主机（`<<<>>>`）；计算能力足够时也可由设备 launch | 设备 | 必须 `void`，即 kernel |
| `__device__` | 设备 | 设备 | 供 kernel 或其他 `__device__` 调用 |
| `__host__` | 主机 | 主机 | 未写限定符时的默认 |
| `__host__ __device__` | 两者 | 对应侧 | nvcc 为主机和设备各生成一份 |

```cuda
__device__ float fma_dev(float a, float b, float c) { return a * b + c; }

__global__ void saxpy(int n, float a, const float* x, float* y) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) y[i] = fma_dev(a, x[i], y[i]);
}

__host__ void launch_saxpy(int n, float a, const float* x, float* y) {
    saxpy<<<(n + 255) / 256, 256>>>(n, a, x, y);
}
```

`__global__` 的参数按值传递。指针参数必须指向设备可访问的存储。参数总量有上限（当代工具链通常为数 KB 到 32KB 量级），大数组应放在 `cudaMalloc` 的缓冲里传指针，不要塞进参数列表。

`__device__` 函数不能用 `<<<>>>` 启动。主机普通函数不能直接调用 `__device__`。

### Kernel 启动

```cuda
kernel<<<grid, block>>>(args...);
kernel<<<grid, block, dyn_shared_bytes, stream>>>(args...);
```

四个配置项：grid 尺寸、block 尺寸、动态 shared memory 字节数（默认 0）、stream（默认 0，即默认 stream）。grid/block 的上限与索引计算见线程模型。动态 shared 是每个 block 一块连续区域，见 [Shared memory](#shared-memory)。`stream` 表示入队到哪一条设备工作队列，见 [Stream 与 Event](#stream-与-event)。

启动本身只检查配置是否明显非法（例如 block 线程数超过 1024）并把 kernel 放入指定 stream。资源不足、设备侧断言、非法访存在启动返回时通常还看不到，需要后续 `cudaGetLastError` 与同步。

### Runtime API 调用序

常用 Runtime（`cuda_runtime.h`）调用顺序：

```cuda
cudaSetDevice(0);
cudaDeviceProp prop{};
cudaGetDeviceProperties(&prop, 0);   // SM 数量、maxThreadsPerBlock、sharedMemPerBlock 等

float *d_x = nullptr, *d_y = nullptr;
cudaMalloc(&d_x, n * sizeof(float));
cudaMalloc(&d_y, n * sizeof(float));
cudaMemcpy(d_x, h_x, n * sizeof(float), cudaMemcpyHostToDevice);
cudaMemcpy(d_y, h_y, n * sizeof(float), cudaMemcpyHostToDevice);

saxpy<<<grid, block>>>(n, a, d_x, d_y);

cudaMemcpy(h_y, d_y, n * sizeof(float), cudaMemcpyDeviceToHost);  // 默认同步，等待 kernel
cudaFree(d_x);
cudaFree(d_y);
```

未调用 `cudaSetDevice` 时默认使用设备 0。分配、拷贝、释放的语义见内存模型；这里只要求：kernel 启动前设备端输入已就绪，读回主机前已同步。

Driver API（`cuInit`、`CUcontext`、`cuLaunchKernel`）更底层，与 Runtime 不要混用同一套未文档化的假设。本笔记默认只用 Runtime。

### 错误处理

返回类型为 `cudaError_t`。`cudaSuccess` 表示该调用本身成功。

* `cudaGetLastError()`：取出并清除当前线程记录的最近一次错误（含非法 launch 配置）。
* `cudaPeekAtLastError()`：取出但不清除。
* kernel 执行中的错误（越界、非法指令、device-side `assert`）是异步的，通常要 `cudaDeviceSynchronize()` 或一次同步拷贝之后，再用 `cudaGetLastError()` 才能看到。

每个会失败的 API 都应检查返回值。只检查 launch 下一行、不同步设备，会漏掉 kernel 体内的失败。

### 同步等待

`cudaDeviceSynchronize()` 等待该设备上所有 stream 完成。只等待一条 stream 用 `cudaStreamSynchronize(stream)`。默认 stream 与隐式同步见 [Stream 与 Event](#stream-与-event)。

### 编译与 Compute Capability

`nvcc` 对 `.cu` 做两次编译：主机部分交给主机 C++ 编译器；设备部分生成 PTX 和/或 SASS。设备代码路径上定义 `__CUDA_ARCH__`（例如 sm_75 为 `750`），主机路径上不定义。因此 `__host__ __device__` 函数里可用它分支：

```cuda
__host__ __device__ int warp_size() {
#ifdef __CUDA_ARCH__
    return warpSize;
#else
    return 32;
#endif
}
```

Compute Capability（CC）标识设备指令集与硬件能力，写成 `主.次`，对应编译目标 `sm_XY`。RTX 2070 为 **7.5**（Turing，`sm_75`）。CMake：

```cmake
set(CMAKE_CUDA_ARCHITECTURES 75)
```

等价于生成针对 `sm_75` 的机器码。只生成 PTX、不生成对应 SASS 时，运行时可在驱动里 JIT；发行构建应包含目标卡的 SASS，避免依赖 JIT。一张卡不能执行高于其 CC 的 SASS（例如不要把只含 `sm_80` 的二进制拿到 sm_75 上跑，除非同时带有可 JIT 的较低/兼容 PTX，且驱动支持）。

与本章相关的能力差异（细节仍以 `cudaDeviceProp` 和该 CC 的文档为准）：

* 独立线程调度、带 mask 的 `__syncwarp` / shuffle：Volta（7.0）及以后，含 Turing 7.5。
* 每 block 最多 1024 线程：Fermi 以后的常见上限。
* 每 SM 最大驻留线程/warp、shared memory 容量：随架构变化，用 `prop.maxThreadsPerMultiProcessor`、`prop.sharedMemPerBlock` 等查询，不要写死。

需要设备链接（`__device__` 跨翻译单元调用）时打开 relocatable device code（`nvcc -rdc=true` / `CMAKE_CUDA_SEPARABLE_COMPILATION`），并做设备链接。单文件 kernel 不必开。

### 最小主机程序

```cuda
#include <cuda_runtime.h>
#include <cstdio>

__global__ void add_one(float* x, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) x[i] += 1.f;
}

int main() {
    const int n = 1 << 20;
    float* d = nullptr;
    cudaMalloc(&d, n * sizeof(float));
    cudaMemset(d, 0, n * sizeof(float));

    add_one<<<(n + 255) / 256, 256>>>(d, n);

    cudaError_t e = cudaDeviceSynchronize();
    if (e != cudaSuccess) {
        std::fprintf(stderr, "%s\n", cudaGetErrorString(e));
        return 1;
    }
    cudaFree(d);
    return 0;
}
```

grid-stride、越界判断、以及为何 `(n + 255) / 256` 可能仍小于 `n` 从而必须 `if (i < n)`，见 [grid-stride](#grid-stride)。

## Stream 与 Event

Stream 是设备端操作的有序队列，类型为 `cudaStream_t`。Kernel 启动、`cudaMemcpyAsync`、部分 `cudaMemsetAsync` 以及 event 记录都进入某条 stream。同一条 stream 内的操作按入队顺序完成。不同 stream 之间没有默认先后关系，硬件在资源允许时可以重叠（例如拷贝引擎与 SM 同时工作）。

`<<<grid, block, dyn_shared_bytes, stream>>>` 的第四个参数指定本次启动进入哪条队列，不改变 grid/block 尺寸，也不参与 `threadIdx` 计算。

### 创建与同步

```cuda
cudaStream_t s;
cudaStreamCreate(&s);
kernel<<<grid, block, 0, s>>>(args);
cudaStreamSynchronize(s);   // 仅等待 s
cudaStreamDestroy(s);
```

| API | 等待范围 |
|-----|----------|
| `cudaStreamSynchronize(s)` | 仅 `s` 中已入队且尚未完成的操作 |
| `cudaDeviceSynchronize()` | 当前设备上全部 stream |
| `cudaEventSynchronize(ev)` | 直到 `ev` 被记录且该点之前的工作完成 |

`cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking)` 创建的 stream 不与默认 stream 隐式互相等待，便于和默认队列上的工作并发。销毁前须保证该 stream 上已无未完成工作，或先同步。

### 默认 stream

第四个参数省略、传入 `0` 或 `cudaStreamDefault`，即默认 stream。

同一默认 stream 内仍按入队顺序执行。`cudaMemcpy`（同步版本）会等待默认 stream 中在它之前的工作，并在拷贝完成前阻塞主机。

传统（legacy）默认 stream 还会与 **同一设备上其它非 `NonBlocking` stream** 隐式同步：默认 stream 上的操作要等其它 stream 中已入队工作完成才开始，其它 stream 上的操作也要等默认 stream 空闲。因此在 legacy 模式下，一边用 `<<<grid, block>>>`、一边用显式 stream 做 `cudaMemcpyAsync`，重叠常被抵消。

`nvcc --default-stream per-thread`（CMake 可设 `CMAKE_CUDA_FLAGS`）改为每个主机线程一条自己的默认 stream，且该默认 stream 不再与其它 stream 做上述隐式同步。要重叠拷贝与计算，仍应创建显式 `cudaStream_t`，使用 `cudaMemcpyAsync`，不要依赖同步 `cudaMemcpy`。

### 异步拷贝与 pinned memory

`cudaMemcpyAsync(..., stream)` 把拷贝入队到指定 stream，调用本身通常立即返回。要与 kernel 重叠，主机端缓冲应是 **页锁定** 内存：

```cuda
float* h;
cudaMallocHost(&h, n * sizeof(float));   // pinned
cudaMemcpyAsync(d, h, n * sizeof(float), cudaMemcpyHostToDevice, s);
kernel<<<grid, block, 0, s>>>(d, n);
```

`malloc` 得到的可分页内存上，`cudaMemcpyAsync` 往往无法与计算真正重叠：驱动可能先把数据拷进内部 pinned 暂存，主机侧表现接近同步。`cudaMalloc` 的设备指针之间的 D2D 异步拷贝不经过 PCIE，但仍占用拷贝引擎，并遵守所在 stream 的顺序。

设备通常另有 DMA/拷贝引擎，与 SM 相对独立。重叠的典型形态是：一条 stream 在做 H2D/D2H，另一条 stream 上的 kernel 占用 SM。单个 kernel 已经占满全部 SM 时，另一条 stream 上的 kernel 仍要等计算资源，此时多 stream 的收益主要在拷贝与计算之间，而不是两个满载 kernel 对打。

### Event

`cudaEvent_t` 标记某条 stream 时间线上的一点，用于跨 stream 依赖或测时。

```cuda
cudaEvent_t ev;
cudaEventCreate(&ev);

kernelA<<<..., stream1>>>(...);
cudaEventRecord(ev, stream1);              // A 在 stream1 中完成后，ev 触发
cudaStreamWaitEvent(stream2, ev, 0);       // stream2 等到 ev
kernelB<<<..., stream2>>>(...);            // 可见 A 对设备存储的写（同设备、经该同步）

cudaEventDestroy(ev);
```

`cudaEventRecord(ev, stream)` 把「记录」本身入队到 `stream`：该 stream 上排在 record 之前的操作完成后，event 变为已触发。`cudaStreamWaitEvent(dst, ev, 0)` 使 `dst` 在后续操作开始前等待该 event。有数据依赖的两项必须排在同一 stream，或用 event 显式等待；排进两条 stream 且不加 event，是数据竞争。

仅用于同步、不测时时可 `cudaEventCreateWithFlags(&ev, cudaEventDisableTiming)`，开销低于默认可计时 event。`cudaEventQuery(ev)` 非阻塞查询是否已触发。

### 测时

```cuda
cudaEvent_t start, stop;
cudaEventCreate(&start);
cudaEventCreate(&stop);

kernel<<<grid, block, 0, s>>>(...);        // 预热
cudaStreamSynchronize(s);

cudaEventRecord(start, s);
kernel<<<grid, block, 0, s>>>(...);
cudaEventRecord(stop, s);
cudaEventSynchronize(stop);

float ms = 0.f;
cudaEventElapsedTime(&ms, start, stop);    // 毫秒，分辨率约 0.5 µs 量级
```

`cudaEventElapsedTime` 要求两个 event 都已触发，且创建时未设 `cudaEventDisableTiming`。主机侧 `std::chrono` 会把 launch 入队时间和 CPU 同步开销算进去，不能代替这对 event。第一次 launch 常含 JIT/上下文初始化，测稳态应预热。

### 多 stream 流水线

彼此无依赖的拷贝与计算可以分到不同 stream。有依赖则用同一 stream 或 event。双缓冲示意（省略错误检查）：

```cuda
cudaStream_t s[2];
cudaStreamCreate(&s[0]);
cudaStreamCreate(&s[1]);

for (int i = 0; i < nbatch; ++i) {
    int k = i % 2;
    cudaMemcpyAsync(d_in[k], h_in[i], bytes, cudaMemcpyHostToDevice, s[k]);
    compute<<<grid, block, 0, s[k]>>>(d_in[k], d_out[k]);
    cudaMemcpyAsync(h_out[i], d_out[k], bytes, cudaMemcpyDeviceToHost, s[k]);
}
cudaStreamSynchronize(s[0]);
cudaStreamSynchronize(s[1]);
```

`h_in` / `h_out` 应为 pinned。`d_in[0]` 与 `d_in[1]` 必须是不同缓冲，否则两次 launch 写同一块设备内存且跨 stream 无 event，结果未定义。

### 不适用的情况

一次计算图是单条依赖链、输入已在设备上时，把各算子拆到多 stream **不会**缩短这一条链：后一层必须等前一层写完。此时用默认 stream 按拓扑顺序 launch 即可。

多 stream 也不能代替 kernel 内部的 `__syncthreads()` 或正确的合并访问。它只排列 **多次** 主机提交的设备操作之间的并发与顺序。

### 优先级（可选）

`cudaDeviceGetStreamPriorityRange` 给出优先级区间；`cudaStreamCreateWithPriority` 创建高/低优先级 stream。资源争用时调度器更偏向高优先级队列中的工作，不改变同一 stream 内的顺序。测量和正确性不依赖优先级，一般可忽略。
