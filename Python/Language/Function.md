# Function

## 定义函数

### 基本语法

```python
def greet(name):
    """返回问候语"""
    return f"Hello, {name}!"

print(greet("Alice"))  # 输出: Hello, Alice!
```

### 嵌套函数

```python
def outer_function(x):
    """外部函数"""
    def inner_function(y):
        """内部函数，可以访问外部函数的变量"""
        return x + y
    
    return inner_function

closure = outer_function(10)
result = closure(5)  # 15
```

### 装饰器函数

装饰器本质上是一个接受函数作为参数并返回一个新函数的函数。可以在不改变原函数的情况下，为函数添加额外的功能。

```python
def my_decorator(func):
    def wrapper():
        print("函数执行前")
        func()  # 执行原函数
        print("函数执行后")
    return wrapper

@my_decorator
def say_hello():
    print("Hello!")

say_hello()
```

### 函数重载

一般情况下python不支持函数重载,后定义的函数会覆盖之前定义的函数

```python
def greet(name):
    return f"Hello, {name}!"

def greet(name, greeting):  # 这会覆盖前面的函数
    return f"{greeting}, {name}!"

# 调用
print(greet("Alice", "Hi"))  # 正常工作
print(greet("Bob"))          # TypeError: 缺少参数
```

一般就是定义一个函数，在这个函数中统一处理.

### @overload标签

`@overload`是`Python`类型注解系统的一部分，用于为同一个函数提供多个类型签名，帮助类型检查器和`IDE`更好地理解函数的不同使用方式。它不提供运行时行为，只是方便IDE补全.

```python
from typing import overload, Union

@overload
def process_data(data: int) -> str:
    ...

@overload
def process_data(data: str) -> int:
    ...

@overload
def process_data(data: list[int]) -> float:
    ...

def process_data(data: Union[int, str, list[int]]) -> Union[str, int, float]:
    """实际实现函数"""
    if isinstance(data, int):
        return f"数字: {data}"
    elif isinstance(data, str):
        return len(data)
    elif isinstance(data, list):
        return sum(data) / len(data) if data else 0.0
    else:
        raise TypeError("不支持的数据类型")

# 使用示例
result1 = process_data(42)        # IDE 知道返回 str
result2 = process_data("hello")    # IDE 知道返回 int  
result3 = process_data([1, 2, 3])  # IDE 知道返回 float
```

## 参数

### 传递参数的两种表现

默认`Python`传递参数是引用传递，但对于不可变对象，则会创建对象的副本.

对于不可变对象

```python
def modify_number(x):
    print(f"传入时 x 的id: {id(x)}")  # id 相同，指向同一个对象
    x = x + 10  # 因为整数不可变，这里创建了一个新对象，x 指向了新对象
    print(f"修改后 x 的id: {id(x)}")  # id 已改变
    print(f"函数内 x 的值: {x}")

num = 5
print(f"函数外 num 的id: {id(num)}")
modify_number(num)
print(f"函数外 num 的值: {num}")  # 输出 5，没有被改变
```

对于可变对象

```python
def modify_list(my_list):
    print(f"传入时 my_list 的id: {id(my_list)}")  # id 相同，指向同一个对象
    my_list.append(4)  # 原地修改列表，没有创建新对象
    my_list[0] = 99    # 原地修改列表元素
    print(f"修改后 my_list 的id: {id(my_list)}")  # id 不变
    print(f"函数内 my_list: {my_list}")

original_list = [1, 2, 3]
print(f"函数外 original_list 的id: {id(original_list)}")
modify_list(original_list)
print(f"函数外 original_list: {original_list}")  # 输出 [99, 2, 3, 4]，已被改变
```

但对于可变对象，重新赋值却会创建对象的副本，相当于创建了一个本地变量，不会影响原对象

```python
def clear_list(my_list):
    my_list = []  # 重新赋值，my_list 现在指向一个新的空列表对象，不影响外部实参

def clear_list_in_place(my_list):
    my_list.clear()  # 原地清空，修改了原对象，影响外部实参

lst = [1, 2, 3]
clear_list(lst)
print(lst)  # 输出 [1, 2, 3]，未被清空

clear_list_in_place(lst)
print(lst)  # 输出 []，已被清空
```

### 参数类型

`python`的参数类型有如下几种.其中可以组合使用.

#### 位置参数

按照指定顺序的形参传入

```python
def greet(name, greeting):
    print(f"{greeting}, {name}!")

greet("Alice", "Hello")  # 正确
# greet("Hello", "Alice") # 错误：顺序不对，逻辑混乱
```

#### 默认参数

在定义函数时为函数指定一个默认值.默认参数**必须**指向不可变对象。

```python
# 错误示范：使用可变对象作为默认值
def bad_append(item, my_list=[]):
    my_list.append(item)
    return my_list

print(bad_append(1))  # 输出 [1]
print(bad_append(2))  # 输出 [1, 2]！因为默认列表是同一个对象

# 正确做法：使用 None 替代
def good_append(item, my_list=None):
    if my_list is None:
        my_list = []  # 每次调用都创建一个新列表
    my_list.append(item)
    return my_list
```

#### 可变位置参数(*args)

在形参前加一个星号`*`,可以接受任意数量的参数,这些参数在函数内部被封装成一个元组`tuple`

```python
def add_all(*args):
    print(f"args 的类型是: {type(args)}")  # <class 'tuple'>
    total = 0
    for num in args:
        total += num
    return total

result = add_all(1, 2, 3, 4, 5)
print(result)  # 输出 15
```

#### 可变关键字参数(**kwargs)

在形参前加两个星号`**`,可以接收任意数量的关键字参数。这些参数在函数内部被封装成一个字典`dir`.

```python
def print_info(**kwargs):
    print(f"kwargs 的类型是: {type(kwargs)}")  # <class 'dict'>
    for key, value in kwargs.items():
        print(f"{key}: {value}")

print_info(name="Alice", age=30, city="New York")
```

#### 混合使用的情况

如果混合使用那么在`*`号后面的参数，只能通过关键字的方式指定。

```python
def func(a, b, *, c, d): # c 和 d 是仅限关键字参数
    print(a, b, c, d)

func(1, 2, c=3, d=4)  # 正确
# func(1, 2, 3, 4)    # 错误：c 和 d 必须用关键字传递

def func2(a, *args, option=True): # option 是仅限关键字参数
    print(a, args, option)
```

### 传递参数

最简单的方式就是位置传参，按照形参顺序传递参数.这种情况下，不需要额外的信息.

```python
add_all(1, 2, 3, 4, 5)
```

还能使用关键字传参，直接指定形参名字,顺序可以任意.

```python
def func(a, b, c):
    print(a, b, c)

func(a=1, c=3, b=2)  # 输出 1 2 3
```

混合使用下,位置参数必须在关键字参数之前

```python
func(1, c=3, b=2)  # 正确
# func(a=1, 2, 3)  # 语法错误
```

使用`*`解包可迭代对象.将一个列表、元组等可迭代对象解包成多个位置参数.

```python
my_list = [1, 2, 3]
func(*my_list)  # 等价于 func(1, 2, 3)
```

使用`**`解包字典.将一个字典解包成关键字参数.

```python
my_dict = {'a': 1, 'b': 2, 'c': 3}
func(**my_dict)  # 等价于 func(a=1, b=2, c=3)
```

### 类型注解

可以在形参后面添加冒号+类型，表示这个形参所期望的类型.这只是一个注解，不会强制要求满足.

```python
from typing import List, Dict, Optional, Union

def process_data(
    name: str,                   # 字符串
    age: int,                    # 整数  
    scores: List[float],         # 浮点数列表
    metadata: Dict[str, any],    # 字典，键为字符串，值为任意类型
    description: Optional[str] = None,  # 可选的字符串（可能是 None）
    timeout: Union[int, float] = 30     # 可以是整数或浮点数
) -> bool:                       # 返回布尔值
    """处理用户数据"""
    # 函数实现...
    return True
```

## 返回值

### 返回一个值

```python
def function_name(parameters):
    # 函数体
    return value  # 返回一个值
```

### 无返回值

```python
def print_hello(name):
    print(f"Hello, {name}!")
    # 没有 return 语句，默认返回 None

def explicit_none():
    print("执行一些操作")
    return None  # 显式返回 None
    # 或者直接省略 return

result = print_hello("Alice")
print(result)  # 输出: None
```

### 返回复杂对象

```python
def create_person():
    return {
        "name": "Alice",
        "age": 30,
        "city": "New York"
    }

def create_list():
    return [1, 2, 3, 4, 5]

def create_custom_object():
    class Person:
        def __init__(self, name):
            self.name = name
    return Person("Bob")

person = create_person()
numbers = create_list()
obj = create_custom_object()
```

### 返回多个值（元组）

```python
def min_max(numbers):
    """返回最小值和最大值"""
    return min(numbers), max(numbers)  # 实际上是返回一个元组

# 接收返回值
result = min_max([3, 1, 4, 1, 5, 9, 2])
print(result)           # (1, 9)
print(type(result))     # <class 'tuple'>

# 解包接收
minimum, maximum = min_max([3, 1, 4, 1, 5, 9, 2])
print(f"最小值: {minimum}, 最大值: {maximum}")
```

可以使用`*`接收其余的值

```python
state, reward, terminated, truncated, info = env.step(action)
state, reward, terminated, *rest = env.step(action)
```

## 生成器函数

生成器函数使用`yield`语句返回一个迭代器，每次调用都会从上次离开的地方继续执行。适用于处理大量数据或无限序列。

```python
def count_up_to(n):
    count = 1
    while count <= n:
        yield count  # 暂停并返回当前值
        count += 1
# 使用生成器
for number in count_up_to(5):
    print(number)
```

当调用生成器函数时，并不会立即执行函数体，而是返回一个生成器对象。每次迭代时，函数体会从上次`yield`语句处继续执行，直到遇到下一个`yield`或函数结束。

函数结束时，生成器会引发`StopIteration`异常，由`for`循环自动处理。
