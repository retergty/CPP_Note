# uORB

`uORB`是一个异步的`publish()`/`subscribe()`消息传递API，用于线程间/进程间通信。数据通过主题`Topic`传递。

使用时需要`include`头文件

```CPP
#include <uORB/Publication.hpp>
#include <uORB/Subscription.hpp>
#include <uORB/SubscriptionCallback.hpp>
```

## 订阅主题

以`trajectory_setpoint`主题为例，在要使用这个主题模块类的头文件中，比如`mc_pos_control`模块`MulticopterPositionControl.hpp`的`MulticopterPositionControl`类中。

`include`主题消息的头文件

```CPP
#include <uORB/topics/trajectory_setpoint.h>
```

这个消息头文件有如下的特点

* 定义了`trajectory_setpoint_s`消息结构体
* 消息结构体里自动添加了`timestamp`，表示这个消息发布时的时间。

同时添加一个订阅

```CPP
uORB::Subscription _trajectory_setpoint_sub{ORB_ID(trajectory_setpoint)};
```

* `ORB_ID(trajectory_setpoint)`指定这个订阅要订阅的主题名。

这样，在代码中便可以获取这个消息

```CPP
_trajectory_setpoint_sub.update(&_setpoint);
```

把更新的消息复制到本地`_setpoint`中。

## 添加一个新的主题

只需要在`msg`文件夹里面添加`.msg`文件，文件名就是主题名。并在`msg`文件夹的顶层`CMakeLists.txt`中加入对应的主题名即可。

## 发布主题

发布主题和订阅主题类似，以`vehicle_attitude_setpoint`为例

`include`主题消息头文件

```CPP
#include <uORB/topics/vehicle_attitude_setpoint.h>
```

添加一个发布

```CPP
uORB::Publication<vehicle_attitude_setpoint_s>       _vehicle_attitude_setpoint_pub{ORB_ID(vehicle_attitude_setpoint)};
```

* `ORB_ID(vehicle_attitude_setpoint)`指定这个订阅要订阅的主题名。

这样，在代码中便可以发布这个信息

```CPP
_vehicle_attitude_setpoint_pub.publish(attitude_setpoint);
```

## 多实例

`uORB`可以在同一个主题上有多个独立的实例，彼此互不关联，相互独立。这些独立的实例通过`instance index`管理，订阅这个主题的特定实例也需要传递这个`intance index`.

创建同一主题的不同实例的发布API函数是`orb_advertise_multi`.

订阅统一主题的不同实例的API函数是`orb_subscribe_multi`.(`orb_subscribe`默认订阅第一个实例)。

## uORB类

### Publication类

```CPP
template<typename T>
class Publication : public PublicationBase
```

是`uORB`的包装类，包装了`uorb`的函数。

```CPP
  bool advertise()
  {
    if (!advertised()) {
      _handle = orb_advertise(get_topic(), nullptr);
    }

    return advertised();
  }
```

* `advertise`广播这个发布者，此时这个发布者才真正加入了这个主题。

```CPP
  /**
   * Publish the struct
   * @param data The uORB message struct we are updating.
   */
  bool publish(const T &data)
  {
    if (!advertised()) {
      advertise();
    }

    return (Manager::orb_publish(get_topic(), _handle, &data) == PX4_OK);
  }
```

* 发布数据

### PublicationData类

```CPP
template<typename T>
class PublicationData : public Publication<T>
```

和`Publication`类相似。但它类似于一个数据，具有传递数据的副本，以及API函数`update`表示更新这个数据。

### Subscription类

```CPP
class Subscription
```

是`uORB`的包装类，包装了`uorb`的函数。

```CPP
bool Subscription::subscribe();
bool advertised();
```

* 把当前订阅加入主题。

```CPP
bool updated();
```

检查是否有新的消息，若有，返回`ture`.

```CPP
bool update(void *dst);
```

检查是否有新的消息，若有，把最新的消息复制到`dst`.

```CPP
bool copy(void *dst);
```

把最新的消息复制到`dst`.不检查这个消息是否已经收到了。

### SubscriptionData类

和`PublicationData`类类似。

### SubscriptionInterval类

```CPP
class SubscriptionInterval
```

和`Subscription`类类似，只不过它具有限制读取消息速率的机制，通常用于缓慢改变的主题，比如参数。

```CPP
  uint64_t  _last_update{0};  // last subscription update in microseconds
  uint32_t  _interval_us{0};  // maximum update interval in microseconds
```

* `_last_update`存储上次更新时间
* `_interval_us`表示最大的更新时间间隔，只有上次更新之后至少经过了`_interval_us`才会更新消息。

### uORB::Manager类
