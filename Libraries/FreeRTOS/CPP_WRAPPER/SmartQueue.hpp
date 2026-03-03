#include <memory>
#include "FreeRTOS.h"
#include "queue.h"

template <typename T>
class SmartQueue {
public:
    explicit SmartQueue(UBaseType_t length) {
        // 队列存储的是原始指针（指针本身是平凡可拷贝的，安全！）
        m_handle = xQueueCreate(length, sizeof(T*));
    }

    ~SmartQueue() {
        if (m_handle) vQueueDelete(m_handle);
    }

    /**
     * @brief 发送所有权
     * @param ptr 传入的 unique_ptr
     */
    bool push(std::unique_ptr<T> ptr, uint32_t timeout_ms = portMAX_DELAY) {
        if (!ptr) return false;

        T* raw_ptr = ptr.get(); // 获取原始指针
        TickType_t ticks = pdMS_TO_TICKS(timeout_ms);

        if (xQueueSend(m_handle, &raw_ptr, ticks) == pdPASS) {
            // 如果发送成功，剥离 unique_ptr 的所有权（它不再负责释放内存）
            ptr.release(); 
            return true;
        }
        
        // 如果发送失败，ptr 会在函数结束时正常析构，内存自动释放，不会泄漏！
        return false;
    }

    /**
     * @brief 接收所有权
     * @return 返回一个新的 unique_ptr，若失败则返回空
     */
    std::unique_ptr<T> pop(uint32_t timeout_ms = portMAX_DELAY) {
        T* raw_ptr = nullptr;
        TickType_t ticks = pdMS_TO_TICKS(timeout_ms);

        if (xQueueReceive(m_handle, &raw_ptr, ticks) == pdPASS) {
            // 拿到指针后，立即封装，接管所有权
            return std::unique_ptr<T>(raw_ptr);
        }

        return nullptr; // 返回空的 unique_ptr
    }

private:
    QueueHandle_t m_handle;
};
