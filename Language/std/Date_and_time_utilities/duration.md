# duration

参考文档

* [duration](https://en.cppreference.com/w/cpp/chrono/duration)

定义在头文件`<chrono>`的名称空间`std::chrono`.

## 类声明

```CPP
template<
    class Rep,
    class Period = std::ratio<1>
> class duration;
```

## 描述

`std::chrono::duration`代表一个时间间隔。

它由一个类型为`Rep`的时钟计数器与`Period`时钟周期.

`duration`只存储类型为`Rep`的时钟计次数，如果`Rep`为浮点类型，那么`duration`可以表示小数的时钟计次数。`Period`包含周期类型，也就是`Rep`的单位，只有在不同的`duration`中传唤时才会使用。

## 成员类定义

* `rep`就是`Rep`.
* `period`就是`Period`.

## 成员函数

### 构造与析构

* [duration](https://en.cppreference.com/w/cpp/chrono/duration/duration)

  ```CPP
  constexpr duration() = default;

  duration( const duration& ) = default;

  template< class Rep2 >
  constexpr explicit duration( const Rep2& r );

  template< class Rep2, class Period2 >
  constexpr duration( const duration<Rep2, Period2>& d );
  ```

### 运算符重载函数

`Duration`定义了一系列的运算符重载函数。可以方便地进行四则运算。

### 获取信息

* [count](https://en.cppreference.com/w/cpp/chrono/duration/count)

  ```CPP
  constexpr rep count() const;
  ```

  返回当前`duration`中存储的时钟滴答值。

## 帮助类型

`duration`定义了一系列的帮助类型，可以方便地指定时间。

* `std::chrono::nanoseconds`就是`std::chrono::duration</* int64 */, std::nano>`
* `std::chrono::microseconds`就是`std::chrono::duration</* int55 */, std::micro>`
* `std::chrono::milliseconds`就是`std::chrono::duration</* int45 */, std::milli>`
* `std::chrono::seconds`就是`std::chrono::duration</* int35 */>`
* `std::chrono::minutes`就是`std::chrono::duration</* int29 */, std::ratio<60>>`
* `std::chrono::hours`就是`std::chrono::duration</* int23 */, std::ratio<3600>>`
* `std::chrono::days (since C++20)`就是`std::chrono::duration</* int25 */, std::ratio<86400>>`
* `std::chrono::weeks (since C++20)`就是`std::chrono::duration</* int22 */, std::ratio<604800>>`
* `std::chrono::months (since C++20)`就是`std::chrono::duration</* int20 */, std::ratio<2629746>>`
* `std::chrono::years (since C++20)`就是`std::chrono::duration</* int17 */, std::ratio<31556952>>`

## 字面值

在`std::literals::chrono_literals`中定义了一系列的字面值`operator`，可以把特定的字面值转换为`duration`.

* [operator""h](https://en.cppreference.com/w/cpp/chrono/operator%22%22h)
* [operator""min](https://en.cppreference.com/w/cpp/chrono/operator%22%22min)
* [operator""s](https://en.cppreference.com/w/cpp/chrono/operator%22%22s)
* [operator""ms](https://en.cppreference.com/w/cpp/chrono/operator%22%22ms)
* [operator""us](https://en.cppreference.com/w/cpp/chrono/operator%22%22us)
* [operator""ns](https://en.cppreference.com/w/cpp/chrono/operator%22%22ns)

## 非成员函数

### 运算符重载函数

* [operator+,-,*,/,%](https://en.cppreference.com/w/cpp/chrono/duration/operator_arith4)

  对两个`duration`进行四则运算，将它们转化为相同类型，tick进行四则运算.

* [operator==,!=,<,<=,>,>=,<=>](https://en.cppreference.com/w/cpp/chrono/duration/operator_cmp)

  对两个`duration`进行比较，比大小.

## 例子

```CPP
#include <chrono>
#include <iostream>
 
using namespace std::chrono_literals;
 
template<typename T1, typename T2>
using mul = std::ratio_multiply<T1, T2>;
 
int main()
{
    using microfortnights = std::chrono::duration<float,
        mul<mul<std::ratio<2>, std::chrono::weeks::period>, std::micro>>;
    using nanocenturies = std::chrono::duration<float,
        mul<mul<std::hecto, std::chrono::years::period>, std::nano>>;
    using fps_24 = std::chrono::duration<double, std::ratio<1, 24>>;
 
    std::cout << "1 second is:\n";
 
    // integer scale conversion with no precision loss: no cast
    std::cout << std::chrono::milliseconds(1s).count() << " milliseconds\n"
              << std::chrono::microseconds(1s).count() << " microseconds\n"
              << std::chrono::nanoseconds(1s).count() << " nanoseconds\n";
 
    // integer scale conversion with precision loss: requires a cast
    std::cout << std::chrono::duration_cast<std::chrono::minutes>(1s).count()
              << " minutes\n";
 
    // floating-point scale conversion: no cast
    std::cout << microfortnights(1s).count() << " microfortnights\n"
              << nanocenturies(1s).count() << " nanocenturies\n"
              << fps_24(1s).count() << " frames at 24fps\n";
}
```
