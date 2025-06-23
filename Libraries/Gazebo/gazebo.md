# gazebo

`gazebo`是一个物理仿真开源工具，可以方便地进行物理仿真。

## 概念

### 实体Entity

实体指的就是`gazebo`里的对象，比如`model`,`joint`,`link`等。

### 组件Components

组件指的就是`gazebo`里的实体所具有的属性，比如`joint`,`link`等.

## 环境变量

* [Gazebo Components](https://classic.gazebosim.org/tutorials?tut=components)

`gazebo`使用环境变量进行搜索资源，比如模型等.可以在`sdf`中使用`<uri>model://a1_description/meshes/trunk.dae</uri>`,`gazebo`就会使用`GZ_SIM_RESOURCE_PATH`指定的路径来查找.

* `GZ_SIM_RESOURCE_PATH`

多个路径采用冒号分割

## 例子

* [大量的例子与例程](https://github.com/gazebosim/gz-sim)
* [Gazebo Fuel website](https://app.gazebosim.org/fuel/models?page=2&per_page=20)有许多的模型可供使用
