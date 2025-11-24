# numpy

`numpy`是`Python`生态中最核心的开源数值计算库，专为高效处理大型多维数组和矩阵运算而设计。它提供了高性能的数学函数和工具，是科学计算、数据分析、机器学习、人工智能等领域的基础依赖库。

## numpy数据类型

* `np.int8`,`np.int16`,`np.int32`,`np.int64`,`np.int128`,`np.int256`.
* `np.uint8`,`np.uint16`,`np.uint32`,`np.uint64`,`np.uint128`,`np.uint256`.
* `np.float16`,`np.float32`,`np.float64`,`np.float80`,`np.float96`，`np.float128`，`np.float256`

## 创建numpy数组

```python
import numpy as np

# 1D 数组（向量）
arr1 = np.array([1, 2, 3, 4])  # shape=(4,), dtype=int64（默认）

# 2D 数组（矩阵）
arr2 = np.array([[1, 2, 3], [4, 5, 6]])  # shape=(2,3)

# 指定数据类型（避免默认推断错误）
arr3 = np.array([1, 2, 3], dtype=np.float32)  # dtype=float32
```

### 数组属性

* `shape`数组维度(元组),`arr2.shape → (2, 3)`表示两行三列的二维数组.
* `ndim`数组维数和`shape`不同，这个表示数组是几维的,`arr2.ndim → 2`表示二维数组.
* `size`总元素个数.`arr2.size → 6`表示数组有6个元素.
* `dtype`元素数据类型.`arr2.dtype→ int64`表示（默认整数类型）.
* `itemsize`单个元素字节大小.`arr2.itemsize → 8`表示8个字节
* `nbytes`表示总字节个数.`arr2.nbytes → 48（6×8=48`

### 特殊数组

特殊数组，用于快速初始化.

* `np.zeros(shape)`,创建全`0`数组,`np.zeros((2, 3))→ [[0,0,0], [0,0,0]]`
* `np.ones(shape)`,创建全`1`数组,`np.ones((3, 2))→ [[1,1], [1,1], [1,1]]`
* `np.eye(n)`,创建`n×n`单位矩阵,`np.eye(3)→ [[1,0,0], [0,1,0], [0,0,1]]`
* `np.empty(shape)`,创建未初始化数组（速度快，值随机）,`np.empty((2, 2))`
* `np.arange(start, stop, step)`,等差数列(类似 range，但返回数组),`np.arange(0, 10, 2)→ [0,2,4,6,8]`
* `np.linspace(start, stop, num)`,等间隔数列（含终点）,`np.linspace(0, 1, 5)→ [0, 0.25, 0.5, 0.75, 1]`
* `np.random.rand(*shape)`均匀分布随机数组（元素 ∈ `[0,1)`）,`np.random.rand(2, 3)` → 2行3列随机数组
* `np.random.randn(*shape)`标准正态分布随机数组（均值 0，方差 1）`np.random.randn(3)` → 3个随机数

## 索引与切片

类似于`List`，但支持更多的索引方法.执行严格边界检查.

### 基础索引与切片

* 一维数组:与`List`完全一致,`[start:stop:step]`

```python
arr = np.arange(10)  # [0,1,2,...,9]
print(arr[3])      # 输出：3（第4个元素，索引从0开始）
print(arr[2:5])    # 输出：[2,3,4]（左闭右开）
print(arr[::2])    # 输出：[0,2,4,6,8]（步长2）
```

* 二维数组:用`[行索引, 列索引]`或`[行切片, 列切片]`

```python
arr2 = np.array([[1,2,3], [4,5,6], [7,8,9]])  # 3行3列矩阵

print(arr2[1, 2])   # 输出：6（第2行第3列元素）
print(arr2[0:2, 1:])  # 输出：[[2,3], [5,6]]（前2行，从第2列开始）
print(arr2[:, 0])    # 输出：[1,4,7]（所有行的第1列）
```

切片索引允许超界，会自动截断到有效范围

```python
arr = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]])  # 3行3列，shape=(3,3)

print(arr[1:5, :2])  # 行切片1:5（超界，取1~2行），列切片:2（取0~1列）
# 输出：[[4,5], [7,8]]（行索引5超界，截断到2）

print(arr[:, -5:4])  # 列切片-5:4（start=-5超界→0，stop=4超界→3）
# 输出：[[1,2,3], [4,5,6], [7,8,9]]（所有列）
```

### 高级索引

* 布尔索引：用布尔数组筛选元素（True保留，False排除）

```python
arr = np.array([1, 3, 5, 7, 9])
mask = arr > 4  # 布尔数组：[False, False, True, True, True]
print(arr[mask])  # 输出：[5,7,9]（筛选大于4的元素）
```

* 花式索引：用整数数组(返回整数的可迭代对象，不能是布尔对象)指定索引位置（可乱序、重复）

```python
arr = np.array([10, 20, 30, 40, 50])
indices = [0, 2, 4]  # 指定索引
print(arr[indices])  # 输出：[10, 30, 50]（按索引取元素）

# 2D 数组花式索引（取指定行）
arr2 = np.array([[1,2], [3,4], [5,6]])
print(arr2[[0, 2]])  # 输出：[[1,2], [5,6]]（取第1行和第3行）
```

## 数组运算

### 矢量化`Element-wise Operation`

对每个元素执行相同的操作,比如加减乘除、幂、三角函数

```python
a = np.array([1, 2, 3])
b = np.array([4, 5, 6])

# 算术运算（对应元素）
print(a + b)   # [5,7,9]（加法）
print(a * b)   # [4,10,18]（乘法）
print(a ** 2)  # [1,4,9]（平方）

# 三角函数（对每个元素）
print(np.sin(a))  # [sin(1), sin(2), sin(3)] ≈ [0.84, 0.91, 0.14]

# 逻辑运算（返回布尔数组）
print(a > 1)   # [False, True, True]
```

### 广播机制Broadcasting

允许不同形状的数组​进行运算，自动扩展较小数组的维度以匹配较大数组（“广播”）。

* 标量与数组运算

```python
arr = np.array([[1, 2], [3, 4]])
print(arr + 10)  # [[11,12], [13,14]]（10广播为[[10,10],[10,10]]）

Q_list = np.array([[1, 2],[3, 4]])
A = Q_list == Q_list.max() # 布尔数组 [[False False] [False  True]]
```

* 不同形状数组运算

```python
a = np.array([[1], [2]])  # shape=(2,1)
b = np.array([3, 4, 5])   # shape=(3,) → 广播为 (1,3) → 再扩展为 (2,3)
print(a + b)  # [[4,5,6], [5,6,7]]（每行都是 [1+3,1+4,1+5] 和 [2+3,2+4,2+5]）
```

广播只有在两数组维度从后往前比对，尺寸相等或其一为1才能广播

### 矩阵乘法

使用`@`运算符或`np.dot()`实现矩阵乘法

```python
a = np.array([[1, 2], [3, 4]])  # 2×2矩阵
b = np.array([[5, 6], [7, 8]])  # 2×2矩阵

# 矩阵乘法（行×列累加）
print(a @ b)  # [[1 * 5+2 * 7, 1 * 6+2 * 8], [3 * 5+4 * 7, 3 * 6+4 * 8]] → [[19,22], [43,50]]
print(np.dot(a, b))  # 等价于 a@b
```

## 重整数组

### 重塑形状

* `reshape(new_shape)`:返回新形状的视图（不修改原数组，需保证元素总数不变）.
* `resize(new_shape)`:直接修改原数组形状（元素总数可不同，不足补0/截断）

```python
arr = np.arange(6)  # [0,1,2,3,4,5]，shape=(6,)

# reshape 为新形状（2行3列）
arr_reshaped = arr.reshape(2, 3)  # [[0,1,2], [3,4,5]]，shape=(2,3)

# resize 修改原数组（3行2列）
arr.resize(3, 2)  # arr 变为 [[0,1], [2,3], [4,5]]，shape=(3,2)
```

### 展平

将多维数组转为`1D`数组（展平）。

* `flatten()`:返回副本（修改不影响原数组）。
* `ravel()`:返回视图（修改可能影响原数组，更快）。

```python
arr = np.array([[1, 2, 3], 
                [4, 5, 6]])

# 方法1: flatten() 默认行优先
flat_c1 = arr.flatten()          # [1, 2, 3, 4, 5, 6]

# 方法2: ravel() 默认行优先
flat_c2 = arr.ravel()            # [1, 2, 3, 4, 5, 6]（视图优先）

# 方法3: 显式指定 order='C'
flat_c3 = arr.flatten(order='C')  # [1, 2, 3, 4, 5, 6]

# 方法1: ravel(order='F')
flat_f1 = arr.ravel(order='F')    # [1, 4, 2, 5, 3, 6]

# 方法2: flatten(order='F')
flat_f2 = arr.flatten(order='F')  # [1, 4, 2, 5, 3, 6]

# 方法3: 转置后行优先展平（等效）
flat_f3 = arr.T.flatten()         # [1, 4, 2, 5, 3, 6]
```

* `flatnonzero`:将输入数组展平，并删除所有零元素，返回做完操作后的下标

```python
arr = np.array([[1, 2], [3, 4]])

arr2 = arr == arr.max() # [[False False] [False  True]]

arr3 = np.flatnonzero(arr2) # [3]
```

### 转置

交换数组的行和列（2D 矩阵转置，高维数组可指定轴顺序）。

```python
arr = np.array([[1,2,3], [4,5,6]])  # shape=(2,3)
print(arr.T)  # [[1,4], [2,5], [3,6]]（shape=(3,2)）
print(arr.transpose(1, 0))  # 等价于 arr.T（指定轴0和1交换）
```

## random

`NumPy`的`random`模块是生成伪随机数的核心工具.旧版API基于全局状态`API`,新版API基于`Generator`对象，更加灵活

```python
np.random.rand(2,3) # 旧版API，无需显式创建即可直接使用
```

```python
rng = np.random.default_rng(seed)
rng.random((2,3)) # 新版API， 需要创建后使用
```

### 伪随机

`NumPy`的随机数本质是算法生成的确定性序列（伪随机），而非物理随机。其特点是：

* 可复现性：通过固定“种子”（Seed），可生成完全相同的随机序列.

### 随机种子Seed

种子是随机数生成的“起点”。设置相同种子，后续所有随机操作结果完全一致。

如果不显式设置随机种子（seed），随机数生成器会使用动态生成的随机种子，确保每次运行的随机序列尽可能不同（不可预测）。

#### 设置seed

旧版AP设置全局种子

```python
import numpy as np

np.random.seed(42)  # 设置种子为42
print(np.random.rand(3))  # 输出：[0.37454012 0.95071431 0.73199394]

np.random.seed(42)  # 重置相同种子
print(np.random.rand(3))  # 输出与上一次完全一致（可复现）
```

`Generator`对象，通过局部种子创建独立随机生成器，避免全局状态污染

```python
from numpy.random import default_rng  # 新版推荐入口

rng = default_rng(seed=42)  # 创建种子为42的局部生成器
print(rng.random(3))  # 输出：[0.77395605 0.43887844 0.85859792]（与旧版seed=42结果不同，因算法升级）

rng2 = default_rng(seed=42)  # 相同种子生成器
print(rng2.random(3))  # 输出与rng.random(3)完全一致（局部可控）
```

### 基础随机数生成函数

* 均匀分布
  
    `random(size)`：生成`[0,1)`浮点数

    ```python
    rng = default_rng(42)
    print(rng.random((2, 3)))  # 2行3列数组，元素∈[0,1)
    # 输出：[[0.77395605 0.43887844 0.85859792]
    #        [0.69736803 0.09417735 0.97562235]]
    ```

* 正态分布

    `randn`或`standard_normal`生成标准正态分布.`N(0,1)`

    ```python
    # 旧版：2行3列标准正态分布
    std_norm_old = np.random.randn(2, 3)
    # 新版：同上
    std_norm_new = rng.standard_normal(size=(2, 3))
    print("旧版 randn:\n", std_norm_old)  # 示例：[[-0.2 1.3 -0.5], [0.8 -1.1 0.3]]
    ```

    `normal`生成任意正态分布.`N(loc,scale)`

    ```python
    # 生成均值为5、标准差为2的10个随机数
    normal_old = np.random.normal(loc=5, scale=2, size=10)
    normal_new = rng.normal(loc=5, scale=2, size=10)
    print("旧版 normal:", normal_old)  # 示例：[4.2 6.8 3.1 5.5 7.2 ...]
    ```

* 随机整数

    `randint`或`integers`生成`[low,high)`随机整数.

    ```python
    # 旧版：生成 [1, 10) 的5个整数（即1-9）
    int_old = np.random.randint(1, 10, size=5)
    # 新版：同上
    int_new = rng.integers(1, 10, size=5)
    print("旧版 randint:", int_old)  # 示例：[3 7 2 9 5]

    # 生成0-5（含0不含5）的2行3列整数矩阵
    int_matrix = rng.integers(0, 5, size=(2, 3))
    print("整数矩阵:\n", int_matrix)  # 示例：[[1 3 0], [4 2 1]]
    ```

### 随机排列

* `shuffle`：原地打乱数组（修改原数组，多维数组仅打乱第0轴）
* `permutation`: 返回打乱后的副本（不修改原数组）

```python
arr = np.array([1, 2, 3, 4, 5])

# 旧版 shuffle（原地打乱）
np.random.shuffle(arr)
print("shuffle后（旧）:", arr)  # 示例：[3 1 5 2 4]（原数组已变）

# 新版 permutation（返回副本）
arr_copy = np.array([1, 2, 3, 4, 5])
perm_new = rng.permutation(arr_copy)
print("permutation副本（新）:", perm_new)  # 示例：[5 2 1 4 3]
print("原数组不变:", arr_copy)  # [1 2 3 4 5]（未修改）

# 对整数n，返回0~n-1的排列
perm_n = rng.permutation(5)  # 等价于打乱 [0,1,2,3,4]
print("0~4的排列:", perm_n)  # 示例：[2 0 4 1 3]
```

### 抽样

* `choice`:从一维数组/序列中随机抽取元素，支持放回/不放回抽样和概率权重.

```python
# 旧版 choice
choices_old = np.random.choice([10, 20, 30, 40], size=5, replace=True, p=[0.1, 0.2, 0.3, 0.4])
# 新版 choice（参数相同）
choices_new = rng.choice([10, 20, 30, 40], size=5, replace=True, p=[0.1, 0.2, 0.3, 0.4])

print("旧版 choice（放回+概率）:", choices_old)  # 示例：[40 30 40 20 40]（按概率抽取）
print("新版 choice（不放回）:", rng.choice([1,2,3,4], size=3, replace=False))  # 示例：[3 1 4]（无重复）
```

概率权重数组`p`需要和数组长度一致.
