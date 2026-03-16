#include <iostream>
#include <utility> // for std::forward

template <typename T>
class ObjectPool {
private:
    // 核心魔法：用一个结构体强行把泛型 T 的内存空间当作指针来用
    struct Node {
        Node* next;
    };

    Node* free_list_head = nullptr; // 指向第一个空闲块的指针
    char* memory_block = nullptr;   // 指向整块大内存的起始位置
    size_t pool_size = 0;

public:
    // 构造函数：一次性向操作系统申请一大块连续内存
    explicit ObjectPool(size_t size) : pool_size(size) {
        // 安全检查：确保对象的大小至少能装下一个指针
        static_assert(sizeof(T) >= sizeof(Node*), "对象类型 T 必须大于等于指针大小");

        // 分配原始的字节内存（注意：这里不调用 T 的构造函数）
        memory_block = new char[pool_size * sizeof(T)];
        free_list_head = reinterpret_cast<Node*>(memory_block);

        // 像穿糖葫芦一样，把这些空闲的内存块用 next 指针串起来
        Node* current = free_list_head;
        for (size_t i = 1; i < pool_size; ++i) {
            current->next = reinterpret_cast<Node*>(memory_block + i * sizeof(T));
            current = current->next;
        }
        current->next = nullptr; // 最后一个节点的 next 为空
    }

    // 分配对象（完美转发构造函数的参数）
    template <typename... Args>
    T* allocate(Args&&... args) {
        if (!free_list_head) {
            // 池子空了！实际工程中可以扩容或抛出异常，这里简单返回 nullptr
            std::cerr << "内存池已耗尽！" << std::endl;
            return nullptr;
        }

        // 1. 从空闲链表头部摘下一个块 (O(1) 操作)
        Node* node = free_list_head;
        free_list_head = free_list_head->next;

        // 2. 定位 new (Placement new) 魔法！
        // 在这块已有的内存地址上，强行调用 T 的构造函数
        return new (node) T(std::forward<Args>(args)...);
    }

    // 释放对象
    void deallocate(T* obj) {
        if (!obj) return;

        // 1. 手动调用析构函数，清理对象的内部资源
        obj->~T();

        // 2. 把这块内存重新当成 Node 指针，挂回空闲链表的头部 (O(1) 操作)
        Node* node = reinterpret_cast<Node*>(obj);
        node->next = free_list_head;
        free_list_head = node;
    }

    // 析构函数：释放整块大内存
    ~ObjectPool() {
        // 注意：这里粗暴地释放了底层内存。
        // 实际工程中，需要确保所有分配出去的对象都已经 deallocate 了，
        // 否则那些还在外面跑的对象的析构函数就不会被执行（会内存泄漏）。
        delete[] memory_block;
    }
};
