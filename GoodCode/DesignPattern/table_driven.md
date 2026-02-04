# 表驱动法

表驱动法（Table-Driven Method）是一种编程技术，通过使用数据结构（如数组、字典或表格）来替代复杂的条件语句（如if-else或switch-case），从而简化代码逻辑，提高代码的可读性和可维护性。

## 例子

假设你需要写一个函数，把“月份的数字”转换成“月份的英文名称”。

```CPP
std::string getMonthName(int month) {
    if (month == 1) return "January";
    else if (month == 2) return "February";
    else if (month == 3) return "March";
    // ... 写到 12 月 ...
    else return "Unknown";
}
```

* 复杂度高，代码冗长，不易维护。

使用表驱动法，可以创建一个数组或字典来存储月份名称：

```CPP
std::string getMonthName(int month) {
    // 静态表：只初始化一次
    static const std::string months[] = {
        "Unknown", "January", "February", "March", "April", /* ... */
    };

    if (month < 1 || month > 12) return months[0];
    
    // 直接访问，O(1) 复杂度
    return months[month]; 
}
```

## 三种常见形式

1. 直接访问表：如上例所示，使用数组或字典直接存储映射关系。
   1. 适用场景：key是连续整数或有限离散值。
   2. 数据结构：数组。
2. 索引访问表：通过计算索引来访问表中的值。
   1. 适用场景：Key 是不连续的整数，或者跨度很大（例如：ID 从 1000 到 9999，但只有 50 个有效）。
   2. 数据结构：哈希表或映射（Map）。
3. 函数指针表：将函数指针存储在表中，根据输入选择调用不同的函数。
   1. 适用场景：需要根据输入执行不同的操作。
   2. 数据结构：数组或字典，存储函数指针。

## 函数指针表

假设需要写一个消息处理器，根据消息类型调用不同的处理函数：

```CPP
void process(int msg_id) {
    switch (msg_id) {
        case 0: handleLogin(); break;
        case 1: handleLogout(); break;
        case 2: handleMessage(); break;
        case 3: handleHeartbeat(); break;
        // ... 如果有 100 个消息，这里会爆炸 ...
    }
}
```

使用表驱动法，可以创建一个函数指针表：

```CPP
#include <functional>
#include <unordered_map>

// 定义函数类型
using Handler = std::function<void()>;

// 建立映射表
std::unordered_map<int, Handler> dispatch_table = {
    {0, handleLogin},
    {1, handleLogout},
    {2, handleMessage},
    {3, handleHeartbeat}
};

void process(int msg_id) {
    auto it = dispatch_table.find(msg_id);
    if (it != dispatch_table.end()) {
        it->second(); // 找到函数并执行
    } else {
        logError("Unknown message");
    }
}
```

* 优点：代码简洁，易于扩展和维护。
* 缺点：需要额外的内存来存储表格。

