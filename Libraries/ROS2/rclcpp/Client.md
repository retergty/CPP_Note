# Client

参考文档

* [Template Class Client](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1Client.html#classrclcpp_1_1Client)

定义在头文件`rclcpp/client.hpp`的`rclcpp`名称空间中。

## 类原型

```CPP
template<typename ServiceT>
class Client : public rclcpp::ClientBase
```

* `ServiceT`服务的消息类型

## 常用成员函数

### 构造与析构

```CPP
inline Client(rclcpp::node_interfaces::NodeBaseInterface *node_base, rclcpp::node_interfaces::NodeGraphInterface::SharedPtr node_graph, const std::string &service_name, rcl_client_options_t &client_options)
```

构造函数，通常不是直接调用的，而是使用`rclcpp::create_client()`构建并获取的。

### 发送请求

```CPP
inline FutureAndRequestId async_send_request(SharedRequest request)
```

向服务端发送请求。

这个函数返回`FutureAndRequestId`对象，可以被传递给`Executor::spin_until_future_complete()`去等待直到完成。

如果`future`无法完成，比如`Executor::spin_until_future_complete()`超时，必须调用`Client::remove_pending_request()`来清理客户端内部状态.否则会导致内存泄漏。

* `request`发送的请求
* 返回值`FutureAndRequestId`表示将要接收到的客户端应答`future`.

```CPP
template<typename CallbackT, typename std::enable_if<rclcpp::function_traits::same_arguments<CallbackT, CallbackType>::value>::type* = nullptr>
inline SharedFutureAndRequestId async_send_request(SharedRequest request, CallbackT &&cb)
```

向服务服务器发送请求并在执行器中安排回调。

和上一个类似，只不过会在接收到响应时自动触发回调。

如果回调没有被调用，因为没有收到服务器的应答，必须调用`Client::remove_pending_request()`来清理客户端内部状态.

* `request`发送的请求
* `cb`回调函数
* 返回值`SharedFutureAndRequestId`表示将要接收到的客户端应答`shared_future`.

```CPP
template<typename CallbackT, typename std::enable_if<rclcpp::function_traits::same_arguments<CallbackT, CallbackWithRequestType>::value>::type* = nullptr>
inline SharedFutureWithRequestAndRequestId async_send_request(SharedRequest request, CallbackT &&cb)
```

### 清理请求

```CPP
inline bool remove_pending_request(int64_t request_id)
```

清理待处理的请求。

这通知客户端我们已经等待了足够长的时间来等待服务器的响应，我们已经放弃并且不再等待响应。

不调用此函数将使客户端开始为每个从未收到服务器回复的请求使用更多内存。

* `request_id`请求`id`.
* 返回值`bool`当待处理的请求被删除时为`true`，如果未删除则为`false`（例如收到响应）。

```CPP
inline bool remove_pending_request(const FutureAndRequestId &future)
inline bool remove_pending_request(const SharedFutureAndRequestId &future)
inline bool remove_pending_request(const SharedFutureWithRequestAndRequestId &future)
```

清理待处理的请求

等价于

```CPP
Client::remove_pending_request(this, future.request_id)
```

* `future`发送请求时获得的`future`.
* 返回值`bool`当待处理的请求被删除时为`true`，如果未删除则为`false`（例如收到响应）。

```CPP
inline size_t prune_pending_requests()
```

清除所有待处理的请求。

* 返回值`size_t`表示清除的请求。

```CPP
template<typename AllocatorT = std::allocator<int64_t>>
inline size_t prune_requests_older_than(std::chrono::time_point<std::chrono::system_clock> time_point, std::vector<int64_t, AllocatorT> *pruned_requests = nullptr)
```

清除所有晚于特定时间`time_point`的请求。

## 回调函数

```CPP
using CallbackType = std::function<void(SharedFuture)>
using CallbackWithRequestType = std::function<void(SharedFutureWithRequest)>
```

接受`shared_future`的回调函数。自动处理接收到服务端应答时的操作。

### 检查服务器是否可用

```CPP
bool service_is_ready() const
```

服务是否准备完成。

* 返回值`bool`如果为`true`意味着服务准备就绪，否则为`false`

```CPP
template<typename RepT = int64_t, typename RatioT = std::milli>
inline bool wait_for_service(std::chrono::duration<RepT, RatioT> timeout = std::chrono::duration<RepT, RatioT>(-1))
```

等待服务准备完成

* `timeout`等待时间，`-1`意味着一直等待
* 返回值`bool`如果为`true`意味着服务准备就绪，为`false`意味着等待超时。
