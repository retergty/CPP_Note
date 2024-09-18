# message queue

`POSIX`消息队列`mq`允许进程间通信.使用时需要包含头文件`<mqueue.h>`

参考文档

* [mq_overview](https://man7.org/linux/man-pages/man7/mq_overview.7.html)

## 描述

消息队列可以认为是一个消息链表，某个进程往一个消息队列中写入消息之前，不需要另外某个进程在该队列上等待消息的达到，这一点与管道和`FIFO`相反。`Posix`消息队列与`System V`消息队列的区别如下：

1. 对`Posix`消息队列的读总是返回最高优先级的最早消息，对`System V`消息队列的读则可以返回任意指定优先级的消息。
2. 当往一个空队列放置一个消息时，`Posix`消息队列允许产生一个信号或启动一个线程，`System V`消息队列则不提供类似的机制。

消息队列通过名字区分，`/somename`,以下划线开头的字符串.两个进程可以通过将相同的名称传递给`mq_open`来对同一队列进行操作。

## 查看系统中的message queue

```shell
mkdir /dev/mqueue
mount -t mqueue none /dev/mqueue
```

挂载完成后,消息队列就可以如同文件一般被操作.

## 常见函数

使用时需要包含头文件

```C
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>
```

* [mq_open()](https://man7.org/linux/man-pages/man3/mq_open.3.html)

  ```C
  mqd_t mq_open(const char *name, int oflag);
  mqd_t mq_open(const char *name, int oflag, mode_t mode,
                     struct mq_attr *attr);
  ```

  创建消息队列,或者是打开一个已有的消息队列.

  `oflag`可以是以下之一

  `O_RDONLY`只接收消息

  `O_WRONLY`只发送消息

  `O_RDWR`收发消息

  此外，`oflag`还可以与以下的标志位或运算.

  `O_CLOEXEC`设置`close-on-exec`标志

  `O_CREAT`如果消息队列没有存在，则创建.创建的消息队列的`user ID`会被设置为调用进程的`user ID`.`group ID`会被设置为调用进程的`group ID`.

  `O_EXCL`和`O_CREAT`连用，表示如果消息队列已存在，则返回`EEXIST`错误.

  `O_NONBLOCK`以非阻塞模式打开消息队列，由于`mq_receive(3)`,`mq_send(3)`可能会阻塞线程，使用这个表示会直接返回`EAGAIN`错误.

  `mode`表示创建的消息队列的使用权限,通常是`S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH`.

  `attr`包含的域如下

  ```C
  struct mq_attr {
      long mq_flags;       /* Flags (ignored for mq_open()) */
      long mq_maxmsg;      /* Max. # of messages on queue */
      long mq_msgsize;     /* Max. message size (bytes) */
      long mq_curmsgs;     /* # of messages currently in queue
                              (ignored for mq_open()) */
  };
  ```

  如果`attr`为`NULL`，则使用默认属性。

  返回消息描述符`mqd_t`

* [mq_send(),mq_timedsend()](https://man7.org/linux/man-pages/man3/mq_send.3.html)

  ```CPP
  #include <mqueue.h>

  int mq_send(mqd_t mqdes, const char msg_ptr[.msg_len],
                size_t msg_len, unsigned int msg_prio);

  #include <time.h>
  #include <mqueue.h>

  int mq_timedsend(mqd_t mqdes, const char msg_ptr[.msg_len],
                size_t msg_len, unsigned int msg_prio,
                const struct timespec *abs_timeout);
  ```

  发送消息，把`msg_ptr`指向的消息加入到`mqdes`中,`msg_len`表示消息的长度必须短于消息属性`attr`中的`mq_msgsize`字段,长度为零的消息也是允许的.
  
  `msg_prio`是一个非负整数，指定了这个消息的优先级。消息在消息队列按照优先级降序排列，同时相同优先级的新消息放置在旧消息后面.

  如果消息队列已经满了，默认情况下`mq_send`会阻塞直到可用，`mq_timedsend`会阻塞特定时间.

* [mq_receive(),mq_timedreceive()](https://man7.org/linux/man-pages/man3/mq_receive.3.html)

  ```CPP
  #include <mqueue.h>

  ssize_t mq_receive(mqd_t mqdes, char msg_ptr[.msg_len],
                    size_t msg_len, unsigned int *msg_prio);

  #include <time.h>
  #include <mqueue.h>

  ssize_t mq_timedreceive(mqd_t mqdes, char *restrict msg_ptr[.msg_len],
                    size_t msg_len, unsigned int *restrict msg_prio,
                    const struct timespec *restrict abs_timeout);
  ```

  `mq_receive()`从消息队列移去最高优先级最旧的消息.并把它放置在`msg_ptr`指向的`buffer`中，`msg_len`表示这个`buffer`的最大长度,它必须不小于比消息属性`attr`中的`mq_msgsize`.如果`msg_prio`非空，还会返回这个消息的优先级.

  如果消息队列为空，默认情况下`mq_send`会阻塞直到可用，`mq_timedsend`会阻塞特定时间.

  返回消息的长度.

* [mq_getattr(),mq_setattr()](https://man7.org/linux/man-pages/man3/mq_setattr.3.html)

  ```CPP
  #include <mqueue.h>

  int mq_getattr(mqd_t mqdes, struct mq_attr *attr);
  int mq_setattr(mqd_t mqdes, const struct mq_attr *restrict newattr,
                struct mq_attr *restrict oldattr);
  ```

  返回或者设置消息队列的属性.

  返回的`oflag`只会包含`O_NONBLOCK`或`0`.

  只能设置属性`O_NONBLOCK`.

* [mq_close()](https://man7.org/linux/man-pages/man3/mq_close.3.html)

  ```CPP
  #include <mqueue.h>

  int mq_close(mqd_t mqdes);
  ```

  关闭消息队列.

* [mq_unlink()](https://man7.org/linux/man-pages/man3/mq_unlink.3.html)

  ```CPP
  #include <mqueue.h>

  int mq_unlink(const char *name);
  ```

  移去`name`指定的消息队列.立即删除指定的消息队列名.对于消息队列所占空间，在`mq_close`后被释放.
