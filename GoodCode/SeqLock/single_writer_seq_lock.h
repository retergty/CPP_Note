#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <type_traits>

// 符合 C++17 内存模型的单写者 SeqLock。
// 仅允许一个写线程调用 Store；任意数量的读线程可以并发调用 Load。
// 为避免普通 T 并发读写造成 data race，数据以原子字块保存。
template <class T>
class SingleWriterSeqLock {
  using Word = unsigned long long;

  // 通过带初值的 atomic 构造函数完成初始化，兼容 C++17
  struct AtomicWord {
    std::atomic<Word> value{0};
  };

 public:
  static_assert(std::is_trivially_copyable<T>::value,
                "T must be trivially copyable");
  static_assert(std::is_default_constructible<T>::value,
                "T must be default constructible");
  static_assert(sizeof(Word) == 8U, "Word must be 64-bit");
  static_assert(ATOMIC_LLONG_LOCK_FREE == 2,
                "64-bit atomics must be lock-free");

  SingleWriterSeqLock() noexcept : SingleWriterSeqLock(T{}) {}

  explicit SingleWriterSeqLock(const T& initial_value) noexcept {
    const auto words = Encode(initial_value);
    for (std::size_t i = 0; i < kWordCount; ++i) {
      words_[i].value.store(words[i], std::memory_order_relaxed);
    }
  }

  // 仅允许唯一的写线程调用。
  void Store(const T& value) noexcept {
    const auto words = Encode(value);

    // 奇数表示写入进行中，偶数表示数据稳定。
    sequence_.fetch_add(1, std::memory_order_relaxed);

    for (std::size_t i = 0; i < kWordCount; ++i) {
      words_[i].value.store(words[i], std::memory_order_release);
    }

    // 发布完整快照，使读者取得该偶数序列号后可以读取全部字块。
    sequence_.fetch_add(1, std::memory_order_release);
  }

  // 如果读取期间发生写入，则自动重试。
  T Load() const noexcept {
    std::array<Word, kWordCount> words{};

    for (;;) {
      const Word before = sequence_.load(std::memory_order_acquire);
      if ((before & 1U) != 0U) {
        continue;
      }

      for (std::size_t i = 0; i < kWordCount; ++i) {
        words[i] = words_[i].value.load(std::memory_order_acquire);
      }

      // 如果读取到了并发写入的字块，其 acquire 会使写入中的奇数序列号
      // happens-before 最终检查，因此旧偶数不能通过检查。
      const Word after = sequence_.load(std::memory_order_relaxed);
      if (before == after) {
        T result{};
        std::memcpy(&result, words.data(), sizeof(T));
        return result;
      }
    }
  }

 private:
  static constexpr std::size_t kWordCount =
      (sizeof(T) + sizeof(Word) - 1U) / sizeof(Word);

  static std::array<Word, kWordCount> Encode(const T& value) noexcept {
    std::array<Word, kWordCount> words{};
    std::memcpy(words.data(), &value, sizeof(T));
    return words;
  }

  std::atomic<Word> sequence_{0};

  // 不能直接保存一个普通的 T：即使 Load 最后通过 sequence_ 发现冲突并
  // 重试，读取 T 的过程也可能已经与 Store 的写入并发，构成 C++17
  // data race。序列号只能检测快照是否一致，不能让普通内存访问变成合法
  // 的并发访问。因此先把可平凡复制的 T 编码成若干 64 位字块，并让每个
  // 字块都通过 atomic 读写；这样推测性读取本身始终合法，再由 sequence_
  // 判断这些字块是否来自同一次完整写入。
  std::array<AtomicWord, kWordCount> words_;
};
