# AddressSanitizer

`AddressSanitizer`是由谷歌开发的，可以检测运行时程序地址错误的检测工具。

参考文档

* [AddressSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer)

## 用法

在编译和连接时指定`-fsanitize=address`即可。

## 可以发现的问题

### 内存释放后访问(Use after free)

指针解引用到一个已经释放的内存区域。

```CPP
int main(int argc, char **argv) {
  int *array = new int[100];
  delete [] array;
  return array[argc];  // BOOM
}
```

### 堆缓冲区溢出(Heap buffer overflow)

指针解引用到超出申请的内存区域

```CPP
int main(int argc, char **argv) {
  int *array = new int[100];
  array[0] = 0;
  int res = array[argc + 100];  // BOOM
  delete [] array;
  return res;
}
```

### 栈缓冲区溢出(Stack buffer overflow)

使用超出申请的栈区域

```CPP
int main(int argc, char **argv) {
  int stack_array[100];
  stack_array[1] = 0;
  return stack_array[argc + 100];  // BOOM
}
```

### 全局缓冲区溢出(Global buffer overflow)

使用超出申请的全局区域

```CPP
int global_array[100] = {-1};
int main(int argc, char **argv) {
  return global_array[argc + 100];  // BOOM
}
```

### 返回后继续使用(Use after return)

```CPP
int *ptr;
__attribute__((noinline))
void FunctionThatEscapesLocalObject() {
  int local[100];
  ptr = &local[0];
}

int main(int argc, char **argv) {
  FunctionThatEscapesLocalObject();
  return ptr[argc];
}
```

### 超出作用域的访问(Use after scope)

```CPP
volatile int *p = 0;

int main() {
  {
    int x = 0;
    p = &x;
  }
  *p = 5;
  return 0;
}
```

### 内存泄漏(Memory leaks)

检测内存泄漏

```CPP
#include <stdlib.h>

void *p;

int main() {
  p = malloc(7);
  p = 0; // The memory is leaked here.
  return 0;
}
```
