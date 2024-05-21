# new and delete

`new`表达式的分配一个空间，并在这个空间上构建对象，可以用户自己定义`new`或者使用全局的`new`.

`delete`表达式析构之前使用`new`所创建的对象并释放`new`所分配的空间，不能用在`placement new`中。

定义在`new`头文件中。

## `new`表达式

参考文档

* [new expression](https://en.cppreference.com/w/cpp/language/new)

### `new`表达式语法

```CPP
::(optional) new (type) new-initializer(optional)
::(optional) new type new-initializer(optional)
::(optional) new (placement-args) (type) new-initializer(optional)
::(optional) new (placement-args) type new-initializer (optional)
```

`1`,`2`尝试分配内存空间，并创建一个类型为`type`的对象。

`3`,`4`和`1`,`2`相同，只不过它们给分配函数提供了额外的参数。

### `new`表达式描述

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

## delete表达式

参考文档

* [delete expression](https://en.cppreference.com/w/cpp/language/delete)

### delete表达式语法

```CPP
::(optional) delete   expression
::(optional) delete[] expression	
```

`expression`是可以转换为指针的类型，或者是`prvalue`的指针，指向要销毁的对象。

`1`销毁一个非数组的对象，是由`new`创建的。

`2`销毁一个数组对象，是由`new[]`创建的。

### delete表达式描述

设`expression`求值结果为`ptr`.

`1`格式中，`ptr`必须满足

* 是空指针
* 指向`new`创建的非数组的对象
* 指向`new`创建的非数组对象的基类

`2`格式中，`ptr`必须满足

* 是空指针
* 指针的值必须与之前使用`new[]`返回的指针的值相等。

`1`和`2`格式必须与之前使用的`new`表达式所匹配，否则行为未定义。

如果`ptr`不为空，`delete`表达式调用对象的析构函数，如果是数组，为数组的每个元素调用析构函数。所以，在`delete`表达式出现的上下文中，析构函数必须可以访问。

之后，`delete`表达式调用释放函数，对于`1`是`operator delete`，对于`2`是`operator delete[]`，释放所分配的空间。

如果`delete`表达式以`::`开头，就只会在全局作用域查找释放函数。否则，会先进行`ADL`。

如果`ptr`为空，不会调用任何析构函数，但是释放函数可能会调用（未指定），但是默认的释放函数保证在传递空指针时不做任何事情。

如果`ptr`是指向`new`分配对象的基类成员的，基类成员的析构函数必须是`virtual`的。

对于`lambda`表达式，不捕获任何变量必须以括号调用。

```CPP
// delete []{ return new int; }(); // parse error
delete ([]{ return new int; })();  // OK
```

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

哪怕没有包含头文件`new`,全局的`operator new`都会在每个翻译单元被隐式定义,如果一个用户定义的非成员函数`operator new`具有和全局`operator new`相同的声明，无论是在哪个源文件中定义的，都会替换掉默认的全局`operator new`,也就是说，标准库的全局`operator new`是`weak`的。

如果有多个自定义的全局`operator new`，则程序错误。

标准库的实现中，不抛出异常的`operator new`是通过调用对应版本的`operator new`实现的，而处理数组的`operator new`是通过连续调用处理对象的`operator new`实现的，所以，用户只需要定义一个抛出异常的处理单个对象的`operator new`就可以了。

```CPP
#include <cstdio>
#include <cstdlib>
#include <new>
 
// no inline, required by [replacement.functions]/3
void* operator new(std::size_t sz)
{
    std::printf("1) new(size_t), size = %zu\n", sz);
    if (sz == 0)
        ++sz; // avoid std::malloc(0) which may return nullptr on success
 
    if (void *ptr = std::malloc(sz))
        return ptr;
 
    throw std::bad_alloc{}; // required by [new.delete.single]/3
}
 
// no inline, required by [replacement.functions]/3
void* operator new[](std::size_t sz)
{
    std::printf("2) new[](size_t), size = %zu\n", sz);
    if (sz == 0)
        ++sz; // avoid std::malloc(0) which may return nullptr on success
 
    if (void *ptr = std::malloc(sz))
        return ptr;
 
    throw std::bad_alloc{}; // required by [new.delete.single]/3
}
 
void operator delete(void* ptr) noexcept
{
    std::puts("3) delete(void*)");
    std::free(ptr);
}
 
void operator delete(void* ptr, std::size_t size) noexcept
{
    std::printf("4) delete(void*, size_t), size = %zu\n", size);
    std::free(ptr);
}
 
void operator delete[](void* ptr) noexcept
{
    std::puts("5) delete[](void* ptr)");
    std::free(ptr);
}
 
void operator delete[](void* ptr, std::size_t size) noexcept
{
    std::printf("6) delete[](void*, size_t), size = %zu\n", size);
    std::free(ptr);
}
 
int main()
{
    int* p1 = new int;
    delete p1;
 
    int* p2 = new int[10]; // guaranteed to call the replacement in C++11
    delete[] p2;
}
```

全局的不分配内存的`placement new`函数,不能被用户自定义，用户可以定义类自有的不分配内存的`operator new`.

```CPP
void* operator new  ( std::size_t count, void* ptr );
void* operator new[]( std::size_t count, void* ptr );
```

### 定义类自有的`operator new`

用于单个对象或者是数组版本的`operator new`都可以定义为类的公有`static`函数，那么，给特定类使用`new`表达式就会匹配这个函数，`static`是可选的，类中的`operator new`都是`static`的。任何定义在类中的`operator new`都会隐藏所有的全局`opreator new`，所以要给出所有的`operator new`定义。

类自有的`operator new`可以被定义为模板函数。

```CPP
#include <cstddef>
#include <iostream>
 
// class-specific allocation functions
struct X
{
    static void* operator new(std::size_t count)
    {
        std::cout << "custom new for size " << count << '\n';
        return ::operator new(count);
    }
 
    static void* operator new[](std::size_t count)
    {
        std::cout << "custom new[] for size " << count << '\n';
        return ::operator new[](count);
    }
};
 
int main()
{
    X* p1 = new X;
    delete p1;
    X* p2 = new X[10];
    delete[] p2;
}
```

## `operator delete`

`operator delete`会在`delete`表达式中调用释放`new`表达式分配的空间，这些空间上的对象已经被`delete`表达式所析构。

当`new`表达式分配失败后，也会调用相应的`operator delete`.

标准库版本的`operator delete`对于`ptr`为空时，不会做任何事。

当标准库版本的`operator delete`返回后，所有指向被释放内存区域的指针变为无效，（但标准没有要求是否程序会检查这些指针，这些指针可能还可以使用，但会带来严重的运行时错误）。

使用指针，重复释放同一个区域的内存是未定义行为。

参考文档

* [operator delete, operator delete[]](https://en.cppreference.com/w/cpp/memory/new/operator_delete)

### 替换全局`operator delete`

哪怕没有包含头文件`new`,全局的`operator delete`都会在每个翻译单元被隐式定义,如果一个用户定义的非成员函数`operator delete`具有和全局`operator delete`相同的声明，无论是在哪个源文件中定义的，都会替换掉默认的全局`operator delete`,也就是说，标准库的全局`operator delete`是`weak`的。

如果有多个自定义的全局`operator delete`，则程序错误。

标准库的实现中，不抛出异常的`operator delete`是通过调用对应版本的`operator delete`实现的，而处理数组的`operator delete`是通过连续调用处理对象的`operator delete`实现的，所以，用户只需要定义一个抛出异常的处理单个对象的`operator delete`就可以了。当然，现在的`operator delete`都不会抛出异常了，但是函数签名不一样。

### 定义类自有的`operator delete`

用于单个对象或者是数组版本的`operator delete`都可以定义为类的公有`static`函数，那么，给特定类使用`delete`表达式就会匹配这个函数，`static`是可选的，类中的`operator delete`都是`static`的。任何定义在类中的`operator delete`都会隐藏所有的全局`opreator delete`，所以要给出所有的`operator delete`定义。

类自有的`operator delete`可以被定义为模板函数。

如果类的静态类型与`delete`表达式传递的动态类型不一致，比如通过一个指向基类的指针可以指向派生类，且类的析构函数是`virtual`的，那么用于单个对象的`delete`表达式会从这个析构函数的`override`修饰的派生类开始查找`operator delete`。换句话说，删除一个数组，但是使用指向基类的指针，或者是析构函数不为`virtual`，但是使用基类的指针，程序行为未定义。

```CPP
#include <cstddef>
#include <iostream>
 
// sized class-specific deallocation functions
struct X
{
    static void operator delete(void* ptr, std::size_t sz)
    {
        std::cout << "custom delete for size " << sz << '\n';
        ::operator delete(ptr);
    }
 
    static void operator delete[](void* ptr, std::size_t sz)
    {
        std::cout << "custom delete for size " << sz << '\n';
        ::operator delete[](ptr);
    }
};
 
int main()
{
    X* p1 = new X;
    delete p1;
 
    X* p2 = new X[10];
    delete[] p2;
}
```

## 线程安全

以下的函数被要求是线程安全的

* 库函数版本的`operator new`和库函数版本的`operator delete`.
* 用户替换全局版本的`operator new`和`operator delete`.
* `std::calloc`,`std::malloc`, `std::realloc`, `std::aligned_alloc`,`std::free`

调用这些函数分配或者释放一个特定单元的存储空间发生在一个总的顺序中，并且每次的释放操作会在下一次的分配操作前完成。
