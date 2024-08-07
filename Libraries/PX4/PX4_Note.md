# PX4

`PX4`是一个开源的飞控系统。

## drawbacks

以下总结`PX4`具有的开销，以及可能的提升方法。

### 空间开销

#### 飞行任务过多

在`flight_mode_manager`模块中定义了许多预定义的飞行任务，许多任务都没有被使用，但是却存在于`FlightTaskIndex`中，无法被编译器可达性分析优化掉。

#### 同一个`WorkQueue`

同一个`WorkQueue`的任务`WorkItem`

### 时间开销

#### 主题消息必须要复制出来

主题订阅的消息必须要复制出来，变为线程本地变量才能使用，复制操作带来了时间上的开销。

#### 每次发布或者订阅的函数都要判断是否这个发布或者是订阅合法

```CPP
  /**
   * Update the struct
   * @param dst The uORB message struct we are updating.
   */
  bool update(void *dst)
  {
    if (!valid()) {
      subscribe();
    }

    return valid() ? Manager::orb_data_copy(_node, dst, _last_generation, true) : false;
  }
```

每次都需要重复判断`valid()`带来了时间开销
