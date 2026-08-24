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

