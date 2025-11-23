# Type

## Union

`Union`是`Python`类型注解系统中的一个类型，用于表示一个值可以是多种类型中的一种。

### 基本语法

```python
from typing import Union
Union[type1, type2, type3, ...]
```

### 变量注解

```python
from typing import Union

# 表示变量可以是 int 或 str 类型
value: Union[int, str] = 42      # 正确
value = "hello"                   # 也正确
# value = 3.14                    # 类型检查器会警告
```

### 函数参数

```python
from typing import Union

def process_value(value: Union[int, str, list]) -> str:
    """处理可能是整数、字符串或列表的值"""
    if isinstance(value, int):
        return f"数字: {value}"
    elif isinstance(value, str):
        return f"字符串: {value.upper()}"
    elif isinstance(value, list):
        return f"列表长度: {len(value)}"
    else:
        # 理论上不会执行，因为类型已经被 Union 限制
        return "未知类型"

print(process_value(42))         # 数字: 42
print(process_value("hello"))     # 字符串: HELLO
print(process_value([1, 2, 3]))  # 列表长度: 3
```

### 函数返回值

```python
from typing import Union

def parse_input(user_input: str) -> Union[int, float, str]:
    """尝试将输入解析为数字，失败则返回原字符串"""
    try:
        if '.' in user_input:
            return float(user_input)  # 返回 float
        else:
            return int(user_input)    # 返回 int
    except ValueError:
        return user_input             # 返回 str

result1: Union[int, float, str] = parse_input("42")     # int
result2: Union[int, float, str] = parse_input("3.14")   # float
result3: Union[int, float, str] = parse_input("hello")  # str
```

## 元组tuple

元组是一个不可变的有序元素序列，一旦创建就不能修改其中的元素.

### 创建元组

```python
# 方法1：使用圆括号（推荐）
tuple1 = (1, 2, 3, 4, 5)

# 方法2：使用逗号（创建元组）
tuple2 = 1, 2, 3, 4, 5  # 注意：逗号是关键！

# 方法3：使用 tuple() 构造函数
tuple3 = tuple([1, 2, 3, 4, 5])  # 从列表转换
tuple4 = tuple("hello")           # 从字符串转换：('h', 'e', 'l', 'l', 'o')

print(tuple1)  # (1, 2, 3, 4, 5)
print(tuple2)  # (1, 2, 3, 4, 5)
print(tuple3)  # (1, 2, 3, 4, 5)
```

特殊情况下的元组

```python
# 空元组
empty_tuple = ()
print("空元组:", empty_tuple)  # ()

# 单元素元组（必须有逗号！）
single_tuple = (42,)           # 正确：单元素元组
not_a_tuple = (42)             # 错误：这只是整数42
single_tuple2 = 42,            # 正确：逗号创建单元素元组

print("单元素元组:", single_tuple)   # (42,)
print("不是元组:", not_a_tuple)      # 42（整数）
print("单元素元组2:", single_tuple2) # (42,)
print("类型对比:", type(single_tuple), type(not_a_tuple))
```

### 访问元组元素

```python
my_tuple = (10, 20, 30, 40, 50)

# 索引访问
print("第一个元素:", my_tuple[0])    # 10
print("最后一个元素:", my_tuple[-1])  # 50

# 切片操作
print("前三个元素:", my_tuple[:3])   # (10, 20, 30)
print("最后两个元素:", my_tuple[-2:]) # (40, 50)
print("反转:", my_tuple[::-1])      # (50, 40, 30, 20, 10)
```

### 解包元组

```python
# 创建元组
person = ("Alice", 30, "Engineer")

# 解包到变量
name, age, job = person
print(f"姓名: {name}, 年龄: {age}, 职业: {job}")
# 输出: 姓名: Alice, 年龄: 30, 职业: Engineer
```

使用`*`收集多余元素，避免报错

```python
# 使用 * 收集多余元素
numbers = (1, 2, 3, 4, 5, 6)
first, second, *rest, last = numbers
print(f"第一个: {first}, 第二个: {second}, 其余: {rest}, 最后: {last}")
# 输出: 第一个: 1, 第二个: 2, 其余: [3, 4, 5], 最后: 6

# 交换变量（经典用法）
a, b = 10, 20
print(f"交换前: a={a}, b={b}")  # a=10, b=20
a, b = b, a  # 使用元组解包交换值
print(f"交换后: a={a}, b={b}")  # a=20, b=10

```
