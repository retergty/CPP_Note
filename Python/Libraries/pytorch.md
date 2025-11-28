# PyTorch

`PyTorch`是一个基于`Python`的科学计算库，主要用于深度学习领域。它提供了灵活的张量计算和动态计算图机制。

## tensor

`Tensor`是 `PyTorch` 的核心数据结构，类似于 `NumPy` 的多维数组，但具有 `GPU` 加速能力和自动微分功能。

### 创建tensor

类似于`Numpy`，很多函数十分类似.

不同的是 `tensor` 可以指定 `device`表示这个 `tensor`要存储在哪种计算设备上.

* 创建

    ```python
    import torch

    # 从列表创建
    tensor_list = torch.tensor([1, 2, 3, 4])  # 1D 张量
    matrix = torch.tensor([[1, 2], [3, 4]])   # 2D 张量

    # 指定数据类型和设备
    float_tensor = torch.tensor([1, 2, 3], dtype=torch.float32, device='cuda')
    ```

* 特殊值

    ```python
    # 零张量
    zeros = torch.zeros(2, 3)  # 2x3 全零张量

    # 单位张量
    ones = torch.ones(3, 3)    # 3x3 全一张量

    # 对角矩阵
    eye = torch.eye(3)         # 3x3 单位矩阵

    # 空张量（未初始化）
    empty = torch.empty(2, 2)  # 内容随机
    ```

* 随机初始化

    ```python
    # 均匀分布 [0, 1)
    uniform = torch.rand(2, 3)

    # 标准正态分布 N(0, 1)
    normal = torch.randn(2, 3)

    # 整数范围
    randint = torch.randint(0, 10, (3, 3))  # [0, 10) 随机整数

    # 序列张量
    arange = torch.arange(0, 10, 2)  # [0, 2, 4, 6, 8]
    linspace = torch.linspace(0, 1, 5)  # [0, 0.25, 0.5, 0.75, 1]
    ```

* 从其它数据结构中创建

    ```python
    import numpy as np

    # 从 NumPy 数组创建
    numpy_arr = np.array([1, 2, 3])
    tensor_from_np = torch.from_numpy(numpy_arr)

    # 从另一个 Tensor 创建（共享内存）
    original = torch.tensor([1, 2, 3])
    clone = original.clone()        # 深拷贝（不共享内存）
    detached = original.detach()    # 分离计算图（共享内存）
    ```

### 内存布局

* 连续内存布局（Contiguous）:元素在内存中按行优先（C-style）或列优先（Fortran-style）顺序连续存储，无间隔.

`tensor`转置，切片等操作后，内存布局变为不连续.

### 常见属性与方法

* 基本属性

    ```python
    x = torch.randn(3, 4, dtype=torch.float32, device='cuda')

    print(x.shape)    # torch.Size([3, 4]) - 张量形状
    print(x.dtype)    # torch.float32 - 数据类型
    print(x.device)   # cuda:0 - 所在设备
    print(x.ndim)     # 2 - 维度数
    print(x.numel())  # 12 - 元素总数
    ```

* 形状操作

    ```python
    x = torch.randn(2, 3, 4)

    # 改变形状（不复制数据）
    reshaped = x.view(6, 4)        # 6x4 视图
    reshaped_alt = x.reshape(4, 6) # 更灵活的 reshape

    # 维度操作
    flattened = x.flatten()        # 展平为 1D 张量 (24,)
    unsqueezed = x.unsqueeze(0)    # 增加维度 -> (1, 2, 3, 4)
    squeezed = unsqueezed.squeeze() # 移除单维度 -> (2, 3, 4)

    # 转置和置换
    transposed = x.transpose(0, 1)  # 交换维度0和1 -> (3, 2, 4)
    permuted = x.permute(2, 0, 1)   # 重排维度 -> (4, 2, 3)
    ```

    `view`函数返回原 `Tensor` 的视图（View），与原 `Tensor` 共享内存,速度快但要求原`Tensor`必须是连续内存布局（Contiguous）.

    `reshape`函数优先返回视图，若无法共享内存则拷贝.发生拷贝后，速度较慢.

* 索引与切片

    ```python
    x = torch.tensor([[1, 2, 3], 
                    [4, 5, 6], 
                    [7, 8, 9]])

    # 基本索引
    element = x[1, 2]      # 第2行第3列 -> 6
    row = x[0]             # 第1行 -> [1, 2, 3]
    column = x[:, 1]       # 第2列 -> [2, 5, 8]

    # 切片
    sub_matrix = x[0:2, 1:] # 前两行，后两列 -> [[2, 3], [5, 6]]

    # 高级索引
    indices = torch.tensor([0, 2])
    selected_rows = x[indices]  # 第1和第3行 -> [[1,2,3], [7,8,9]]

    # 布尔索引
    mask = x > 5
    filtered = x[mask]  # [6, 7, 8, 9]
    ```

### 数学运算

* 逐元素运算

    ```python
    a = torch.tensor([1, 2, 3])
    b = torch.tensor([4, 5, 6])

    # 算术运算
    add = a + b          # [5, 7, 9]
    sub = a - b          # [-3, -3, -3]
    mul = a * b          # [4, 10, 18]
    div = a / b          # [0.25, 0.4, 0.5]

    # 原位操作（修改原张量）
    a.add_(b)  # 相当于 a = a + b

    # 数学函数
    exp = torch.exp(a)    # 指数
    log = torch.log(a)    # 自然对数
    sqrt = torch.sqrt(a)  # 平方根
    sin = torch.sin(a)    # 正弦
    ```

* 归约运算

    ```python
    x = torch.tensor([[1, 2, 3], 
                      [4, 5, 6]])

    # 求和
    total_sum = x.sum()          # 21
    row_sum = x.sum(dim=1)       # [6, 15] 每行求和
    col_sum = x.sum(dim=0)       # [5, 7, 9] 每列求和

    # 均值
    mean_val = x.mean()          # 3.5

    # 极值
    max_val, max_idx = x.max(dim=1)  # 每行最大值和索引
    min_val = x.min()             # 1

    # 范数
    norm = x.norm(p=2)            # Frobenius 范数
    ```

    `dim`参数表示沿着对应维度操作（聚合）元素，执行完毕后，对应维度消失.

* 矩阵运算

    ```python
    A = torch.tensor([[1, 2], [3, 4]])
    B = torch.tensor([[5, 6], [7, 8]])

    # 矩阵乘法
    matmul = torch.matmul(A, B)  # 或 A @ B
    # 结果: [[19, 22], [43, 50]]

    # 点积
    dot_product = torch.dot(A[0], B[0])  # 1 * 5 + 2 * 6 = 17

    # 转置
    A_T = A.t()  # 或 A.transpose(0, 1)

    # 逆矩阵（仅方阵）
    inv_A = torch.inverse(A)

    # 解线性方程组
    solution = torch.linalg.solve(A, torch.tensor([1, 2]))
    ```

### 自动求导

* 反向传播计算

    ```python
    # 创建需要梯度的张量
    x = torch.tensor([2.0], requires_grad=True)

    # 执行计算
    y = x**2 + 3*x + 1  # y = x² + 3x + 1

    # 反向传播
    y.backward()

    # 获取梯度
    print(x.grad)  # dy/dx = 2x + 3 = 2 * 2 + 3 = 7
    ```

* 复杂计算图

    ```python
    x = torch.tensor([1.0], requires_grad=True)
    y = torch.tensor([2.0], requires_grad=True)

    # 计算图: z = (x + y) * y
    z = (x + y) * y

    # 反向传播
    z.backward()

    print(x.grad)  # ∂z/∂x = y = 2.0
    print(y.grad)  # ∂z/∂y = (x + y)*1 + y*1 = (1+2) + 2 = 5.0
    ```

* 梯度控制

    ```python
    # 临时禁用梯度
    with torch.no_grad():
        y = x * 2  # 不会跟踪梯度
        
    # 停止梯度跟踪
    detached_x = x.detach()  # 创建不需要梯度的副本

    # 梯度清零（重要！）
    optimizer.zero_grad()  # 在训练循环中常用
    ```

### 设备管理

* 设备间转移

    ```python
    # 检查可用设备
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')

    # 创建在 GPU 上的张量
    gpu_tensor = torch.tensor([1, 2, 3], device=device)

    # 在设备间移动张量
    cpu_tensor = gpu_tensor.cpu()
    gpu_tensor_alt = cpu_tensor.to('cuda')
    ```

* 多 GPU 支持

    ```python
    # 使用特定 GPU
    if torch.cuda.device_count() > 1:
        print(f"可用 GPU 数量: {torch.cuda.device_count()}")
        tensor = tensor.to(f'cuda:{1}')  # 使用第二个 GPU
    ```

* GPU 内存管理

    ```python
    # 清空 GPU 缓存
    torch.cuda.empty_cache()

    # 监控显存使用
    print(f"已分配显存: {torch.cuda.memory_allocated() / 1024**2:.2f} MB")
    print(f"缓存显存: {torch.cuda.memory_reserved() / 1024**2:.2f} MB")
    ```

## 自动微分

自动求导（`Autograd`）是`PyTorch`实现深度学习模型训练的核心，其本质是通过动态计算图和反向传播算法，自动计算张量（`Tensor`）的梯度。

自动求导记录了`tensor`间的操作，对操作进行自动微分，存储在对应`tensor`的`.grad`属性中.


### 计算图

`pytorch`将计算过程表示为一个有向无环图,图的节点是`tensor`,边是`Function`对象，这样不同的`tensor`就通过计算操作连成一个图。

通过前向传播，`pytorch`建立了一个计算图，随后通过调用`backward`函数，利用链式法则反向传播计算梯度.

### 叶子节点

叶子节点是用户直接创建、未被其他`Tensor`操作生成的原始`Tensor`,可以理解为梯度计算的自变量。

当一个`tensor`调用`y.backward()`函数反向传播时，`pytorch`利用链式法则反向遍历计算图，计算梯度，当遇到属性满足`requires_grad=True,is_leaf=True`的`tensor`节点时，将计算出来的梯度**加到**这个节点的`.grad`属性中，实现了 $\frac{\partial y}{\partial x} \mid_{x=x_0}$的自动梯度计算，$x_0$就是叶子节点的值。

### tensor单元

`tensor`是Autograd 的基本单元，其关键属性控制梯度计算:

* `requires_grad`:布尔值，标记是否需要跟踪梯度（仅 True时参与梯度计算）.
* `grad`:存储梯度值（仅叶子节点默认保留，非叶子节点需手动保留）
* `grad_fn`:指向创建该 Tensor 的 Function对象（非叶子节点特有，记录操作历史）
* `is_leaf`:是否为叶子节点（用户直接创建的 Tensor，非中间计算结果）

相关函数

* `tensor.requires_grad_()`：原地设置`requires_grad=True`
* `tensor.detach()`:创建一个`tensor`,它脱离计算图的“视图”，`requires_grad=False，is_leaf=True`（共享数据但不跟踪梯度）

### Function 对象

每个`Tensor`操作（如加减乘除、卷积、激活函数）对应一个`torch.autograd.Function`子类实例,负责

* `forward(ctx, *inputs)`,前向计算，沿着计算图到下一个节点.
* `backward(ctx, *grad_outputs)`,反向传播，沿着计算图反向到上一个节点，计算梯度.

### backward()函数

它通过遍历`Tensor`参与构建的动态计算图，自动计算当前`Tensor`相对于图中所有叶子节点的梯度，并将结果存储在叶子节点的`.grad`属性中

```python
def backward(
    gradient=None,  # 上游梯度（grad_tensors），默认 None（对标量求导时为 1.0）
    retain_graph=None,  # 是否保留计算图（默认 None，单次 backward 后释放）
    create_graph=False,  # 是否创建导数图（用于高阶求导，如二阶导）
    inputs=None  # 指定需要计算梯度的叶子节点（默认所有叶子节点）
)
```

* `gradient`上游梯度，表示 $\frac{\partial \text{loss}}{\partial \text{current\_tensor}}$ 的值.也就是链式法则的起始点.

注意梯度累加问题，`backward()`会把梯度累加到叶子节点的`.grad`中，每次迭代前需要调用`optimizer.zero_grad()`或`tensor.grad.zero_()`清空梯度.

```python
import torch
import torch.nn as nn
import torch.optim as optim

# 模型、数据、损失函数
model = nn.Linear(10, 1)  # 简单线性模型
x = torch.randn(5, 10)  # 输入（5个样本，10维特征）
y_true = torch.randn(5, 1)  # 真实标签
criterion = nn.MSELoss()  # 均方误差损失

# 前向传播
y_pred = model(x)  # 模型输出（5x1）
loss = criterion(y_pred, y_true)  # 标量损失

# 反向传播 + 参数更新
optimizer = optim.SGD(model.parameters(), lr=0.01)
optimizer.zero_grad()  # 清空历史梯度（关键！避免累加）
loss.backward()  # 计算梯度（存入 model 参数的 .grad）
optimizer.step()  # 优化器根据梯度更新参数
```
