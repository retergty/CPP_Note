template <typename T>
class LockGuard {
public:
    // 构造时自动加锁
    explicit LockGuard(T& mutex) : m_mutex(mutex) {
        m_mutex.lock();
    }

    // 析构时自动解锁 (离开作用域时由编译器自动调用)
    ~LockGuard() {
        m_mutex.unlock();
    }

    // 严禁拷贝
    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;

private:
    T& m_mutex;
};
