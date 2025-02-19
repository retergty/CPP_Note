# modules

`PX4`使用模块(modules)来管理程序的功能,每个模块都实现一种功能，彼此独立运行，通过`uORB`通信总线通信。

目前为止，模块使用模块名区分，相同的模块只能有一个实例，不会有多个实例。

## 添加一个新的模块

在`modules/templates/template_module`有一个模板模块参考。

一个模块必须包含`CMakeLists.txt`,`kConfig`文件.`kConfig`用来在`PX4`中注册模块，否则不会编译.

### ModuleBase类

```CPP
template<class T>
class ModuleBase
```

* `ModuleBase`类是所有模块的基类，是纯虚类，采用`CRTP`方法，`T`便是派生类类型
* 提供了模块的基本方法，比如开始，停止，状态命令。

#### 关键成员变量

```CPP
static int _task_id;
static px4::atomic<T *> _object;
```

* `_task_id`存储模块线程号，如果为`-1`意味着模块没有运行，`-2`意味着模块当前由一个`WorkQueue`管理。
* `_object`存储实例化的模块对象指针。

#### main函数

```CPP
static int main(int argc, char *argv[])
{
  if (argc <= 1 ||
      strcmp(argv[1], "-h")    == 0 ||
      strcmp(argv[1], "help")  == 0 ||
      strcmp(argv[1], "info")  == 0 ||
      strcmp(argv[1], "usage") == 0) {
    return T::print_usage();
  }

  if (strcmp(argv[1], "start") == 0) {
    // Pass the 'start' argument too, because later on px4_getopt() will ignore the first argument.
    return start_command_base(argc - 1, argv + 1);
  }

  if (strcmp(argv[1], "status") == 0) {
    return status_command();
  }

  if (strcmp(argv[1], "stop") == 0) {
    return stop_command();
  }

  lock_module(); // Lock here, as the method could access _object.
  int ret = T::custom_command(argc - 1, argv + 1);
  unlock_module();

  return ret;
}
```

* 是所有模块的入口点，必须在模块的`main`方法里调用，通常是`<module_name>_main`函数.
* `start`命令表示启动这个模块，检查当前模块是否已经运行，如果运行，报错。否则实例化一个模块并运行.
* `status`命令打印模块当前状态.
* `stop`命令停止模块运行.
* `custom_command`处理模块独有的命令，比如`commander takeoff`,注意，哪怕是在当前模块已经运行了，`custom_command`也会运行。

### 独占一个线程的类

独占一个线程的模块声明如下

```CPP
class TemplateModule : public ModuleBase<TemplateModule>, public ModuleParams
```

* 继承`ModuleBase<TemplateModule>`获得模块必须的架构
* 继承`ModuleParams`获得定义与使用参数的功能

独占一个线程的类需要实现以下函数

#### task_spawn函数

```CPP
int TemplateModule::task_spawn(int argc, char *argv[])
```

* 在`ModuleBase::main`里调用.
* 需要在这个函数里调用`px4_task_spawn_cmd`,创建一个线程，线程的主函数为`run_trampoline`.
* 使用`px4_task_spawn_cmd`的返回值设置`_task_id`.
* 可以等待`_object`不为空，这意味着模块已经实例化了。
* 如果成功，函数返回`0`.函数返回非零值，同时`_task_id`必须为`-1`.

```CPP
int TemplateModule::task_spawn(int argc, char *argv[])
{
  _task_id = px4_task_spawn_cmd("module",
              SCHED_DEFAULT,
              SCHED_PRIORITY_DEFAULT,
              1024,
              (px4_main_t)&run_trampoline,
              (char *const *)argv);

  if (_task_id < 0) {
    task_id = -1;
    return -errno;
  }

  return 0;
}
```

#### instantiate

```CPP
static T *instantiate(int argc, char *argv[])
```

* 在`run_trampoline`里调用，也就是在新创建的线程里调用。
* 处理传递的参数
* 实例化对应的模块类，通常是`T`,分配内存并返回`T*`.
* 如果错误，返回`nullptr`.

```CPP
TemplateModule *TemplateModule::instantiate(int argc, char *argv[])
{
  int example_param = 0;
  bool example_flag = false;
  bool error_flag = false;

  int myoptind = 1;
  int ch;
  const char *myoptarg = nullptr;

  // parse CLI arguments
  while ((ch = px4_getopt(argc, argv, "p:f", &myoptind, &myoptarg)) != EOF) {
    switch (ch) {
    case 'p':
      example_param = (int)strtol(myoptarg, nullptr, 10);
      break;

    case 'f':
      example_flag = true;
      break;

    case '?':
      error_flag = true;
      break;

    default:
      PX4_WARN("unrecognized flag");
      error_flag = true;
      break;
    }
  }

  if (error_flag) {
    return nullptr;
  }

  TemplateModule *instance = new TemplateModule(example_param, example_flag);

  if (instance == nullptr) {
    PX4_ERR("alloc failed");
  }

  return instance;
}
```

#### run

```CPP
void run() override;
```

* 代码运行的主函数，在新的线程里调用。
* 用户自己保持循环状态，该函数退出就意味着这个线程的退出。

#### custom_command

```CPP
static int custom_command(int argc, char *argv[]);
```

* 自定义的命令行参数,在`ModuleBase::main`中调用。
* 如果不支持，可以直接`return print_usage("unrecognized command");`

```CPP
int TemplateModule::custom_command(int argc, char *argv[])
{
  /*
  if (!is_running()) {
    print_usage("not running");
    return 1;
  }

  // additional custom commands can be handled like this:
  if (!strcmp(argv[0], "do-something")) {
    get_instance()->do_something();
    return 0;
  }
   */

  return print_usage("unknown command");
}
```

#### print_usage

```CPP
static int print_usage(const char *reason = nullptr)
```

* 打印帮助信息的函数，在`ModuleBase::main`中调用。

```CPP
int TemplateModule::print_usage(const char *reason)
{
  if (reason) {
    PX4_WARN("%s\n", reason);
  }

  PRINT_MODULE_DESCRIPTION(
    R"DESCR_STR(
### Description
Section that describes the provided module functionality.

This is a template for a module running as a task in the background with start/stop/status functionality.

### Implementation
Section describing the high-level implementation of this module.

### Examples
CLI usage example:
$ module start -f -p 42

)DESCR_STR");

  PRINT_MODULE_USAGE_NAME("module", "template");
  PRINT_MODULE_USAGE_COMMAND("start");
  PRINT_MODULE_USAGE_PARAM_FLAG('f', "Optional example flag", true);
  PRINT_MODULE_USAGE_PARAM_INT('p', 0, 0, 1000, "Optional example parameter", true);
  PRINT_MODULE_USAGE_DEFAULT_COMMANDS();

  return 0;
}
```

#### `<module_name>_main`

```CPP
extern "C" __EXPORT int template_module_main(int argc, char *argv[]);
```

* 通用的函数入口点，需要使用`extern "C"`。
* 需要调用`ModuleBase::main`.

```CPP
int template_module_main(int argc, char *argv[])
{
  return TemplateModule::main(argc, argv);
}
```

#### CMakeList.txt例子

```CMake
px4_add_module(
  MODULE templates__template_module
  MAIN template_module
  SRCS
    template_module.cpp
  )
```

* 使用`px4_add_module`加入一个模块。

### 由`WorkQueue`管理的模块类

在`examples/work_item`中有例子。

这种类型的模块与其它同为`WorkQueue`的模块共享栈空间。

由`WorkQueue`管理的模块类的声明如下

```CPP
class ControlAllocator : 
public ModuleBase<ControlAllocator>, 
public ModuleParams, 
public px4::ScheduledWorkItem
```

* 继承`ModuleBase<TemplateModule>`获得模块必须的架构
* 继承`ModuleParams`获得定义与使用参数的功能
* 继承`px4::ScheduledWorkItem`表示`WorkItem`.

由`WorkQueue`管理的模块类需要实现以下函数

#### 构造函数

在构造函数的初始化内，初始化`ScheduledWorkItem`

```CPP
ControlAllocator::ControlAllocator() :
  ModuleParams(nullptr),
  ScheduledWorkItem(MODULE_NAME, px4::wq_configurations::rate_ctrl),
  _loop_perf(perf_alloc(PC_ELAPSED, MODULE_NAME": cycle"))
{
  //...
}
```

* 指定`ScheduledWorkItem`所在的`WorkQueue`.
* 此时便会关联`WorkItem`与`WorkQueue`，如果`WorkQueue`还未存在，还会创建它，但不会立即把`WorkItem`插入到`WorkQueue`的运行队列中。

#### task_spawn函数

```CPP
static int task_spawn(int argc, char *argv[])
```

* 在`ModuleBase::main`里调用.
* 读取处理输入的参数
* 实例化一个模块类，或者可以预先实例化，比如全局变量的形式，此时需要注意在`task_spawn`运行前，这个任务不会被插入到`WorkQueue`的运行队列，可以通过定义初始化函数解决。
* 把实例化的模块类存储在`_object`，并设置`_task_id`为`task_id_is_work_queue`，表示这个任务目前由`WorkQueue`管理。
* 初始化创建的模块类。
* 如果这个模块类创建失败，注意销毁创建的实例，并把`_object`和`_task_id`还原。

```CPP
int FlightModeManager::task_spawn(int argc, char *argv[])
{
  FlightModeManager *instance = new FlightModeManager();

  if (instance) {
    _object.store(instance);
    _task_id = task_id_is_work_queue;

    if (instance->init()) {
      return PX4_OK;
    }

  } else {
    PX4_ERR("alloc failed");
  }

  delete instance;
  _object.store(nullptr);
  _task_id = -1;

  return PX4_ERROR;
}
```

#### 初始化函数

```CPP
bool init();
```

* 推荐实现初始化函数，在`task_spawn`内调用。
* 初始化必要的类内外资源，比如`SubscriptionCallbackWorkItem`.

```CPP
bool FlightModeManager::init()
{
  if (!_vehicle_local_position_sub.registerCallback()) {
    PX4_ERR("callback registration failed");
    return false;
  }

  // limit to every other vehicle_local_position update (50 Hz)
  _vehicle_local_position_sub.set_interval_us(20_ms);
  _time_stamp_last_loop = hrt_absolute_time();
  return true;
}

```

#### Run函数

```CPP
void Run() override;
```

* 在`WorkQueue`线程中运行，是模块运行的主函数。
* 当函数运行完毕后，对应的`WorkItem`便已从`WorkQueue`的运行队列中移除。

#### `<module_name>_main`

```CPP
extern "C" __EXPORT int template_module_main(int argc, char *argv[]);
```

* 通用的函数入口点，需要使用`extern "C"`。
* 需要调用`ModuleBase::main`.

```CPP
int template_module_main(int argc, char *argv[])
{
  return TemplateModule::main(argc, argv);
}
```

#### custom_command，print_usage

如同在新线程里的模块一般。

#### CMakeList.txt例子

```CPP
px4_add_module(
  MODULE modules__flight_mode_manager
  MAIN flight_mode_manager
  COMPILE_FLAGS
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
  SRCS
    FlightModeManager.cpp
    FlightModeManager.hpp

    ${CMAKE_CURRENT_BINARY_DIR}/FlightTasks_generated.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/FlightTasks_generated.cpp
  DEPENDS
    px4_work_queue
    WeatherVane
    flighttasks_generated
)
```
