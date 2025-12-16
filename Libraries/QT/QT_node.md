# QT

## 信号量与槽

QT的信号与槽机制是一种类型安全的回调机制，用于对象之间的通信。

* 信号（Signal）：当某个事件发生时，对象会发出一个信号。信号可以携带参数。
* 槽（Slot）：槽是一个函数，用于响应信号。槽可以有与信号相同的参数类型。
* 连接（Connect）：使用`QObject::connect`函数将信号与槽连接起来。当信号发出时，连接的槽会被调用。

### 线程安全

QT的信号与槽机制是线程安全的。可以在不同线程中发出信号和连接槽，QT会自动处理线程间的通信。

* 直接连接（Direct Connection）：槽在发出信号的线程中执行，发送者和接受者在同一个线程中。此时`emit`信号后，槽函数立即执行。
* 队列连接（Queued Connection）：槽在接收信号的线程中执行，发送者和接受者在不同的线程中。此时`emit`信号后，槽函数不会立即执行，而是被放入接收线程的事件队列中，等待该线程的事件循环处理。

### 声明与实现

* 声明信号：在类的`signals`部分声明信号。信号只需要声明，不需要实现。
* 声明槽：在类的`public slots`、`protected slots`或`private slots`部分声明槽。槽需要实现。
* 发射信号：使用`emit`关键字发射信号.

```CPP
class MyClassSignal : public QObject {
    Q_OBJECT // 【必须加】
public:
    // ...
signals:
    void mySignal(int value); // 【声明信号】
}
```

```CPP
class MyClassSlot : public QObject {
    Q_OBJECT // 【必须加】
public slots:
    void mySlot(int value) { // 【声明并实现槽】
        // 处理信号
    }
}
```

信号与槽的参数有一下三种情况

* 参数类型完全相同：信号和槽的参数类型和数量完全相同。
* 信号参数多于槽：信号的参数数量多于槽，槽只接收信号的前几个参数。
* 参数类型可转换：信号和槽的参数类型不同，但可以通过隐式转换进行转换。

否则，连接会失败，程序在运行时会输出警告信息。

### 使用示例

#### 使用QT自带的信号与槽

```CPP
// 假设你有一个按钮 btn 和一个窗口 window
QPushButton *btn = new QPushButton("关闭", this);

// 【连接】
// 发射者：按钮
// 信号：被点击了 (clicked)
// 接收者：这个窗口 (this)
// 槽：关闭 (close)
connect(btn, &QPushButton::clicked, this, &QWidget::close);
```

`QPushButton`类继承自`QObject`，它有一个`clicked`信号。当按钮被点击时，会发出这个信号。通过`connect`函数，我们将这个信号连接到`QWidget`的`close`槽。当按钮被点击时，窗口会关闭。

#### 自定义信号与槽

```CPP
class Decoder : public QObject {
    Q_OBJECT // 【必须加】
public:
    // ...
signals:
    // 【定义信号】只声明，不用写函数体！
    void positionChanged(int currentSec); 

public:
    void doWork() {
        // ... 解码逻辑 ...
        int sec = 120; // 假设算出当前是第120秒
        emit positionChanged(sec); // 【发射信号】
};
}
```

在这个例子中，我们定义了一个`Decoder`类，继承自`QObject`。我们使用`signals`关键字定义了一个信号`positionChanged`，它携带一个整数参数。当解码器的工作进展到某个位置时，我们调用`emit positionChanged(sec);`来发出这个信号。

#### 一对多广播模式

```CPP
QPushButton *btnStop = new QPushButton("停止");

// 连接 1：通知解码器
connect(btnStop, &QPushButton::clicked, decoder, &Decoder::stop);

// 连接 2：通知进度条
connect(btnStop, &QPushButton::clicked, progressBar, &QProgressBar::reset);
```

在这个例子中，我们将按钮的`clicked`信号连接到了两个不同的槽：一个是解码器的`stop`槽，另一个是进度条的`reset`槽。当按钮被点击时，两个槽都会被调用，实现了一对多的广播效果。

## QObject

`QObject`是QT中所有对象的基类，给继承它的类提供了对象树、事件处理、信号与槽等功能。

通过继承`QObject`，类可以获得以下功能：

* 对象通信能力: 通过信号与槽机制实现`QObject`之间的通信。
* 对象树管理: `QObject`可以有父对象和子对象，父对象负责管理子对象的生命周期,当父对象被销毁时，所有子对象也会被自动销毁。
* 跨线程生存能力：通过调用 moveToThread(targetThread)，你可以改变这个对象所在的线程
* 反射能力: 通过`QObject`的元对象系统，可以在运行时查询类的信息，如类名、属性、信号和槽等。
* 事件处理能力: `QObject`可以接收和处理事件，如鼠标点击、键盘输入等。
