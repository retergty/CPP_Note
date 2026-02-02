# 吸纳参数模式

吸纳参数模式（Sink Parameter Pattern）是一种设计模式，旨在简化函数或方法的参数传递。

## 场景

假设有一个机器人类，需要设置它的路径点（Path），这个`std::vector`将会被保存到类成员变量中。

### 传统的重载法

```CPP
class Robot {
    std::vector<int> path_;
public:
    // 针对左值：拷贝 (1 Copy)
    void setPath(const std::vector<int>& p) {
        path_ = p;
    }
    
    // 针对右值：移动 (1 Move)
    void setPath(std::vector<int>&& p) {
        path_ = std::move(p);
    }
};
```

* 优点：性能较好，避免了不必要的拷贝。
* 缺点：代码膨胀，如果有多个参数需要这样处理，代码量会成倍增加。

### 吸纳参数模式

```CPP
class Robot {
    std::vector<int> path_;
public:
    // 统一处理：进来时或是拷贝或是移动，内部再移动一次
    void setPath(std::vector<int> p) {
        path_ = std::move(p);
    }
};
```

* 优点：代码简洁，只有一个函数实现。
* 缺点：会多一次“移动构造”的开销。

### 完美转发

```CPP
template <typename T>
void setPath(T&& p) {
    path_ = std::forward<T>(p);
}
```

* 优点：代码简洁，性能最佳。
* 缺点：语法复杂，可能不易理解，且需要进行模板类型限制以防止不必要的类型被传入。
