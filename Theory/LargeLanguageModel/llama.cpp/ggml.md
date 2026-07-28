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

## 常见算子

### `GGML_OP_VIEW`

`GGML_OP_VIEW` 表示对已有张量存储的零拷贝视图。视图不拥有独立的数据存储，但可以通过自己的形状和步长描述源张量的子区域或不同布局。

视图张量的 `view_src` 指向共享存储的源张量，`view_offs` 表示相对源存储的字节偏移，`nb` 描述各维度的字节步长。执行计算图时该算子不进行数值计算，但会保留数据依赖，并为内存分配器和 backend 提供存储别名信息。
