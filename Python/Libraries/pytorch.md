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
    reshaped_auto = x.view(-1,1)   # 根据其它维数自动计算大小 24 x 1试图

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

### 其它常用函数

* 用于在指定位置根据索引值填充数据。

    ```python
    torch.scatter_(dim, index, src, reduce=None) → Tensor

    # dim: 沿着哪个维度进行散布（0=行方向，1=列方向）
    # index: 索引张量，指定要修改的位置
    # src: 源张量（包含要填充的值）或标量值
    # reduce: 可选的归约操作（'add', 'multiply'等）

    out = out.scatter_(dim=1, index=indexes, value=1)

    #index = [2, 0, 1]
    # 初始张量 [[0,0,0,0], [0,0,0,0], [0,0,0,0]]
    #样本0 (索引2) → [0,0,1,0]
    #样本1 (索引0) → [1,0,0,0]
    #样本2 (索引1) → [0,1,0,0]
    ```

* 将一个单元素`tensor`转换为标准数值，用于防止`total_loss`保存了过多的计算图，导致显存爆炸

    ```python
    x = torch.tensor([3.14159]) 

    print(x)        # 输出: tensor([3.1416]) -> 这是一个对象

    # 使用 .item()
    val = x.item()  

    print(val)      # 输出: 3.14159 -> 这是一个纯 float
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

## 神经网络

`pytorch.nn`是一个模块化的神经网络，有着丰富的预定义的层，激活函数，损失函数等.

### `nn.Module`

`nn.Module`是所有神经网络的基类，继承这一个类并重写`__init__`与`forward`函数就可以实现一个神经网络

```python
import torch.nn as nn

class MyNetwork(nn.Module):
    def __init__(self):
        super().__init__()  # 必须调用父类初始化
        # 定义网络层
        self.layer1 = nn.Linear(10, 5)
        self.layer2 = nn.ReLU()
        self.layer3 = nn.Linear(5, 2)
    
    def forward(self, x):
        # 定义前向传播
        x = self.layer1(x)
        x = self.layer2(x)
        x = self.layer3(x)
        return x

# 实例化网络
model = MyNetwork()
print(model)
```

#### `nn.Module`作用

* 自动化的参数管理系统，自动管理所有的神经网络参数

    ```python
    class MyModel(nn.Module):
        def __init__(self):
            super().__init__()
            self.linear = nn.Linear(10, 5)  # 自动注册参数
            
    model = MyModel()
    print(list(model.parameters()))  # 包含 linear.weight 和 linear.bias
    ```

* 子模块管理，自动管理嵌套模块

    ```python
    class ComplexModel(nn.Module):
        def __init__(self):
            super().__init__()
            self.block1 = nn.Sequential(
                nn.Linear(10, 20),
                nn.ReLU()
            )
            self.block2 = AnotherModule()  # 自定义子模块
            
    model = ComplexModel()
    print(model)  # 自动显示完整层次结构
    ```

* 自动处理设备间转移

    ```python
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')

    model = MyModel()
    model.to(device)  # 自动将所有参数和缓冲区转移到指定设备

    # 无需手动处理每个参数
    ```

* 提供训练与评估机制

    ```python
    model.train()  # 启用训练模式（Dropout生效）
    # ...训练代码...

    model.eval()   # 启用评估模式（Dropout关闭）
    # ...验证代码...
    ```

#### 重写`__init__`与`forward`

在`__init__`函数中

1. 声明子模块：创建网络层（线性层、卷积层等）
2. 初始化参数：设置层的超参数（输入输出维度等）
3. 配置辅助组件：如激活函数、归一化层等

在`__init__`函数中初始化，声明要使用的网络层，此时网络的结构还没有指定。

在`forward`函数中

1. 指定计算流程：定义输入如何通过各层
2. 实现前向传播：执行实际的张量运算
3. 返回输出结果：产生预测值

在`forward`函数中，指定网络的结构，指定数据的流向，计算`backward`时便是反过来进行.

### 常用层

* 线性层，执行线性变换 $y = xW^T + b$，层的参数就是 $W^T$ 与 $b$

    ```python
    nn.Linear(in_features, out_features, bias=True)
    # in_features 输入特征维度
    # out_features 输出特征维度
    # bias 是否添加偏置项
    ```

* 卷积层,执行卷积，层的参数就是卷积核中元素的值

    ```python
    # 1D卷积（序列数据）
    nn.Conv1d(in_channels, out_channels, kernel_size)

    # 2D卷积（图像）
    nn.Conv2d(in_channels, out_channels, kernel_size, stride=1, padding=0)

    # 3D卷积（视频/医学影像）
    nn.Conv3d(in_channels, out_channels, kernel_size)

    # in_channels: 输入通道数（如RGB图像为3）表示一张图片的一个像素有多少个数字
    # out_channels： 输出通道数（卷积核数量） 表述输出的一张图片一个像素有多少的数字
    # kernel_size： 卷积核尺寸
    # stride： 步长
    # padding： 填充大小
    ```

* 池化层，降维、减少计算量、增强平移不变性

    ```python
    # 最大池化：2x2窗口，步长2
    max_pool = nn.MaxPool2d(kernel_size=2, stride=2)

    # 平均池化：3x3窗口，步长1
    avg_pool = nn.AvgPool2d(kernel_size=3, stride=1)

    # 全局平均池化
    global_avg_pool = nn.AdaptiveAvgPool2d(output_size=1)

    # 前向传播示例
    input_feature = torch.randn(16, 64, 112, 112)  # 卷积层输出
    output_max = max_pool(input_feature)  # 输出形状: (16, 64, 56, 56)
    output_avg = avg_pool(input_feature)  # 输出形状: (16, 64, 110, 110)
    output_global = global_avg_pool(input_feature)  # 输出形状: (16, 64, 1, 1)
    ```

* 循环层,处理序列数据（时间序列、文本）

    ```python
    # LSTM层：输入特征100维，隐藏状态200维，2层堆叠
    lstm_layer = nn.LSTM(
        input_size=100,
        hidden_size=200,
        num_layers=2,
        batch_first=True,  # 输入格式为(batch, seq, feature)
        bidirectional=False
    )

    # 前向传播示例
    sequence = torch.randn(32, 10, 100)  # 32个序列，每个序列10个时间步，每步100维特征
    output, (hn, cn) = lstm_layer(sequence)

    # output形状: (32, 10, 200) 所有时间步的输出
    # hn形状: (2, 32, 200) 最后一层所有时间步的隐藏状态
    # cn形状: (2, 32, 200) 最后一层所有时间步的细胞状态
    ```

* 归一化层,标准化数据分布，加速训练，提高稳定性

    ```python
    # 2D图像批归一化：输入通道64
    bn_layer = nn.BatchNorm2d(num_features=64)

    # 前向传播示例
    input_feature = torch.randn(32, 64, 56, 56)  # 32张图，64通道，56x56
    output = bn_layer(input_feature)  # 输出形状不变: (32, 64, 56, 56)
    ```

* 激活函数层,引入非线性，使网络能拟合复杂函数

    ```python
    relu = nn.ReLU(inplace=True)  # inplace=True节省内存
    sigmoid = nn.Sigmoid()
    softmax = nn.Softmax(dim=1)  # 沿类别维度计算

    # 前向传播示例
    input_data = torch.tensor([-2.0, 0.5, 3.0])
    output_relu = relu(input_data)  # tensor([0.0, 0.5, 3.0])
    ```

    $$
    \begin{align*}
        Sigmoid &: \sigma(x) = \frac{1}{1+e^{-x}} \\
        Tanh&: \tanh(x) \\
        ReLU &: max(0,x) \\
        LeakyReLU &: max(0,01x,x) \\
        Softmax &: \frac{e^{x_i}}{\sum_j e^{x_j}}
    \end{align*}
    $$

* Dropout,随机丢弃神经元，防止过拟合

    ```python
    # 标准Dropout
    dropout = nn.Dropout(p=0.5)

    # 2D Dropout（用于卷积层后）
    spatial_dropout = nn.Dropout2d(p=0.2)

    # 前向传播示例
    input_data = torch.randn(32, 256)
    output = dropout(input_data)  # 随机50%元素置零
    ```

* 损失函数层,量化预测值与真实值的差异

    ```python
    # 交叉熵损失（含Softmax）
    criterion = nn.CrossEntropyLoss()

    # 前向传播示例
    outputs = model(inputs)  # 模型预测，形状(batch, num_classes)
    loss = criterion(outputs, targets)  # targets为类别索引
    ```

### 常用函数

* `nn.Sequential​`用于按顺序组合多个神经网络，输入数据流经每个层后输出最终结果

    ```python
    import torch.nn as nn

    model = nn.Sequential(
        nn.Linear(784, 256),  # 全连接层
        nn.ReLU(),            # 激活函数
        nn.Linear(256, 10),   # 全连接层
        nn.Softmax(dim=1)     # 分类概率
    )
    ```

## 优化器

`PyTorch`的`torch.optim`根据损失函数的梯度，找到一组参数，让损失函数的值最小。

### 经典工作流程

```python
import torch.optim as optim

# 1. 定义优化器：传入模型参数+超参数（学习率等）
optimizer = optim.Adam(model.parameters(), lr=0.001)

for epoch in range(10):  # 训练10轮
    for x, y in dataloader:  # 遍历数据
        # 2. 前向传播：计算预测值
        pred = model(x)
        # 3. 计算损失：预测 vs 真实值
        loss = criterion(pred, y)
        # 4. 清零梯度（关键！避免梯度累积）
        optimizer.zero_grad()
        # 5. 反向传播：计算损失对参数的梯度
        loss.backward()
        # 6. 更新参数：优化器根据梯度调整参数
        optimizer.step()
```

* `optimizer.zero_grad()`清空模型参数的梯度`.grad`值
* `loss.backward()`自动计算所有参数的梯度
* `optimizer.step()`根据梯度更新参数.

### `torch.optim`模块

`PyTorch`的`torch.optim`模块包含了几乎所有主流优化器，按算法原理可分为三大类：

* 基础梯度下降类：SGD（随机梯度下降）及其变种（带动量、Nesterov动量）
* 自适应学习率类：Adam、AdamW、RMSprop（为每个参数动态调整学习率）。
* 经典自适应类：Adagrad、Adadelta（早期自适应算法，现较少用）。

### 常用优化器

* Adam

    ```py
    optimizer = optim.Adam(
        params=model.parameters(),
        lr=0.001,          # 学习率（默认0.001，常用0.001~0.0001）
        betas=(0.9, 0.999), # 一阶矩/二阶矩衰减率（默认即可）
        eps=1e-8,          # 数值稳定项（默认即可）
        weight_decay=0     # L2正则化系数（默认0，建议用AdamW代替）
    )
    ```

## 分布distributions

让神经网络输出概率分布，并且支持反向传播.

```python
import torch
from torch.distributions import Normal

# 假设网络输出：建议推力 5.0N，但网络有点不确定，标准差给了 2.0
mu = torch.tensor([5.0])    # 均值 loc
sigma = torch.tensor([2.0]) # 标准差 scale (必须 > 0)

# 1. 创建分布
dist = Normal(loc=mu, scale=sigma)

# 2. 采样 (用于探索)
action = dist.sample()
print(f"实际执行推力: {action.item():.2f} N")

# 3. 计算对数概率 (用于反向传播 Loss)
log_prob = dist.log_prob(action)
print(f"该动作的 LogProb: {log_prob.item():.4f}")

# 4. 熵 (Entropy) - 在 RL 中很重要
# 熵越大，代表分布越平坦，探索性越强
print(f"当前策略的熵 (探索程度): {dist.entropy().item():.4f}")
```

### 正态分布Normal

接受均值与标准差，生成一个正态分布

* $\mu$:均值
* $\sigma$:标准差

### 分类分布Categorical

接受一个列向量，向量每个值表示这个index被采样的概率

```python
import torch
from torch.distributions import Categorical

# 神经网络通常输出原始分数 (Logits)，还没经过 Softmax
# 假设网络认为“直行(索引1)”的分数最高
logits = torch.tensor([1.0, 3.5, 0.5]) 

# 1. 创建分布
# 注意：可以直接传 logits，PyTorch 会内部帮你做 softmax，数值更稳定
dist = Categorical(logits=logits) 
# 或者如果你已经有了概率，也可以用 Categorical(probs=probs)

# 2. 采样
# 返回的是索引 (index)，不是具体的数值
action_index = dist.sample()
print(f"选择的动作索引: {action_index.item()}") 
# 很大几率输出 1，偶尔输出 0 或 2

# 3. 这里的 LogProb 是 log(p_i)
print(f"选中该动作的 LogProb: {dist.log_prob(action_index).item():.4f}")
```

### 伯努利分布Bernoulli

`0,1`的二分类

### 均匀分布Uniform
