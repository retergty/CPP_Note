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
