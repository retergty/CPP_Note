# ThreadSanitizer

`ThreadSanitizer`是一个线程数据竞争检测工具，数据竞争发生在当两个线程**同时访问一个变量**且至少有一个访问是**写访问**。`C++11`后，数据竞争被禁用，是未定义行为。

参考文档

* [ThreadSanitizerCppManual](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)

```CPP
#include <pthread.h>
#include <stdio.h>
#include <string>
#include <map>

typedef std::map<std::string, std::string> map_t;

void *threadfunc(void *p) {
  map_t& m = *(map_t*)p;
  m["foo"] = "bar";
  return 0;
}

int main() {
  map_t m;
  pthread_t t;
  pthread_create(&t, 0, threadfunc, &m);
  printf("foo=%s\n", m["foo"].c_str());
  pthread_join(t, 0);
}
```

## 用法

编译时指定`-fsanitize=thread`即可。
