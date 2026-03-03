#include <cstdlib>
#include <new>
#include "FreeRTOS.h"
#include "task.h"

// 1. 全局重载单对象 new
void* operator new(std::size_t size) {
    void* p = pvPortMalloc(size);
    // 注意：如果禁用了异常 (-fno-exceptions)，new 失败应该返回 nullptr
    // 如果启用了异常，这里应该抛出 std::bad_alloc()
    return p;
}

// 2. 全局重载数组 new[]
void* operator new[](std::size_t size) {
    return pvPortMalloc(size);
}

// 3. 全局重载单对象 delete
void operator delete(void* p) noexcept {
    if (p) {
        vPortFree(p);
    }
}

// 4. 全局重载数组 delete[]
void operator delete[](void* p) noexcept {
    if (p) {
        vPortFree(p);
    }
}

// 5. C++14 之后需要的“带大小的 delete”（可选）
void operator delete(void* p, std::size_t size) noexcept {
    (void)size; // 屏蔽未使用警告
    vPortFree(p);
}

void operator delete[](void* p, std::size_t size) noexcept {
    (void)size;
    vPortFree(p);
}
