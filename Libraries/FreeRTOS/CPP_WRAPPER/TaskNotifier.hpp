#include "FreeRTOS.h"
#include "task.h"

class TaskNotifier {
public:
    /**
     * @brief 默认构造，绑定到当前调用该构造函数的任务
     */
    TaskNotifier() : m_taskToNotify(xTaskGetCurrentTaskHandle()) {}

    /**
     * @brief 绑定到指定任务
     * @param taskHandle 目标任务的句柄
     */
    explicit TaskNotifier(TaskHandle_t taskHandle) : m_taskToNotify(taskHandle) {}

    // --- 发送端接口 ---

    /**
     * @brief 发送通知（累加值，类似计数信号量）
     */
    void give() {
        xTaskNotifyGive(m_taskToNotify);
    }

    /**
     * @brief 从中断中发送通知（ISR 专用）
     */
    void giveFromISR() {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(m_taskToNotify, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    /**
     * @brief 发送特定位（Bitwise，类似事件组）
     */
    void setBits(uint32_t bits) {
        xTaskNotify(m_taskToNotify, bits, eSetBits);
    }

    // --- 接收端接口（仅在当前任务中调用） ---

    /**
     * @brief 等待通知（类似 Take）
     * @param timeout_ms 超时时间
     * @return uint32_t 返回当前的通知值
     */
    static uint32_t take(bool clearOnExit = true, uint32_t timeout_ms = portMAX_DELAY) {
        TickType_t ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
        return ulTaskNotifyTake(clearOnExit ? pdTRUE : pdFALSE, ticks);
    }

    /**
     * @brief 等待特定的位
     */
    static bool waitBits(uint32_t bitsToWaitFor, uint32_t& receivedBits, uint32_t timeout_ms = portMAX_DELAY) {
        TickType_t ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
        return xTaskNotifyWait(0, bitsToWaitFor, &receivedBits, ticks) == pdPASS;
    }

private:
    TaskHandle_t m_taskToNotify;
};
