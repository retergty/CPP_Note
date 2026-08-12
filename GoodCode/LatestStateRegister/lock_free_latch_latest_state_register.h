#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>

// 无锁的最新状态寄存器（Linux seqcount_latch 风格），单写者、多读者。
// sequence_ 最低位选择 data[0]/data[1]：写前把读者赶到另一边，改完再切回
// 并同步副本。读侧返回快照拷贝，可能因切边而重试。
//
// 冲突时刻：读写冲突只发生在 sequence_ 切边（seq++），而非改暗边整段写入。
// 读者暴露窗口是整次读亮边的时间；若其间发生任意一次 seq++ 则 Load 重试。
// 切边本身极短；T 越大/读越慢/写越勤，撞上重试的概率越高。
//
// 对比同目录三缓冲：三缓冲为 SPSC、可按引用读且不重试；latch 支持多读者，
// 但 T 须可平凡复制，且读路径有拷贝/重试。
template <class T>
class LockFreeLatchLatestStateRegister {
  using Word = unsigned long long;

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

  LockFreeLatchLatestStateRegister() noexcept
      : LockFreeLatchLatestStateRegister(T{}) {}

  explicit LockFreeLatchLatestStateRegister(const T& initial_value) noexcept {
    const auto words = Encode(initial_value);
    StoreWords(0, words);
    StoreWords(1, words);
  }

  // 仅允许唯一写线程调用。先切换读者到 data[1]，写入 data[0]，再切回
  // data[0]，并同步 data[1]（对应 write_seqcount_latch_* 范式）。
  // 两次 seq++ 即为冲突时刻：正在读亮边的读者若跨越其中任意一次须重试；
  // 随后对暗边的 StoreWords 不构成与亮边读者的逻辑冲突。
  void Publish(const T& value) noexcept {
    const auto words = Encode(value);

    // 冲突时刻 1：偶数 -> 奇数，读者改跟 data[1]。
    sequence_.fetch_add(1, std::memory_order_release);
    StoreWords(0, words);

    // 冲突时刻 2：奇数 -> 偶数，读者改跟已更新的 data[0]。
    sequence_.fetch_add(1, std::memory_order_release);
    StoreWords(1, words);
  }

  // 读取当前亮边快照。暴露窗口为 before..after 之间的整次 LoadWords；
  // 仅当该窗口内发生 seq++（切边）时 before != after，从而重试。
  T Load() const noexcept {
    std::array<Word, kWordCount> words{};

    for (;;) {
      const Word before = sequence_.load(std::memory_order_acquire);
      const std::size_t idx = static_cast<std::size_t>(before & 1U);
      LoadWords(idx, words);

      const Word after = sequence_.load(std::memory_order_acquire);
      if (before == after) {
        T result{};
        std::memcpy(&result, words.data(), sizeof(T));
        return result;
      }
    }
  }

  // 加载快照后执行 reader。引用仅指向局部快照，不得保存到回调之外。
  template <class F>
  void Read(F&& reader) const {
    const T snapshot = Load();
    std::invoke(std::forward<F>(reader), std::as_const(snapshot));
  }

 private:
  static constexpr std::size_t kWordCount =
      (sizeof(T) + sizeof(Word) - 1U) / sizeof(Word);

  static std::array<Word, kWordCount> Encode(const T& value) noexcept {
    std::array<Word, kWordCount> words{};
    std::memcpy(words.data(), &value, sizeof(T));
    return words;
  }

  void StoreWords(std::size_t slot,
                  const std::array<Word, kWordCount>& words) noexcept {
    for (std::size_t i = 0; i < kWordCount; ++i) {
      slots_[slot][i].value.store(words[i], std::memory_order_release);
    }
  }

  void LoadWords(std::size_t slot,
                 std::array<Word, kWordCount>& words) const noexcept {
    for (std::size_t i = 0; i < kWordCount; ++i) {
      words[i] = slots_[slot][i].value.load(std::memory_order_acquire);
    }
  }

  // 偶数读 slots_[0]，奇数读 slots_[1]。
  std::atomic<Word> sequence_{0};
  // 不能直接存放普通 T：亮边切换前后仍可能与未完成的读者并发访问同一槽，
  // 在 C++ 中构成 data race；故与 SingleWriterSeqLock 一样按原子字块存取。
  std::array<std::array<AtomicWord, kWordCount>, 2> slots_{};
};
