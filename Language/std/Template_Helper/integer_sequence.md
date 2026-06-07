# integer_sequence

在头文件`<utility>`中定义的一个模板类，表示一个编译期确定的整数序列。当用作函数模板的参数时，参数包`Ints`可以被推导并用于包扩展。

参考文档

* CPP reference [std::integer_sequence](https://en.cppreference.com/w/cpp/utility/integer_sequence)

## 语法

```CPP
template< class T, T... Ints >
struct integer_sequence;
```

* `T`：整数类型.
* `Ints`：整数序列。

### 成员类型

* `value_type`：整数类型`T`。

### 成员函数

* `size()`：返回整数序列中整数的数量。也就是`sizeof...(Ints)`。

## 帮助类型

```CPP
template< std::size_t... Ints >
using index_sequence = std::integer_sequence<std::size_t, Ints...>;
```

表示一个`std::size_t`类型的整数序列。

```CPP
template< class T, T N >
using make_integer_sequence = std::integer_sequence<T, /* a sequence 0, 1, 2, ..., N-1 */>;
template< std::size_t N >
using make_index_sequence = std::make_integer_sequence<std::size_t, N>;
```

`make_integer_sequence`生成一个整数序列，包含从0到N-1的整数。`make_index_sequence`是`make_integer_sequence`的特例，生成一个`std::size_t`类型的整数序列。

## 例子

```CPP
#include <array>
#include <cstddef>
#include <iostream>
#include <tuple>
#include <utility>

namespace details {
template <typename Array, std::size_t... I>
constexpr auto array_to_tuple_impl(const Array& a, std::index_sequence<I...>)
{
    return std::make_tuple(a[I]...);
}

template <class Ch, class Tr, class Tuple, std::size_t... Is>
void print_tuple_impl(std::basic_ostream<Ch, Tr>& os,
                      const Tuple& t,
                      std::index_sequence<Is...>)
{
    ((os << (Is ? ", " : "") << std::get<Is>(t)), ...);
}
}

template <typename T, T... ints>
void print_sequence(int id, std::integer_sequence<T, ints...> int_seq)
{
    std::cout << id << ") The sequence of size " << int_seq.size() << ": ";
    ((std::cout << ints << ' '), ...);
    std::cout << '\n';
}

template <typename T, std::size_t N, typename Indx = std::make_index_sequence<N>>
constexpr auto array_to_tuple(const std::array<T, N>& a)
{
    return details::array_to_tuple_impl(a, Indx{});
}

template <class Ch, class Tr, class... Args>
auto& operator<<(std::basic_ostream<Ch, Tr>& os, const std::tuple<Args...>& t)
{
    os << '(';
    details::print_tuple_impl(os, t, std::index_sequence_for<Args...>{});
    return os << ')';
}

int main()
{
    print_sequence(1, std::integer_sequence<unsigned, 9, 2, 5, 1, 9, 1, 6>{});
    print_sequence(2, std::make_integer_sequence<int, 12>{});
    print_sequence(3, std::make_index_sequence<10>{});
    print_sequence(4, std::index_sequence_for<std::ios, float, signed>{});

    constexpr std::array<int, 4> array{1, 2, 3, 4};

    auto tuple1 = array_to_tuple(array);
    static_assert(std::is_same_v<decltype(tuple1),
                                 std::tuple<int, int, int, int>>, "");
    std::cout << "5) tuple1: " << tuple1 << '\n';
    
    constexpr auto tuple2 = array_to_tuple<int, 4,
        std::integer_sequence<std::size_t, 1, 0, 3, 2>>(array);
    std::cout << "6) tuple2: " << tuple2 << '\n';
}
```

## 可能的实现方式

```CPP
namespace detail {
template<class T, T I, T N, T... integers>
struct make_integer_sequence_helper
{
    using type = typename make_integer_sequence_helper<T, I + 1, N, integers..., I>::type;
};

template<class T, T N, T... integers>
struct make_integer_sequence_helper<T, N, N, integers...>
{
    using type = std::integer_sequence<T, integers...>;
};
}

template<class T, T N>
using make_integer_sequence = typename detail::make_integer_sequence_helper<T, 0, N>::type;
```