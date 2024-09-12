# 工作队列

`PX4`使用工作队列(`Work Queue`)来运行几乎所有的模块任务。它具有以下的特点

* 这些模块都公有继承了`WorkItem`或其子类`ScheduledWorkItem`,这个类给模块类提供了在对应`WorkQueue`运行模块类里的`Run`函数的能力。
* 这些模块必须实现`WorkItem`的纯虚函数`WorkItem::Run()`,这就是会在`WorkQueue`运行的主函数。
* 这些模块的主函数都是`<module_name>_main`,操作系统会分配一个线程，并调用这个函数，这个函数通常完成初始化模块，初始化订阅与发布的任务。
* `WorkItem`与对应的`WorkQueue`相关联，并在特定时候(通常是`SubscriptionCallbackWorkItem`的回调函数内)把`WorkItem`插入到对应的`WorkQueue`的运行队列里去。
* 一个`WorkQueue`便是一个线程，具有优先级，由`wq_config_t`类配置，由`WorkQueueManager`线程在适当情况(`lazy initialization`)下创建。
* 同一个`WorkQueue`的`WorkItem`是顺序运行的，不会发生抢占，所以不需要额外的同步。

总的来说，工作队列就像是软件定时器任务，实现了软中断的功能，也可以说是简单版的协程.

## WorkItem类

```CPP
class WorkItem : public IntrusiveSortedListNode<WorkItem *>, public IntrusiveQueueNode<WorkItem *>
```

* 一个`WorkItem`就是一个要执行的任务，它会与一个`WorkQueue`在**构造函数**阶段关联。
* `WorkItem`没有公有构造函数，不能单独创建，但是有保护构造函数，可以继承它。
* 它公有继承的两个类提供了把它插入到`WorkQueue`的能力

### 关键成员变量

```CPP
WorkQueue *_wq{nullptr};
```

* `_wq`与一个特定的`WorkQueue`相关联，发生在构造函数阶段。

### 关键成员函数

```CPP
explicit WorkItem(const char *name, const wq_config_t &config);
explicit WorkItem(const char *name, const WorkItem &work_item);
```

* `name`就是这个模块的名称，通常是`MODULE_NAME`，会自动定义成模块名称。
* `WorkItem`仅有的构造函数，把`WorkItem`与`config`指定的`WorkQueue`相关联。
* `config`类似于`px4::wq_configurations::nav_and_controllers`，这指定了`nav_and_controllers`的工作队列，这也是除了传感器以外最高的优先级的工作队列。

```CPP
virtual void Run() = 0;
```

* 继承类必须要实现的成员函数，也是在`WorkQueue`运行的任务函数。

```CPP
inline void ScheduleNow()
{
  if (_wq != nullptr) {
    wq->Add(this);
  }
}
```

* 把当前的`WorkItem`加入到`WorkQueue`中，可以在任何时候调用，比如其它模块的`Run`函数，本模块的`Run`函数，中断上下文。通常是由`SubscriptionCallbackWorkItem`调用的，是一个基于消息的异步触发架构。

```CPP
bool WorkItem::Init(const wq_config_t &config)
{
  // clear any existing first
  Deinit();

  px4::WorkQueue *wq = WorkQueueFindOrCreate(config);

  if ((wq != nullptr) && wq->Attach(this)) {
    _wq = wq;
    _time_first_run = 0;
    return true;
  }

  PX4_ERR("%s not available", config.name);
  return false;
}
```

* 初始化`WorkItem`，在构造函数内调用。
* 此时会根据`config`寻找对应的`WorkQueue`，调用函数`WorkQueueFindOrCreate`如果没有找到，则通知给运行`WorkQueueManagerRun`的线程创建一个`WorkQueue`，并把这个`WorkItem`与`WorkQueue`相关联起来。

## ScheduledWorkItem类

是`WorkItem`的派生类，实现了一些调度函数。

## WorkQueue类

```CPP
class WorkQueue : public IntrusiveSortedListNode<WorkQueue *>
```

* 管理`WorkItem`的类，一个`WorkQueue`就是一个线程，
* 它由`WorkQueueManager`类管理，`WorkQueueManager`管理了`WorkQueue`的创建。

### 关键成员变量

```CPP
IntrusiveQueue<WorkItem *>  _q;
const wq_config_t &_config;
BlockingList<WorkItem *>  _work_items;
```

* `_q`就是要处理的任务所在的队列。
* `_work_items`包含所有与之关联的`WorkItem`.

### 关键成员函数

```CPP
void WorkQueue::Run()
{
  while (!should_exit()) {
    // loop as the wait may be interrupted by a signal
    do {} while (px4_sem_wait(&_process_lock) != 0);

    work_lock();

    // process queued work
    while (!_q.empty()) {
      WorkItem *work = _q.pop();

      work_unlock(); // unlock work queue to run (item may requeue itself)
      work->RunPreamble();
      work->Run();
      // Note: after Run() we cannot access work anymore, as it might have been deleted
      work_lock(); // re-lock
    }

#if defined(ENABLE_LOCKSTEP_SCHEDULER)

    if (_q.empty()) {
      px4_lockstep_unregister_component(_lockstep_component);
      _lockstep_component = -1;
    }

#endif // ENABLE_LOCKSTEP_SCHEDULER

    work_unlock();
  }

  PX4_DEBUG("%s: exiting", _config.name);
}
```

* `WorkQueue`线程所运行的主函数，几乎不会退出。
* 等待信号量`_process_lock`的通知,才会执行一遍。
* 执行直到`_q`为空，执行的函数便是每个`WorkItem`的`Run`函数。

```CPP
void WorkQueue::Add(WorkItem *item)
{
  work_lock();

#if defined(ENABLE_LOCKSTEP_SCHEDULER)

  if (_lockstep_component == -1) {
    _lockstep_component = px4_lockstep_register_component();
  }

#endif // ENABLE_LOCKSTEP_SCHEDULER

  _q.push(item);
  work_unlock();

  SignalWorkerThread();
}
```

* 把`WorkItem`加入到工作队列中，之后会异步执行。
* 异步通知`WorkQueue`的线程，使用信号量`_process_lock`从`Run`的等待中继续执行。
* 这个函数通常是在中断上下文中调用的，至少不是在`WorkQueue`的线程。

## 管理线程`WorkQueueManagerRun`

此外，还有一个管理线程，它管理`WorkQueue`的创建，创建新的线程来运行`WorkQueue`.
