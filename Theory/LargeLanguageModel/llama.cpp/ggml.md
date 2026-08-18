# GGML

`GGML` 是 `llama.cpp` 的底层张量计算引擎，负责表示张量和计算图、管理计算内存，并将模型计算调度到 CPU、CUDA、Metal 等后端执行。

## 构建张量

在 GGML 中，张量既描述数据，也可以表示计算图中的一个节点。张量之间通过算子建立依赖关系。

### `struct ggml_tensor`

```cpp
struct ggml_tensor {
    enum ggml_type type;

    struct ggml_backend_buffer * buffer;

    int64_t ne[GGML_MAX_DIMS]; // number of elements
    size_t  nb[GGML_MAX_DIMS]; // stride in bytes:
                                // nb[0] = ggml_type_size(type)
                                // nb[1] = nb[0]   * (ne[0] / ggml_blck_size(type)) + padding
                                // nb[i] = nb[i-1] * ne[i-1]

    // compute data
    enum ggml_op op;

    // op params - allocated as int32_t for alignment
    int32_t op_params[GGML_MAX_OP_PARAMS / sizeof(int32_t)];

    int32_t flags;

    struct ggml_tensor * src[GGML_MAX_SRC];

    // source tensor and offset for views
    struct ggml_tensor * view_src;
    size_t               view_offs;

    void * data;

    char name[GGML_MAX_NAME];

    void * extra; // extra things e.g. for ggml-cuda.cu

    char padding[8];
};
```

主要字段如下：

- `type`：数据类型，如 `GGML_TYPE_F32`、`GGML_TYPE_F16`、`GGML_TYPE_Q4_0`。
- `buffer`：承载张量数据的后端缓冲区，例如 CPU 内存或 GPU 显存。
- `ne`：各维度的元素数。二维张量中，`ne[0]` 为列数，`ne[1]` 为行数；2 行 3 列对应 `ne[0] = 3`、`ne[1] = 2`。
- `nb`：各维度的字节步长；普通类型沿第 `i` 维前进一个元素，地址增加 `nb[i]`，块量化类型的第 0 维则以量化块为存储单位。
- `op`：生成该张量的算子；没有生成算子时为 `GGML_OP_NONE`。
- `op_params`：算子的专用参数区，具体含义由 `op` 决定。
- `flags`：描述张量角色的位标志，例如输入、输出或模型参数。
- `src`：当前算子的输入张量，同时表示该节点的直接依赖。
- `view_src`、`view_offs`：视图共享存储的源张量及字节偏移；非视图张量的 `view_src` 为 `NULL`。

GGML 的连续张量默认采用行优先布局：第 0 维变化最快，更高维依次变慢。视图、转置等张量可能不连续，实际布局应以 `nb` 为准。

### 创建张量

```cpp
struct ggml_tensor * ggml_new_tensor(
        struct ggml_context * ctx,
        enum   ggml_type      type,
        int                   n_dims,
        const int64_t       * ne);
```

`ggml_new_tensor` 在 `ctx` 中创建一个 `n_dims` 维张量，维度由 `ne` 指定。新张量没有生成算子，因此 `op` 初始为 `GGML_OP_NONE`；是否立即分配数据空间取决于`context`的分配配置。这个函数通常只是分配`tensor`的元数据，而不分配实际数据；实际数据的分配通常在后续调用`process_ubatch`时通过`ggml_backend_sched_alloc_graph`分配.

### 描述张量计算关系

调用 GGML 算子时通常不会立即计算，而是创建结果张量并记录：

- `op`：使用的算子；
- `src`：输入张量；
- `op_params`：算子参数。

例如，下面的代码描述了“先进行矩阵乘，再加上 `b`”的计算关系：

```cpp
struct ggml_tensor * wx = ggml_mul_mat(ctx, w, x);
struct ggml_tensor * y  = ggml_add(ctx, wx, b);
```

此时各张量之间的关系为：

```text
w ─┐
   ├─ MUL_MAT ─> wx ─┐
x ─┘                  ├─ ADD ─> y
b ────────────────────┘
```

`wx` 记录 `GGML_OP_MUL_MAT` 及输入 `w`、`x`；`y` 记录 `GGML_OP_ADD` 及输入 `wx`、`b`。此时只建立了依赖关系，尚未执行数值计算。

## 构建计算图

计算图从一个或多个输出张量出发，收集其依赖的全部张量，并将需要执行的节点排列为拓扑序列。

### `struct ggml_cgraph`

```cpp
struct ggml_cgraph {
    int size;    // maximum number of nodes/leafs/grads/grad_accs
    int n_nodes; // number of nodes currently in use
    int n_leafs; // number of leafs currently in use

    struct ggml_tensor ** nodes;     // tensors with data that can change if the graph is evaluated
    struct ggml_tensor ** grads;     // the outputs of these tensors are the gradients of the nodes
    struct ggml_tensor ** grad_accs; // accumulators for node gradients
    struct ggml_tensor ** leafs;     // tensors with constant data
    int32_t             * use_counts;// number of uses of each tensor, indexed by hash table slot

    struct ggml_hash_set visited_hash_set;

    enum ggml_cgraph_eval_order order;

    // an optional identifier that can be utilized to recognize same graphs if two non-zero values match
    // a value of 0 means it is not set and should be ignored
    uint64_t uid;
};
```

主要字段如下：

- `size`：图的容量上限，限制 `nodes` 和 `leafs` 可容纳的张量数，并决定相关辅助存储的规模。
- `n_nodes`、`n_leafs`：`nodes` 和 `leafs` 中当前有效的元素数。
- `nodes`：节点张量数组，按拓扑顺序排列；包含算子输出和标记为模型参数的张量。
- `grads`：各已访问张量的梯度张量，按 `visited_hash_set` 的槽位索引。
- `grad_accs`：各已访问张量的梯度累加器，索引方式与 `grads` 相同。
- `leafs`：不由图中算子生成的输入张量数组。
- `use_counts`：每个已访问张量作为其他节点输入的次数，按 `visited_hash_set` 的槽位索引。
- `visited_hash_set`：记录构图过程中访问过的全部张量，用于去重并为 `use_counts`、`grads` 等数组提供索引。
- `order`：构图时遍历各节点 `src` 的顺序，可选择从左到右或从右到左，并影响同级分支在拓扑序列中的排列。
- `uid`：可选的图标识符；值为 `0` 表示未设置，两个非零值相同的图可被识别为同一图。

`llama.cpp` 的常规推理不需要反向传播，图通常在禁用梯度的情况下创建，此时 `grads` 和 `grad_accs` 均为 `NULL`。

### 叶子节点和计算节点

构建计算图时，GGML 根据 `op` 和 `GGML_TENSOR_FLAG_PARAM` 对张量分类：`op == GGML_OP_NONE` 且未标记为参数的张量进入 `leafs`，其余张量进入 `nodes`。因此，`nodes` 除了算子生成的计算节点，还可能包含用于训练的参数节点。

```cpp
if (tensor->op == GGML_OP_NONE &&
    !(tensor->flags & GGML_TENSOR_FLAG_PARAM)) {
    // 叶子节点
} else {
    // nodes 中的节点
}
```

- **叶子节点**：没有生成算子且未标记为模型参数。模型权重、输入和常量通常属于此类。执行计算图时会读取它们的数据，但不会为它们执行算子。
- **计算节点**：由算子生成的结果张量，例如 `wx` 和 `y`。后端按拓扑顺序执行其 `op`。
- **参数节点**：标记了 `GGML_TENSOR_FLAG_PARAM` 的张量，即使 `op == GGML_OP_NONE` 也会进入 `nodes`，以便构建梯度图；该情况主要用于训练。

在上面的示例中，若未设置参数标志，`w`、`x`、`b` 是叶子节点，`wx`、`y` 是计算节点。

### 从输出张量构建计算图

```cpp
struct ggml_cgraph * graph = ggml_new_graph(ctx);
ggml_build_forward_expand(graph, y);
```

```cpp
void ggml_build_forward_expand(struct ggml_cgraph * cgraph, struct ggml_tensor * tensor) {
    ggml_build_forward_impl(cgraph, tensor, true, true);
}
```

```cpp
static size_t ggml_visit_parents_graph(struct ggml_cgraph * cgraph, struct ggml_tensor * node, bool compute) {
    if (node->op != GGML_OP_NONE && compute) {
        node->flags |= GGML_TENSOR_FLAG_COMPUTE;
    }

    const size_t node_hash_pos = ggml_hash_find(&cgraph->visited_hash_set, node);
    GGML_ASSERT(node_hash_pos != GGML_HASHSET_FULL);

    if (ggml_bitset_get(cgraph->visited_hash_set.used, node_hash_pos)) {
        // already visited

        if (compute) {
            // update the compute flag regardless
            for (int i = 0; i < GGML_MAX_SRC; ++i) {
                struct ggml_tensor * src = node->src[i];
                if (src && ((src->flags & GGML_TENSOR_FLAG_COMPUTE) == 0)) {
                    ggml_visit_parents_graph(cgraph, src, true);
                }
            }
        }

        return node_hash_pos;
    }

    // This is the first time we see this node in the current graph.
    cgraph->visited_hash_set.keys[node_hash_pos] = node;
    ggml_bitset_set(cgraph->visited_hash_set.used, node_hash_pos);
    cgraph->use_counts[node_hash_pos] = 0;

    for (int i = 0; i < GGML_MAX_SRC; ++i) {
        const int k =
            (cgraph->order == GGML_CGRAPH_EVAL_ORDER_LEFT_TO_RIGHT) ? i :
            (cgraph->order == GGML_CGRAPH_EVAL_ORDER_RIGHT_TO_LEFT) ? (GGML_MAX_SRC-1-i) :
            /* unknown order, just fall back to using i */ i;

        struct ggml_tensor * src = node->src[k];
        if (src) {
            const size_t src_hash_pos = ggml_visit_parents_graph(cgraph, src, compute);

            // Update the use count for this operand.
            cgraph->use_counts[src_hash_pos]++;
        }
    }

    if (node->op == GGML_OP_NONE && !(node->flags & GGML_TENSOR_FLAG_PARAM)) {
        // reached a leaf node, not part of the gradient graph (e.g. a constant)
        GGML_ASSERT(cgraph->n_leafs < cgraph->size);

        if (strlen(node->name) == 0) {
            ggml_format_name(node, "leaf_%d", cgraph->n_leafs);
        }

        cgraph->leafs[cgraph->n_leafs] = node;
        cgraph->n_leafs++;
    } else {
        GGML_ASSERT(cgraph->n_nodes < cgraph->size);

        if (strlen(node->name) == 0) {
            ggml_format_name(node, "node_%d", cgraph->n_nodes);
        }

        cgraph->nodes[cgraph->n_nodes] = node;
        cgraph->n_nodes++;
    }

    return node_hash_pos;
}
```

`ggml_build_forward_expand` 只扩展计算图，不执行数值计算，主要完成以下工作：

- 从 `tensor` 出发，按照 `cgraph->order` 指定的顺序，对 `src` 进行深度优先遍历。
- 首次访问张量时将其登记到 `visited_hash_set`，避免共享依赖被重复加入图中。
- 每遇到一条指向输入张量的边，就增加该输入张量在 `use_counts` 中的使用次数。
- 当 `compute == true` 时，为具有算子的张量设置 `GGML_TENSOR_FLAG_COMPUTE`。
- 先递归处理全部依赖，再将当前张量加入 `leafs` 或 `nodes`。因此数组记录的是经过分类的 DFS 后序，`nodes` 中的依赖节点位于使用者之前，构成拓扑顺序。
- 在已有 `cgraph` 上追加尚未访问的节点，因此可以多次调用以加入多个输出张量。

### 分配数据内存

```cpp
bool ggml_backend_sched_alloc_graph(ggml_backend_sched_t sched, struct ggml_cgraph * graph) {
    GGML_ASSERT(sched);
    GGML_ASSERT((int)sched->hash_set.size >= graph->n_nodes + graph->n_leafs);
    GGML_ASSERT(!sched->is_alloc);

    sched->cur_copy = sched->next_copy;
    sched->next_copy = (sched->next_copy + 1) % sched->n_copies;

    ggml_backend_sched_split_graph(sched, graph);

    if (!ggml_backend_sched_alloc_splits(sched)) {
        return false;
    }

    sched->is_alloc = true;

    return true;
}
```

`ggml_backend_sched_alloc_graph` 先按 backend 切分计算图，再为张量分配对应 backend 的存储空间。buffer 不一定位于 CPU 内存，也可能位于 CUDA、Metal、Vulkan 等设备可用的显存或共享内存中。该函数不执行计算或数据传输。

- `cur_copy`：选择本次使用的流水线并行副本。
- `ggml_backend_sched_split_graph`：根据张量位置、backend 优先级和算子支持情况分配节点，将连续的同 backend 节点划为 split，并为跨 backend 输入创建副本元数据、改写 `src`。实际复制在执行 split 时发生。
- `ggml_backend_sched_alloc_splits`：使用 graph allocator 在各 backend 的 buffer 中分配或复用张量空间；若 backend 分配变化或原有规划不再适用，则同步设备、重新预留 buffer 并重试。

分配成功后设置 `is_alloc`；无法分配时返回 `false`。

## 执行计算图

以 CPU 后端为例，计算节点按 `cgraph->nodes` 中的拓扑顺序执行。对于支持并行的算子，所有工作线程共同执行同一个 kernel，并根据线程编号或动态 chunk 划分数据；部分算子可能只使用一个线程。节点之间通过 barrier 同步，以保证依赖节点已经完成。

```cpp
static enum ggml_status ggml_backend_cpu_graph_compute(ggml_backend_t backend, struct ggml_cgraph * cgraph) {
    struct ggml_backend_cpu_context * cpu_ctx = (struct ggml_backend_cpu_context *)backend->context;

    struct ggml_cplan cplan = ggml_graph_plan(cgraph, cpu_ctx->n_threads, cpu_ctx->threadpool);

    if (cpu_ctx->work_size < cplan.work_size) {
        delete[] cpu_ctx->work_data;
        cpu_ctx->work_data = new uint8_t[cplan.work_size];
        if (cpu_ctx->work_data == NULL) {
            cpu_ctx->work_size = 0;
            return GGML_STATUS_ALLOC_FAILED;
        }
        cpu_ctx->work_size = cplan.work_size;
    }
    cplan.work_data = (uint8_t *)cpu_ctx->work_data;

    cplan.abort_callback      = cpu_ctx->abort_callback;
    cplan.abort_callback_data = cpu_ctx->abort_callback_data;
    cplan.use_ref             = cpu_ctx->use_ref;

    return ggml_graph_compute(cgraph, &cplan);
}

enum ggml_status ggml_graph_compute(struct ggml_cgraph * cgraph, struct ggml_cplan * cplan) {
    ggml_cpu_init();

    GGML_ASSERT(cplan);
    GGML_ASSERT(cplan->n_threads > 0);
    GGML_ASSERT(cplan->work_size == 0 || cplan->work_data != NULL);

    int n_threads                               = cplan->n_threads;
    struct ggml_threadpool * threadpool = cplan->threadpool;

    bool disposable_threadpool = false;

    if (threadpool == NULL) {
        //GGML_PRINT_DEBUG("Threadpool is not specified. Will create a disposable threadpool : n_threads %d\n", n_threads);
        disposable_threadpool = true;

        struct ggml_threadpool_params ttp = ggml_threadpool_params_default(n_threads);
        threadpool = ggml_threadpool_new_impl(&ttp, cgraph, cplan);
    } else {
        // Reset some of the parameters that need resetting
        // No worker threads should be accessing the parameters below at this stage
        threadpool->cgraph           = cgraph;
        threadpool->cplan            = cplan;
        threadpool->current_chunk    = 0;
        threadpool->abort            = -1;
        threadpool->ec               = GGML_STATUS_SUCCESS;
    }

#ifdef GGML_USE_OPENMP
    if (n_threads > 1) {
        #pragma omp parallel num_threads(n_threads)
        {
            #pragma omp single
            {
                // update the number of threads from the actual number of threads that we got from OpenMP
                n_threads = omp_get_num_threads();
                atomic_store_explicit(&threadpool->n_graph, n_threads, memory_order_relaxed);
            }

            // Apply thread CPU mask and priority
            int ith = omp_get_thread_num();

            ggml_thread_apply_priority(threadpool->prio);
            if (ggml_thread_cpumask_is_valid(threadpool->workers[ith].cpumask)) {
                ggml_thread_apply_affinity(threadpool->workers[ith].cpumask);
            }
            ggml_graph_compute_thread(&threadpool->workers[ith]);
        }
    } else {
        atomic_store_explicit(&threadpool->n_graph, 1, memory_order_relaxed);
        ggml_graph_compute_thread(&threadpool->workers[0]);
    }
#else
    if (n_threads > threadpool->n_threads) {
        GGML_LOG_WARN("cplan requested more threads (%d) than available (%d)\n", n_threads, threadpool->n_threads);
        n_threads = threadpool->n_threads;
    }

    // Kick all threads to start the new graph
    ggml_graph_compute_kickoff(threadpool, n_threads);

    // This is a work thread too
    ggml_graph_compute_thread(&threadpool->workers[0]);
#endif

    // don't leave affinity set on the main thread
    clear_numa_thread_affinity();

    enum ggml_status ret = threadpool->ec;

    if (disposable_threadpool) {
        ggml_threadpool_free(threadpool);
    }

    return ret;
}
```

`ggml_backend_cpu_graph_compute` 调用 `ggml_graph_plan` 确定线程配置和临时工作区大小，复用或扩展 `work_data`，然后调用 `ggml_graph_compute`。后者复用已有线程池；若未提供线程池，则临时创建，并让主线程也参与计算。

```cpp
static thread_ret_t ggml_graph_compute_thread(void * data) {
    struct ggml_compute_state * state = (struct ggml_compute_state *) data;
    struct ggml_threadpool    * tp    = state->threadpool;

    const struct ggml_cgraph * cgraph = tp->cgraph;
    const struct ggml_cplan  * cplan  = tp->cplan;

#ifdef GGML_USE_CPU_RISCV64_SPACEMIT
    ggml_backend_cpu_riscv64_spacemit_set_numa_thread_affinity(state->ith);
#else
    set_numa_thread_affinity(state->ith);
#endif

    struct ggml_compute_params params = {
        /*.ith        =*/ state->ith,
        /*.nth        =*/ atomic_load_explicit(&tp->n_graph, memory_order_relaxed) & GGML_THREADPOOL_N_THREADS_MASK,
        /*.wsize      =*/ cplan->work_size,
        /*.wdata      =*/ cplan->work_data,
        /*.threadpool =*/ tp,
        /*.use_ref    =*/ cplan->use_ref,
    };

#ifdef GGML_USE_OPENMP
    GGML_PRINT_DEBUG("thread #%d compute-start cplan %p\n", state->ith, (const void *)cplan);
#else
    GGML_PRINT_DEBUG("thread #%d compute-start cplan %p last-graph %d\n", state->ith, (const void *)cplan, state->last_graph);
#endif

    for (int node_n = 0; node_n < cgraph->n_nodes && atomic_load_explicit(&tp->abort, memory_order_relaxed) != node_n; node_n++) {
        struct ggml_tensor * node = cgraph->nodes[node_n];

        if (ggml_op_is_empty(node->op)) {
            // skip NOPs
            continue;
        }

        if ((node->flags & GGML_TENSOR_FLAG_COMPUTE) == 0) {
            continue;
        }

        // TODO: move fused-op detection into ggml_graph_plan so fusion decisions are made once at planning time
        // Try fused ops, fall back to normal compute
        const int n_fused = ggml_cpu_try_fuse_ops(cgraph, node_n, &params, cplan);
        if (n_fused > 0) {
            node_n += n_fused;
        } else {
            ggml_compute_forward(&params, node);
        }

        if (state->ith == 0 && cplan->abort_callback &&
                cplan->abort_callback(cplan->abort_callback_data)) {
            atomic_store_explicit(&tp->abort, node_n + 1, memory_order_relaxed);
            tp->ec    = GGML_STATUS_ABORTED;
        }

        if (node_n + 1 < cgraph->n_nodes) {
            ggml_barrier(state->threadpool);
        }
    }

#ifdef GGML_USE_OPENMP
    GGML_PRINT_DEBUG("thread #%d compute-done cplan %p\n", state->ith, (const void *)cplan);
#else
    GGML_PRINT_DEBUG("thread #%d compute-done cplan %p last-graph %d\n", state->ith, (const void *)cplan, state->last_graph);
#endif

    ggml_barrier(state->threadpool);

#ifdef GGML_USE_CPU_RISCV64_SPACEMIT
    ggml_backend_cpu_riscv64_spacemit_clear_numa_thread_affinity_threaded(state->ith);
#endif

    return 0;
}
```

`ggml_graph_compute_thread` 是每个工作线程的执行入口。各线程根据 `cplan` 创建包含 `ith`、`nth` 和工作区的 `params`，再以相同顺序遍历 `nodes`：跳过空操作和未标记为计算的节点，优先尝试融合，否则由 `ggml_compute_forward` 分派到具体 kernel。节点之间使用 barrier 同步；线程 0 还负责检查中止回调。

### 融合算子

CPU 后端在执行普通 kernel 前，会检查相邻节点是否匹配已支持的融合模式并满足类型、形状和引用关系等条件。禁用融合或启用参考实现时，不会进行融合。

```cpp
static int ggml_cpu_try_fuse_ops(
        const struct ggml_cgraph * cgraph,
        const int node_n,
        const struct ggml_compute_params * params,
        const struct ggml_cplan * cplan) {

    if (ggml_cpu_disable_fusion || cplan->use_ref) {
        return 0;
    }

    struct ggml_tensor * node = cgraph->nodes[node_n];

    if (node->op == GGML_OP_RMS_NORM) {
        // RMS_NORM + MUL fusion
        const enum ggml_op fuse_ops[] = { GGML_OP_RMS_NORM, GGML_OP_MUL };
        if (ggml_can_fuse(cgraph, node_n, fuse_ops, 2)) {
            struct ggml_tensor * mul_node = cgraph->nodes[node_n + 1];
            const struct ggml_tensor * mul_w = (mul_node->src[0] == node)
                ? mul_node->src[1] : mul_node->src[0];
            if (node->src[0]->type  == GGML_TYPE_F32 &&
                mul_node->type      == GGML_TYPE_F32 &&
                mul_w->type         == GGML_TYPE_F32 &&
                mul_w->ne[0]        == node->ne[0]   &&
                mul_w->nb[0]        == sizeof(float)) {

                ggml_compute_forward_rms_norm_mul_fused(params, node, mul_node);
                return 1;
            }
        }
    }

    return 0;
}
```

`ggml_cpu_try_fuse_ops` 不修改计算图，而是直接调用融合 kernel，并返回已覆盖的后续节点数。调用方据此增加 `node_n`，跳过已经由融合 kernel 完成的节点；例如返回 `1` 表示跳过紧随其后的 `MUL` 节点。

## 常见函数

### GGML_TENSOR_BINARY_OP_LOCALS

这个宏，把二元算子中经常需要计算的形状与步长从`src0`,`src1`,`dst`抽成局部变量.

展开后相当于

```txt
// src0
ne00, ne01, ne02, ne03   = src0->ne[0..3]
nb00, nb01, nb02, nb03   = src0->nb[0..3]

// src1
ne10, ne11, ne12, ne13   = src1->ne[0..3]
nb10, nb11, nb12, nb13   = src1->nb[0..3]

// dst
ne0,  ne1,  ne2,  ne3    = dst->ne[0..3]
nb0,  nb1,  nb2,  nb3    = dst->nb[0..3]
```

## 常见算子

### `GGML_OP_VIEW`

`GGML_OP_VIEW` 表示对已有张量存储的零拷贝视图。视图不拥有独立的数据存储，但可以通过自己的形状和步长描述源张量的子区域或不同布局。

视图张量的 `view_src` 指向共享存储的源张量，`view_offs` 表示相对源存储的字节偏移，`nb` 描述各维度的字节步长。执行计算图时该算子不进行数值计算，但会保留数据依赖，并为内存分配器和 backend 提供存储别名信息。

### `GGML_OP_PERMUTE`

```CPP
struct ggml_tensor * ggml_permute(
        struct ggml_context * ctx,
        struct ggml_tensor  * a,
        int                   axis0,
        int                   axis1,
        int                   axis2,
        int                   axis3)
```

`GGML_OP_PERMUTE`表示重排张量轴的view.不拷贝数据，而是只改每个维度长度`ne[]`和步长`nb[]`,让后续算子用新的轴顺序去读同一块内存.`axis0`,`axis1`,`axis2`,`axis3`的含义是把输入的第`i`维放到输出的`axis_i`位置。

对于一个`tensor`它的维度为`(nb[0], nb[1], nb[2], nb[3])`,对当中的某个元素的访问公式是

```CPP
tensor(i0, i1, i2, i3) = base + i0*nb[0] + i1*nb[1] + i2*nb[2] + i3*nb[3]
```

所以`PERMUTE`操作就是重排了一下ne和nb.

```CPP
ne[axis0] = a->ne[0];
ne[axis1] = a->ne[1];
ne[axis2] = a->ne[2];
ne[axis3] = a->ne[3];

nb[axis0] = a->nb[0];
nb[axis1] = a->nb[1];
nb[axis2] = a->nb[2];
nb[axis3] = a->nb[3];
```

### `GGML_OP_MUL_MAT`

```CPP
struct ggml_tensor * ggml_mul_mat(
        struct ggml_context * ctx,
        struct ggml_tensor  * a,
        struct ggml_tensor  * b)
```

`GGML_OP_MUL_MAT`表示进行矩阵乘法，对最内层维度进行点积计算。设`a: [K, N]`,`b: [K, M]`，则`ggml_mul_mat(a,b)`的结果是`c: [N,M]`,其中`c[n, m] = dot(a[:, n], b[:, m])`.

数学含义可以按照两种方式理解:

1. 按照头文件的注释，理解为$A \in \mathbb{R}^{N \times K}$,$B \in \mathbb{R}^{M \times K}$,此时$C = AB^T$,$C \in \mathbb{R}^{N \times M}$.此时矩阵理解为行主序.
2. 直接理解为$A \in \mathbb{R}^{K \times N}$,$B \in \mathbb{R}^{K \times M}$,此时$C = A^TB$,$C \in \mathbb{R}^{N \times M}$,此时矩阵理解为列主序.

在函数`ggml_compute_forward_mul_mat`中就是实际线程进行矩阵乘法的计算代码.

当`a: [K, N, A2, A3]`,`b: [K, M, B2, B3]`,`dst: [N, M, B2, B3]`，其中必须满足`B2 % A2 ==0`,`B3 % A3 ==0`.

#### 广播

当`a: [K, N, A2, A3]`,`b: [K, M, B2, B3]`,`dst: [N, M, B2, B3]`，其中必须满足`B2 % A2 ==0`,`B3 % A3 ==0`.此时`a`的第2维和第3维会被广播到`b`的第2维和第3维.

例如

```text
k:  [D, n_kv, 2, 1]
q:  [D, n_q,  4, 1]

kq[:, :, 0, :] = k[:, :, 0, :]^T  @  q[:, :, 0, :]   // Q0 用 K0
kq[:, :, 1, :] = k[:, :, 0, :]^T  @  q[:, :, 1, :]   // Q1 也用 K0
kq[:, :, 2, :] = k[:, :, 1, :]^T  @  q[:, :, 2, :]   // Q2 用 K1
kq[:, :, 3, :] = k[:, :, 1, :]^T  @  q[:, :, 3, :]   // Q3 也用 K1
```

#### ggml_compute_forward_mul_mat

```CPP
const struct ggml_tensor * src0 = dst->src[0];
const struct ggml_tensor * src1 = dst->src[1];

const int32_t hint = ggml_get_op_params_i32(dst, 1);
if (hint == GGML_HINT_SRC0_IS_HADAMARD && !params->use_ref) {
    ggml_compute_forward_fwht(params, dst);
    return;
}

GGML_TENSOR_BINARY_OP_LOCALS

const int ith = params->ith;
const int nth = params->nth;

enum ggml_type           const vec_dot_type         = type_traits_cpu[src0->type].vec_dot_type;
ggml_from_float_t        const from_float           = type_traits_cpu[vec_dot_type].from_float;
int64_t                  const vec_dot_num_rows     = type_traits_cpu[src0->type].nrows;

GGML_ASSERT(ne0 == ne01);
GGML_ASSERT(ne1 == ne11);
GGML_ASSERT(ne2 == ne12);
GGML_ASSERT(ne3 == ne13);

// we don't support permuted src0 or src1
GGML_ASSERT(nb00 == ggml_type_size(src0->type));
GGML_ASSERT(nb10 == ggml_type_size(src1->type));

// dst cannot be transposed or permuted
GGML_ASSERT(nb0 == sizeof(float));
GGML_ASSERT(nb0 <= nb1);
GGML_ASSERT(nb1 <= nb2);
GGML_ASSERT(nb2 <= nb3);
```

先读取线程号`ith`和总线程数`nth`.

通过`src0`的类型，获取`src1`需要的目标类型`vec_dot_type`，将`F32`转化为`vec_dot_type`的函数`from_float`,`vec_dot_num_rows`表示一次`vec_dot`同时能算的输出数。

进行形状布局断言.确认

```text
dst: [N, M, ...] 与 a:[K,N,...], b:[K,M,...] 对齐
src0/src1 的 ne[0] 连续（未 permute）
dst 是普通 F32、未转置
```

```CPP
    if (src1->type != vec_dot_type) {
        char * wdata = params->wdata;

        const size_t nbw0 = ggml_type_size(vec_dot_type);
        const size_t nbw1 = ggml_row_size(vec_dot_type, ne10);
        const size_t nbw2 = nbw1*ne11;
        const size_t nbw3 = nbw2*ne12;

        assert(params->wsize >= ne13*nbw3);
        GGML_ASSERT(src1->type == GGML_TYPE_F32);

    #if 0
        for (int64_t i13 = 0; i13 < ne13; ++i13) {
            for (int64_t i12 = 0; i12 < ne12; ++i12) {
                for (int64_t i11 = ith; i11 < ne11; i11 += nth) {
                    from_float((float *)((char *) src1->data + i13*nb13 + i12*nb12 + i11*nb11),
                               (void *)               (wdata + i13*nbw3 + i12*nbw2 + i11*nbw1),
                                ne10);
                }
            }
        }
    #else
        for (int64_t i13 = 0; i13 < ne13; ++i13) {
            for (int64_t i12 = 0; i12 < ne12; ++i12) {
                for (int64_t i11 = 0; i11 < ne11; ++i11) {
                    size_t bs = ggml_blck_size(vec_dot_type);
                    int64_t ne10_block_start = (ith * ne10/bs) / nth;
                    int64_t ne10_block_end   = ((ith + 1) * ne10/bs) / nth;
                    from_float((float *)((char *) src1->data + i13*nb13 + i12*nb12 + i11*nb11 + ne10_block_start*bs*nb10),
                               (void *)               (wdata + i13*nbw3 + i12*nbw2 + i11*nbw1 + ne10_block_start*nbw0),
                               (ne10_block_end - ne10_block_start) * bs);
                }
            }
        }
    #endif
    }

    if (ith == 0) {
        // Every thread starts at ith, so the first unprocessed chunk is nth.  This save a bit of coordination right at the start.
        atomic_store_explicit(&params->threadpool->current_chunk, nth, memory_order_relaxed);
    }

    ggml_barrier(params->threadpool);
```

将`src1`转化为`vec_dot_type`类型.进行多线程优化,当前线程只转`src1[k_start : k_end, i11, i12, i13]`这一段

```CPP
// This is the size of the first dimension of the result, so we can iterate that way. (see the ASSERT above, these are the same numbers)
const int64_t nr0 = ne0;

// This is the size of the rest of the dimensions of the result
const int64_t nr1 = ne1 * ne2 * ne3;

// Now select a reasonable chunk size.
int chunk_size = 16;

// We need to step up the size if it's small
if (nr0 == 1 || nr1 == 1) {
    chunk_size = 64;
}

// distribute the work across the inner or outer loop based on which one is larger
// The number of chunks in the 0/1 dim.
// CEIL(nr0/chunk_size)
int64_t nchunk0 = (nr0 + chunk_size - 1) / chunk_size;
int64_t nchunk1 = (nr1 + chunk_size - 1) / chunk_size;

// If the chunking is poor for the number of threads on this setup, scrap the whole plan.  Re-chunk it by thread.
//   Also, chunking by thread was measured to have perform better on NUMA systems.  See https://github.com/ggml-org/llama.cpp/pull/6915
//   In theory, chunking should be just as useful on NUMA and non NUMA systems, but testing disagreed with that.
if (nchunk0 * nchunk1 < nth * 4 || ggml_is_numa()) {
    // distribute the thread work across the inner or outer loop based on which one is larger
    nchunk0 = nr0 > nr1 ? nth : 1; // parallelize by src0 rows
    nchunk1 = nr0 > nr1 ? 1 : nth; // parallelize by src1 rows
}

// The number of elements in each chunk
const int64_t dr0 = (nr0 + nchunk0 - 1) / nchunk0;
const int64_t dr1 = (nr1 + nchunk1 - 1) / nchunk1;

// The first chunk comes from our thread_id, the rest will get auto-assigned.
int current_chunk = ith;
```

在实际计算前,规划如何把输出矩阵切成`chunk`，将不同chunk分配给不同线程并行计算。先将输出收束为二维，计算`nchunk0`,`nchunk1`两个维度下切分的chunk数，计算`dr0`,`dr1`每个chunk中实际跨度。

例如

```txt
nr0 = 50
chunk_size = 16
nchunk0 = ceil(50/16) = 4
dr0     = ceil(50/4)  = 13
块0: [0, 13)
块1: [13, 26)
块2: [26, 39)
块3: [39, 52) -> 实际 [39, 50)  只有 11 个
```

```CPP
// The first chunk comes from our thread_id, the rest will get auto-assigned.
int current_chunk = ith;

while (current_chunk < nchunk0 * nchunk1) {
    const int64_t ith0 = current_chunk % nchunk0;
    const int64_t ith1 = current_chunk / nchunk0;

    const int64_t ir0_start = dr0 * ith0;
    const int64_t ir0_end = MIN(ir0_start + dr0, nr0);

    const int64_t ir1_start = dr1 * ith1;
    const int64_t ir1_end = MIN(ir1_start + dr1, nr1);

    // dot kernels can handle 1 row and col at a time, but mmla kernels can process 2 rows and cols
    int64_t num_rows_per_vec_dot = vec_dot_num_rows;

    // these checks are needed to avoid crossing dim1 boundaries
    // can be optimized, but the logic would become more complicated, so keeping it like this for simplicity
    if ((nr0 % 2 != 0) || (ne11 % 2 != 0) || ((ir0_end - ir0_start) % 2 != 0) || ((ir1_end - ir1_start) % 2 != 0)) {
        num_rows_per_vec_dot = 1;
    }
    ggml_compute_forward_mul_mat_one_chunk(params, dst, src0->type, num_rows_per_vec_dot, ir0_start, ir0_end, ir1_start, ir1_end);

    if (nth >= nchunk0 * nchunk1) {
        break;
    }

    current_chunk = atomic_fetch_add_explicit(&params->threadpool->current_chunk, 1, memory_order_relaxed);
}
```

实际进行chunk矩阵乘法计算，调用`ggml_compute_forward_mul_mat_one_chunk`实际进行计算.

#### ggml_compute_forward_mul_mat_one_chunk

```CPP
static void ggml_compute_forward_mul_mat_one_chunk(
    const struct ggml_compute_params * params,
    struct ggml_tensor * dst,
    const enum ggml_type type,
    const int64_t num_rows_per_vec_dot,
    const int64_t ir0_start,
    const int64_t ir0_end,
    const int64_t ir1_start,
    const int64_t ir1_end);
```

这个函数计算输出矩阵上的一个矩形`chunk`，内部再进行`16×16 tiling`，使用`vec_dot`计算点积.同时处理广播.

输入参数

* `type`是`src0`的类型，用来选`vec_dot`.
* `num_rows_per_vec_dot`一次点积算几行.
* `ir0_start/end`输出`ne[0]=N`方向范围.
* `ir1_start/end`摊平后的`nr1`方向范围。

```CPP
    for (int64_t iir1 = ir1_start; iir1 < ir1_end; iir1 += blck_1) {
        for (int64_t iir0 = ir0_start; iir0 < ir0_end; iir0 += blck_0) {
            for (int64_t ir1 = iir1; ir1 < iir1 + blck_1 && ir1 < ir1_end; ir1 += num_rows_per_vec_dot) {
                const int64_t i13 = (ir1 / (ne12 * ne1));
                const int64_t i12 = (ir1 - i13 * ne12 * ne1) / ne1;
                const int64_t i11 = (ir1 - i13 * ne12 * ne1 - i12 * ne1);

                // broadcast src0 into src1
                const int64_t i03 = i13 / r3;
                const int64_t i02 = i12 / r2;

                const int64_t i1 = i11;
                const int64_t i2 = i12;
                const int64_t i3 = i13;

                const char * src0_row = (const char*)src0->data + (0 + i02 * nb02 + i03 * nb03);

                // desc: when src1 is not a contiguous memory block we have to calculate the offset using the strides
                //       if it is, then we have either copied the data to params->wdata and made it contiguous or we are using
                //       the original src1 data pointer, so we should index using the indices directly
                // TODO: this is a bit of a hack, we should probably have a better way to handle this
                const char * src1_col = (const char*)wdata +
                    (src1_cont || src1->type != vec_dot_type
                        ? (i11 + i12 * ne11 + i13 * ne12 * ne11) * row_size
                        : (i11 * nb11 + i12 * nb12 + i13 * nb13));
                float * dst_col = (float*)((char*)dst->data + (i1 * nb1 + i2 * nb2 + i3 * nb3));

                //for (int64_t ir0 = iir0; ir0 < iir0 + blck_0 && ir0 < ir0_end; ++ir0) {
                //    vec_dot(ne00, &dst_col[ir0], src0_row + ir0*nb01, src1_col);
                //}

                for (int64_t ir0 = iir0; ir0 < iir0 + blck_0 && ir0 < ir0_end; ir0 += num_rows_per_vec_dot) {
                    vec_dot(ne00, &tmp[ir0 - iir0], (num_rows_per_vec_dot > 1 ? 16 : 0), src0_row + ir0 * nb01, (num_rows_per_vec_dot > 1 ? nb01 : 0), src1_col, (num_rows_per_vec_dot > 1 ? src1_col_stride : 0), num_rows_per_vec_dot);
                }

                for (int cn = 0; cn < num_rows_per_vec_dot; ++cn) {
                    memcpy(&dst_col[iir0 + cn * nb1 / nb0], tmp + (cn * 16), (MIN(iir0 + blck_0, ir0_end) - iir0) * sizeof(float));
                }
            }
        }
    }
```

实际调用`vec_dot`计算矩阵乘法.

### `GGML_OP_FLASH_ATTN_EXT`

```CPP
struct ggml_tensor * ggml_flash_attn_ext(
        struct ggml_context * ctx,
        struct ggml_tensor  * q,
        struct ggml_tensor  * k,
        struct ggml_tensor  * v,
        struct ggml_tensor  * mask,
        float                 scale,
        float                 max_bias,
        float                 logit_softcap)
```

`GGML_OP_FLASH_ATTN_EXT`进行`flash attention`的实现.它将`Q/K/V`和`mask`融合计算注意力，不物化完整的`KQ`矩阵：

$$
\operatorname{Attn}(Q,K,V)=\operatorname{softmax}(\operatorname{scale} \cdot QK^T+ \operatorname{mask})V
$$

输入布局：

* `q`：`[head_dim, n_seq_tokens, n_head, n_stream]`
* `k`：`[head_dim, n_kv, n_head_kv, n_stream]`
* `v`：`[head_dim, n_kv, n_head_kv, n_stream]`.
* `mask`：可选，必须是连续的`F16`，维度`[n_kv, n_seq_tokens, 1, n_stream]`
* `scale`：注意力缩放，通常`1/\sqrt{head_dim}`
* `max_bias`：`ALiBi`斜率上界，`>0`时必须提供`mask`
* `logit_softcap`：对`logits`做`tanh`软截断，`0`表示关闭

输出`res`：`[head_dim, n_head, n_seq_tokens, n_stream]`

约束：

* `q.ne[0] == k.ne[0]`，`q.ne[3] == k.ne[3] == v.ne[3]`
* `GQA`广播：`n_head % n_head_kv == 0`，`n_head % mask.ne[2] == 0`，`n_stream % mask.ne[3] == 0`

调用后还可以：

* `ggml_flash_attn_ext_set_prec`：设置累加精度，`llama.cpp`里通常设为`GGML_PREC_F32`
* `ggml_flash_attn_ext_add_sinks`：附加`Attention Sink`，长度等于`n_head`

#### ggml_compute_forward_flash_attn_ext_f16

这个函数是`CPU Flash Attention`的调度入口.按照输入的`qkv`矩阵的维度选择使用什么切分方法.对于`Prefill`阶段，按照`Q`的行切分,对于单个Token的Decode阶段，按照`KV`切分.

```CPP
// When use_ref is set, force the vec-only reference implementation (no tiling, no KV-chunking)
const bool use_ref = params->use_ref;

const bool kv_is_f32_or_f16 = (k->type == GGML_TYPE_F32 || k->type == GGML_TYPE_F16);
const bool use_split_kv_path = !use_ref && (neq1 == 1 && neq3 == 1) && kv_is_f32_or_f16 && (k->type == v->type) && q->type == GGML_TYPE_F32 && nek1 >= 512;
```

* `use_ref`测试开关，用来强制使用vec实现，不走`tiling`和`kv-chunking`.
* `use_split_kv_path`,如果是单解码情况，且没有进行kv的量化，同时是长解码时，使用`split-kv`分支

```CPP
if (use_split_kv_path) {
    const int64_t chunk_size = (nek1 + nth - 1) / nth;

    // Partials buffer layout: [q_head][kv_chunk][M, S, VKQ]
    const int64_t partial_size  = 2 + DV;
    float *       partials_base = (float *) params->wdata + nth * (DK + 2*DV + CACHE_LINE_SIZE_F32);

    const int64_t ic_start = ith * chunk_size;
    const int64_t ic_end   = std::min(ic_start + chunk_size, nek1);

    const int64_t partial_stride = nth * partial_size;
    float *       chunk_partials = partials_base + ith * partial_size;

    if (ic_start < nek1) {
        for (int64_t q_head = 0; q_head < neq2; q_head++) {
            ggml_compute_forward_flash_attn_ext_f16_one_chunk(
                params, dst, q_head, q_head + 1, ic_start, ic_end,
                chunk_partials, partial_stride);
        }
    } else {
        for (int64_t q_head = 0; q_head < neq2; q_head++) {
            float * q_partials = chunk_partials + q_head * partial_stride;
            q_partials[0] = -INFINITY;  // M
            q_partials[1] = 0.0f;       // S
        }
    }

    ggml_barrier(params->threadpool);
    ggml_flash_attn_ext_reduce_partials(params, dst, nth, chunk_size);
}
```

这是长`decode`的`split-KV`：把`n_kv`切成`nth`段，每线程先算自己那段的`(M, S, O)`，`barrier`后再合成。还不写最终`dst`

首先先按照`kv`进行划分，`chunk_size = ceil(n_kv / nth)`,

给每个线程保留`DK + 2*DV + CACHE_LINE_SIZE_F32`的工作内存，用来存储:

1. 还未能归一化的O，长度为DV(head_dim)
2. 转成F32的O，长度为DV(head_dim)
3. 类型转换后的Q向量，长度为DK(head_dim).

之后给每个线程分配`partial`用来存储自己那段的`(M, S, O)`的内存，长度是`2 + head_dim`

之后循环每个head，调用`ggml_compute_forward_flash_attn_ext_f16_one_chunk`计算注意力.

```CPP
// total rows in q
const int64_t nr = neq1*neq2*neq3;

// disable for NUMA
const bool disable_chunking = ggml_is_numa();

// 4x chunks per thread
int nth_scaled = nth * 4;
int64_t chunk_size = (nr + nth_scaled - 1) / nth_scaled;
int64_t nchunk     = (nr + chunk_size - 1) / chunk_size;

if (nth == 1 || nchunk < nth || disable_chunking) {
    nchunk = nth;
}

if (ith == 0) {
    ggml_threadpool_chunk_set(params->threadpool, nth);
}

ggml_barrier(params->threadpool);

const int64_t dr = (nr + nchunk - 1) / nchunk;

static constexpr int64_t Q_TILE_SZ  = ggml_fa_tile_config::Q;
bool use_tiled = !use_ref &&
                        (q->type == GGML_TYPE_F32 &&
                        kv_is_f32_or_f16 &&
                        k->type == v->type &&
                        neq1 >= Q_TILE_SZ);
#ifdef GGML_SIMD
#if defined(__ARM_FEATURE_SVE)
const int64_t f32_epr = svcntw();
#else
const int64_t f32_epr = GGML_F32_EPR;
#endif
use_tiled &= (DV % f32_epr == 0);
#endif
int current_chunk = ith;

while (current_chunk < nchunk) {
    const int64_t ir0 = dr * current_chunk;
    const int64_t ir1 = MIN(ir0 + dr, nr);

    if (use_tiled) {
        ggml_compute_forward_flash_attn_ext_tiled(params, dst, ir0, ir1);
    } else {
        ggml_compute_forward_flash_attn_ext_f16_one_chunk(params, dst, ir0, ir1, 0, nek1, nullptr, 0);
    }

    current_chunk = ggml_threadpool_chunk_add(params->threadpool, 1);
}
```

这是在Prefill阶段或者是短Decode阶段走的分支，它还会按照当前有多少token决定是实际使用tile还是chunk。

首先计算q中的行数`nr`

再计算有多少块,块数大约是线程数的`4`倍.

* `chunk_size = ceil(nr / (4*nth))`,`chunk`大小.
* `nchunk = ceil(nr / chunk_size)`,`chunk`个数
* `dr=ceil(nr/nchunk)`,每一块负责的Q行数.

多线程处理`chunk`或`tiled`

#### ggml_compute_forward_flash_attn_ext_f16_one_chunk

```CPP
static void ggml_compute_forward_flash_attn_ext_f16_one_chunk(
        const ggml_compute_params * params,
        ggml_tensor * dst,
        int ir0, int ir1,
        int64_t ic_start, int64_t ic_end,
        float * partials, int64_t partial_stride)
```

`ggml_compute_forward_flash_attn_ext_f16_one_chunk`函数实际进行Flash Attention.取决于参数决定是计算一个二维子块还是只计算一段KV.

```text
Q 行范围 [ir0, ir1) x KV 范围 [ic_start, ic_end)
```

* `[ir0,ir1)`:当前线程处理的Q行范围.
* `[ic_start, ic_end)`:当前线程处理的KV token范围.
* `partials != nullptr`:只输出当前`KV`分块的中间结果，稍后与其他线程归并.
* `partials == nullptr`: 直接归一化并写入 dst.

此时张量布局

* `q`：`[head_dim, n_seq_tokens, n_head, n_stream]`
* `k`：`[head_dim, n_kv, n_head_kv, n_stream]`
* `v`：`[head_dim, n_kv, n_head_kv, n_stream]`.
* `dst`:`[head_dim, n_head, n_seq_tokens, n_stream]`

```CPP
// broadcast factors
const int64_t rk2 = neq2/nek2;
const int64_t rk3 = neq3/nek3;

const int64_t rv2 = neq2/nev2;
const int64_t rv3 = neq3/nev3;
```

计算比值，实现GQA的广播.

例如

```text
n_head = 32
n_head_kv = 8
rk2 = rv2 = 32 / 8 = 4
```

```CPP
// parallelize by q rows using ggml_vec_dot_f32

float scale         = 1.0f;
float max_bias      = 0.0f;
float logit_softcap = 0.0f;

memcpy(&scale,         (float *) dst->op_params + 0, sizeof(float));
memcpy(&max_bias,      (float *) dst->op_params + 1, sizeof(float));
memcpy(&logit_softcap, (float *) dst->op_params + 2, sizeof(float));

if (logit_softcap != 0) {
    scale /= logit_softcap;
}
const uint32_t n_head      = neq2;
const uint32_t n_head_log2 = 1u << (uint32_t) floor(log2(n_head));

const float m0 = powf(2.0f, -(max_bias       ) / n_head_log2);
const float m1 = powf(2.0f, -(max_bias / 2.0f) / n_head_log2);
```

读取参数.

```CPP
ggml_type         const k_vec_dot_type = ggml_get_type_traits_cpu(k->type)->vec_dot_type;
ggml_from_float_t const q_to_vec_dot   = ggml_get_type_traits_cpu(k_vec_dot_type)->from_float;
ggml_vec_dot_t    const kq_vec_dot     = ggml_get_type_traits_cpu(k->type)->vec_dot;
ggml_to_float_t   const v_to_float     = ggml_get_type_traits(v->type)->to_float;

GGML_ASSERT((                            q_to_vec_dot) && "fattn: unsupported K-type");
GGML_ASSERT((v->type == GGML_TYPE_F32 || v_to_float  ) && "fattn: unsupported V-type");
```

这里使用 GGML 的类型 traits：

* 根据`K`类型选择点积函数
* 把`F32 Q`转成`K`点积所要求的格式
* 如果`V`是量化类型，将其转换成`F32`后累计

```CPP
for (int ir = ir0; ir < ir1; ++ir) {
    // q indices
    const int iq3 = ir/(neq2*neq1);
    const int iq2 = (ir - iq3*neq2*neq1)/neq1;
    const int iq1 = (ir - iq3*neq2*neq1 - iq2*neq1);

    const uint32_t h = iq2; // head index
    const float slope = (max_bias > 0.0f) ? h < n_head_log2 ? powf(m0, h + 1) : powf(m1, 2*(h - n_head_log2) + 1) : 1.0f;

    float S = 0.0f;      // sum
    float M = -INFINITY; // maximum KQ value

    float       * VKQ32 = (float       *) params->wdata + ith*(1*DK + 2*DV + CACHE_LINE_SIZE_F32); // FP32 VKQ accumulator
    float       * V32   =                 (VKQ32 + 1*DV); // (temporary) FP32 V buffer
    ggml_fp16_t * VKQ16 = (ggml_fp16_t *) (VKQ32 + 1*DV); // (temporary) FP16 VKQ accumulator
    ggml_fp16_t * Q_q   = (ggml_fp16_t *) (VKQ32 + 2*DV); // (temporary) buffer for Q converted to quantized/FP16

    if (v->type == GGML_TYPE_F16) {
        memset(VKQ16, 0, DV*sizeof(ggml_fp16_t));
    } else {
        memset(VKQ32, 0, DV*sizeof(float));
    }

    const ggml_fp16_t * mp = mask ? (ggml_fp16_t *)((char *) mask->data + iq1*mask->nb[1] + (iq2%mask->ne[2])*mask->nb[2] + (iq3%mask->ne[3])*mask->nb[3]) : NULL;

    // k indices
    const int ik3 = iq3 / rk3;
    const int ik2 = iq2 / rk2;

    // v indices
    const int iv3 = iq3 / rv3;
    const int iv2 = iq2 / rv2;

    const float * pq = (const float *) ((char *) q->data + (iq1*nbq1 + iq2*nbq2 + iq3*nbq3));
    q_to_vec_dot(pq, Q_q, DK);
}
```

这是外循环，沿着Q的行遍历。

将线性行号`ir`还原为`q`坐标。

* `iq1 = ir % N`                 // query token
* `iq2 = (ir / N) % Hq`          // query head
* `iq3 = ir / (N * Hq)`         // batch/stream

初始化`online softmax`状态.`M`和`S`.

获取线程私有工作区

```text
[VKQ32: head_dim floats]
[V32:   head_dim floats]  或 [VKQ16: head_dim fp16]
[Q_q:   head_dim 转换缓冲区]
```

获取`mask`,`mask`的维度是`[n_kv, n_seq_tokens, n_head_h, n_stream_h]`，后两个维度不同的原因是，`mask`支持广播.代码中使用了`%`进行广播.

获取`k`和`q`的后两维的index，使用了广播.

最后填充`Q_q`转换为点积需要的类型.

```CPP
for (int64_t ic = ic_start; ic < ic_end; ++ic) {
    ...
}
```

进行`online softmax`.针对一个固定的`Q`行，扫`KV`区间`[ic_start, ic_end)`，使用`online softmax`累加`attention`输出，不生成完整的`QK`矩阵.

固定的q行就是`Q[:, iq1, iq2, iq3]`.它会和`k`的`[：，ic_start : ic_end, ik2, ik3]`,`v`的`[:, ic_start : ic_end, iv2, iv3]`.交互

```CPP
const float mv = mp ? slope*GGML_CPU_FP16_TO_FP32(mp[ic]) : 0.0f;
if (mv == -INFINITY) {
    continue;
}

float s; // KQ value

const char * k_data = (const char *) k->data + ( ic*nbk1 + ik2*nbk2 + ik3*nbk3);
kq_vec_dot(DK, &s, 0, k_data, 0, Q_q, 0, 1);

s = s*scale; // scale KQ value
```

获取`mask`，`mv`如果是`-INF`直接跳过即可.

计算`kq`,`q: [:, iq1, iq2, iq3]`和`k: [:, ic, ik2, ik3]`进行点积乘法.获得对`ic`位置的`token`的`query`.并进行`scale`.

```CPP
if (logit_softcap != 0.0f) {
    s = logit_softcap*tanhf(s);
}

s += mv; // apply mask

const float Mold = M;

float ms = 1.0f; // upon new higher max val, scale VKQ and KQ sum with this value
float vs = 1.0f; // post-softmax KQ value, expf(s - M)

const char * v_data = ((const char *) v->data + (ic*nbv1 + iv2*nbv2 + iv3*nbv3));
```

将`query`应用`mask`.将当前最大值保存在`Mold`，`ms`是对`S`的缩放系数，如果有更大的值出现，那么就要用这个值缩放.`vs`是对`V`的缩放系数。获取对应的`v: [:, ic, iv2, iv3]`.

```CPP
if (v->type == GGML_TYPE_F16) {
    if (s > M) {
        // s is new maximum, ms < 1.0f, vs == expf(s - s) == 1.0f
        M = s;
        ms = expf(Mold - M);

        // V = V*expf(Mold - M)
        ggml_vec_scale_f16(DV, VKQ16, ms);
    } else {
        // no new maximum, ms == 1.0f, vs != 1.0f
        vs = expf(s - M);
    }

    // V += v*expf(s - M)
    ggml_vec_mad_f16(DV, VKQ16, (const ggml_fp16_t *) v_data, vs);
} 
```

这是当`v`的类型是`F16`时走的路径，先看当前计算出来的`s`是否比最大值`M`要大.如果大于，就说明这个是新的最大值，需要保存最大值至`M`，同时计算`ms`，将之前计算出来的注意力分数缩放`ms`;如果不大于，那就需要缩放计算`vs`，大于的时候最大值就是其本身，所以不用缩放`vs`.最后累加至`VKQ16`.

```CPP
for (int64_t ic = ic_start; ic < ic_end; ++ic) {
...
    if (v->type == GGML_TYPE_F16) {
    ...
    } else {
        if (s > M) {
            // s is new maximum, ms < 1.0f, vs == expf(s - s) == 1.0f
            M = s;
            ms = expf(Mold - M);

            // V = V*expf(Mold - M)
            ggml_vec_scale_f32(DV, VKQ32, ms);
        } else {
            // no new maximum, ms == 1.0f, vs != 1.0f
            vs = expf(s - M);
        }

        // V += v*expf(s - M)
        if (v_to_float) {
            v_to_float(v_data, V32, DV);
            ggml_vec_mad_f32(DV, VKQ32, V32, vs);
        } else {
            // V is F32
            ggml_vec_mad_f32(DV, VKQ32, (const float *) v_data, vs);
        }
    }

    S = S*ms + vs; // scale and increment sum with partial sum
}
```

这是当`v`类型不是`F16`时走的路径，类似于`F16`的路径，也是判断`s`和`M`的大小.将`v`转换为`float`,再使用`ggml_vec_mad_f32`.累加至`VKQ16`.

同时累加分母`S`.

```CPP
if (v->type == GGML_TYPE_F16) {
    for (int64_t d = 0; d < DV; ++d) {
        VKQ32[d] = GGML_CPU_FP16_TO_FP32(VKQ16[d]);
    }
}
```

将`v`转为`F16`.

```CPP
// sinks - apply only on the first kv-chunk
if (sinks && ic_start == 0) {
    const float s = ((float *)((char *) sinks->data))[h];

    float ms = 1.0f;
    float vs = 1.0f;

    if (s > M) {
        ms = expf(M - s);
        M = s;
        ggml_vec_scale_f32(DV, VKQ32, ms);
    } else {
        vs = expf(s - M);
    }

    S = S*ms + vs;
}
```

这个是`attention sink`的操作，特定模型会使用.

```CPP
if (write_partials) {
    // Write M, S, VKQ to partials for later reduction
    // partials layout: [M, S, VKQ[DV]] per query head
    float * partial = partials + ir * partial_stride;
    partial[0] = M;
    partial[1] = S;
    memcpy(partial + 2, VKQ32, DV * sizeof(float));
} 
```

这个是将`n_kv`给分成多段的时候的`Flash Decoding`.

写入`partial`,`[M | S | VKQ32(head_dim)]`.

```CPP
if (write_partials) {
...
} else {
    // V /= S
    const float S_inv = S == 0.0f ? 0.0f : 1.0f/S;
    ggml_vec_scale_f32(DV, VKQ32, S_inv);

    // dst indices
    const int i1 = iq1;
    const int i2 = iq2;
    const int i3 = iq3;

    // permute(0, 2, 1, 3)
    memcpy((char *) dst->data + (i3*ne2*ne1 + i2 + i1*ne1)*nb1, VKQ32, nb1);
}
```

首先对`VKQ32`除去`S`.

拷贝至输出，`dst`并重新排列.`dst`的维度是`[head_dim, n_head, n_seq_tokens, n_stream]`.所以需要拷贝到正确的维度上，也就是`[:, iq2, iq1, iq3]`.

#### `ggml_flash_attn_ext_reduce_partials`

```CPP
static void ggml_flash_attn_ext_reduce_partials(
        const ggml_compute_params * params,
        ggml_tensor * dst,
        const int64_t n_chunks,
        const int64_t chunk_size)
```

这个函数是`Flash Decoding`的第二阶段.多线程按照head并行,将数据写入最后dst.

* `params`当前线程的计算上下文
* `dst`,输出向量.维度是`[head_dim, n_head, n_seq_tokens, n_stream]`.
* `n_chunks`,KV的分段数.
* `chunk_size`,一个chunk的长度.

```CPP
const ggml_tensor * q = dst->src[0];
const ggml_tensor * k = dst->src[1];
const ggml_tensor * v = dst->src[2];

const int64_t DK        = k->ne[0];
const int64_t DV        = v->ne[0];
const int64_t nek1      = k->ne[1];
const int64_t n_q_heads = q->ne[2];

const int ith = params->ith;
const int nth = params->nth;
const int64_t wdata_per_thread = DK + 2*DV + CACHE_LINE_SIZE_F32;
float *       thread_wdata     = (float *) params->wdata + ith * wdata_per_thread;

const int64_t partials_offset  = nth * (DK + 2*DV + CACHE_LINE_SIZE_F32);
const int64_t partial_size     = 2 + DV;
const float * partials_base    = (const float *) params->wdata + partials_offset;

// Output layout
const int64_t ne1 = dst->ne[1];
const int64_t ne2 = dst->ne[2];
const size_t  nb1 = dst->nb[1];
```

读取相关数据`wdata`的布局是`[ 每线程工作区 × nth ][ partials ]`

* `q`：`[head_dim, n_seq_tokens, n_head, n_stream]`.
* `k`：`[head_dim, n_kv, n_head_kv, n_stream]`.
* `v`：`[head_dim, n_kv, n_head_kv, n_stream]`.
* `wdata_per_thread`是每个线程在`wdata`中的私有数据大小.
* `thread_wdata`当前线程私有数据位置.

```CPP
// Each thread reduces a subset of query heads
for (int64_t q_head = ith; q_head < n_q_heads; q_head += nth) {
    float   M_final   = -INFINITY;
    float   S_final   = 0.0f;
    float * VKQ_final = thread_wdata;
    memset(VKQ_final, 0, DV * sizeof(float));

    // Combine partials from all chunks
    for (int64_t chunk_idx = 0; chunk_idx < n_chunks; ++chunk_idx) {
        const int64_t ic_start = chunk_idx * chunk_size;
        if (ic_start >= nek1) continue;

        const float * partial   = partials_base + (q_head * n_chunks + chunk_idx) * partial_size;
        const float   M_chunk   = partial[0];
        const float   S_chunk   = partial[1];
        const float * VKQ_chunk = partial + 2;

        if (S_chunk == 0.0f) continue;

        const float M_new     = fmaxf(M_final, M_chunk);
        const float scale_old = expf(M_final - M_new);
        const float scale_new = expf(M_chunk - M_new);

        for (int64_t d = 0; d < DV; ++d) {
            VKQ_final[d] = VKQ_final[d] * scale_old + VKQ_chunk[d] * scale_new;
        }
        S_final = S_final * scale_old + S_chunk * scale_new;
        M_final = M_new;
    }

    // Normalize and write to output
    if (S_final != 0.0f) {
        const float S_inv = 1.0f / S_final;
        ggml_vec_scale_f32(DV, VKQ_final, S_inv);
    }
    // iq1=0, iq3=0 for decode
    memcpy((char *) dst->data + (0*ne2*ne1 + q_head + 0*ne1)*nb1, VKQ_final, nb1);
}
```

沿着`q`的`head`进行多线程处理.一个循环写入`dst: [:, q_head, n_seq_tokens, n_streams]`.

首先清空`VKQ_final`

对某个`head`,第`chunk_idx`段存的是一段`kv`的结果.

$$
M_c \\
S_c = \sum_{j \in c} \exp(s_j - M_c) \\
o_c = \sum_{j \in c} \exp(s_j - M_c)v_j
$$

合并逻辑是

$$
M \leftarrow \max (M_a, M_b) \\
o \leftarrow \exp(M_a - M)o_a + \exp(Mb - M)o_b \\
S \leftarrow \exp(M_a - M)S_a + \exp(Mb - M)S_b
$$

#### `ggml_compute_forward_flash_attn_ext_f16_one_chunk`

```CPP
static void ggml_compute_forward_flash_attn_ext_tiled(
        const ggml_compute_params * params,
        ggml_tensor * dst,
        int ir0, int ir1)
```

这个函数是`CPU`上的`tiled flash attention`。

* `params`线程上下文.
* `dst`输出向量.维度是`[head_dim, n_head, n_seq_tokens, n_stream]`.
* `[ir0,ir1)`当前函数需要处理的Q行区间.

```CPP
const ggml_tensor * q     = dst->src[0];
const ggml_tensor * k     = dst->src[1];
const ggml_tensor * v     = dst->src[2];
const ggml_tensor * mask  = dst->src[3];
const ggml_tensor * sinks = dst->src[4];

GGML_TENSOR_LOCALS(int64_t, neq, q,   ne)
GGML_TENSOR_LOCALS(size_t,  nbq, q,   nb)
GGML_TENSOR_LOCALS(int64_t, nek, k,   ne)
GGML_TENSOR_LOCALS(size_t,  nbk, k,   nb)
GGML_TENSOR_LOCALS(int64_t, nev, v,   ne)
GGML_TENSOR_LOCALS(size_t,  nbv, v,   nb)
GGML_TENSOR_LOCALS(int64_t, ne,  dst, ne)
GGML_TENSOR_LOCALS(size_t,  nb,  dst, nb)

const int64_t DK = nek0;
const int64_t DV = nev0;
const int64_t N  = neq1;

GGML_ASSERT(ne0 == DV);
GGML_ASSERT(ne2 == N);

// input tensor rows must be contiguous
GGML_ASSERT(nbq0 == ggml_type_size(q->type));
GGML_ASSERT(nbk0 == ggml_type_size(k->type));
GGML_ASSERT(nbv0 == ggml_type_size(v->type));

GGML_ASSERT(neq0 == DK);
GGML_ASSERT(nek0 == DK);
GGML_ASSERT(nev0 == DV);

GGML_ASSERT(neq1 == N);

// dst cannot be transposed or permuted
GGML_ASSERT(nb0 == sizeof(float));
GGML_ASSERT(nb0 <= nb1);
GGML_ASSERT(nb1 <= nb2);
GGML_ASSERT(nb2 <= nb3);

GGML_ASSERT(k->type == v->type);
const ggml_type kv_type = k->type;
```

* `q`：`[head_dim, n_seq_tokens, n_head, n_stream]`.
* `k`：`[head_dim, n_kv, n_head_kv, n_stream]`.
* `v`：`[head_dim, n_kv, n_head_kv, n_stream]`.
* `mask`:`[n_kv, n_seq_tokens, n_head_h, n_stream_h]`

进行一系列判断.

```CPP
// broadcast factors
const int64_t rk2 = neq2/nek2;
const int64_t rk3 = neq3/nek3;

const int64_t rv2 = neq2/nev2;
const int64_t rv3 = neq3/nev3;
```

计算比值，实现GQA的广播.

例如

```text
n_head = 32
n_head_kv = 8
rk2 = rv2 = 32 / 8 = 4
```

```CPP
float scale         = 1.0f;
float max_bias      = 0.0f;
float logit_softcap = 0.0f;

memcpy(&scale,         (float *) dst->op_params + 0, sizeof(float));
memcpy(&max_bias,      (float *) dst->op_params + 1, sizeof(float));
memcpy(&logit_softcap, (float *) dst->op_params + 2, sizeof(float));

if (logit_softcap != 0) {
    scale /= logit_softcap;
}
```

读取注意力计算的参数.

```CPP
int ith = params->ith;

static constexpr int Q_TILE_SZ  = ggml_fa_tile_config::Q;
static constexpr int KV_TILE_SZ = ggml_fa_tile_config::KV;
```

获取当前线程号和`Q_TILE_SZ = 64`,`KV_TILE_SZ = 64`.

```CPP
int ir = ir0;
while (ir < ir1) {
    ...
}
```

这是`tiled flash attention`的外层循环，一次处理一块Q.

```CPP
while(ir < ir1){
    // q indices for the start of this tile
    const int iq3 = ir/(neq2*neq1);
    const int iq2 = (ir - iq3*neq2*neq1)/neq1;
    const int iq1 = (ir - iq3*neq2*neq1 - iq2*neq1);

    // Number of valid rows in this tile:
    // - limited by tile size (Q_TILE_SZ)
    // - limited by chunk boundary (ir1 - ir)
    // - limited by head boundary (neq1 - iq1) to avoid crossing into next head
    const int tile_rows = MIN(Q_TILE_SZ, MIN((int)(ir1 - ir), (int)(neq1 - iq1)));
    GGML_ASSERT(tile_rows > 0);

    const uint32_t h = iq2; // head index
    const float slope = (max_bias > 0.0f) ? h < n_head_log2 ? powf(m0, h + 1) : powf(m1, 2*(h - n_head_log2) + 1) : 1.0f;

    float S[Q_TILE_SZ];
    float M[Q_TILE_SZ];

    for (int i = 0 ; i < Q_TILE_SZ; ++i) {
        S[i] = 0.;
        M[i] = -INFINITY;
    }
    ...
}
```

将展平的q行还原，就是`q:[:, iq1, iq2, iq3]`.

计算`tile_rows`,是满`64`行、不超出本`chunk`、不跨`head`。

生成为每一个tile生成`S`和`M`。

```CPP
while(ir < ir1){
...
    // Per-thread scratch layout:
    // Q_q:    Q_TILE_SZ * DK (converted Q tile — F32 for GEMM, KV type for scalar)
    // KQ:     Q_TILE_SZ * KV_TILE_SZ (attention scores in float)
    // mask:   Q_TILE_SZ * KV_TILE_SZ (mask in float)
    // VKQ32:  Q_TILE_SZ * DV (FP32 output accumulator)
    // V32:    KV_TILE_SZ * DV (F32 buffer for V tile)
    // K_f32:  KV_TILE_SZ * DK (F32 buffer for K tile — GEMM path)
    float * base  = (float *) params->wdata + ith*(Q_TILE_SZ*DK + 2*Q_TILE_SZ*KV_TILE_SZ + Q_TILE_SZ*DV + KV_TILE_SZ*DV + KV_TILE_SZ*DK + CACHE_LINE_SIZE_F32);

    void  * Q_q    = base;
    float * KQ     = (float *)((char *)base + Q_TILE_SZ * DK * sizeof(float));
    float * mask32 = KQ + Q_TILE_SZ * KV_TILE_SZ;
    float * VKQ32  = mask32 + Q_TILE_SZ * KV_TILE_SZ;
    float * V32    = VKQ32 + Q_TILE_SZ * DV;
    float * K_f32  = V32 + KV_TILE_SZ * DV;

    memset(VKQ32, 0, Q_TILE_SZ * DV * sizeof(float));
    memset(mask32, 0, Q_TILE_SZ * KV_TILE_SZ * sizeof(float));

    // k indices
    const int ik3 = iq3 / rk3;
    const int ik2 = iq2 / rk2;

    // v indices
    const int iv3 = iq3 / rv3;
    const int iv2 = iq2 / rv2;

    {
        float * Q_f32 = (float *)Q_q;
        for (int tq = 0; tq < tile_rows; tq++) {
            const float * pq = (const float *) ((char *) q->data + ((iq1 + tq)*nbq1 + iq2*nbq2 + iq3*nbq3));
            memcpy(Q_f32 + tq * DK, pq, DK * sizeof(float));
        }
        for (int tq = tile_rows; tq < Q_TILE_SZ; tq++) {
            memset(Q_f32 + tq * DK, 0, DK * sizeof(float));
        }
    }

    memset(K_f32, 0, DK * KV_TILE_SZ * sizeof(float));
    memset(V32,   0, KV_TILE_SZ * DV * sizeof(float));
...
}
```

获取线程私有工作区.

* `Q_q`维度是`[Q_TILE_SZ, head_dim]`当前`Q tile`
* `KQ`维度是`[Q_TILE_SZ, KV_TILE_SZ]`,`scores`，`softmax`后变权重
* `mask`维度是`[Q_TILE_SZ, KV_TILE_SZ]`,F32 mask
* `VKQ32`维度是`[Q_TILE_SZ, head_dim]`,输出累加器
* `V32`维度是`[KV_TILE_SZ, head_dim]`,当前`V tile`
* `K_f32`维度是`[KV_TILE_SZ, head_dim]`,当前`K tile`
