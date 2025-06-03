# protobuf

`protobuf`是谷歌开发的平台无关，语言无关可扩展的机制，用于序列化(serialize)串行数据.

如同`ROS2`的`Message`一样.

参考文档

* [Protobuf Overview](https://protobuf.dev/overview/)
* [Protobuf Github](https://github.com/protocolbuffers/protobuf)

## .proto文件

`.proto`文件是`protobuf`用来生成对应语言代码的原型文件.定义方法和`ROS2`的`msg`文件类似.

```proto
syntax = "proto3";
/**
 * SearchRequest represents a search query, with pagination options to
 * indicate which results to include in the response.
 */
message SearchRequest {
  string query = 1;

  // Which page number do we want?
  repeated int32 page_number = 2;

  // Number of results to return per page.
  optional int32 results_per_page = 3;
}
```

`syntax`指明语法格式.

每个字段都有一个独有的数字用于序列化标识.

`optional`修饰符表示这是可选字段,如果没有设置，不会被序列化.

`repeated`修饰符表示这个字段会重复，与`C++`的动态数组`std::vector`对应.

### 基本类型与`C++`对应类型

* double -- double
* float -- float
* int32 -- int32_t
* int64 -- int64_t
* uint32 -- uint32_t
* uint64 -- uint64_t
* sint32 -- int32_t
* sint64 -- int64_t
* fixed32 -- uint32_t
* fixed64 -- uint64_t
* sfixed32 -- int32_t
* sfixed64 -- int64_t
* bool -- bool
* string -- std::string
* bytes -- std::string

每个基本类型都有默认值.

### 枚举类型

```proto
enum Corpus {
  CORPUS_UNSPECIFIED = 0;
  CORPUS_UNIVERSAL = 1;
  CORPUS_WEB = 2;
  CORPUS_IMAGES = 3;
  CORPUS_LOCAL = 4;
  CORPUS_NEWS = 5;
  CORPUS_PRODUCTS = 6;
  CORPUS_VIDEO = 7;
}

message SearchRequest {
  string query = 1;
  int32 page_number = 2;
  int32 results_per_page = 3;
  Corpus corpus = 4;
}
```

枚举类型只能是预定义的一系列枚举值.

枚举类型的默认值是第一个枚举成员.且第一个成员的值必须是`0`，并且以`ENUM_TYPE_NAME_UNSPECIFIED`或`ENUM_TYPE_NAME_UNKNOWN`.

### Oneof修饰符

`Oneof`修饰符用于将其内的类型共同使用同一块内存存储，一次只能使用其中一个类型.如同`C++`的`union`一样.

```protobuf
message SampleMessage {
  oneof test_oneof {
    string name = 4;
    SubMessage sub_message = 9;
  }
}
```

注意，`Oneof`里的字段也要有独特的序列号.

### Map

```protobuf
map<string, Project> projects = 3;
```

类似于`C++`的`std::map`不再赘述.

## C++实例

参考文档

* [C++ Generated Code Guide](https://protobuf.dev/reference/cpp/cpp-generated/)

本节使用`C++`演示一个实例.

### .proto文件

```protobuf
syntax = "proto2";

package tutorial;

message Person {
  optional string name = 1;
  optional int32 id = 2;
  optional string email = 3;

  enum PhoneType {
    PHONE_TYPE_UNSPECIFIED = 0;
    PHONE_TYPE_MOBILE = 1;
    PHONE_TYPE_HOME = 2;
    PHONE_TYPE_WORK = 3;
  }

  message PhoneNumber {
    optional string number = 1;
    optional PhoneType type = 2 [default = PHONE_TYPE_HOME];
  }

  repeated PhoneNumber phones = 4;
}

message AddressBook {
  repeated Person people = 1;
}
```

* `package`会把生成的类放到`tutorial`名称空间中.
* 在`message`中定义`message`,会生成`Person`,`Person_PhoneNumber`两个类，并且在`Person`中还会定义`typedef Person_PhoneNumber PhoneNumber;`.

### 生成`.h`,`.cc`文件

```shell
protoc -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/addressbook.proto
```

头文件如下

```CPP
// name
  inline bool has_name() const;
  inline void clear_name();
  inline const ::std::string& name() const;
  inline void set_name(const ::std::string& value);
  inline void set_name(const char* value);
  inline ::std::string* mutable_name();

  // id
  inline bool has_id() const;
  inline void clear_id();
  inline int32_t id() const;
  inline void set_id(int32_t value);

  // email
  inline bool has_email() const;
  inline void clear_email();
  inline const ::std::string& email() const;
  inline void set_email(const ::std::string& value);
  inline void set_email(const char* value);
  inline ::std::string* mutable_email();

  // phones
  inline int phones_size() const;
  inline void clear_phones();
  inline const ::google::protobuf::RepeatedPtrField< ::tutorial::Person_PhoneNumber >& phones() const;
  inline ::google::protobuf::RepeatedPtrField< ::tutorial::Person_PhoneNumber >* mutable_phones();
  inline const ::tutorial::Person_PhoneNumber& phones(int index) const;
  inline ::tutorial::Person_PhoneNumber* mutable_phones(int index);
  inline ::tutorial::Person_PhoneNumber* add_phones();
```

* `*`字段名函数获取该字段的值.
* `set_*`函数设置字段值.
* `has_*`函数检查字段值是否存在。
* `clear_*`函数清除字段.

`repeated`字段特有函数

* `*_size`函数用于`repeated`字段，获取数组长度.
* `*(index)`使用`index`访问数组.
* `mutable_*`用于修改指定的元素。
* `add_*`用于添加新的元素.必须先使用这个函数来添加之后才可以`set`.

字符串有

* `mutable_*`用于获取string的指针.

### 标准方法

每个`message`都有以下的成员函数.

* `bool IsInitialized() const`检查是否所有的`required`字段被设置.
* `string DebugString() const`返回消息的可读表示，对于调试特别有用。
* `void CopyFrom(const Person& from)`
* `void Clear()`清理消息.

### 读取与序列化

每个`message`都有用于写和读的成员函数.

* `bool SerializeToString(string* output) const`将它序列化到`output`中，注意，`std::string`只是一个便捷的容器.
* `bool ParseFromString(const string& data)`从`string`中读取.
* `bool SerializeToOstream(ostream* output) const`,将它序列化到`C++ ostream`中.
* `bool ParseFromIstream(istream* input)`从`C++ istream`中读取.

这只是一部分的函数.
