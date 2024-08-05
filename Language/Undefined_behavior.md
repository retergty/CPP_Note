# undefined behavior

本文讲解`C++`常见未定义行为，这些行为的结果取决于编译器的具体实现，有时甚至与编译器优化等级有关。需要避免程序的任何未定义行为，尤其是运行时的未定义行为。

## strict aliasing rule

参考文档

* [What is the Strict Aliasing Rule and Why do we care?](https://gist.github.com/shafik/848ae25ee209f698763cffee272a58f8)
* [What is the strict aliasing rule](https://stackoverflow.com/questions/98650/what-is-the-strict-aliasing-rule)

严格别名规则(Strict Aliasing Rule)是`C/C++`编译器的优化假设，指的是解引用不同类型的指针永远不会引用到相同的内存位置，这个假设利用了`C/C++`的类型系统，从而编译器可以更加激进地进行内存访问优化。

### valid aliasing

别名(aliasing)，指的是指向某个对象的指针（或引用），这个指针就是这个对象的别名，合法的别名就是不需要进行类型转换就可以指向某个对象的指针类型。

```CPP
int x = 10;
int *ip = &x;
    
std::cout << *ip << "\n";
*ip = 12;
std::cout << x << "\n";
```

类型为`int*`的指针指向了`int`类型的对象，这是一个合法的别名。所以，编译器必须假定通过`*ip`进行的内存写入会更新`x`所在的内存区域，从而生成出正确的代码。

### invalid aliasing

不合法别名（invalid aliasing），指的是两个不同类型的指针指向了相同的对象，也就是相同的内存位置，不合法别名直接违反了严格别名规则，并在编译器的激进优化下产生错误的结果。

```CPP
int foo( float *f, int *i ) { 
    *i = 1;               
    *f = 0.f;            
   
   return *i;
}

int main() {
    int x = 0;
    
    std::cout << x << "\n";   // Expect 0
    x = foo(reinterpret_cast<float*>(&x), &x);
    std::cout << x << "\n";   // Expect 0?
}
```

当使用优化选项`-O2`时，无论是`gcc`或者是`clang`都会输出

```text
0
1
```

但实际上，`*f`也只向`x`，对`*f`的修改也会影响`x`的值。但是由于严格别名规则，编译器直接计算出返回值并返回`1`了。汇编代码如下。

```assembly
foo(float*, int*): # @foo(float*, int*)
mov dword ptr [rsi], 1  
mov dword ptr [rdi], 0
mov eax, 1                       
ret
```

编译器根据严格别名规则，`float*`不会修改`int*`指向的类型，所以`*i`自然就是`1`了，程序没有必要再访问内存了，所以直接返回`1`.

### 标准

标准没有直接提到这个规则，但是间接暗示了这个规则。

#### C11

`C11`标准提到了，一个对象应该只能让满足以下条件之一的左值表达式访问它所存储的值（包括读写），（1）一个与对象类型相容的类型。（2）一个与对象类型相容的类型，但是加上了`const`.（3）一个与对象相容的类型，但是是它的有符号或无符号版本。（4）一个与对象相容的类型，但是是它的有符号或无符号版本，并加上了`const`.（5）一个结构体或者是联合体有与对象类型相容的成员。（6）字符类型,比如`char`.

#### C++17

`C++17`标准中提到了，如果程序尝试通过一个`glvalue`访问对象存储的值，`glvalue`必须满足以下条件之一，否则行为未定义。

* 对象的动态类型

  ```CPP
  void *p = malloc( sizeof(int) ); // We have allocated storage but not started the lifetime of an object
  int *ip = new (p) int{0};        // Placement new changes the dynamic type of the object to int
  std::cout << *ip << "\n";        // *ip gives us a glvalue expression of type int which matches the dynamic type 
                                    // of the allocated object
  ```

* `cv`限定符版本的对象动态类型。

  ```CPP
  int x = 1;
  const int *cip = &x;
  std::cout << *cip << "\n";  // *cip gives us a glvalue expression of type const int which is a cv-qualified 
                              // version of the dynamic type of x
  ```

* 与对象动态类型相似的的类型

  ```CPP
  int *a[3];
  const int *const *p = a;
  const int *q = p[1]; // ok, read of 'int*' through lvalue of similar type 'const int*'
  ```

* 与对象动态类型相同的，但是是有符号或者是无符号版本

  ```CPP
  // Both si and ui are signed or unsigned types corresponding to each others dynamic types
  // We can see from this godbolt(https://godbolt.org/g/KowGXB) the optimizer assumes aliasing.
  signed int foo( signed int &si, unsigned int &ui ) {
    si = 1;
    ui = 2;

    return si;
  }
  ```

* 与对象动态类型相同的，但是是有符号或者是无符号的`cv`限定符版本

  ```CPP
  signed int foo( const signed int &si1, int &si2); // Hard to show this one assumes aliasing
  ```

* 类或者是联合体包含对象动态类型的成员（非静态成员）

  ```CPP
  struct foo {
  int x;
  };

  // Compiler Explorer example(https://godbolt.org/g/z2wJTC) shows aliasing assumption
  int foobar( foo &fp, int &ip ) {
  fp.x = 1;
  ip = 2;

  return fp.x;
  }

  foo f; 
  foobar( f, f.x ); 
  ```

* 对象动态类型的基类

  ```CPP
  struct foo { int x ; };

  struct bar : public foo {};

  int foobar( foo &f, bar &b ) {
    f.x = 1;
    b.x = 2;

    return f.x;
  }
  ```

* 字符类型，无符号字符类型，`std::byte`类型。

  ```CPP
  int foo( std::byte &b, uint32_t &ui ) {
    b = static_cast<std::byte>('a');
    ui = 0xFFFFFFFF;                   
    
    return std::to_integer<int>( b );  // b gives us a glvalue expression of type std::byte which can alias
                                        // an object of type uint32_t
  }
  ```

### type punning

我们需要违反别名规则的情况大部分是我们需要把一个相同的内存表达不同的意思,比如当网络处理，串口信息读取等情况时,也就是类型双关`type punning`。

```CPP
int x =  1 ;

// In C
float *fp = (float*)&x ;  // Not a valid aliasing

// In C++
float *fp = reinterpret_cast<float*>(&x) ;  // Not a valid aliasing

printf( “%f\n”, *fp ) ;
```

正如上文提到的，这不是一个合法的别名。

#### 使用类型双关

使用`memcpy`，编译器会识别到`memcpy`。

```CPP
void func1( double d ) {
  std::int64_t n;
  std::memcpy(&n, &d, sizeof d); 
  //...
```

```CPP
// Simple operation just return the value back
int foo( unsigned int x ) { return x ; }

// Assume len is a multiple of sizeof(unsigned int) 
int bar( unsigned char *p, size_t len ) {
  int result = 0;

  for( size_t index = 0; index < len; index += sizeof(unsigned int) ) {
    unsigned int ui = 0;                                 
    std::memcpy( &ui, &p[index], sizeof(unsigned int) );

    result += foo( ui ) ;
  }

  return result;
}
```

但是使用`reinterpret_cast`会破坏严格别名规则。

```CPP
// Assume len is a multiple of sizeof(unsigned int) 
int bar( unsigned char *p, size_t len ) {
 int result = 0;

 for( size_t index = 0; index < len; index += sizeof(unsigned int) ) {
   unsigned int ui = *reinterpret_cast<unsigned int*>(&p[index]);

   result += foo( ui );
 }

 return result;
}
```

#### C++20 `bit_cast`

使用`C++20`的`bit_cast`可以安全地使用类型双关。

### 关闭`strict aliasing rule`

如果程序严重依赖类型双关，可以直接关闭严格别名规则，使用选项`-fno-strict-aliasing`

## 线程终止约定

`C++`假设所有线程最终必须满足其中一项

* 终止(`terminates`)
* 调用标准库IO函数
* 进行`volatile`访问
* 同步操作
* 原子操作

如果一个线程并不满足如下的选项，那么代码行为未定义。

比如

```CPP
#include <iostream>

int main() {
    while (1)
        ;
}

void unreachable() {
    std::cout << "Hello World!" << std::endl;
}
```

在有的编译器上会输出`Hello World`.
