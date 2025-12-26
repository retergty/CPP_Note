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

### 监听多路复用

#### select函数

```C++
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
```

`select`函数用于监听多个Socket的状态变化，实现多路复用。

* `nfds`：监听的文件描述符数量，通常设置为最大文件描述符加一。
* `readfds`：监听可读事件的文件描述符集合。
* `writefds`：监听可写事件的文件描述符集合。
* `exceptfds`：监听异常事件的文件描述符集合。
* `timeout`：指定等待的时间，`NULL`表示无限等待。

#### pool函数

```C++
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

`poll`函数用于监听多个Socket的状态变化，实现多路复用。

* `fds`：指向`pollfd`结构数组的指针，包含要监听的文件描述符和事件类型。
* `nfds`：监听的文件描述符数量。
* `timeout`：指定等待的时间，单位为毫秒，`-1`表示无限等待。

`struct pollfd`结构体：

```C++
struct pollfd {
    int   fd;         // 要监听的文件描述符
    short events;     // 要监听的事件类型 (掩码)
    short revents;    // 实际发生的事件类型 (由内核填写)
};
```

#### poll工作流

1. 拷贝：用户将一个包含所有关注 FD 的数组 (struct pollfd *) 拷贝到内核空间。
2. 挂起：内核遍历这个数组，如果当前没有 FD 就绪，进程进入睡眠。
3. 唤醒：当有设备数据到来（如网卡收包），驱动程序唤醒进程。
4. 再次遍历：内核再次遍历整个数组，标记哪些 FD 有数据（revents 字段），然后把整个数组拷贝回用户态。
5. 用户检查：用户拿到数组，还是不知道具体谁有数据，必须再遍历一次数组检查 revents。

#### epoll函数

##### epoll_create

```C++
int epoll_create(int size);
```

创建一个`epoll`实例。

#### epoll_ctl

```C++
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
```

控制`epoll`实例，添加、修改或删除监听的文件描述符。

* `epfd`：`epoll`实例的文件描述符。
* `op`：操作类型，如`EPOLL_CTL_ADD`（添加）、`EPOLL_CTL_MOD`（修改）、`EPOLL_CTL_DEL`（删除）。
* `fd`：要操作的文件描述符。
* `event`：指向`epoll_event`结构的指针，包含事件类型和用户数据。

核心结构体：

```C++
struct epoll_event {
    uint32_t     events;      // 监听什么事件？(掩码)
    epoll_data_t data;        // 附带数据 (通常存 fd 本身)
};
```

* `events`常用的值
  * `EPOLLIN`：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）。
  * `EPOLLOUT`：表示对应的文件描述符可以写。
  * `EPOLLERR`：表示对应的文件描述符发生错误。
  * `EPOLLHUP`：表示对应的文件描述符被挂断。
* `data`这是一个联合体 (Union)。最常用的是`data.fd = fd`，这样当事件发生时，内核会把这个`fd`原样还返还。

##### epoll_wait

```C++
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

阻塞并等待事件的发生。

* `epfd`：`epoll`实例的文件描述符。
* `events`：指向`epoll_event`结构数组的指针，用于存储发生的事件。内核会把发生的事件复制到这个数组里给你。
* `maxevents`：`events`数组的大小。
* `timeout`：等待的时间，单位为毫秒，`-1`表示无限等待。

返回值，成功时返回发生事件的文件描述符数量，失败时返回`-1`。

##### epoll的模式

1. 水平触发（Level-Triggered，`LT`）：默认模式，当文件描述符可读或可写时，`epoll_wait`会持续返回该事件，直到事件被处理。
2. 边缘触发（Edge-Triggered，`ET`）：高效模式，当文件描述符状态发生变化时，`epoll_wait`只返回一次该事件，需要非阻塞读取或写入数据，直到返回`EAGAIN`错误。

### epoll工作流

1. `epoll_create`： 在内核开辟一块空间，创建一个红黑树（用于存所有监控的`FD`）和一个双向链表（用于存就绪的`FD`）。
2. `epoll_ctl (Add/Mod/Del)`：当你想要监控某个`FD`时，内核把它插入红黑树中。同时，内核会向该`FD`对应的设备驱动注册一个回调函数 (`Callback`)。
    * 关键点：这个回调函数是性能的核心。当网卡收到数据，驱动会调用这个回调，把该`FD`直接扔到就绪链表里。
3. `epoll_wait`： 进程只需要检查就绪链表是否为空。
    * 如果不空，直接把链表里的`FD`拷贝给用户。
    * 用户拿到的全是“有数据”的`FD`，不需要挨个找。

#### epoll和select/poll的区别

1. 性能：`epoll`在处理大量文件描述符时性能更好，`select`和`poll`在文件描述符数量增加时性能下降。
2. 可扩展性：`epoll`没有文件描述符数量的限制，而`select`有最大文件描述符数量的限制（通常为1024）。
3. 事件通知方式：`epoll`支持边缘触发模式，而`select`和`poll`仅支持水平触发模式。

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

## 常见问题

### 粘包

假设发送端发送了两次数据：

```CPP
// 发送端
send(fd, "Hello", 5, 0); // 第一次发 5 字节
send(fd, "World", 5, 0); // 第二次发 5 字节
```

接受端可能会一次性接收到10字节的数据：

```CPP
// 接收端
char buffer[20];
int n = recv(fd, buffer, sizeof(buffer), 0); // 可能一次性接收到 "HelloWorld"
``` 

粘包是指在TCP通信中，多个数据包被合并在一起发送，导致接收方无法正确区分每个数据包的边界。

TCP只保证有序的**字节流**，不保证**消息边界**。因此，接收方需要自行处理粘包问题。

常见的解决方法包括：

* 定长消息：每个消息固定长度，接收方按固定长度读取数据。
* 消息分隔符：在每个消息末尾添加特殊的分隔符，接收方根据分隔符拆分消息。
* 消息头：在每个消息前添加消息长度信息，接收方先读取长度，再读取对应长度的数据。

### 拆包

假设发送端发送了一个较大的数据包：

```CPP
// 发送端
send(fd, large_data, large_data_size, 0); // 发送一个大数据包
```

接受端可能会分多次接收到数据：

```CPP
// 接收端
char buffer[20];
int n1 = recv(fd, buffer, sizeof(buffer), 0); // 第一次接收部分数据
int n2 = recv(fd, buffer + n1, sizeof(buffer) - n1, 0); // 第二次接收剩余数据
```

拆包是指在TCP通信中，一个大数据包被分割成多个小数据包发送，导致接收方需要多次读取才能完整接收到一个消息。
