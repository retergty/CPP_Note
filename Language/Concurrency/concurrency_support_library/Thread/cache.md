# cache

`C++`标准库提供了两个`constexpr`变量表示缓存线的大小，定义在`new`头文件中。

这些变都严重地与硬件相关。

* [std::hardware_destructive_interference_size,std::hardware_constructive_interference_size](https://en.cppreference.com/w/cpp/thread/hardware_destructive_interference_size)

## 定义

```CPP
inline constexpr std::size_t
    hardware_destructive_interference_size = /*implementation-defined*/;
inline constexpr std::size_t
    hardware_constructive_interference_size = /*implementation-defined*/;
```

### hardware_destructive_interference_size

这个常量防止假共享所需要最小的偏移，保证至少是`alignof(std::max_align_t)`.

```CPP
struct keep_apart
{
    alignas(std::hardware_destructive_interference_size) std::atomic<int> cat;
    alignas(std::hardware_destructive_interference_size) std::atomic<int> dog;
};
```

此时，多线程访问`cat`就不会使得`dog`的缓存失效了，反之亦然。

### hardware_constructive_interference_size

促进真共享的最大的连续内存大小，小于这个量的变量是真共享的，保证至少是`alignof(std::max_align_t)`

```CPP
struct together
{
    std::atomic<int> dog;
    int puppy;
};
 
struct kennel
{
    // Other data members...
 
    alignas(sizeof(together)) together pack;
 
    // Other data members...
};
 
static_assert(sizeof(together) <= std::hardware_constructive_interference_size);
```
