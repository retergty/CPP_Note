# modules

`PX4`使用模块(modules)来管理程序的功能,每个模块都是一个独立的线程，彼此独立运行，通过`uORB`通信总线通信。

## 添加一个新的模块

在`modules/templates/template_module`有一个模板模块参考。

### 模块类

一个模块通常都由一个`C++`类所管理，这个类管理所有模块的信息并实现模块的功能。模块声明如下

```CPP
class TemplateModule : public ModuleBase<TemplateModule>, public ModuleParams
```

* 继承`ModuleBase<TemplateModule>`获得模块必须的架构
* 继承`ModuleParams`获得定义与使用参数的功能
