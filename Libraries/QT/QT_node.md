# QT

## 信号与槽

QT的信号与槽机制是一种类型安全的回调机制，用于对象之间的通信。

* 信号（Signal）：当某个事件发生时，对象会发出一个信号。信号可以携带参数。
* 槽（Slot）：槽是一个函数，用于响应信号。槽可以有与信号相同的参数类型。
* 连接（Connect）：使用`QObject::connect`函数将信号与槽连接起来。当信号发出时，连接的槽会被调用。

### 线程安全

QT的信号与槽机制是线程安全的。可以在不同线程中发出信号和连接槽，QT会自动处理线程间的通信。

* 直接连接（Direct Connection）：槽在发出信号的线程中执行，发送者和接受者在同一个线程中。此时`emit`信号后，槽函数立即执行。
* 队列连接（Queued Connection）：槽在接收信号的线程中执行，发送者和接受者在不同的线程中。此时`emit`信号后，槽函数不会立即执行，而是被放入接收线程的事件队列`event loop`中，等待该线程的事件循环处理`exec()`。

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

### 底层实现

QT的信号与槽并非C++标准特性，而是通过QT的元对象系统（Meta-Object System）实现的。QT使用`moc`（Meta-Object Compiler）工具在编译时生成额外的代码，以支持信号与槽机制。

本质是,代码生成器(MOC) + 索引查找表 + 回调函数(Callback)

它比传统的回调函数更灵活、更强大，支持类型安全、参数传递和跨线程通信等功能。

### connect函数

```CPP
static QMetaObject::Connection connect(
    const QObject *sender,           // 发送者指针
    PointerToMemberFunction signal,  // 信号的地址 (例如 &QPushButton::clicked)
    const QObject *receiver,         // 接收者指针
    PointerToMemberFunction method,  // 槽函数的地址 (例如 &MyClass::onClicked)
    Qt::ConnectionType type = Qt::AutoConnection // 连接类型
);
```

* `sender`：发出信号的对象。
* `signal`：信号的地址，通常使用成员函数指针的语法，例如`&QPushButton::clicked`。
* `receiver`：接收信号的对象。
* `method`：槽函数的地址，通常使用成员函数指针的语法，例如`&MyClass::onClicked`。
* `type`：连接类型，默认是`Qt::AutoConnection`，表示自动选择连接类型（直接连接或队列连接）。

#### lambda表达式连接

在现代`QT`中，可以使用`Lambda`表达式连接槽函数，此时`receiver`参数用于connect的生命周期管理，运行的事件循环，逻辑归属。

```CPP
connect(sender, &Sender::dataReady, this, [this](int value) {
    // 处理信号
});
```

#### connect的生命周期

`connect`函数返回一个`QMetaObject::Connection`对象，可以用来管理连接的生命周期。通过调用`QObject::disconnect`函数，可以断开连接。

此外，当`sender`或`receiver`对象被销毁时，QT会自动断开所有与该对象相关的连接，避免了悬挂指针和内存泄漏的问题。

#### MOC

在编译代码前，QT的`moc`工具会扫描源代码，查找包含`Q_OBJECT`宏的类，并为这些类生成额外的代码。这些代码包括信号和槽的实现、元对象信息等。

```CPP
// MyClass.h
class MyClass : public QObject {
    Q_OBJECT
signals:
    void dataReady(int value); // 信号只是声明，没有实现
public slots:
    void processData(int value) { ... }
};
```

`MOC`扫描后，会在生成一个名为`moc_MyClass.cpp`的文件，自动实现`dataReady`函数，并生成一张巨大的元数据表 (Meta Data Table)。

在`moc_xxx.cpp`里，`MOC`生成了一组数组，存储了该类所有信号和槽的名字、参数类型、以及它们的索引`ID`

index | type | name          | parameters
------|------|---------------|----------------
0     | signal| dataReady     | int
1     | slot  | processData   | int

在`connect`时，QT会根据信号和槽的名字，在这张表里查找对应的索引`ID`，然后把它们关联起来。

```CPP
connect(sender, &Sender::dataReady, receiver, &Receiver::processData);
```

1. 校验： 编译器检查`dataReady`和 `processData` 的参数是否匹配
2. 注册： Qt 在`sender`对象内部的一个链表（ConnectionList）中记录信息

在`emit`时，调用了实际上就是调用MOC生成的函数

```CPP
// moc_MyClass.cpp 中自动生成的
void MyClass::dataReady(int value)
{
    // 1. 打包参数
    // 将参数转换为通用指针数组，以便统一处理
    void *_a[] = { 
        nullptr, 
        const_cast<void*>(reinterpret_cast<const void*>(&value)) 
    };
    
    // 2. 调用父类的 activate 函数
    // QMetaObject::activate(发送者, 信号的索引ID, 参数包);
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
```

接下来，`QMetaObject::activate`函数会根据索引ID，在发送者对象的连接链表中查找所有连接的槽，并判断

* 如果是直接连接（Direct Connection），则直接调用槽函数。
* 如果是队列连接（Queued Connection），则将槽函数调用打包成一个事件，放入接收者线程的事件队列中。

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

## 事件循环Event Loop

当调用`QCoreApplication::exec()`时，QT会启动一个事件循环（Event Loop）。事件循环会不断地检查事件队列，并分发事件给相应的对象进行处理。

每个QT线程都有自己的事件循环。主线程默认会启动事件循环，而其他线程需要手动调用`exec()`来启动事件循环。

事件循环的主要作用包括：

1. 系统事件（System Events）：
   * GUI 操作： 鼠标移动、点击、键盘输入、窗口重绘（Paint Event）。
   * 定时器： QTimer 到期了，Event Loop 负责调用连接的槽函数。
   * 网络/IO： Socket 收到数据了（ReadyRead），通知对应对象。
2. 信号与槽（跨线程通信）：
   * 如果从`Thread B`发送一个信号给`Thread A`中的对象（Queued Connection），这个信号会被打包成一个“事件”放入 `Thread A` 的事件队列中。
   * `Thread A`的`Event Loop`转到这一圈时，取出事件，执行对应的槽函数。
3. 自定义事件（Custom Events）：
   * 可以通过`QCoreApplication::postEvent()`向对象发送自定义事件，这些事件也会被放入事件队列中，由事件循环处理。
4. 延迟删除（Deferred Deletion）：
   * 当你调用`obj->deleteLater()`时，对象不会马上销毁，而是向`Event Loop`投递一个删除事件。等当前所有逻辑跑完，回到`Loop`中时再安全删除。

每个QT对象都与一个线程关联，通常是创建它的线程。这个线程负责运行该对象的事件循环，从而处理该对象的事件和信号槽调用。

## 生命周期管理

`QT`通过对象树（Object Tree）来管理对象的生命周期。每个`QObject`对象可以有一个父对象和多个子对象。当父对象被销毁时，所有子对象也会被自动销毁。

```CPP
QObject *parent = new QObject();
QObject *child1 = new QObject(parent); // child1 的父对象是 parent
QObject *child2 = new QObject(parent); // child2 的父对象也是 parent
delete parent; // 删除 parent 会自动删除 child1 和 child2
```

在这个例子中，我们创建了一个父对象`parent`，并创建了两个子对象`child1`和`child2`，它们的父对象都是`parent`。当我们删除`parent`时，`child1`和`child2`也会被自动删除，无需手动管理它们的内存。

子类先于父类被销毁，父类的析构函数会在子类的析构函数之后被调用。这确保了在父类析构时，子类已经被销毁，不会访问到已经销毁的子类对象。
