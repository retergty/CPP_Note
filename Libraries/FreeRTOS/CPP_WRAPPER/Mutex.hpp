#include "FreeRTOS.h"
#include "semphr.h"

class Mutex {
public:
    Mutex() {
        // 创建 FreeRTOS 互斥锁 (支持优先级继承)
        handle = xSemaphoreCreateMutex();
        // 实际项目中应当检查 handle 是否为 NULL (内存不足)
    }

    ~Mutex() {
        if (handle != nullptr) {
            vSemaphoreDelete(handle);
        }
    }

    // 禁用拷贝构造和赋值操作（锁不能被复制）
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;

    void lock() {
        // 阻塞等待直到获取锁 (portMAX_DELAY 表示死等)
        if (handle != nullptr) {
            xSemaphoreTake(handle, portMAX_DELAY);
        }
    }

    void unlock() {
        if (handle != nullptr) {
            xSemaphoreGive(handle);
        }
    }

    // 可选：带超时的 try_lock
    bool try_lock(TickType_t timeout_ticks) {
        if (handle != nullptr) {
            return xSemaphoreTake(handle, timeout_ticks) == pdTRUE;
        }
        return false;
    }

private:
    SemaphoreHandle_t handle;
};
