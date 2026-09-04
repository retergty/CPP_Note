# onnx

ONNX（Open Neural Network Exchange）是模型的交换格式：一张算子图 + 权重 + 版本信息。 

ONNX是protobuf格式的，官方schema是

* [onnx schema](https://github.com/onnx/onnx/blob/main/onnx/onnx.proto)

## 架构

### 总体架构

```txt
ModelProto                         整份模型（.onnx 根）
├── OperatorSetIdProto[]           用了哪套算子
├── StringStringEntryProto[]       元数据键值
├── GraphProto                     主计算图
│   ├── ValueInfoProto[]           input / output / value_info
│   │     └── TypeProto            类型 + 形状
│   │           └── TensorShapeProto
│   ├── TensorProto[]              initializer（权重）
│   ├── SparseTensorProto[]        稀疏权重（少见）
│   ├── NodeProto[]                算子
│   │     ├── 边：input/output 名字
│   │     └── AttributeProto[]     参数（可再嵌 Tensor / Graph）
│   └── TensorAnnotation[]         量化标注（可选）
├── FunctionProto[]                模型内复合函数（可选）
└── TrainingInfoProto[]            训练算法图（可选）
```

### 计算图

计算被建模成DAG有向无环图，张量沿边流动，节点是算子

```txt
ModelProto                 一份模型
├── 版本 / 生产者 / opset
└── GraphProto             一张图
    ├── Value（名字）       每条边一个字符串
    ├── Node               算子：op_type + 入边 + 出边 + attribute
    └── Initializer        已经有值的张量（权重）
```

### 算子说明书Opset

ONNX中只存`op_type = "Softmax"`算子名字, 具体计算方法在`Operator Set`里.

### 张量tensor

ONNX张量通过字符串名字在多个`Message`里面对齐，同一个tensor名字对应DAG上一条边。

* `ValueInfoProto.name + type`表示这个`tensor`类型和形状
* `TensorProto.name + raw_data`表示这个`tensor`已有的数据
* `NodeProto.input / output`表示在DAG中算子连接关系

## 关键Message

### ModelProto

`ModelProto`是一种顶层文件/容器格式，用于捆绑`ML`模型并将其计算图与元数据关联起来。把「一张（或几张）计算图」和「元数据、算子版本」打包成一个文件。

```proto
Model
  ir_version: 10
  producer_name: "pytorch"
  opset_import:
    domain: ""
    version: 17
  graph:
    name: "SoftmaxGraph"
```

#### optional int64 ir_version

这份文件按哪一版`IR`语法写的,必须有。例如

```proto
enum Version {
    ...
    // IR VERSION 10 published on March 25, 2024
    // Added UINT4, INT4, overload field for functions and metadata_props on multiple proto definitions.
    IR_VERSION_2024_3_25 = 0x000000000000000A;
    ...
}
```

```proto
message ModelProto {
  ...
  // The version of the IR this model targets. See Version enum above.
  // This field MUST be present.
  optional int64 ir_version = 1;
  ...
}
```

#### repeated OperatorSetIdProto opset_import

本模型依赖的算子集

```proto
message ModelProto {
  ...
  // The OperatorSets this model relies on.
  // All ModelProtos MUST have at least one entry that
  // specifies which version of the ONNX OperatorSet is
  // being imported.
  //
  // All nodes in the ModelProto's graph will bind against the operator
  // with the same-domain/same-op_type operator with the HIGHEST version
  // in the referenced operator sets.
  repeated OperatorSetIdProto opset_import = 8;
  ...
}
```

#### optional GraphProto graph

推理要执行的那张主图。加载后几乎所有工作都进这里。

```proto
message ModelProto {
  ...
  // The parameterized graph that is evaluated to execute the model.
  optional GraphProto graph = 7;
    ...
}
```

#### optional string producer_name / optional string producer_version

导出这个ONNX的框架/工具名和版本。应当填写。

```proto
message ModelProto {
  ...
  // The name of the framework or tool used to generate this model.
  // This field SHOULD be present to indicate which implementation/tool/framework
  // emitted the model.
  optional string producer_name = 2;

  // The version of the framework or tool used to generate this model.
  // This field SHOULD be present to indicate which implementation/tool/framework
  // emitted the model.
  optional string producer_version = 3;
  ...
}
```

#### optional string domain

模型自己的命名空间，用反向域名。和 `model_version`、`GraphProto.name` 一起构成图的唯一身份。

```proto
message ModelProto {
  ...
  // Domain name of the model.
  // We use reverse domain names as name space indicators. For example:
  // `com.facebook.fair` or `com.microsoft.cognitiveservices`
  //
  // Together with `model_version` and GraphProto.name, this forms the unique identity of
  // the graph.
  optional string domain = 4;
  ...
}
```

#### optional int64 model_version

模型（图）的版本。

```proto
message ModelProto {
  ...
  // The version of the graph encoded. See Version enum below.
  optional int64 model_version = 5;
  ...
}
```

#### optional string doc_string

人类可读的说明，允许 Markdown。

```proto
message ModelProto {
  ...
  // A human-readable documentation for this model. Markdown is allowed.
  optional string doc_string = 6;
  ...
}
```

#### repeated StringStringEntryProto metadata_props

任意键值元数据，key 应互不相同。

```proto
message ModelProto {
  ...
  // Named metadata values; keys should be distinct.
  repeated StringStringEntryProto metadata_props = 14;
  ...
}
```

#### repeated TrainingInfoProto training_info

训练用的初始化图、更新图。空则训练行为未定义。

```proto
message ModelProto {
  ...
  // Training-specific information. Sequentially executing all stored
  // `TrainingInfoProto.algorithm`s and assigning their outputs following
  // the corresponding `TrainingInfoProto.update_binding`s is one training
  // iteration. Similarly, to initialize the model
  // (as if training hasn't happened), the user should sequentially execute
  // all stored `TrainingInfoProto.initialization`s and assigns their outputs
  // using `TrainingInfoProto.initialization_binding`s.
  //
  // If this field is empty, the training behavior of the model is undefined.
  repeated TrainingInfoProto training_info = 20;
  ...
}
```

#### repeated FunctionProto functions

模型内部的复合算子定义。`(domain, name, overload)` 必须唯一；函数之间可以互相引用，但不能递归。

```proto
message ModelProto {
  ...
  // A list of function protos local to the model.
  //
  // The (domain, name, overload) tuple must be unique across the function protos in this list.
  // In case of any conflicts the behavior (whether the model local functions are given higher priority,
  // or standard operator sets are given higher priority or this is treated as error) is defined by
  // the runtimes.
  //
  // The operator sets imported by FunctionProto should be compatible with the ones
  // imported by ModelProto and other model local FunctionProtos.
  // Example, if same operator set say 'A' is imported by a FunctionProto and ModelProto
  // or by 2 FunctionProtos then versions for the operator set may be different but,
  // the operator schema returned for op_type, domain, version combination
  // for both the versions should be same for every node in the function body.
  //
  // One FunctionProto can reference other FunctionProto in the model, however, recursive reference
  // is not allowed.
  repeated FunctionProto functions = 25;
  ...
}
```

#### repeated DeviceConfigurationProto configuration

多设备配置。一份模型可以描述多种执行配置。

```proto
message ModelProto {
  ...
  // Describes different target configurations for a multi-device use case.
  // A model MAY describe multiple multi-device configurations for execution.
  repeated DeviceConfigurationProto configuration = 26;
  ...
}
```

### OperatorSetIdProto

`OperatorSetIdProto` 是一份算子集的身份：`(domain, version)`。`ModelProto.opset_import` 里至少要有一条。

#### optional string domain

算子家族。空字符串 `""` 或未填 = 官方 ONNX 算子集（`ai.onnx`）其它`ai.onnx.ml`、`com.microsoft`。

```proto
message OperatorSetIdProto {
  ...
  // The domain of the operator set being identified.
  // The empty string ("") or absence of this field implies the operator
  // set that is defined as part of the ONNX specification.
  // This field MUST be present in this version of the IR when referring to any other operator set.
  optional string domain = 1;
  ...
}
```

#### optional int64 version

算子集的具体版本（11、14、17…）。

```proto
message OperatorSetIdProto {
  ...
  // The version of the operator set being identified.
  // This field MUST be present in this version of the IR.
  optional int64 version = 2;
  ...
}
```

### GraphProto

一张 DAG，就是模型的网络本身：节点列表 + 权重 + 边界。节点按拓扑序排列，边用张量名字连接。

```proto
Graph
  name: "SoftmaxGraph"
  input:  ["attn_scores"]
  output: ["attn_probs"]
  node:
    op_type: "Softmax"
    input:  ["attn_scores"]
    output: ["attn_probs"]
```

#### repeated NodeProto node

图里所有算子，按拓扑序。

```proto
message GraphProto {
  ...
  // The nodes in the graph, sorted topologically.
  repeated NodeProto node = 1;
  ...
}
```

#### optional string name

图的名字。和 `ModelProto.domain`、`model_version` 一起构成图的唯一身份。

```proto
message GraphProto {
  ...
  // The name of the graph.
  optional string name = 2; // namespace Graph
  ...
}
```

#### repeated TensorProto initializer

图的常量输入，大部分都是权重。每个必须有 `name`，在 `initializer` 与 `sparse_initializer` 里唯一；

```proto
message GraphProto {
  ...
  // A list of named tensor values, used to specify constant inputs of the graph.
  // Each initializer (both TensorProto as well SparseTensorProto) MUST have a name.
  // The name MUST be unique across both initializer and sparse_initializer,
  // but the name MAY also appear in the input list.
  repeated TensorProto initializer = 5;
  ...
}
```

#### repeated SparseTensorProto sparse_initializer

稀疏格式的常量，语义同 `initializer`。

```proto
message GraphProto {
  ...
  // Initializers (see above) stored in sparse format.
  repeated SparseTensorProto sparse_initializer = 15;
  ...
}
```

#### repeated ValueInfoProto input

图入口的名字 + 类型。

```proto
message GraphProto {
  ...
  // The inputs and outputs of the graph.
  repeated ValueInfoProto input = 11;
  ...
}
```

#### repeated ValueInfoProto output

图出口的名字 + 类型。

```proto
message GraphProto {
  ...
  // The inputs and outputs of the graph.
  repeated ValueInfoProto output = 12;
  ...
}
```

#### repeated ValueInfoProto value_info

中间张量的名字 + 类型，可选。有则方便推断 shape；`name` 必须互不相同。

```proto
message GraphProto {
  ...
  // Information for the values in the graph. The ValueInfoProto.name's
  // must be distinct. It is optional for a value to appear in value_info list.
  repeated ValueInfoProto value_info = 13;
  ...
}
```

#### repeated TensorAnnotation quantization_annotation

某个张量和它的 `scale` / `zero_point` 的对应。

```proto
message GraphProto {
  ...
  // This field carries information to indicate the mapping among a tensor and its
  // quantization parameter tensors. For example:
  // For tensor 'a', it may have {'SCALE_TENSOR', 'a_scale'} and {'ZERO_POINT_TENSOR', 'a_zero_point'} annotated,
  // which means, tensor 'a_scale' and tensor 'a_zero_point' are scale and zero point of tensor 'a' in the model.
  repeated TensorAnnotation quantization_annotation = 14;
  ...
}
```

#### optional string doc_string / repeated StringStringEntryProto metadata_props

图的可读说明（允许 Markdown）和任意键值元数据。

```proto
message GraphProto {
  ...
  // A human-readable documentation for this graph. Markdown is allowed.
  optional string doc_string = 10;

  // Named metadata values; keys should be distinct.
  repeated StringStringEntryProto metadata_props = 16;
  ...
}
```

### NodeProto

一个具体算子例如

```proto
Node
  op_type: "Softmax"
  input:  ["attn_scores"]
  output: ["attn_probs"]
  attribute:
    name: "axis"
    type: INT
    i:    -1
```

#### repeated string input

输入边的名字列表，有顺序，对应该算子 `schema` 的第 `1、2、3` 个输入。空字符串 `""` 表示这个可选输入没接。

```proto
message NodeProto {
  ...
  repeated string input = 1; // namespace Value
  ...
}
```

#### repeated string output

输出边的名字，同样有顺序。

```proto
message NodeProto {
  ...
  repeated string output = 2; // namespace Value
  ...
}
```

#### optional string op_type

算子符号名，如 `MatMul`、`Gather`、`Softmax`。

```proto
message NodeProto {
  ...
  // The symbolic identifier of the Operator to execute.
  optional string op_type = 4; // namespace Operator
  ...
}
```

#### optional string domain

这个 `op_type` 属于哪套 `opset`。空 = 标准 ONNX。必须和 `opset_import` 对得上。

```proto
message NodeProto {
  ...
  // The domain of the OperatorSet that specifies the operator named by op_type.
  optional string domain = 7; // namespace Domain
  ...
}
```

#### optional string overload

仅当这个节点绑定模型内 `Function` 且有重载时用。

```proto
message NodeProto {
  ...
  // Overload identifier, used only to map this to a model-local function.
  optional string overload = 8;
  ...
}
```

#### repeated AttributeProto attribute

节点的静态参数（如 Softmax 的 `axis`）。

```proto
message NodeProto {
  ...
  // Additional named attributes.
  repeated AttributeProto attribute = 5;
  ...
}
```

#### optional string name

节点标识，可缺。

```proto
message NodeProto {
  ...
  // An optional identifier for this node in a graph.
  // This field MAY be absent in this version of the IR.
  optional string name = 3; // namespace Node
  ...
}
```

#### optional string doc_string / repeated StringStringEntryProto metadata_props

节点的可读说明（允许 Markdown）和任意键值元数据。

```proto
message NodeProto {
  ...
  // A human-readable documentation for this node. Markdown is allowed.
  optional string doc_string = 6;

  // Named metadata values; keys should be distinct.
  repeated StringStringEntryProto metadata_props = 9;
  ...
}
```

#### repeated NodeDeviceConfigurationProto device_configurations

多设备切分标注，对应 `ModelProto.configuration`。

```proto
message NodeProto {
  ...
  // Configuration of multi-device annotations.
  repeated NodeDeviceConfigurationProto device_configurations = 10;
  ...
}
```

### AttributeProto

节点的静态参数。有 `name`，并且 `f / i / s / t / g / ...` 里只填其中一个。

```proto
Attribute
  name: "axis"
  type: INT
  i:    -1
```

#### optional string name

参数名字。必须填写。

```proto
message AttributeProto {
  ...
  // The name field MUST be present for this version of the IR.
  optional string name = 1; // namespace Attribute
  ...
}
```

#### optional AttributeType type

判别器：INT / INTS / FLOAT / TENSOR / GRAPH … 必须填写，且必须和下面实际有值的那个字段对应，否则不知道该读 `f` 还是 `i`。

```proto
message AttributeProto {
  ...
  // Note: this enum is structurally identical to the OpSchema::AttrType
  // enum defined in schema.h. If you rev one, you likely need to rev the other.
  enum AttributeType {
    UNDEFINED = 0;
    FLOAT = 1;
    INT = 2;
    STRING = 3;
    TENSOR = 4;
    GRAPH = 5;
    SPARSE_TENSOR = 11;
    TYPE_PROTO = 13;
    FLOATS = 6;
    INTS = 7;
    STRINGS = 8;
    TENSORS = 9;
    GRAPHS = 10;
    SPARSE_TENSORS = 12;
    TYPE_PROTOS = 14;
  }

  // The type field MUST be present for this version of the IR.
  // For 0.0.1 versions of the IR, this field was not defined, and
  // implementations needed to use has_field heuristics to determine
  // which value field was in use. For IR_VERSION 0.0.2 or later, this
  // field MUST be set and match the f|i|s|t|... field in use. This
  // change was made to accommodate proto3 implementations.
  optional AttributeType type = 20; // discriminator that indicates which field below is in use
  ...
}
```

#### optional string ref_attr_name

不装数据，去父 Function 里引用同名属性。只允许出现在函数子图里，主图里非法。

```proto
message AttributeProto {
  ...
  // if ref_attr_name is not empty, ref_attr_name is the attribute name in parent function.
  // In this case, this AttributeProto does not contain data, and it's a reference of attribute
  // in parent scope.
  // NOTE: This should ONLY be used in function (sub-graph). It's invalid to be used in main graph.
  optional string ref_attr_name = 21;
  ...
}
```

#### optional string doc_string

可读说明，允许 Markdown。

```proto
message AttributeProto {
  ...
  // A human-readable documentation for this attribute. Markdown is allowed.
  optional string doc_string = 13;
  ...
}
```

#### optional float f / optional int64 i / optional bytes s

单个 `float` / `int64` / UTF-8 字节串。和下面的 `t / g / ...` 一起，有且仅有一个有值。

```proto
message AttributeProto {
  ...
  // Exactly ONE of the following fields must be present for this version of the IR
  optional float f = 2; // float
  optional int64 i = 3; // int
  optional bytes s = 4; // UTF-8 string
  ...
}
```

#### optional TensorProto t

单个嵌套 `TensorProto`（小常量有时放属性里，不走 `initializer`）。

```proto
message AttributeProto {
  ...
  optional TensorProto t = 5; // tensor value
  ...
}
```

#### optional GraphProto g

单个子图。`If` 的 then/else、`Loop` 的 body。

```proto
message AttributeProto {
  ...
  optional GraphProto g = 6; // graph
  ...
}
```

#### optional SparseTensorProto sparse_tensor / optional TypeProto tp

稀疏张量或一个类型。少见。

```proto
message AttributeProto {
  ...
  optional SparseTensorProto sparse_tensor = 22; // sparse tensor value
  optional TypeProto tp = 14; // type proto
  ...
}
```

#### repeated float floats / repeated int64 ints / repeated bytes strings

标量列表，对应 `type = FLOATS / INTS / STRINGS`。

```proto
message AttributeProto {
  ...
  repeated float floats = 7; // list of floats
  repeated int64 ints = 8; // list of ints
  repeated bytes strings = 9; // list of UTF-8 strings
  ...
}
```

#### repeated TensorProto tensors / repeated GraphProto graphs

张量列表、子图列表（`Loop` 等），对应 `type = TENSORS / GRAPHS`。

```proto
message AttributeProto {
  ...
  repeated TensorProto tensors = 10; // list of tensors
  repeated GraphProto graphs = 11; // list of graph
  ...
}
```

#### repeated SparseTensorProto sparse_tensors / repeated TypeProto type_protos

对应的列表形式。

```proto
message AttributeProto {
  ...
  repeated SparseTensorProto sparse_tensors = 23; // list of sparse tensors
  repeated TypeProto type_protos = 15;// list of type protos
  ...
}
```

### ValueInfoProto

声明某个名字的类型和形状，不带数值。`name` 必须填写；顶层图的 `input` / `output` 的 `type` 也必须填写。

```proto
ValueInfo
  name: "attn_scores"
  type:
    tensor_type:
      elem_type: FLOAT
      shape: [batch, heads, seq, seq]
```

#### optional string name

值的名字，和 Node 的边、`initializer` 对齐。必须填写。

```proto
message ValueInfoProto {
  ...
  // This field MUST be present in this version of the IR.
  optional string name = 1; // namespace Value
  ...
}
```

#### optional TypeProto type

是 tensor 还是 sequence / map / optional，以及元素类型和形状。顶层图的入口、出口必须填写。

```proto
message ValueInfoProto {
  ...
  // This field MUST be present in this version of the IR for
  // inputs and outputs of the top-level graph.
  optional TypeProto type = 2;
  ...
}
```

#### optional string doc_string

可读说明，允许 Markdown。

```proto
message ValueInfoProto {
  ...
  // A human-readable documentation for this value. Markdown is allowed.
  optional string doc_string = 3;
  ...
}
```

#### repeated StringStringEntryProto metadata_props

额外键值，key 应互不相同。

```proto
message ValueInfoProto {
  ...
  // Named metadata values; keys should be distinct.
  repeated StringStringEntryProto metadata_props = 4;
  ...
}
```

### TypeProto

值的类型，描述「这个名字是什么种类的值」。`tensor_type` / `sequence_type` / `map_type` / `optional_type` / `sparse_tensor_type` / `opaque_type` 是 `oneof`，有且仅有一个。

```proto
Type
  tensor_type:
    elem_type: FLOAT
    shape: [batch, heads, seq, seq]
```

#### Tensor tensor_type

普通张量。DNN 算子通常只用这一种。

```proto
message TypeProto {
  ...
  message Tensor {
    // This field MUST NOT have the value of UNDEFINED
    // This field MUST have a valid TensorProto.DataType value
    // This field MUST be present for this version of the IR.
    optional int32 elem_type = 1;
    optional TensorShapeProto shape = 2;
  }

  oneof value {
    // The type of a tensor.
    Tensor tensor_type = 1;
    ...
  }
  ...
}
```

##### optional int32 elem_type

元素类型，取值是 `TensorProto.DataType`（`FLOAT`、`INT64` …）。必须填写，且不能是 `UNDEFINED`。

```proto
message TypeProto {
  message Tensor {
    ...
    optional int32 elem_type = 1;
    ...
  }
}
```

##### optional TensorShapeProto shape

形状，类型是 `TensorShapeProto`。可缺，缺则秩未知。

```proto
message TypeProto {
  message Tensor {
    ...
    optional TensorShapeProto shape = 2;
    ...
  }
}
```

#### Sequence sequence_type

张量（或再套类型）的列表。`elem_type` 必须填写。

```proto
message TypeProto {
  ...
  // repeated T
  message Sequence {
    // The type and optional shape of each element of the sequence.
    // This field MUST be present for this version of the IR.
    optional TypeProto elem_type = 1;
  };

  oneof value {
    ...
    // The type of a sequence.
    Sequence sequence_type = 4;
    ...
  }
  ...
}
```

#### Map map_type

映射：`key_type` 必须是整数或 `STRING`，`value_type` 再套 `TypeProto`。两者都必须填写。

```proto
message TypeProto {
  ...
  // map<K,V>
  message Map {
    // This field MUST have a valid TensorProto.DataType value
    // This field MUST be present for this version of the IR.
    // This field MUST refer to an integral type ([U]INT{8|16|32|64}) or STRING
    optional int32 key_type = 1;
    // This field MUST be present for this version of the IR.
    optional TypeProto value_type = 2;
  };

  oneof value {
    ...
    // The type of a map.
    Map map_type = 5;
    ...
  }
  ...
}
```

#### Optional optional_type

可空包装，包一层 Tensor / Sequence / Map。`elem_type` 必须填写。

```proto
message TypeProto {
  ...
  // wrapper for Tensor, Sequence, or Map
  message Optional {
    // The type and optional shape of the element wrapped.
    // This field MUST be present for this version of the IR.
    // Possible values correspond to OptionalProto.DataType enum
    optional TypeProto elem_type = 1;
  };

  oneof value {
    ...
    // The type of an optional.
    Optional optional_type = 9;
    ...
  }
  ...
}
```

#### SparseTensor sparse_tensor_type

稀疏张量的类型（和 `SparseTensorProto` 数据相对）。`elem_type` 必须填写，且不能是 `UNDEFINED`。

```proto
message TypeProto {
  ...
  message SparseTensor {
    // This field MUST NOT have the value of UNDEFINED
    // This field MUST have a valid TensorProto.DataType value
    // This field MUST be present for this version of the IR.
    optional int32 elem_type = 1;
    optional TensorShapeProto shape = 2;
  }

  oneof value {
    ...
    // Type of the sparse tensor
    SparseTensor sparse_tensor_type = 8;
    ...
  }
  ...
}
```

#### Opaque opaque_type

不透明/自定义类型。`domain` 缺省则用模型的 domain；`name` 可选，但填了就有意义。

```proto
message TypeProto {
  ...
  message Opaque {
    // When missing, the domain is the same as the model's.
    optional string domain = 1;
    // The name is optional but significant when provided.
    optional string name = 2;
  }

  oneof value {
    ...
    Opaque opaque_type = 7;
    ...
  }
  ...
}
```

#### optional string denotation

语义标注（例如这是一张图、一个音频），给工具用，算子不用。

```proto
message TypeProto {
  ...
  // An optional denotation can be used to denote the whole
  // type with a standard semantic description as to what is
  // stored inside. Refer to https://github.com/onnx/onnx/blob/main/docs/TypeDenotation.md#type-denotation-definition
  // for pre-defined type denotations.
  optional string denotation = 6;
  ...
}
```

### TensorShapeProto

描述各维长度。每一维可以是定数，也可以是符号。`[1, 8, 768]` 是三个 `dim_value`；`[batch, seq, 768]` 前两维是 `dim_param`。

```proto
Shape
  dim: [
    { dim_param: "batch" },
    { dim_param: "seq" },
    { dim_value: 768 }
  ]
```

#### repeated Dimension dim

从外到内的各维。每一维是一个 `Dimension`：`dim_value` 和 `dim_param` 是 `oneof`，有且仅有一个。

```proto
message TensorShapeProto {
  // Defines a tensor shape. A dimension can be either an integer value
  // or a symbolic variable. A symbolic variable represents an unknown
  // dimension.
  message Dimension {
    oneof value {
      int64 dim_value = 1;
      string dim_param = 2; // namespace Shape
    };
    ...
  };
  repeated Dimension dim = 1;
}
```

##### int64 dim_value

写死的长度，如 `768`、`50257`。

```proto
message TensorShapeProto {
  message Dimension {
    oneof value {
      int64 dim_value = 1;
      ...
    };
    ...
  };
}
```

##### string dim_param

符号维，如 `batch`、`seq_len`。同一符号在全图应表示同一长度。

```proto
message TensorShapeProto {
  message Dimension {
    oneof value {
      ...
      string dim_param = 2; // namespace Shape
    };
    ...
  };
}
```

##### optional string denotation

维的语义（`DATA_BATCH`、`DATA_CHANNEL` 等），可选，用来保证算子作用在正确的轴上。

```proto
message TensorShapeProto {
  message Dimension {
    ...
    // Standard denotation can optionally be used to denote tensor
    // dimensions with standard semantic descriptions to ensure
    // that operations are applied to the correct axis of a tensor.
    // Refer to https://github.com/onnx/onnx/blob/main/docs/DimensionDenotation.md#denotation-definition
    // for pre-defined dimension denotations.
    optional string denotation = 3;
  };
}
```

### TensorProto

序列化后的一块数据，带着真正的数值。出现在 `initializer`、属性 `t`，以及任何常量。行优先（row-major）。

读权重：先看 `data_location`；`DEFAULT` 时优先 `raw_data`，空再读 typed 字段。只看 `float_data` 会以为权重是空的。

```proto
Tensor
  name: "W"
  dims: [768, 768]
  data_type: FLOAT
  data_location: DEFAULT
  raw_data: ...
```

#### optional string name

作为图中值的名字，和边对齐。在 `initializer` 里必须有。

```proto
message TensorProto {
  ...
  // Optionally, a name for the tensor.
  optional string name = 8; // namespace Value
  ...
}
```

#### repeated int64 dims

各维大小，乘起来是元素个数。这里是具体整数，没有符号维（符号在 `TypeProto` / `TensorShapeProto` 里）。

```proto
message TensorProto {
  ...
  // The shape of the tensor.
  repeated int64 dims = 1;
  ...
}
```

#### optional int32 data_type

元素类型枚举。必须是合法的 `TensorProto.DataType`。决定下面哪个 `*_data` 字段合法，以及 `raw_data` 怎么解释。

```proto
message TensorProto {
  enum DataType {
    UNDEFINED = 0;
    FLOAT = 1;
    UINT8 = 2;
    INT8 = 3;
    ...
    INT32 = 6;
    INT64 = 7;
    STRING = 8;
    BOOL = 9;
    FLOAT16 = 10;
    DOUBLE = 11;
    ...
  }

  // The data type of the tensor.
  // This field MUST have a valid TensorProto.DataType value
  optional int32 data_type = 2;
  ...
}
```

#### optional string doc_string / repeated StringStringEntryProto metadata_props

可读说明（允许 Markdown）和任意键值元数据。

```proto
message TensorProto {
  ...
  // A human-readable documentation for this tensor. Markdown is allowed.
  optional string doc_string = 12;

  // Named metadata values; keys should be distinct.
  repeated StringStringEntryProto metadata_props = 16;
  ...
}
```

#### optional Segment segment

把超大张量切成多段存时的 `[begin, end)`。少见。

```proto
message TensorProto {
  ...
  // For very large tensors, we may want to store them in chunks, in which
  // case the following fields will specify the segment that is stored in
  // the current TensorProto.
  message Segment {
    optional int64 begin = 1;
    optional int64 end = 2;
  }
  optional Segment segment = 3;
  ...
}
```

数据在哪：`DEFAULT` 存在本 message 里，`EXTERNAL` 在旁边文件。本 message 里时，`raw_data` 和各个 typed `*_data` 有且仅有一类有值；字符串必须走 `string_data`。

#### optional DataLocation data_location

`DEFAULT`：在本 message 里；`EXTERNAL`：在旁边文件。未设则当 `DEFAULT`：先 `raw_data`，空再读 typed 字段。

```proto
message TensorProto {
  ...
  // Location of the data for this tensor. MUST be one of:
  // - DEFAULT - data stored inside the protobuf message. Data is stored in raw_data (if set) otherwise in type-specified field.
  // - EXTERNAL - data stored in an external location as described by external_data field.
  enum DataLocation {
    DEFAULT = 0;
    EXTERNAL = 1;
  }

  // If value not set, data is stored in raw_data (if set) otherwise in type-specified field.
  optional DataLocation data_location = 14;
  ...
}
```

#### repeated StringStringEntryProto external_data

外置时的键值：`location`（相对 `.onnx` 的路径，必填）、`offset`、`length`、`checksum`。

```proto
message TensorProto {
  ...
  // Data can be stored inside the protobuf file using type-specific fields or raw_data.
  // Alternatively, raw bytes data can be stored in an external file, using the external_data field.
  // external_data stores key-value pairs describing data location. Recognized keys are:
  // - "location" (required) - POSIX filesystem path relative to the directory where the ONNX
  // protobuf model was stored
  // - "offset" (optional) - position of byte at which stored data begins. Integer stored as string.
  // Offset values SHOULD be multiples 4096 (page size) to enable mmap support.
  // - "length" (optional) - number of bytes containing data. Integer stored as string.
  // - "checksum" (optional) - SHA1 digest of file specified in under 'location' key.
  repeated StringStringEntryProto external_data = 13;
  ...
}
```

#### optional bytes raw_data

权重主战场。定宽、小端、IEEE754。`STRING` / `UNDEFINED` 禁止用这个字段。

```proto
message TensorProto {
  ...
  // Serializations can either use one of the fields above, or use this
  // raw bytes field. The only exception is the string case, where one is
  // required to store the content in the repeated bytes string_data field.
  //
  // When this raw_data field is used to store tensor value, elements MUST
  // be stored in as fixed-width, little-endian order.
  // Floating-point data types MUST be stored in IEEE 754 format.
  // ...
  // When this field is present, the data_type field MUST NOT be STRING or UNDEFINED
  optional bytes raw_data = 9;
  ...
}
```

#### repeated float float_data

`FLOAT` / `COMPLEX64` 的 `repeated float`。小常量常见。`COMPLEX64` 按 `[real, imag, real, imag, ...]` 排。

```proto
message TensorProto {
  ...
  // Tensor content must be organized in row-major order.
  //
  // Depending on the data_type field, exactly one of the fields below with
  // name ending in _data is used to store the elements of the tensor.

  // For float and complex64 values
  // ...
  // When this field is present, the data_type field MUST be FLOAT or COMPLEX64.
  repeated float float_data = 4 [packed = true];
  ...
}
```

#### repeated int32 int32_data

多种窄类型的载体：`int8/16`、`bool`、`(b)float16`、`float8` 等按位放进 `int32`。

```proto
message TensorProto {
  ...
  // For int32, uint8, int8, uint16, int16, uint4, int4, uint2, int2, bool, (b)float16, float8, and float4:
  // - (b)float16 and float8 values MUST be converted bit-wise into an unsigned integer
  // representation before being written to the buffer.
  // ...
  // When this field is present, the data_type field MUST be
  // INT32, INT16, INT8, ...
  repeated int32 int32_data = 5 [packed = true];
  ...
}
```

#### repeated int64 int64_data

`INT64`，如 shape 常量、`Gather` 的 indices 常量。

```proto
message TensorProto {
  ...
  // For int64.
  // When this field is present, the data_type field MUST be INT64
  repeated int64 int64_data = 7 [packed = true];
  ...
}
```

#### repeated double double_data / repeated uint64 uint64_data

`DOUBLE` / `COMPLEX128` 走 `double_data`；`UINT32` / `UINT64` 走 `uint64_data`。

```proto
message TensorProto {
  ...
  // For double
  // ...
  // When this field is present, the data_type field MUST be DOUBLE or COMPLEX128
  repeated double double_data = 10 [packed = true];

  // For uint64 and uint32 values
  // When this field is present, the data_type field MUST be
  // UINT32 or UINT64
  repeated uint64 uint64_data = 11 [packed = true];
  ...
}
```

#### repeated bytes string_data

字符串张量必须走这里，不能用 `raw_data`。每个元素是 UTF-8，无尾 `NUL`、无 BOM。

```proto
message TensorProto {
  ...
  // For strings.
  // Each element of string_data is a UTF-8 encoded Unicode
  // string. No trailing null, no leading BOM. The protobuf "string"
  // scalar type is not used to match ML community conventions.
  // When this field is present, the data_type field MUST be STRING
  repeated bytes string_data = 6;
  ...
}
```
