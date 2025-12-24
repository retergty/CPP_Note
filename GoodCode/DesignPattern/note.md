# 设计模式

## 单例模式

单例模式下，一个类只有一个实例，并提供一个全局访问点。

```CPP
class SerialManager 
{
public:
    // 1. 获取唯一实例的接口
    // C++11 规定：静态局部变量的初始化是线程安全的。
    static SerialManager& getInstance() {
        static SerialManager instance; 
        return instance;
    }

    // 2. 删除拷贝构造和赋值操作符 (防拷贝)
    // 否则别人 auto a = SerialManager::getInstance(); 就会复制出一个新对象
    SerialManager(const SerialManager&) = delete;
    void operator=(const SerialManager&) = delete;

    // 业务函数
    void sendData(const char* data) { 
        // 模拟串口发送
    }

private:
    // 3. 构造函数私有化 (防外部 new)
    SerialManager() { 
        // 在这里初始化串口，比如 HAL_UART_Init
    }
    ~SerialManager() {}
};

// 使用方法：
void taskA() {
    SerialManager::getInstance().sendData("Hello A");
}
```

## 工厂模式

工厂模式通过定义一个创建对象的接口，让子类决定实例化哪一个类。工厂方法使一个类的实例化延迟到其子类。

```CPP
#include <memory>

// 1. 抽象产品 (接口)
class IMU {
public:
    virtual void readData() = 0; // 纯虚函数
    virtual ~IMU() = default;    // 虚析构函数必加！防止内存泄露
};

// 2. 具体产品 A (STM32 硬件)
class MPU6050 : public IMU {
public:
    void readData() override { /* 读取 I2C 寄存器 */ }
};

// 3. 具体产品 B (仿真)
class SimIMU : public IMU {
public:
    void readData() override { /* 从 Gazebo 话题获取数据 */ }
};

// 4. 工厂
class IMUFactory {
public:
    enum Type { REAL_HARDWARE, SIMULATION };

    // 返回 unique_ptr，自动管理内存
    static std::unique_ptr<IMU> createIMU(Type type) {
        if (type == REAL_HARDWARE) {
            return std::make_unique<MPU6050>();
        } else {
            return std::make_unique<SimIMU>();
        }
    }
};

// 使用：
auto myImu = IMUFactory::createIMU(IMUFactory::REAL_HARDWARE);
myImu->readData(); // 业务层完全不用管是哪个子类
```

## 观察者模式

观察者模式定义了一种一对多的依赖关系，使得当一个对象状态发生改变时，所有依赖于它的对象都得到通知并被自动更新。

```CPP
#include <vector>
#include <algorithm>

// 1. 观察者接口
class IOdomListener {
public:
    virtual void onPoseUpdate(float x, float y) = 0;
};

// 2. 被观察者 (里程计)
class Odometry {
    std::vector<IOdomListener*> listeners; // 粉丝列表
public:
    void addListener(IOdomListener* l) { listeners.push_back(l); }
    
    // 比如在 10ms 定时器中断里调用这个
    void update(float x, float y) {
        // 通知所有粉丝
        for (auto l : listeners) {
            l->onPoseUpdate(x, y);
        }
    }
};

// 3. 具体的观察者 (比如导航模块)
class Navigator : public IOdomListener {
    void onPoseUpdate(float x, float y) override {
        // 重规划路径...
    }
};
```

## 策略模式

策略模式定义了一系列算法，并将每一个算法封装起来，使它们可以相互替换。策略模式让算法独立于使用它的客户而变化。

```CPP
// 1. 策略接口
class PathPlanner {
public:
    virtual void computePath(Point start, Point end) = 0;
    virtual ~PathPlanner() = default;
};

// 2. 具体策略
class AStarPlanner : public PathPlanner {
    void computePath(Point s, Point e) override { /* A* 实现 */ }
};

class RRTPlanner : public PathPlanner {
    void computePath(Point s, Point e) override { /* RRT 实现 */ }
};

// 3. 上下文 (机器人导航器)
class Navigator {
    std::unique_ptr<PathPlanner> planner; // 持有策略的指针
public:
    // 动态设置策略
    void setStrategy(std::unique_ptr<PathPlanner> newPlanner) {
        planner = std::move(newPlanner);
    }

    void doNavigation(Point s, Point e) {
        if (planner) {
            planner->computePath(s, e); // 多态调用
        }
    }
};

// 使用：
Navigator nav;
nav.setStrategy(std::make_unique<AStarPlanner>()); // 用 A*
// ... 发现进入死胡同 ...
nav.setStrategy(std::make_unique<RRTPlanner>());   // 换 RRT
```

## 状态模式

状态模式允许一个对象在其内部状态改变时改变它的行为，对象看起来似乎修改了它的类。

```CPP
class RobotContext; // 前置声明

// 1. 状态接口
class State {
public:
    virtual void handleStartBtn(RobotContext& ctx) = 0;
    virtual void handleStopBtn(RobotContext& ctx) = 0;
};

// 2. 上下文 (机器人本身)
class RobotContext {
    std::shared_ptr<State> currentState;
public:
    void setState(std::shared_ptr<State> s) { currentState = s; }
    void onStartBtn() { currentState->handleStartBtn(*this); }
    void onStopBtn() { currentState->handleStopBtn(*this); }
};

// 3. 具体状态：待机
class IdleState : public State {
public:
    void handleStartBtn(RobotContext& ctx) override {
        // 切换到运行状态
        ctx.setState(std::make_shared<RunState>()); 
        printf("Starting...\n");
    }
    void handleStopBtn(RobotContext& ctx) override {
        // 待机时按停止没反应
    }
};

// 4. 具体状态：运行中
class RunState : public State {
public:
    void handleStartBtn(RobotContext& ctx) override {
        // 运行时按开始无效
    }
    void handleStopBtn(RobotContext& ctx) override {
        // 切换回待机
        ctx.setState(std::make_shared<IdleState>());
        printf("Stopping...\n");
    }
};
```

## 总结

| 模式 | 核心思想 | 你的应用场景 |
| :--- | :--- | :--- |
| **单例** | **独生子女** | 串口管理器、日志系统、内存池 |
| **工厂** | **外包生产** | 硬件抽象层 (HAL)、多型号传感器适配 |
| **观察者** | **朋友圈发动态** | 传感器数据分发、报警系统 |
| **策略** | **锦囊妙计** | 路径规划算法切换、PID/LQR 控制器切换 |
| **状态** | **变身** | 机器人主流程控制、复杂的按键交互 |