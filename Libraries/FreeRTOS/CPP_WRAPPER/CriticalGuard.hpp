#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief 临界区 RAII 包装类
 * 作用：在构造时屏蔽中断/调度，析构时自动恢复
 */
class CriticalGuard {
public:
    // 默认构造：进入临界区
    CriticalGuard() noexcept {
        taskENTER_CRITICAL();
    }

    // 析构：退出临界区
    ~CriticalGuard() noexcept {
        taskEXIT_CRITICAL();
    }

    // --- 核心安全设计 ---

    // 1. 禁止在堆上创建 (防止生命周期失控)
    void* operator new(size_t) = delete;
    void* operator new[](size_t) = delete;

    // 2. 禁止拷贝和移动 (临界区不能被传递)
    CriticalGuard(const CriticalGuard&) = delete;
    CriticalGuard& operator=(const CriticalGuard&) = delete;
    CriticalGuard(CriticalGuard&&) = delete;
    CriticalGuard& operator=(CriticalGuard&&) = delete;
};
