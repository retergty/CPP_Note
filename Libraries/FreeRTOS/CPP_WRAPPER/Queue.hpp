#include "FreeRTOS.h"
#include "queue.h"
#include <type_traits>

template <typename T>
class Queue {
    // 嵌入式安全检查：确保 T 是可以被简单拷贝的（因为 FreeRTOS 内部使用 memcpy）
    static_assert(std::is_trivially_copyable<T>::value, "Queue elements must be trivially copyable!");

public:
    /**
     * @brief 构造函数
     * @param length 队列深度（能放多少个 T）
     */
    explicit Queue(UBaseType_t length) : m_handle(nullptr) {
        m_handle = xQueueCreate(length, sizeof(T));
    }

    ~Queue() {
        if (m_handle != nullptr) {
            vQueueDelete(m_handle);
        }
    }

    // 禁止拷贝，防止重复删除句柄
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    /**
     * @brief 推送数据到队尾
     * @param item 数据项
     * @param timeout_ms 超时时间（毫秒），默认为死等
     * @return true 发送成功, false 超时或失败
     */
    bool push(const T& item, uint32_t timeout_ms = portMAX_DELAY) noexcept {
        TickType_t ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
        return xQueueSend(m_handle, &item, ticks) == pdPASS;
    }

    /**
     * @brief 从队首获取数据
     * @param out_item 输出变量
     * @param timeout_ms 超时时间（毫秒）
     * @return true 获取成功, false 超时
     */
    bool pop(T& out_item, uint32_t timeout_ms = portMAX_DELAY) noexcept {
        TickType_t ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
        return xQueueReceive(m_handle, &out_item, ticks) == pdPASS;
    }

    // 常用辅助函数
    UBaseType_t count() const { return uxQueueMessagesWaiting(m_handle); }
    bool isFull() const { return uxQueueSpacesAvailable(m_handle) == 0; }

private:
    QueueHandle_t m_handle;
};
