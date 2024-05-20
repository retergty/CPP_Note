# new

`new`表达式的分配一个空间，并在这个空间上构建对象，可以用户自己定义`new`或者使用全局的`new`.

定义在`new`头文件中。

## `new`表达式

参考文档

* [new expression](https://en.cppreference.com/w/cpp/language/new)

### 语法

```CPP
::(optional) new (type) new-initializer(optional)
::(optional) new type new-initializer(optional)
::(optional) new (placement-args) (type) new-initializer(optional)
::(optional) new (placement-args) type new-initializer (optional)
```

`1`,`2`尝试分配内存空间，并创建一个类型为`type`的对象。

`3`,`4`和`1`,`2`相同，只不过它们给分配函数提供了额外的参数。

### 描述

`new`表达式尝试去分配内存空间，并在分配的内存空间上构造一个对象或者数组，`new`表达式返回`prvalue`的指针，指向构建完毕的对象或者是构建完毕的对象数组的第一个元素。

`1`和`3`是用在`type`本身包含括号时的情况

```CPP
new int(*[10])();    // error: parsed as (new int) (*[10]) ()
new (int (*[10])()); // okay: allocates an array of 10 pointers to functions
```

此外`type`类型的处理是贪心的，它会尽可能处理更长的标识符。

```CPP
new int + 1; // okay: parsed as (new int) + 1, increments a pointer returned by new int
new int * 1; // error: parsed as (new int*) (1)
```

### 分配空间

`new`表达式通过调用合适的分配函数来分配空间，如果`type`不是数组类型，调用函数`operator new`，如果`type`是数组类型，调用函数`operator new[]`.

如果`new`表达式以`::`开头，就只会在全局作用域查找分配函数。否则，会先进行`ADL`。

### Placement new

如果提供了`placement-args`,它们就会作为参数被传递给分配函数，这些分配函数就是`Placement new`,位于标准分配函数`void* operator new(std::size_t, void*)`之后,这个标准分配函数只会**简单地原样返回第二个参数**，这个格式用于在已分配的空间上构建对象。

```CPP
// within any block scope...
{
    // Statically allocate the storage with automatic storage duration
    // which is large enough for any object of type “T”.
    alignas(T) unsigned char buf[sizeof(T)];
 
    T* tptr = new(buf) T; // Construct a “T” object, placing it directly into your 
                          // pre-allocated storage at memory address “buf”.
 
    tptr->~T();           // You must **manually** call the object's destructor
                          // if its side effects is depended by the program.
}                         // Leaving this block scope automatically deallocates “buf”.
```

注意，不能使用`delete`来删除`placement new`，需要手动调用析构函数。

### 初始化

由`new`表达式创建的对象进行初始化的顺序如下

如果`type`不是数组类型

* 如果没有提供`new-initializer`则进行默认初始化。
* 如果`new-initializer`是括号包围的，进行直接初始化
* 如果`new-initializer`是大括号包围的，进行列表初始化

如果`type`是数组类型

* 如果没有提供`new-initializer`,每个元素都进行默认初始化
* 如果`new-initializer`是括号包围的，每个元素都进行值初始化
* 如果`new-initializer`是大括号包围的，数组进行列表初始化

## `operator new`

`operator new`函数会在`new`表达式里被调用，分配存储空间，用户可以自定义特定类的`operator new`，还可以自定义全局`operator new`.

对于`new`表达式，它会调用的`operator new`会有如下的格式

```CPP
void* T::opeartor new(std::size_t count,placement-args)
```

也就是说，会匹配能接受`placement-args`所传递的参数的`opeartor new`函数。

参考文档

* [operator new, operator new[]](https://en.cppreference.com/w/cpp/memory/new/operator_new)

### 替换全局的`operator new`

哪怕没有包含头文件`new`,全局的`operator new`都会在每个翻译单元被隐式定义