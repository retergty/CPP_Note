# allocator

分配器(allocator)解决动态生命周期对象的存储空间的分配。

分配器需求文档

* [C++ named requirements: Allocator](https://en.cppreference.com/w/cpp/named_req/Allocator#Allocator_completeness_requirements)

## 默认分配器

参考文档

* [allocator](https://en.cppreference.com/w/cpp/memory/allocator)

定义在`<memory>`中

### 原型

```CPP
template< class T >
struct allocator;
```

### 描述

`std::allocator`是所有标准库容器的默认分配器。默认分配器是无状态的，它所有的实例都是可交换的，被认为是相等的。

### 成员函数

* [allocate](https://en.cppreference.com/w/cpp/memory/allocator/allocate)

  ```CPP
  T* allocate( std::size_t n );
  ```

  分配`n*sizeof(T)`的未初始化存储空间，通过调用`::operator new(std::size_t)`或`::operator new(std::size_t, std::align_val_t)`，之后，这个函数创造了一个数组类型`T[n]`并开始了它的生命周期，但是不开始它元素的生命周期。

  返回指向数组第一个元素的指针`T*`,这些元素尚未被构建。

* [deallocate](https://en.cppreference.com/w/cpp/memory/allocator/deallocate)

  ```CPP
  void deallocate( T* p, std::size_t n );
  ```

  释放`p`指向的内存空间，`p`必须与之前使用`allocate`函数所返回的指针相等，`n`必须与之前使用`allocate`所传递的参数相等。`p`指向的数组元素应该已经析构。

  调用`::operator delete(void*)`或`::operator delete(void*, std::align_val_t)`.

### 非成员函数

* [construct_at](https://en.cppreference.com/w/cpp/memory/construct_at)

  ```CPP
  template< class T, class... Args >
  constexpr T* construct_at( T* p, Args&&... args );
  ```

  在指定位置`p`使用参数`args`构建`T`.

  等价于

  ```CPP
  return ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
  ```

* [destroy_n](https://en.cppreference.com/w/cpp/memory/destroy_n)

  ```CPP
  template< class ForwardIt, class Size >
  ForwardIt destroy_n( ForwardIt first, Size n );
  template< class ExecutionPolicy, class ForwardIt, class Size >
  ForwardIt destroy_n( ExecutionPolicy&& policy, ForwardIt first, Size n );
  ```

  销毁从`first`开始的`n`个对象，为每个对象调用它的析构函数，对象类型为`*first`.

* [destroy_at](https://en.cppreference.com/w/cpp/memory/destroy_at)

  ```CPP
  template< class T >
  void destroy_at( T* p );
  ```

  销毁`p`指向的地址的`T`对象，为它调用析构函数。

* [destroy](https://en.cppreference.com/w/cpp/memory/destroy)

  ```CPP
  template< class ForwardIt >
  void destroy( ForwardIt first, ForwardIt last );
  template< class ExecutionPolicy, class ForwardIt >
  void destroy( ExecutionPolicy&& policy, ForwardIt first, ForwardIt last );
  ```

  销毁范围为`[first,last)`的元素，为每个对象调用它的析构函数。

## 自定义分配器

自定义分配器，我们只需要给出一个类，并定义以下内容

* `value_type`,是一个类型别名，表示我们分配器可以分配的数据类型。
* `allocate`函数，如同默认分配器一般。
* `deallocate`函数，如同默认分配器一般。

### 自定义分配器实例

```CPP
// C++ Program to show how to create custom allocator 
#include <iostream> 
#include <vector> 
using namespace std; 

// Custom memory allocator class 
template <typename T> class myClass { 
public: 
	typedef T value_type; 
	// Constructor 
	myClass() noexcept {} 
	// Allocate memory for n objects of type T 
	T* allocate(std::size_t n) 
	{ 
		return static_cast<T*>( 
			::operator new(n * sizeof(T))); 
	} 
	// Deallocate memory 
	void deallocate(T* p, std::size_t n) noexcept 
	{ 
		::operator delete(p); 
	} 
}; 
int main() 
{ 
	// Define a vector with the custom allocator 
	vector<int, myClass<int> > vec; 
	for (int i = 1; i <= 5; ++i) { 
		vec.push_back(i); 
	} 
	// Print the elements 
	for (const auto& elem : vec) { 
		cout << elem << " "; 
	} 
	cout << endl; 
	return 0; 
}
```