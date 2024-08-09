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

## 发布主题

如同上文订阅主题的内容，包含要发布消息的头文件。

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

#### 重要成员变量

```CPP
void *_node{nullptr};
```

* `_node`存储了订阅消息的信息，如果为空,意味着这个订阅还没有加入到主题。

#### 重要成员函数

```CPP
bool Subscription::subscribe();
bool advertised();
```

* 把当前订阅加入主题。

```CPP
bool updated();
```

* 检查是否有新的消息，若有，返回`ture`.

```CPP
bool update(void *dst);
```

* 检查是否有新的消息，若有，把最新的消息复制到`dst`.

```CPP
bool copy(void *dst);
```

* 把最新的消息复制到`dst`.不检查这个消息是否已经收到了。

### SubscriptionData类

和`PublicationData`类类似。`update()`函数会直接更新到内部储存的结构体中。

### SubscriptionInterval类

```CPP
class SubscriptionInterval
```

和`Subscription`类类似，只不过它具有限制读取消息速率的机制，通常用于缓慢改变的主题，比如参数。

#### 重要成员变量

```CPP
  Subscription _subscription；
  uint64_t  _last_update{0};  // last subscription update in microseconds
  uint32_t  _interval_us{0};  // maximum update interval in microseconds
```

* `_subscription`保存订阅
* `_last_update`存储上次更新时间
* `_interval_us`表示最大的更新时间间隔，只有上次更新之后至少经过了`_interval_us`才会更新消息。

### SubscriptionCallback类

```CPP
class SubscriptionCallback : public SubscriptionInterval, public ListNode<SubscriptionCallback *>
```

是订阅回调函数类，当有订阅产生时便触发回调。

#### 重要成员函数

```CPP
bool registerCallback()
```

* 注册回调函数，把这个类加入到回调函数列表中，会在接收到订阅消息时触发回调函数

```CPP
virtual void call() = 0;
```

* 触发的回调函数，纯虚函数需要继承于它的类实现，这个回调函数**不是**在当前线程调用的，而是在**设备节点的线程下**调用的。

### SubscriptionCallbackWorkItem

```CPP
class SubscriptionCallbackWorkItem : public SubscriptionCallback
```

* 把这个订阅与特定的`WorkItem`连接，在订阅发生时，自动调用回调函数，回调函数可以把`WorkItem`置于`WorkQueue`运行队列中。

#### 重要成员变量

```CPP
px4::WorkItem *_work_item;
uint8_t _required_updates{0};
```

* `_work_item`存储个这个订阅所属于的`WorkItem`工作线程。在构造函数时传递。
* `_required_updates`需求的更新个数，只有大于或者等于其才会获取订阅。

#### 重要成员函数

```CPP
void call() override
{
  // schedule immediately if updated (queue depth or subscription interval)
  if ((_required_updates == 0)
      || (Manager::updates_available(_subscription.get_node(), _subscription.get_last_generation()) >= _required_updates)) {
  if (updated()) {
    _work_item->ScheduleNow();
    }
  }
}
```

* 订阅发生时的回调函数。
* 检查是否需要更新，已有的更新大于设置且更新间隔大于设置值。
* 若需要更新，则**调度**当前的`WorkItem`,也就是把`WorkItem`加入到对应的`WorkQueue`的运行队列中。注意，这个函数是在中断上下文中，**由`uORB`调用的**。

```CPP
SubscriptionCallbackWorkItem(px4::WorkItem *work_item, const orb_metadata *meta, uint8_t instance = 0) :
  SubscriptionCallback(meta, 0, instance),  // interval 0
  work_item(work_item)
{
}
```

* 构造函数，保存指向`WorkItem`的指针`work_item`.

### uORB::Manager类
