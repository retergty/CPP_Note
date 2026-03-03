# optional

在头文件`<optional>`中定义,用于表示一个值可能存在也可能不存在的类型。

`std::optional`常见用途是作为可能失败的函数的返回值。与其他方法（例如 std::pair<T, bool>）相比，可选类型可以很好地处理构造成本高昂的对象，并且更易读，因为意图表达得非常明确。

在任何给定时刻，`std::optional`对象要么包含一个值，要么不包含值。一个不包含值的`std::optional`对象被称为“空的”。当一个`std::optional`对象包含一个值时，我们说它是“engaged”。

当`std::optional`在上下文中转换为`bool`时，如果它包含一个值，则返回`true`，否则返回`false`。

当满足以下情况时，`std::optional`包含一个值：

* 对象使用类型为`T`的值或其他包含值的`std::optional`对象进行初始化。

当满足以下情况时，`std::optional`不包含值：

* 对象使用默认构造函数进行初始化。
* 对象使用`std::nullopt_t`进行初始化。
* 调用了`std::optional`的成员函数`reset()`。

参考文档

* [std::optional](https://en.cppreference.com/w/cpp/utility/optional)

## 类原型

```CPP
template< class T >
class optional;
```

## 成员函数

### 构造函数

* [optional](https://en.cppreference.com/w/cpp/utility/optional/optional.html)

  ```CPP
  optional() noexcept;
  optional( std::nullopt_t ) noexcept;
  optional( const optional& other );
  optional( optional&& other ) noexcept( std::is_nothrow_move_constructible_v<T> );
  template< class... Args >
  explicit optional( std::in_place_type_t<T>, Args&&... args );
  template< class U, class... Args >
  explicit optional( std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args );
  ```

  默认构造函数创建一个不包含值的`std::optional`对象。

  `std::nullopt_t`构造函数创建一个不包含值的`std::optional`对象。

  拷贝构造函数创建一个包含与`other`相同值的`std::optional`对象。如果`other`不包含值，则新对象也不包含值。

  移动构造函数创建一个包含与`other`相同值的`std::optional`对象。如果`other`不包含值，则新对象也不包含值。之后，`other`将处于未定义状态。

  `std::in_place_type_t<T>`构造函数直接在可选对象内构造一个类型为T的对象，使用提供的参数进行初始化。

  `std::in_place_type_t<T>`构造函数直接在可选对象内构造一个类型为T的对象，使用提供的初始化列表和其他参数进行初始化。

### 操作值

* [value](https://en.cppreference.com/w/cpp/utility/optional/value.html)

  ```CPP
  T& value();
  const T& value() const;
  ```

  返回`std::optional`对象中包含的值的引用。如果对象不包含值，则抛出`std::bad_optional_access`异常。

* [operator->, operator*](https://en.cppreference.com/w/cpp/utility/optional/operator_arithmetic.html)

  ```CPP
  T& operator*();
  const T& operator*() const;
  T* operator->();
  const T* operator->() const;
  ```

  如果`std::optional`对象包含值，则返回指向该值的指针或引用。否则，行为未定义。

* [reset](https://en.cppreference.com/w/cpp/utility/optional/reset.html)

  ```CPP
  void reset() noexcept;
  ```

  将`std::optional`对象重置为不包含值的状态。

* [emplace](https://en.cppreference.com/w/cpp/utility/optional/emplace.html)

  ```CPP
  template< class... Args >
  T& emplace( Args&&... args );
  template< class U, class... Args >
  T& emplace( std::initializer_list<U> il, Args&&... args );
  ```

  在`std::optional`对象内直接构造一个类型为T的对象，使用提供的参数进行初始化。如果对象已经包含值，则先销毁当前值，然后构造新值。

* [operator bool](https://en.cppreference.com/w/cpp/utility/optional/operator_bool.html)

  ```CPP
  explicit operator bool() const noexcept;
  ```

  当`std::optional`对象包含值时返回`true`，否则返回`false`。

## 常见使用方法

### 使用std::in_place构造函数直接构造值

```CPP
std::optional<std::string> opt(std::in_place, "Hello, World!");
```

可以用于在函数内部构造一个`std::optional`对象并返回，利用编译期的NRVO优化：

```CPP
std::optional<std::string> createOptionalString() {
    std::optional<std::string> opt(std::in_place, "Hello, World!");
    return opt; // NRVO优化，避免不必要的拷贝或移动
}
```

## 示例

使用 std::optional 最核心的场景是：当一个函数可能无法返回有效结果时，用来代替“错误码”或“空指针”。

```CPP
#include <iostream>
#include <string>
#include <optional> // 必须包含这个头文件
#include <vector>

// 模拟一个数据库查找函数
// 如果找到了返回字符串，找不到则返回 std::nullopt
std::optional<std::string> findUserName(int id) {
    if (id == 42) {
        return "Douglas Adams"; // 隐式转换为 std::optional<std::string>
    }
    return std::nullopt; // 明确表示“没有值”
}

int main() {
    auto result = findUserName(42);

    // 1. 检查是否有值 (像指针一样使用)
    if (result) { 
        // 2. 解引用获取值
        std::cout << "找到用户: " << *result << std::endl;
    } else {
        std::cout << "用户不存在。" << std::endl;
    }

    // 3. 安全获取：如果没值，给一个默认值 (非常常用！)
    std::string name = findUserName(10).value_or("访客用户");
    std::cout << "当前登录: " << name << std::endl;

    // 4. 使用 .value() 获取 (如果不确定有值，不建议直接用，会抛异常)
    try {
        auto invalid = findUserName(99).value(); 
    } catch (const std::bad_optional_access& e) {
        std::cout << "错误：尝试访问一个空的 optional 对象！" << std::endl;
    }

    return 0;
}
```
