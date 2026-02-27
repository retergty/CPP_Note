#include "pimpl_api.hpp"
#include <sndfile.h> // 脏活累活全放在这里

// 给出真实定义
struct AudioDumper::Impl {
    SNDFILE* fp;
    uint32_t write_idx{0};
    uint32_t read_idx{0};
    
    // 甚至可以有内部专用的辅助方法
    void update_indices() { /* ... */ }
};

// 构造函数：真正分配内存
AudioDumper::AudioDumper() : pImpl(std::make_unique<Impl>()) {
    // pImpl->fp = sf_open(...);
}

// 【关键必错点】：必须在 Impl 完整定义之后，再 default 析构函数
// 否则 unique_ptr 会因为无法 delete 不完整类型而报错
AudioDumper::~AudioDumper() = default; 

// 业务方法转发
void AudioDumper::write(const void* data, int size) {
    // 实际干活的是内部指针
    // pImpl->write_idx += size; 
}
