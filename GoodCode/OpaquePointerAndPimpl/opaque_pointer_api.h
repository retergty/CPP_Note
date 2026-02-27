#ifndef XXX_API_H
#define XXX_API_H

// 1. 前向声明：外界只知道有这么个类型，不知道里面有什么
typedef struct dump_thread_data_t dump_handle;

// 2. 必须提供成对的生命周期管理函数
dump_handle* dump_thread_create(void);
void dump_thread_destroy(dump_handle* handle);

// 3. 业务 API，必须把 handle 作为第一个参数传入
void dump_thread_write(dump_handle* handle, const void* data, int size);

#endif
