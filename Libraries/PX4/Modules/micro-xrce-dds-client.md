# micro-xrce-dds-client

`micro-xrce-dds-client`是`ROS2`的通信中间件，可以使得单片机如同在`ROS2`节点里一般收发数据。

## 概念

### 事务session

通信开始于服务端与客户端的握手，通过握手，服务端

## 使用方法

* [github repositories](https://github.com/eProsima/Micro-XRCE-DDS-Client)

### 设置协议回调函数

```CPP
UXRDLLAPI void uxr_set_custom_transport_callbacks(
        uxrCustomTransport* transport,
        bool framing,
        open_custom_func open,
        close_custom_func close,
        write_custom_func write,
        read_custom_func read);
```

通过设置回调函数`open`,`close`,`write`,`read`就可以实现用任何通信协议与主机通信。

### 