# clock

一个时钟由一个起始点(`epoch`)与时钟频率有关,比如一个时钟的起始点是`1970`年一月一号，时钟频率为每秒一次。

`clock`都定义在头文件`<chrono>`的名称空间`std::chrono`中。

## system_clock类

参考文档

* [system_clock](https://en.cppreference.com/w/cpp/chrono/system_clock)

### 类原型

```CPP
class system_clock;
```

### 描述

`std::chrono::system_clock`表示的是系统级别的实时壁钟`wall timer`.

这个时钟的值可能不是单调上升的，在大多数系统上，系统时间可以随时调整。它是唯一有能力映射其时间点到`C`风格时间的`C++`时钟。

`std::chrono::system_clock`满足平凡时钟`TrivialClock`的要求。

### 成员类型

* `rep`表示时钟时长中的计次数的有符号算术类型
* `period`表示时钟计次周期的`std::ratio`类型，单位为秒
* `duration`就是`std::chrono::duration<rep, period>`，能够表示负时长
* `time_point`就是`std::chrono::time_point<std::chrono::system_clock>`.

### 成员函数

* [now](https://en.cppreference.com/w/cpp/chrono/system_clock/now)

  ```CPP
  static std::chrono::time_point<std::chrono::system_clock> now() noexcept;
  ```

  这是一个静态成员函数，返回当前时间的`std::chrono::time_point<std::chrono::system_clock>`.

## steady_clock类

参考文档

* [steady_clock](https://en.cppreference.com/w/cpp/chrono/steady_clock)

### 描述

`std::chrono::steady_clock`代表一个单调上升的时钟，并且时钟频率是恒定的。这个时钟与壁钟无关，它可能是自从上次重启后所经过的时间，适用于测量间隔。

`std::chrono::steady_clock`满足平凡时钟`TrivialClock`的要求。

### 成员类型

* `rep`表示时钟时长中的计次数的有符号算术类型
* `period`表示时钟计次周期的`std::ratio`类型，单位为秒
* `duration`就是`std::chrono::duration<rep, period>`，能够表示负时长
* `time_point`就是`std::chrono::time_poin<std::chrono::steady_clock>`.

### 成员函数

* [now](https://zh.cppreference.com/w/cpp/chrono/steady_clock/now)

  ```CPP
  static std::chrono::time_point<std::chrono::steady_clock> now() noexcept;
  ```

  这是一个静态成员函数，返回当前时间的`std::chrono::time_point<std::chrono::steady_clock>`.

## high_resolution_clock类

参考文档

* [high_resolution_clock](https://en.cppreference.com/w/cpp/chrono/high_resolution_clock)

### 描述

`std::chrono::high_resolution_clock`表示具有由实现提供的最小滴答周期的时钟(最高时钟频率).

在目前主流编译器实现中，它是`std::chrono::system_clock`或者是`std::chrono::steady_clock`的别名，也有可能是一个独立的时钟，取决于实现。

`std::chrono::high_resolution_clock`满足平凡时钟`TrivialClock`的要求。

### 成员类型

* `rep`表示时钟时长中的计次数的有符号算术类型
* `period`表示时钟计次周期的`std::ratio`类型，单位为秒
* `duration`就是`std::chrono::duration<rep, period>`，能够表示负时长
* `time_point`就是`std::chrono::time_point<std::chrono::high_resolution_clock>`.

### 成员函数

* [now](https://en.cppreference.com/w/cpp/chrono/high_resolution_clock/now)

  ```CPP
  static std::chrono::time_point<std::chrono::high_resolution_clock> now() noexcept;
  ```

## 例子

```CPP
#include <chrono>
#include <iostream>
 
int main()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    std::cout << "The system clock is currently at " << std::ctime(&t_c);
}
```

```shell
The system clock is currently at Thu Mar 30 13:28:27 2023
```
