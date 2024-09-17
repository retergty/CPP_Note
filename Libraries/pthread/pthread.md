# pthread

`POSIX`线程是`POSIX`的线程标准，定义了创建和操纵线程的一套API.

参考文档

* [【LinuxC】C语言线程（pthread）](https://blog.csdn.net/weixin_43764974/article/details/136723966)
* [Threads](https://pubs.opengroup.org/onlinepubs/7908799/xsh/threads.html)

## 概念

### 线程实现模型

最常用的实现模型是混合模型，一个进程会分配相对较少的内核调度实体，但是可能会有比较多的线程.所以线程会复用内核调度实体，也就是说：

* 运行时库管理把线程绑定到内核调度实体与解绑.
* 内核只会调度内核调度实体.

### 线程互斥锁Thread Mutexes

当以下情况之一发生时，线程拥有互斥锁`m`：

* 函数`pthread_mutex_lock()`的成功返回
* 函数`pthread_mutex_trylock()`的成功返回
* 函数`pthread_cond_wait()`的返回(无论成功与否，除非某些特定的错误)
* 函数`pthread_cond_timedwait()`的返回(无论成功与否，除非某些特定的错误)

线程拥有互斥锁`m`直到以下情况之一发生时:

* 函数`pthread_mutex_unlock()`的执行.
* 因为函数`pthread_cond_wait()`的阻塞时.
* 因为函数`pthread_cond_timedwait()`的阻塞时.

### 线程调度属性Thread Scheduling Attributes

线程具有属性，这些属性可以通过类型为`pthread_attr_t`的线程属性对象访问或者通过特定函数访问.主要包括`scope`属性、`detach`属性、堆栈地址、堆栈大小、优先级.

线程属性对象`pthread_attr_t`可以通过`pthread_attr_set*`类型函数设置，或者通过`pthread_attr_get*`获取.

线程属性也可以通过`pthread_set*`类型函数在运行时设置，或者通过`pthread_get*`获取.

* `detachstate`，表示新线程是否与进程中其他线程脱离同步。如果设置为`PTHREAD_CREATE_DETACHED`，则新线程不能用`pthread_join()`来同步，且在退出时自行释放所占用的资源。缺省为`PTHREAD_CREATE_JOINABLE`状态。可以在线程创建并运行以后用`pthread_detach()`来设置。一旦设置为`PTHREAD_CREATE_DETACHED`状态，不论是创建时设置还是运行时设置，则不能再恢复到`PTHREAD_CREATE_JOINABLE`状态。
* `schedpolicy`，表示新线程的调度策略，包括`SCHED_OTHER`（正常、非实时）、`SCHED_RR`（实时、轮转法）和`SCHED_FIFO`（实时、先入先出）三种，缺省为`SCHED_OTHER`，后两种调度策略仅对超级用户有效。运行时可以用`pthread_setschedparam()`来改变。
* `schedparam`，一个`struct sched_param`结构，目前仅有一个`sched_priority`整型变量表示线程的运行优先级。这个参数仅当调度策略为实时（即`SCHED_RR`或`SCHED_FIFO`）时才有效，并可以在运行时通过`pthread_setschedparam()`函数来改变，缺省为0。系统支持的最大和最小的优先级值可以用函数`sched_get_priority_max`和`sched_get_priority_min`得到。
* `inheritsched`，有两种值可供选择：`PTHREAD_EXPLICIT_SCHED`和`PTHREAD_INHERIT_SCHED`，前者表示新线程使用显式指定调度策略和调度参数（即`attr``中的值），而后者表示继承调用者线程的值。缺省为PTHREAD_EXPLICIT_SCHED。`
* `scope`，表示线程间竞争CPU的范围.
* `stacksize`,表示程序希望线程分配的最小栈空间大小.

### 线程取消Thread Cancellation

线程取消机制允许一个线程终止其它线程的执行，目标线程（即正在被取消的线程）可以通过多种方式将取消请求挂起，并在收到取消通知后执行特定于应用程序的清理处理。

线程取消是通过取消控制接口管理的,每个线程拥有一个取消状态对象.线程只有可能在特定的取消点或者是线程处于异步可取消的状态时被终止.

#### 取消状态

取消状态对象通常实现为两个比特的位域.

* `Cancelability Enable`,当为`PTHREAD_CANCEL_DISABLE`时，挂起所有的取消请求.默认为`PTHREAD_CANCEL_ENABLE`.
* `Cancelability Type`，当为`PTHREAD_CANCEL_ASYNCHRONOUS`时，线程可能会在任意时刻被终止；当为`PTHREAD_CANCEL_DEFERRED`时，线程只有可能在特定的取消点处被终止.

#### 线程取消清理句柄

每个线程维护一个包含线程取消时执行清理的句柄列表.使用函数`pthread_cleanup_push()`,`pthread_cleanup_pop()`.压入或者弹出句柄.

当取消请求被执行时，列表中的函数句柄将按照`LIFO`顺序一一调用.

当线程调用`pthread_exit()`时，也会调用清理句柄。

## 数据类型

* `pthread_t`,线程标识符.它是一个结构体数据类型，用于唯一标识一个线程.
* `pthread_attr_t`，线程属性类型
* `pthread_mutex_t`，互斥锁类型
* `pthread_cond_t`,条件变量类型
* `pthread_rwlock_t`，读写锁类型

## 常见函数

### 线程操纵函数

* [pthread_create()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_create.html)

  ```C
  int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
      void *(*start_routine)(void*), void *arg);
  ```

  创建一个线程

  `start_routine`函数返回时等价于调用`pthread_exit()`,返回值就是线程退出状态.对于`main()`函数执行的线程，等价于调用`exit()`.

* [pthread_exit()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_exit.html)

  ```C
  void pthread_exit(void *value_ptr);
  ```

  终止当前线程,同时使得`value_ptr`可用于任何使用了对应的`pthread_join`的线程。不会自动地释放进程资源，比如`mutex`,文件描述符等.

  当线程结束时，其栈上的任何变量将不可用，所以`value_ptr`不能指向这些变量.

* [pthread_join()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_join.html)

  ```C
  int pthread_join(pthread_t thread, void **value_ptr);
  ```

  阻塞当前的线程,直到`thread`运行完毕.函数成功返回时，`value_ptr`就是`pthread_exit()`传递的值.

* [pthread_cancel()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_cancel.html)

  ```C
  int pthread_cancel(pthread_t thread);
  ```

  取消指定线程.

* [pthread_kill()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_kill.html)

  ```C
  int pthread_kill(pthread_t thread, int sig);
  ```

  向线程发送信号，如果线程不处理该信号，则按照信号默认的行为作用于整个进程。信号值`0`为保留信号，作用是根据函数的返回值判断线程是不是还活着。

* [pthread_detach()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_detach.html)

  ```C
  int pthread_detach(pthread_t thread);
  ```

  `detach`指定线程.

* [pthread_cleanup_push(),pthread_cleanup_pop()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_cleanup_pop.html)

  ```C
  void pthread_cleanup_push(void (*routine)(void*), void *arg);
  void pthread_cleanup_pop(int execute);
  ```

### 线程属性函数

`pthread_attr_*`类型的函数是操作`pthread_attr_t`线程属性的函数

`pthread_attr_t`可以用于`pthread_create()`传递线程属性.

* [pthread_attr_init(),pthread_attr_destroy()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_attr_init.html)

  ```C
  int pthread_attr_init(pthread_attr_t *attr);
  int pthread_attr_destroy(pthread_attr_t *attr);
  ```

  初始化或者销毁`attr`，初始化为系统默认值.

* [pthread_attr_setdetachstate(),pthread_attr_getdetachstate()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_attr_getdetachstate.html)

  ```C
  int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
  int pthread_attr_getdetachstate(const pthread_attr_t *attr, 
      int *detachstate);
  ```

  设置或者获取`attr`的`detachstate`属性.`dedatched`线程不能被`join()`同步.

* [pthread_attr_setstacksize(),pthread_attr_getstacksize()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_attr_getstacksize.html)

  ```C
  int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
  int pthread_attr_getstacksize(const pthread_attr_t *attr, 
      size_t *stacksize);
  ```

  设置或者获取`attr`的`stacksize`属性.决定了最小会被分配给这个线程的栈空间(按照字节).

* [pthread_getschedparam(),pthread_setschedparam()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_setschedparam.html)

  ```C
  int pthread_getschedparam(pthread_t thread, int *policy,
      struct sched_param *param);
  int pthread_setschedparam(pthread_t thread, int policy,
      const struct sched_param *param);
  ```

  设置或者获取`attr`的`sched_param`属性，用于设置线程优先级.

* [pthread_attr_setschedpolicy(),pthread_attr_getschedpolicy()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_attr_setschedpolicy.html)

  ```C
  int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);
  int pthread_attr_getschedpolicy(const pthread_attr_t *attr, 
      int *policy);
  ```

  获取或者设置`attr`的`schedpolicy`属性，用于设置线程调度属性.

### 互斥锁函数

* [pthread_mutex_init(),pthread_mutex_destroy()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_mutex_init.html)

  ```C
  int pthread_mutex_init(pthread_mutex_t *mutex, 
      const pthread_mutexattr_t *attr);
  int pthread_mutex_destroy(pthread_mutex_t *mutex);
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  ```

  初始化`mutex`,使用`attr`作为属性，`NULL`时为默认属性.初始化完毕后，`mutex`处于已初始化未锁定的状态.

  销毁`mutex`,结束后`mutex`处于未初始化状态.不能销毁已锁定的`mutex`.

* [pthread_mutex_lock(),pthread_mutex_trylock(),pthread_mutex_unlock()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_mutex_lock.html)

  ```C
  int pthread_mutex_lock(pthread_mutex_t *mutex);
  int pthread_mutex_trylock(pthread_mutex_t *mutex);
  int pthread_mutex_unlock(pthread_mutex_t *mutex);
  ```

  取决于`mutex`类型，阻塞或者非阻塞锁定`mutex`.解锁`mutex`.

### 互斥锁属性函数

`pthread_mutexattr_*`类型的函数表示互斥锁的属性

* [pthread_mutexattr_gettype(),pthread_mutexattr_settype()](https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_mutexattr_gettype.html)

  ```C
  int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);
  int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
  ```

  设置或者获取互斥锁类型，可用的类型如下

  `PTHREAD_MUTEX_NORMAL`

  `PTHREAD_MUTEX_ERRORCHECK`

  `PTHREAD_MUTEX_RECURSIVE`

  `PTHREAD_MUTEX_DEFAULT`