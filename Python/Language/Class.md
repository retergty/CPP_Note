# Class

## 定义类

使用`class`关键字定义，类名通常采用大驼峰命名法(如`MyClass`).

```python
class MyClass:
    """类的文档字符串（可选）"""
    pass  # 空类
```

### 定义构造函数

`__init__`为类的构造函数

```python
class Car:
    def __init__(self, brand, color):
        self.brand = brand
        self.color = color
        self.speed = 0  # 默认值
  
car = Car("Tesla", "red")
print(car.brand)  # 输出 "Tesla"
```

### 定义析构函数

`__del__`为类的析构函数,类被垃圾回收时调用（如手动 del或程序结束）.

```python
class FileHandler:
    def __init__(self, filename):
        self.file = open(filename, 'w')
    
    def __del__(self):
        self.file.close()  # 关闭文件
        print("File closed")

fh = FileHandler("test.txt")
del fh  # 触发 __del__，输出 "File closed"
```

### 类属性

类属性就是类成员变量，分为实例属性和类属性，实例属性为每个实例独有，类属性则是每个类共享。

* 实例属性：通常在`__init__`函数中，通过`self.属性名`定义

    ```python
    class Person:
    def __init__(self, name, age):
        self.name = name  # 实例属性
        self.age = age    # 实例属性

    p = Person("Alice", 30)
    print(p.name)  # 输出 "Alice"（访问实例属性）
    ```

* 类属性：定义在类内部、方法外部，所有实例共享同一内存地址。

    ```python
    class Person:
    species = "Homo sapiens"  # 类属性

    p1 = Person()
    p2 = Person()
    print(p1.species)  # 输出 "Homo sapiens"
    print(p2.species)  # 同上（共享类属性）
    ```

注意，当通过类名直接修改类属性时，所有实例访问该属性时都会看到变化

```python
class MyClass:
    shared_attr = 100  # 类属性

obj1 = MyClass()
obj2 = MyClass()

# 通过类名修改类属性
MyClass.shared_attr = 200  

print(obj1.shared_attr)  # 输出 200（所有实例共享新值）
print(obj2.shared_attr)  # 输出 200
```

但通过类实例修改类属性时，实际上是创建了一个只有当前实例的实例属性。

```python
class MyClass:
    shared_attr = 100

obj1 = MyClass()
obj2 = MyClass()

# 通过实例修改（实际创建实例属性）
obj1.shared_attr = 300  

print(obj1.shared_attr)  # 输出 300（访问的是实例自己的属性）
print(obj2.shared_attr)  # 输出 100（访问的是类属性，未被修改）
print(MyClass.shared_attr)  # 输出 100（类属性本身不变）
```

实例属性优先于类属性，只有当某个名字的实例属性没有定义时，才会查找类属性.

### 方法

方法也分为三种，实例方法，类方法与静态方法。

* 实例方法:最常用，第一个参数必须是`self`(指向当前实例，名称可自定义但通常用 self)，用于操作实例属性。

    ```python
    class Person:
        def __init__(self, name):
            self.name = name

        def greet(self):  # 实例方法
            return f"Hello, I'm {self.name}!"

    p = Person("Bob")
    print(p.greet())  # 输出 "Hello, I'm Bob!"（自动传递 self=p）
    ```

* 类方法:使用`@classmethod`装饰器，第一个参数通常是`cls`（指向当前类），用于操作类属性或创建实例。

    ```python
    class Person:
        count = 0  # 类属性记录实例数量

        @classmethod
        def increment_count(cls):
            cls.count += 1

        def __init__(self):
            Person.increment_count()  # 或直接 cls.count += 1（需传 cls）

    p1 = Person()
    p2 = Person()
    print(Person.count)  # 输出 2
    ```

* 静态方法:使用`@staticmethod`装饰器，无默认参数（self/cls），逻辑上属于类但无需访问类/实例状态

    ```python
    class MathUtils:
        @staticmethod
        def add(a, b):
            return a + b

    print(MathUtils.add(3, 5))  # 输出 8（无需实例化）
    ```

注意类方法也可以通过类实例调用，这是`self`就会转化为`cls`.

## 类继承

```python
class ParentClass:
    # 父类定义
    pass

class ChildClass(ParentClass):  # 单继承
    # 子类定义
    pass

class MultiChildClass(ParentClass1, ParentClass2):  # 多继承
    # 子类定义
    pass
```

会继承父类的方法与属性

### MRO顺序

`MRO(Method Resolution Order)`是Python中确定类继承体系中方法查找顺序的算法。它定义了当调用一个方法时，Python解释器按照什么顺序在类层次结构中查找该方法。

查看`mro`

```python
class A: pass
class B(A): pass
class C(A): pass
class D(B, C): pass

print(D.mro())
# 输出: [<class '__main__.D'>, 
#        <class '__main__.B'>, 
#        <class '__main__.C'>, 
#        <class '__main__.A'>, 
#        <class 'object'>]

print(D.__mro__)  # 同上
```

方法解析顺序为

* 子类优先于父类
* 同级别类中按声明顺序
* 保持单调性（子类不会在父类之前出现）

属性查找机制为

* 实例属性字典 __dict__
* 类属性字典
* 父类属性（按MRO顺序）
* _getattr__方法（如果定义）

```python
class Parent:
    attr = "Parent Attribute"

class Child(Parent):
    attr = "Child Attribute"  # 隐藏父类同名属性

print(Child.attr)  # 输出: Child Attribute
print(Parent.attr)  # 输出: Parent Attribute
```

### 方法调用

#### 直接调用

使用`父类名.方法名(self, 其他参数)`便可调用实例方法

```python
class Parent:
    def greet(self):
        print("Hello from Parent")

class Child(Parent):
    def greet(self):
        Parent.greet(self)  # 直接调用父类Parent的greet方法
        print("Hi from Child")

child = Child()
child.greet()
# 输出：
# Hello from Parent
# Hi from Child
```

使用`父类名.方法名(cls, 其他参数)`便可调用类方法.

```python
class Parent:
    @classmethod
    def class_greet(cls):
        print(f"Hello from {cls.__name__} (Parent)")

class Child(Parent):
    @classmethod
    def class_greet(cls):
        Parent.class_greet(cls)  # 直接调用父类类方法
        print(f"Hi from {cls.__name__} (Child)")

Child.class_greet()
# 输出：
# Hello from Child (Parent)
# Hi from Child (Child)
```

#### super()方法

`super()`是一个内置函数，用于在子类中调用父类（超类）的方法。它的核心作用是动态查找并调用父类（或MRO顺序中下一个类）的方法.同时可以确保所有父类方法只会被调用一次，避免了菱形继承的问题.

```python
class Parent:
    def greet(self):
        print("Hello from Parent")

class Child(Parent):
    def greet(self):
        super().greet()  # 调用父类的greet
        print("Hi from Child")  # 扩展新逻辑

child = Child()
child.greet()
# 输出：
# Hello from Parent
# Hi from Child
```

### 初始化与构造过程

```python
class Parent:
    def __init__(self, name):
        self.name = name
        print("Parent initialized")

class Child(Parent):
    def __init__(self, name, age):
        super().__init__(name)  # 必须显式调用
        self.age = age
        print("Child initialized")

c = Child("Alice", 10)
# 输出:
# Parent initialized
# Child initialized
```

子类必须显式调用父类的`__init__`函数
