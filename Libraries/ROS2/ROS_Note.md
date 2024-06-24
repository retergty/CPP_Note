# ROS Note

本文介绍ROS系统以及它的关键概念与关键部件。

## 关键概念

参考文档

* [Concepts](https://docs.ros.org/en/jazzy/Concepts.html)

### Domain ID

参考文档

* [The ROS_DOMAIN_ID](https://docs.ros.org/en/jazzy/Concepts/Intermediate/About-Domain-ID.html)

DDS通信中间件使用域ID(Domain ID)把一个共享的物理网络分成几个逻辑上的网络域。在同一个域内的ROS2节点可以自由地发现彼此并发送消息，而不同域上的节点则不能。默认所有的节点都使用域ID 0.域ID可以避免在同一个网络上的不同组之间的干扰。

DDS是网络端口，协议为UDP进行通信的，域ID用于计算同一个域中的节点通信应该使用的端口号，由于端口号为`32768-60999`是临时端口号，进程不应该持续地占用它，所以域ID`0-101`和`215-232`可以安全地使用。

每当一个ROS2进程创建时，都会有一个DDS参与者(participant)被创建。每个DDS参与者会占据两个端口，所以如果在同一个域中创建多于120个ROS2进程，进程占据的端口号可能超出了域限定的范围，占据到了下一个域的端口号，是否会破坏程序的运行取决于是否另一个域有进程。

ROS2通信中间件本质上是网络通信，所以也会使用网络通信，默认传输层协议是`UDP`，监听的地址是`0.0.0.0`.

域`1`使用`7650`,`7651`作为组播端口，域`2`使用`7900`,`7901`.

在域`1`中创建第`1`个进程（第`0`个参与者）时，端口`7660`和`7661`用于单播,在域`1`中创建第`120`个进程（第`119`个参与者）时，端口`7898`和`7899`用于单播。在域`1`中创建第`121`个进程（第`120`个参与者）时，端口`7900`和`7901`用于单播并与域`2`重叠。

设置进程域ID的方法就是设置环境变量`ROS_DOMAIN_ID`

```shell
export ROS_DOMAIN_ID=<your_domain_id>
```

### Node

节点(Node)是ROS2抽象概念，每个节点实现单一的模块化的功能，比如控制车轮或者从传感器接收数据并发送。每个节点都可以通过主题(topics)、服务(services)、操作(actions)或参数(parameters)从其他节点发送和接收数据。

一个完全的机器人控制系统是由许多节点组成。在`ROS2`中，一个可执行文件可以包含一个或者多个节点。

![Node](Picture/Nodes-TopicandService.gif)

### Discovery

节点的发现通过`ROS2`的底层中间件自动发生的，不需要用户参与。总结如下：

1. 当一个节点启动时，它会向网络上具有相同`ROS`域（使用`ROS_DOMAIN_ID`环境变量设置）的其它节点通告其存在。其它节点使用有关其自身的信息响应这个通告，从而可以建立起合适的连接，节点间可以互相通信。
2. 节点会定期通告其存在，以便即使在初始发现期之后也可以与新发现的节点建立连接。
3. 当节点离线时会向其他节点通告。

仅当节点具有兼容的`Quality of Service`时，它们才会与其他节点建立连接。

### Interfaces

ROS进程通常使用以下三种接口进行互相通信：主题(topic),服务(service)，动作(action).ROS2使用简化的描述语言——接口定义语言(IDL)来描述这些接口。这种描述使得ROS工具可以自动生成多种目标语言的接口类型的源代码。

三种接口都有独特的扩展名：

* `.msg`文件是简单的文本文件，包含描述`ROS`消息的字段，它用于生成不同语言的消息的源代码。
* `.srv`文件描述了一个服务，由两个部分组成:请求(request)与响应(response)。每个部分本身就是一个消息声明.
* `.action`文件描述了一个动作，由三个部分组成：目标(goal)，结果(result)，反馈(feedback).每个部分本身就是一个消息声明.

### Messages

消息(Messages)是ROS2节点通过网络发送数据给另一个节点的方法，不会返回响应。比如，如果`ROS2`的一个节点读取了温度传感器，那么它就可以使用`Temperature`消息发布数据，其他节点可以订阅这个数据从而接收到`Temperature`消息。

消息使用`.msg`文件描述并定义，这些文件统一存放在`ROS`包中的`msg/`文件夹里，`.msg`文件由两个区域组成，域(`fields`)与常数(`constants`).

### message fields

每一个域由一个类型与名字组成，使用空格分隔，格式如下

```msg
fieldtype1 fieldname1
fieldtype2 fieldname2
fieldtype3 fieldname3
```

比如

```msg
int32 my_int
string my_string
```

还可以引用已定义好的消息类型,只需要把消息类型当作域类型声明即可，如果是其它包的消息定义，还需要加上包名。

```msg
another_pkg/AnotherMessage msg
CustomMessageDefinedInThisPackage value
```

#### 域类型

域类型可以是

* 内置类型
* 自己定义的消息描述的名称，例如“geometry_msgs/PoseStamped”

![build-in](Picture/Built-in-types_currently_supported.png)

每个内置类型都可以用来定义数组.

![define array](Picture/define_arrays.png)

#### 域名

域名由小写字母和下划线组成，且下划线不能出现在域名头部与尾部，也不能有两个连续的下划线。

#### 域默认值

可以给域设置默认值，但目前不支持字符串数组与复合类型的域默认值。

格式如下

```msg
fieldtype fieldname fielddefaultvalue
```

比如

```msg
uint8 x 42
int16 y -2000
string full_name "John Doe"
int32[] samples [-200, -100, 0, 100, 200]
```

### message constants

消息里的常量指的就是程序无法改变的数字，格式为，常量必须使用大写字母定义

```msg
constanttype CONSTANTNAME=constantvalue
```

比如

```msg
int32 X=123
int32 Y=-123
string FOO="foo"
string EXAMPLE='bar'
```

### Topic

主题(Topic)是ROS2抽象通信概念，主题是ROS系统中关键的元素，用于在节点中交换信息。

![Topic](Picture/Topic-SinglePublisherandSingleSubscriber.gif)

向主题发送讯息(messages)的节点叫做发布者(Publisher),从主题接收讯息(messages)的节点叫做订阅者(Subscriber),一个节点可以同时是任意多的主题的发布者与任意多的主题的订阅者。一个主题也可以有任意多的发布者与任意多的订阅者。

### Service

服务(Topic)是ROS2抽象通信概念，实现类似于网络服务器般的功能,用于在节点中交换信息。

![Service](Picture/Service-MultipleServiceClient.gif)

服务是基于请求(call)-响应(response)模型的。发送请求的叫做客户端(client)，发送响应的叫做服务器(server),同一个服务可以有多个客户端，但是只能有一个服务器。只有当客户端发送请求后，服务器才会发送响应（也有可能不发送响应），并且请求响应是一个节点对一个节点的，也就是说，其它客户端收不到这个响应。

请求和响应可能会带有负载数据，但也可能不带有，具体情况有服务的类型定义。

服务是通过`.srv`文件描述并定义的，统一保存在ROS包中的`srv/`目录中。

一个`.srv`文件由请求与响应两个部分组成，每个部分都是一个`msg`类型，使用`---`分隔。任意两个以`---`连接的`.msg`文件都是合法的服务描述.空的`msg`部分则表示没有对应的请求与响应数据负载。格式如下

```srv
string str
---
string str
```

```srv
# request constants
int8 FOO=1
int8 BAR=2
# request fields
int8 foobar
another_pkg/AnotherMessage msg
---
# response constants
uint32 SECRET=123456
# response fields
another_pkg/YetAnotherMessage val
CustomMessageDefinedInThisPackage value
uint32 an_integer
```

### Action

动作(Actions)是ROS2抽象通信概念，适用于长时间运行的通信任务，用于在节点中交换信息。它由三个部分组成，目标(goal),反馈(feedback),结果(result).

![actions](Picture/Action-SingleActionClient.gif)

动作是建立在主题和服务上的。它的功能类似于服务，但是动作可以被取消.动作可以提供持续的反馈，而服务只有一次的响应。

动作使用客户端-服务器(client-server)服务器，动作客户端节点发送一个目标给动作服务器，动作服务器接收到这个目标并返回了一个持续的反馈和一个结果。

`.action`文件描述了一个动作，由三个部分组成：目标(goal)，结果(result)，反馈(feedback).每个部分本身就是一个消息声明.空的`msg`部分则表示没有对应的数据负载。格式如下

```act
<request_type> <request_fieldname>
---
<response_type> <response_fieldname>
---
<feedback_type> <feedback_fieldname>
```

比如`Fibonacci`动作定义如下

```act
int32 order
---
int32[] sequence
---
int32[] sequence
```

这是一个动作定义，当动作客户端发送一个`int32`的包含斐波那契数列阶数的请求，并期望服务器返回一个数组，包含每一步的计算结果，同时还实时反馈回中间的计算数组。

### parameters

参数(parameters)是节点可配置的属性，节点可以支持整数，浮点数，布尔数，字符串和列表类型的参数，