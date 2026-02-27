#include "opaque_pointer_api.h"
#include <stdlib.h>
// 这里可以随意 include 底层库，外界完全看不见

// 在这里给出真实定义
struct dump_thread_data_t {
    void* fp;
    uint32_t write_idx; // 使用无锁队列索引
    uint32_t read_idx;
    // ... 添加再多变量，外界的 ABI 都不受影响
};

dump_handle* dump_thread_create(void) {
    struct dump_thread_data_t* ctx = malloc(sizeof(*ctx));
    ctx->write_idx = 0;
    ctx->read_idx = 0;
    return ctx; // 指针隐式/显式转换交出去
}
