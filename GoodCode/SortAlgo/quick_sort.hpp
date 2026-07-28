#pragma once

/*
 * Quick Sort（快速排序）—— 基础版
 *
 * 思想：分治。选一个基准 pivot，把区间分成「小于 pivot」和「大于等于 pivot」
 * 两段，pivot 落在最终位置；再对左右两段递归，直到区间长度 <= 1。
 *
 * 步骤：
 *   1. 选区间末尾元素为 pivot
 *   2. Lomuto 分区，返回 pivot 最终下标
 *   3. 对左右子区间递归排序
 *
 * 复杂度：
 *   平均 / 最好  O(n log n)
 *   最坏         O(n²)（已序数组且总选末尾为 pivot 时）
 *   空间         O(log n) 平均递归栈；最坏 O(n)
 *   稳定性       不稳定
 *
 * 用法：
 *   sort_algo::quick_sort(v.begin(), v.end());
 *   sort_algo::quick_sort(v.begin(), v.end(), std::greater<>{});
 */

#include <functional>
#include <iterator>
#include <utility>

namespace sort_algo {
namespace detail {

// ---------------------------------------------------------------------------
// Lomuto 分区（本质是双指针）
//
// 以区间末尾 *pivot_it 为基准，i / j 双指针从左往右扫描。
// 循环过程中始终维护：
//   [first, i)       —— 已确认 <  pivot
//   [i, j)           —— 已确认 >= pivot（相等也归这边）
//   [j, pivot_it)    —— 尚未扫描
//   pivot_it         —— 基准本身
//
// 若 *j < pivot，则 swap(*i, *j) 并 ++i，把小数并入左边；
// 否则只推进 j。扫描结束后 swap(*i, *pivot_it)，pivot 就位。
//
// 返回值 p 满足：
//   [first, p)  中元素均 <  *p
//   (p, last)   中元素均 >= *p
// ---------------------------------------------------------------------------
template <class RandomIt, class Compare>
RandomIt lomuto_partition(RandomIt first, RandomIt last, Compare comp) {
  auto pivot_it = last - 1;
  auto pivot = *pivot_it;
  auto i = first; // [first, i) 为 < pivot 区

  for (auto j = first; j != pivot_it; ++j) { // [i, j) 为 >= pivot 区
    if (comp(*j, pivot)) {
      std::swap(*i, *j);
      ++i;
    }
  }
  std::swap(*i, *pivot_it);
  return i;
}

} // namespace detail

template <class RandomIt, class Compare>
void quick_sort(RandomIt first, RandomIt last, Compare comp) {
  if (last - first <= 1) {
    return;
  }
  auto pivot = detail::lomuto_partition(first, last, comp);
  quick_sort(first, pivot, comp);
  quick_sort(pivot + 1, last, comp);
}

// 默认按升序（operator< / std::less）排序
template <class RandomIt> void quick_sort(RandomIt first, RandomIt last) {
  using T = typename std::iterator_traits<RandomIt>::value_type;
  quick_sort(first, last, std::less<T>{});
}

} // namespace sort_algo
