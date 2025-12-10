# python基础知识点

## 拷贝

`python`中，对于可变对象，直接赋值相当于引用赋值。

```python
a = [1, 2, [3, 4]]
b = a          # 赋值：b 和 a 指向同一个列表对象

b[0] = 99      # 修改 b
print(a)       # 输出: [99, 2, [3, 4]] → a 也被修改！
```

`python`的`copy`模块便提供了复制对象的能力.

### 浅拷贝

浅拷贝，创建一个新对象，但嵌套对象仍是引用。适用对象无嵌套结构

```python
import copy

a = [1, 2, [3, 4]]
b = copy.copy(a)  # 浅拷贝

# 修改表层元素（不影响原对象）
b[0] = 99
print(a)         # [1, 2, [3, 4]] → a 不变

# 修改嵌套对象（影响原对象！）
b[2][0] = 77
print(a)         # [1, 2, [77, 4]] → a 的嵌套列表被修改！
```

### 深拷贝

递归复制所有层级的对象，生成完全独立的新对象.

```python
import copy

a = [1, 2, [3, 4]]
c = copy.deepcopy(a)  # 深拷贝

# 修改任意部分均不影响原对象
c[0] = 99
c[2][0] = 77
print(a)             # [1, 2, [3, 4]] → a 完全不变
```

### 不可变对象

对于不可变对象，赋值和浅拷贝效果相同，但不可变对象内部有可变对象的话，深拷贝会把可变对象也拷贝.

```python
t = (1, [2, 3])
t_deep = copy.deepcopy(t)
t_deep[1][0] = 99
print(t)           # (1, [2, 3]) → 原元组未变（因元组本身不可变，但深拷贝复制了内层列表）
```

## 虚拟环境

为每个项目配置独立的环境，防止全局环境冲突.

### conda

使用`conda`作为管理软件，下载`Miniconda`

### 创建环境

```shell
# 创建一个名为 myenv 的环境，并指定 python 版本为 3.9
conda create -n myenv python=3.9
```

### 激活环境

```shell
conda activate myenv
```

### 安装包

```shell
# 安装 numpy
conda install numpy

# 一次安装多个包
conda install numpy pandas matplotlib
```

### 查看与退出

```shell
# 查看当前环境装了哪些包
conda list

# 退出当前环境（回到 base）
conda deactivate

# 查看环境
conda env list
```

### 删除环境

```shell
conda env remove -n myenv
```
