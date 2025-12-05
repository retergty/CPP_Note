# 内置函数

本文总结`python`的内置函数

## range()

生成一个不可变的整数序列，惰性求值，不立即生成所有数字。

```python
range(stop)              # 从 0 开始，到 stop-1 结束
range(start, stop)       # 从 start 开始，到 stop-1 结束
range(start, stop, step) # 从 start 开始，以 step 为步长，到 stop-1 结束
```

### 惰性求值

```python
r = range(5)
print(r)          # 输出：range(0, 5)
print(type(r))    # 输出：<class 'range'>
```

### 左闭右开区间

```python
list(range(3))     # [0, 1, 2] （不包含 3）
list(range(2, 5))  # [2, 3, 4] （不包含 5）
```

### 支持负步长

当`step<0`时，序列递减，确保`start > stop`

```python
list(range(5, 0, -1))  # [5, 4, 3, 2, 1] （不包含 0）
```

## zip()

将多个可迭代对象（如列表、元组、字符串等）的元素按位置一一配对，生成一个迭代器（iterator）。迭代器中的每个元素是一个元组，包含来自各个可迭代对象的对应位置元素。

```python
zip(*iterables)
```

* `*iterables`表示任意数量的可迭代对象（至少1个，也可以没有）。
* 返回一个可迭代的`zip`对象.

### 基础示例

```python
names = ["Alice", "Bob", "Charlie"]
ages = [25, 30, 35]

# 用 zip 配对姓名和年龄
paired = zip(names, ages)
print(paired)  # <zip object at 0x...>（迭代器，需转换后查看）
print(list(paired))  # [( 'Alice', 25 ), ( 'Bob', 30 ), ( 'Charlie', 35 )]
```

### 多个可迭代对象（等长）

```python
names = ["Alice", "Bob", "Charlie"]
ages = [25, 30, 35]
scores = [90, 85, 95]

# 三个列表配对：(姓名, 年龄, 分数)
triples = zip(names, ages, scores)
print(list(triples))  # [('Alice', 25, 90), ('Bob', 30, 85), ('Charlie', 35, 95)]
```

### 可迭代对象长度不同（以最短为准）

```python
list1 = [1, 2, 3, 4, 5]  # 长度5
list2 = ["a", "b", "c"]   # 长度3

paired = zip(list1, list2)
print(list(paired))  # [(1, 'a'), (2, 'b'), (3, 'c')] （仅前3对有效）
```
