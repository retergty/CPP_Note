# Timer

定时器创建定时任务，达到事件后自动执行特定的任务。

## TimerBase类

定义在头文件`rclcpp/timer.hpp`的`rclcpp`名称空间中。

参考文档

* [Class TimerBase](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1TimerBase.html#classrclcpp_1_1TimerBase)

```CPP
class TimerBase
```

### 派生类型

* `public rclcpp::GenericTimer< FunctorT, >`

### 描述

是定时器实现的基类，不接受模板参数的特性可以使得我们使用`shared_ptr<TimerBase>`绑定`node->create_timer`返回的共享指针。方便使用

### 常用成员函数

#### 检查可用性

```CPP
bool is_ready()
```

检查计时器是否准备好触发回调。

## GenericTimer类

定义在头文件`rclcpp/timer.hpp`的`rclcpp`名称空间中。

参考文档

* [Template Class GenericTimer](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1GenericTimer.html#exhale-class-classrclcpp-1-1generictimer)

```CPP
template<typename FunctorT, typename std::enable_if<rclcpp::function_traits::same_arguments<FunctorT, VoidCallbackType>::value || rclcpp::function_traits::same_arguments<FunctorT, TimerCallbackType>::value || rclcpp::function_traits::same_arguments<FunctorT, TimerInfoCallbackType>::value>::type* = nullptr>
class GenericTimer : public rclcpp::TimerBase
```

### 派生类型

* `public rclcpp::WallTimer< FunctorT, >`

### 描述

是通用定时器，周期性地执行回调函数。

### 常用成员函数

#### 构造与析构

```CPP
inline explicit GenericTimer(Clock::SharedPtr clock, std::chrono::nanoseconds period, FunctorT &&callback, rclcpp::Context::SharedPtr context, bool autostart = true)
```

#### 获取信息

```CPP
inline virtual bool is_steady() override
```

检查时钟是否稳定。

## 回调函数

回调函数的类型如下

```CPP
using rclcpp::TimerCallbackType = std::function<void(TimerBase&)>
using rclcpp::VoidCallbackType = std::function<void()>
```
