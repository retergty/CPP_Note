#pragma once
#include <memory>
// 绝对不要在这里 include 底层库（如 sndfile.h）

class AudioDumper {
public:
    AudioDumper();
    ~AudioDumper(); // 【关键必错点】：必须在头文件声明，不能写内联实现！

    void write(const void* data, int size);

private:
    struct Impl; // 前向声明内部结构
    std::unique_ptr<Impl> pImpl; // 依靠智能指针持有
};
