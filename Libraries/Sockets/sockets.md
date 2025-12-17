# sockets

`Socket`就是IP 地址与端口号的组合，用于实现网络间的通信。`Socket`是网络通信的端点，是应用层与传输层之间的接口。

## 常用的Socket类型

* 流式套接字（`SOCK_STREAM`）：提供面向连接的、可靠的、基于字节流的通信服务。它使用传输控制协议（`TCP`）作为传输协议。流式套接字确保数据按顺序到达，并且没有数据丢失或重复。
* 数据报套接字（`SOCK_DGRAM`）：提供无连接的、不可靠的、基于数据报的通信服务。它使用用户数据报协议（`UDP`）作为传输协议。数据报套接字不保证数据的顺序或完整性，适用于对速度要求较高且可以容忍数据丢失的应用。

## Socket编程

### 创建Socket

```C++
int socket(int domain, int type, int protocol);
```

* `domain`：指定协议族，如`AF_INET`（IPv4）、`AF_INET6`（IPv6）等。
* `type`：指定Socket类型，如`SOCK_STREAM`（流式套接字）、`SOCK_DGRAM`（数据报套接字）等。
* `protocol`：通常设置为`0`，让系统自动选择合适的协议。

返回值，成功时返回Socket文件描述符，失败时返回`-1`。

### 绑定Socket

```C++
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

服务端使用`bind`函数将Socket绑定到特定的IP地址和端口号上。

* `sockfd`：要绑定的Socket文件描述符。
* `addr`：指向包含要绑定的地址信息的`sockaddr`结构
* `addrlen`：地址结构的长度。

返回值，成功时返回`0`，失败时返回`-1`。

`struct sockaddr_in`是IPv4地址的常用结构：

```C++
struct sockaddr_in {
    sa_family_t    sin_family; // 地址族，填 AF_INET
    in_port_t      sin_port;   // 端口号 (必须是网络字节序，用 htons转换)
    struct in_addr sin_addr;   // IP地址
};
```

### 监听Socket

```C++
int listen(int sockfd, int backlog);
```

服务端使用`listen`函数将Socket设置为监听状态，等待客户端的连接请求。仅用于流式套接字（`SOCK_STREAM`）TCP。

* `sockfd`：要监听的Socket文件描述符。
* `backlog`：指定连接请求队列的最大长度。

返回值，成功时返回`0`，失败时返回`-1`。

### 建立连接

```CPP
int new_fd = accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

服务端使用`accept`函数阻塞等待客户端连接。一旦有连接，返回一个新的 fd。

* `sockfd`：监听Socket的文件描述符。
* `addr`：返回指向`sockaddr`结构的指针，用于存储客户端的地址信息。
* `addrlen`：返回指向地址结构长度的指针。

原来的`sockfd`继续负责监听新的连接，返回的`new_fd`专门负责和刚才连上来的这个客户端通信。

```C++
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

客户端使用`connect`函数向服务端发起连接请求。

* `sockfd`：要连接的Socket文件描述符。
* `addr`：指向包含服务端地址信息的`sockaddr`结构。
* `addrlen`：地址结构的长度。

返回值，成功时返回`0`，失败时返回`-1`。

### 收发数据

#### TCP Socket

```C++
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

面向连接的Socket（如TCP）使用`send`和`recv`函数进行数据传输。

* `send`函数用于发送数据，`recv`函数用于接收数据。
* `sockfd`：Socket文件描述符。
* `buf`：指向数据缓冲区的指针。
* `len`：要发送或接收的数据长度。
* `flags`：通常设置为`0`。

返回值，成功时返回实际发送或接收的字节数，失败时返回`-1`。

#### UDP Socket

```C++
// 发送给 dest_addr
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);

// 从 src_addr 接收
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
```

无连接的Socket（如UDP）使用`sendto`和`recvfrom`函数进行数据传输。

* `sendto`函数用于发送数据到指定地址，`recvfrom`函数用于从指定地址接收数据。
* `dest_addr`和`src_addr`：指向`sockaddr`结构的指针，分别表示目标地址和源地址。
* `addrlen`：地址结构的长度。

返回值，成功时返回实际发送或接收的字节数，失败时返回`-1`。

### 关闭Socket

```C++
int close(int sockfd);
```

关闭Socket，释放资源。

* `sockfd`：要关闭的Socket文件描述符。

## TCP连接

### 服务端

1. 启动，调用`socket`创建Socket。
2. 调用`bind`绑定IP地址和端口号。
3. 调用`listen`将Socket设置为监听状态。
4. 调用`accept`等待客户端连接，返回新的Socket用于通信。
5. 使用`send`和`recv`进行数据传输。
6. 传输完成后，调用`close`关闭Socket。

### 客户端

1. 启动，调用`socket`创建Socket。
2. 调用`connect`向服务端发起连接请求。
3. 使用`send`和`recv`进行数据传输。
4. 传输完成后，调用`close`关闭Socket。

### 连接建立过程

三次握手（Three-way Handshake）过程：

1. 客户端发送一个`SYN`（同步）包到服务端，表示请求建立连接。
2. 服务端收到`SYN`包后，回复一个`SYN-ACK`包，表示同意建立连接。
3. 客户端收到`SYN-ACK`包后，回复一个`ACK`包，表示连接建立成功。


### 断开连接过程

四次挥手（Four-way Handshake）过程：

1. 客户端发送一个`FIN`（结束）包到服务端，表示请求断开连接。
2. 服务端收到`FIN`包后，回复一个`ACK`包，表示确认断开请求。
3. 服务端发送一个`FIN`包到客户端，表示同意断开连接。
4. 客户端收到`FIN`包后，回复一个`ACK`包，表示连接断开成功。

## UDP通信

### 服务端

1. 启动，调用`socket`创建Socket。
2. 调用`bind`绑定IP地址和端口号。
3. 使用`recvfrom`接收数据。
4. 使用`sendto`发送数据。
5. 传输完成后，调用`close`关闭Socket。

### 客户端

1. 启动，调用`socket`创建Socket。
2. 使用`sendto`发送数据到服务端。
3. 使用`recvfrom`接收服务端的数据。
4. 传输完成后，调用`close`关闭Socket。
