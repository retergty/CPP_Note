# time_point

参考文档

* [time_point](https://en.cppreference.com/w/cpp/chrono/time_point)

定义在头文件`<chrono>`的名称空间`std::chrono`.

## 类原型

```CPP
template<
    class Clock,
    class Duration = typename Clock::duration
> class time_point;
```

## 描述

`std::chrono::time_point`表示时间点,它就像存储一个`Duration`类型的值，指示从`clock`起始点开始的时间间隔。

## 成员类型定义

* `clock`就是模板类型`Clock`.
* `duration`就是模板类型`Duration`.
* `rep`,表示时长的计次数的有符号算术类型
* `period`表示时长周期的`std::ratio`类型

## 例子

```CPP
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
 
void slow_motion()
{
    static int a[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    // Generate Γ(13) == 12! permutations:
    while (std::ranges::next_permutation(a).found) {}
}
 
int main()
{
    using namespace std::literals; // enables literal suffixes, e.g. 24h, 1ms, 1s.
 
    const std::chrono::time_point<std::chrono::system_clock> now =
        std::chrono::system_clock::now();
 
    const std::time_t t_c = std::chrono::system_clock::to_time_t(now - 24h);
    std::cout << "24 hours ago, the time was "
              << std::put_time(std::localtime(&t_c), "%F %T.\n") << std::flush;
 
    const std::chrono::time_point<std::chrono::steady_clock> start =
        std::chrono::steady_clock::now();
 
    std::cout << "Different clocks are not comparable: \n"
                 "  System time: " << now.time_since_epoch() << "\n"
                 "  Steady time: " << start.time_since_epoch() << '\n';
 
    slow_motion();
 
    const auto end = std::chrono::steady_clock::now();
    std::cout
        << "Slow calculations took "
        << std::chrono::duration_cast<std::chrono::microseconds>(end - start) << " ≈ "
        << (end - start) / 1ms << "ms ≈ " // almost equivalent form of the above, but
        << (end - start) / 1s << "s.\n";  // using milliseconds and seconds accordingly
}
```

```shell
24 hours ago, the time was 2021-02-15 18:28:52.
Different clocks are not comparable:
  System time: 1666497022681282572ns
  Steady time: 413668317434475ns
Slow calculations took 2090448µs ≈ 2090ms ≈ 2s.
```
