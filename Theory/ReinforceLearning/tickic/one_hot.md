# one-hot 编码

`One-hot`编码是一种用二进制向量表示离散类别.在许多方面都有使用.

对于一个包含`N`个类别的系统，用一个长度为`N`的向量表示类别,特定位置为`1`表示这个这个类别.

## 将动作分类

若动作空间有三个动作，索引为`0,1,2`.

* 动作0:  [1, 0, 0]
* 动作1:  [0, 1, 0]
* 动作2:  [0, 0, 1]

## 实现

```python
def one_hot(index_list, clas_num):
    if type(index_list) == torch.Tensor:
        index_list = index_list.detach().numpy()

    indexes = torch.LongTensor(index_list).view(-1, 1)

    out = torch.zeros(len(index_list), clas_num)

    out = out.scatter_(dim=1, index=indexes, value=1)

    return out
```

生成一个`one_hot`掩码.
