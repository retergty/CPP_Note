# micro-xrce-dds-client

`micro-xrce-dds-client`是`ROS2`的通信中间件，可以使得单片机如同在`ROS2`节点里一般收发数据。

## 概念

### 通信抽象

客户端与服务器的通讯基于操作(operation)与应答(response).客户端向服务器请求操作，服务器处理请求并返回应答。客户端收到服务器的应答后，可以继续请求操作。

### 事务session

通信开始于服务端与客户端的握手，通过握手，服务器认识到了当前客户端的存在.这是通过`Create session`操作实现的.创建事务操作必须是客户端与服务器通信的开始，其它任何的操作都会被服务器拒绝。成功创建事务后，客户端可以进一步请求`create entities`等操作.

在代码中，所有的其它操作都与特定的`session`关联，接受`session`作为参数。

### 操作operation

操作是客户端可能向服务器请求的操作，操作围绕实体展开.服务器会对所有操作请求做出成功或失败的应答》



### 实体Entity

具体与服务器通信是通过实体管理的，通过`Create entity`操作可以创建实体，一个实体与一个DDS对象一一对应.

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