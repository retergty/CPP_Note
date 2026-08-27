# sampler

本文讲解当大模型输出logits时如何进行采样，变为唯一的`token`的过程.

## logits

```CPP
// LM head
cur = build_lora_mm(model.output, cur, model.output_s);
```

输出`cur: [n_vocab, n_outputs]`就是logits.每个输出位置是一个`n_vocab`的向量，需要进行采样变为长度1.

`Logit`不是概率。它是无约束实数，常见范围大约`-20 ~ +20`

## 核心数据结构

```CPP
typedef struct llama_token_data {
    llama_token id; // token id
    float logit;    // log-odds of the token
    float p;        // probability of the token
} llama_token_data;
```

`llama_token_data`表示一个`token`

* `id`当前`token`的`id`
* `logit`当前分数，后面的`sampler`会修改它
* `p`需要概率时才填

```CPP
typedef struct llama_token_data_array {
    llama_token_data * data;
    size_t size;
    int64_t selected; // this is the index in the data array (i.e. not the token id)
    bool sorted;
} llama_token_data_array;
```

* `data`目前所有候选`token`的列表
* `size`是候选`token`的列表大小
* `selected`最终选中的数组序号
* `sorted`是否已按`logit`降序。

## 采样过程

```CPP
struct llama_sampler_i {
    const char *           (*name)  (const struct llama_sampler * smpl);                                 // can be NULL
    void                   (*accept)(      struct llama_sampler * smpl, llama_token token);              // can be NULL
    void                   (*apply) (      struct llama_sampler * smpl, llama_token_data_array * cur_p); // required
    void                   (*reset) (      struct llama_sampler * smpl);                                 // can be NULL
    struct llama_sampler * (*clone) (const struct llama_sampler * smpl);                                 // can be NULL if ctx is NULL
    void                   (*free)  (      struct llama_sampler * smpl);                                 // can be NULL if ctx is NULL

    // [EXPERIMENTAL]
    // backend sampling interface:

    // return true if the backend supports all ops needed by the sampler
    // note: call once per sampler
    bool (*backend_init)(struct llama_sampler * smpl, ggml_backend_buffer_type_t buft);

    // call after .backend_apply()
    void (*backend_accept)(
            struct llama_sampler * smpl,
            struct ggml_context  * ctx,
            struct ggml_cgraph   * gf,
            struct ggml_tensor   * selected_token);

    // call after .backend_init()
    void (*backend_apply)(
            struct llama_sampler      * smpl,
            struct ggml_context       * ctx,
            struct ggml_cgraph        * gf,
            struct llama_sampler_data * data);

    // called before graph execution to set inputs for the current ubatch
    void (*backend_set_input)(struct llama_sampler * smpl);
};
```

每个采样器都是函数指针类，通过`apply`间接调用，对`llama_token_data_array`进行采样.

```CPP
// initialize the sampler
llama_sampler * smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
llama_sampler_chain_add(smpl, llama_sampler_init_min_p(0.05f, 1));
llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.8f));
llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
```

采样器通过`chain`连接.

```CPP
static void llama_sampler_chain_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    auto * chain = (llama_sampler_chain *) smpl->ctx;

    time_meas tm(chain->t_sample_us, chain->params.no_perf);

    bool is_backend = chain->is_init;

    for (auto & smpl : chain->samplers) {
        if (is_backend && smpl.is_backend) {
            continue;
        }

        is_backend = false;

        if (smpl.ptr->iface->apply == nullptr) {
            continue;
        }

        llama_sampler_apply(smpl.ptr, cur_p);
    }
}
```

顺序调用chain里面的所有采样器.

采样器分为两种角色

* 过滤器/变形器,修改`logit`，或缩小`size`，不设`selected`.例如`logit-bias`, `penalties`, `top-k`, `top-p`, `min-p`, `temp`.
* 选择器，必须写`cur_p->selected`,例如`greedy`, `dist`, `mirostat`, `adaptive-p`.

## 常见采样器

### Penalties

### DRY

### Top-n-σ

### Top-k

```CPP
static void llama_sampler_top_k_impl(llama_token_data_array * cur_p, int32_t k) {
    // if (k >= (int32_t)cur_p->size) {
    //     return;
    // }

    if (k <= 0) {
        return;
    }

    k = std::min(k, (int) cur_p->size);

    // Sort scores in descending order
    if (!cur_p->sorted) {
        llama_token_data_array_partial_sort_inplace(cur_p, k);
    }

    cur_p->size = k;
}
```

`Top-k`只保留前`k`大的`logits`.

### Typical-p

### Top-p

```CPP

static void llama_sampler_top_p_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    auto * ctx = (llama_sampler_top_p *) smpl->ctx;

    if (ctx->p >= 1.0f) {
        return;
    }

    llama_sampler_softmax_impl(cur_p, false);

    size_t k = cur_p->size;
    auto * pdata = cur_p->data;

    auto & buf_sort = ctx->buf_sort;

    // if not sorted, try adaptive top-k sorting
    if (!cur_p->sorted && cur_p->size > 1024) {
        k = std::min<size_t>(256, cur_p->size);
        llama_token_data_array_partial_sort(*cur_p, k, buf_sort);
        pdata = buf_sort.data();
    } else if (!cur_p->sorted) {
        // small candidates -> sort inplace
        llama_token_data_array_partial_sort_inplace(cur_p, k);
    }

    // Compute the cumulative probabilities
    float cum_sum = 0.0f;
    size_t last_idx = cur_p->size;

    for (size_t i = 0; i < cur_p->size; ++i) {
        cum_sum += pdata[i].p;

        // Check if the running sum is at least p or if we have kept at least min_keep tokens
        // we set the last index to i+1 to indicate that the current iterate should be included in the set
        if (cum_sum >= ctx->p && i + 1 >= ctx->min_keep) {
            last_idx = i + 1;
            break;
        }

        // we exceeded the current top-k heuristic -> increase k and continue
        if (!cur_p->sorted && i == k - 1) {
            k = cur_p->size;
            llama_token_data_array_partial_sort(*cur_p, k, buf_sort);
            pdata = buf_sort.data();
        }
    }

    // Resize the output vector to keep only the top-p tokens
    if (!cur_p->sorted) {
        std::copy(buf_sort.data(), buf_sort.data() + last_idx, cur_p->data);
        cur_p->sorted = true;
    }

    cur_p->size = last_idx;
}
```

### Min-p

### XTC

### Temperature

```CPP
static void llama_sampler_temp_impl(llama_token_data_array * cur_p, float temp) {
    if (temp <= 0.0f) {
        // find the token with the highest logit and set the rest to -inf
        size_t max_i = 0;
        float  max_l = cur_p->data[0].logit;

        for (size_t i = 1; i < cur_p->size; ++i) {
            if (cur_p->data[i    ].logit > max_l) {
                cur_p->data[max_i].logit = -INFINITY;
                max_i = i;
                max_l = cur_p->data[i].logit;
            } else {
                cur_p->data[i].logit = -INFINITY;
            }
        }

        return;
    }

    for (size_t i = 0; i < cur_p->size; ++i) {
        cur_p->data[i].logit /= temp;
    }
}
```

`temperature T`用来调节`logit`分布的锐度

### Dist