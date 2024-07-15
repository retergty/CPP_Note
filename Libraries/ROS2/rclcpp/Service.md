# Service

参考文档

* [Template Class Service](https://docs.ros.org/en/jazzy/p/rclcpp/generated/classrclcpp_1_1Service.html#classrclcpp_1_1Service)

定义在头文件`rclcpp/service.hpp`的`rclcpp`名称空间中。

## 类原型

```CPP
template<typename ServiceT>
class Service : public rclcpp::ServiceBase, public std::enable_shared_from_this<Service<ServiceT>>
```

* `ServiceT`服务消息类型。

## 常见成员函数

### 构造与析构

```CPP
inline Service(std::shared_ptr<rcl_node_t> node_handle, const std::string &service_name, AnyServiceCallback<ServiceT> any_callback, rcl_service_options_t &service_options)
```

构造函数，通常不是直接调用的，而是使用`rclcpp::create_service()`构建并获取的。

* `node_handle`指向这个服务所属的节点
* `service_name`发布的主题名
* `any_callback`服务回调函数
* `service_options`服务选项

```CPP
inline Service(std::shared_ptr<rcl_node_t> node_handle, std::shared_ptr<rcl_service_t> service_handle, AnyServiceCallback<ServiceT> any_callback)
```

构造函数，通常不是直接调用的，而是使用`rclcpp::create_service()`构建并获取的。

* `node_handle`指向这个服务所属的节点
* `service_handle`服务句柄
* `any_callback`服务回调函数

## 回调函数

```CPP
  using SharedPtrCallback = std::function<
    void (
      std::shared_ptr<typename ServiceT::Request>,
      std::shared_ptr<typename ServiceT::Response>
    )>;
  using SharedPtrWithRequestHeaderCallback = std::function<
    void (
      std::shared_ptr<rmw_request_id_t>,
      std::shared_ptr<typename ServiceT::Request>,
      std::shared_ptr<typename ServiceT::Response>
    )>;
  using SharedPtrDeferResponseCallback = std::function<
    void (
      std::shared_ptr<rmw_request_id_t>,
      std::shared_ptr<typename ServiceT::Request>
    )>;
  using SharedPtrDeferResponseCallbackWithServiceHandle = std::function<
    void (
      std::shared_ptr<rclcpp::Service<ServiceT>>,
      std::shared_ptr<rmw_request_id_t>,
      std::shared_ptr<typename ServiceT::Request>
    )>;
```

* `std::shared_ptr<typename ServiceT::Request>`是收到的请求
* `std::shared_ptr<typename ServiceT::Response>`是发送的应答
