# decode

本文完整讲解`llama.cpp`的`decode`解码流程.

## llama_decode

```CPP
int32_t llama_decode(
        llama_context * ctx,
          llama_batch   batch) {
    const int ret = ctx->decode(batch);
    if (ret != 0 && ret != 1) {
        LLAMA_LOG_ERROR("%s: failed to decode, ret = %d\n", __func__, ret);
    }

    return ret;
}
```

包装层，内在调用`ctx->decode`处理解码流程.

## llama_context::decode

```CPP
int llama_context::decode(const llama_batch & batch_inp)
```

这个函数是一次`decoder`前向的调度器：把`llama_batch`切成`ubatch`，逐块建图调用`process_ubatch`，再把`logits / embedding`拷回`host`。

### 输入参数

```CPP
typedef struct llama_batch {
    int32_t n_tokens;

    llama_token  *  token;
    float        *  embd;
    llama_pos    *  pos;
    int32_t      *  n_seq_id;
    llama_seq_id ** seq_id;
    int8_t       *  logits;   // TODO: rename this to "output"
} llama_batch;
```

* `n_tokens`,本次送入模型的`token`数量。
* `token`,`token ID`数组，例如`[1, 15043, 29892, ...]`。
* `embd`,直接输入`embedding`，与`token`二选一；多模态和特殊模型会使用。
* `pos`,每个`token`在序列中的位置。
* `n_seq_id`,每个`token`属于的序列数.
* `seq_id`,具体属于哪些序列，用于并行请求.
* `logits`哪些`token`需要输出结果

### 刚进入时内存状态

#### 模型权重内存

模型加载阶段已经完成：

* `token embedding`权重
* `attention/FFN`权重
* `output head`
* `CPU/GPU`上的模型`tensor`
* `vocabulary`和`tokenizer`信息
* `decode()`不会重新加载模型权重。

#### KV Cache

在创建`llama_context`构造函数时已经创建`memory`模块,

```CPP
llama_context::llama_context
{
    ...
    // init the memory module
    if (!hparams.vocab_only) {
        llama_memory_params params_mem = {
            /*.type_k    =*/ params.type_k,
            /*.type_v    =*/ params.type_v,
            /*.swa_full  =*/ params.swa_full,
            /*.ctx_type  =*/ cparams.ctx_type,
            /*.mem_other =*/ llama_get_memory(cparams.ctx_other),
        };

        memory.reset(model.create_memory(params_mem, cparams));
    }
    ...
}
```

* `KV cache`的容量已经分配。
* 本次`token`使用的`KV slot`还未决定.

#### 临时缓冲区

在创建`llama_context`构造函数会调用`sched_reserve()`，在`decode`里面会重复调用一次，按最坏情况构造测试图并预留计算缓冲区.

* 已预留最大容量.
* 本次`batch`使用的计算图还没有建立，第一次可能需要构建，后续可能复用上一张图。

#### 输出缓冲区

```CPP
llama_context::llama_context
{
    ...
    if (output_reserve(params.n_seq_max) < params.n_seq_max) {
        throw std::runtime_error("failed to reserve initial output buffer");
    }
    LLAMA_LOG_INFO("%s: %10s  output buffer size = %8.2f MiB\n", __func__,
            ggml_backend_buffer_name    (buf_output.get()),
            ggml_backend_buffer_get_size(buf_output.get()) / 1024.0 / 1024.0);
    ...
}
```

* 初始容量已准备.
* 本次所需容量尚未确认
* 本次输出数据尚未计算

#### llama_batch

* 已有用户输入经过`tokenizer`后的结果.
* `ubatch`拆分和内部副本尚未准备
