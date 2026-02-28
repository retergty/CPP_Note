# 检查模板实参

在头文件`<type_traits>`中定义了许多模板类，用来检测模板实参是什么一类的类型以及它支持什么操作。

参考文档

* CPP Reference[type_traits](https://en.cppreference.com/w/cpp/header/type_traits)

这些模板类的使用方法都是`typeclass<T>::value`就可以得出判断的布尔值。

## 检测基础类型

本节都是一些基础类型的检测

* `is_void`
* `is_null_pointer`
* `is_integral`
* `is_floating_point`
* `is_array`
* `is_enum`
* `is_union`
* `is_class`
* `is_function`
* `is_pointer`
* `is_lvalue_reference`
* `is_rvalue_reference`
* `is_member_object_pointer`
* `is_member_function_pointer`
  
## 检测复合类型

本节都是一些复合类型的检测

* `is_fundamental`
* `is_arithmetic`
* `is_scalar`
* `is_object`
* `is_compound`
* `is_reference`
* `is_member_pointer`
  
## 检测类型属性
  
本节都是一些类型属性的检测

* `is_const`
* `is_volatile`
* `is_trivial`
* `is_trivially_copyable`
* `is_pod`
* `is_literal_type`
* `has_unique_object_representations`
* `is_empty`
* `is_polymorphic`
* `is_abstract`
* `is_final`
* `is_aggregate`
* `is_implicit_lifetime`
* `is_signed`
* `is_unsigned`
* `is_bounded_array`
* `is_unbounded_array`
* `is_scoped_enum`

## 检测类型支持的操作

本节都是一些类型支持操作的检测

* `is_constructible`,`is_trivially_constructible`,`is_nothrow_constructible`
* `is_default_constructible`,`is_trivially_default_constructible`.`is_nothrow_default_constructible`
* `is_copy_constructible`,`is_trivially_copy_constructible`,`is_nothrow_copy_constructible`
* `is_move_constructible`,`is_trivially_move_constructible`,`is_nothrow_move_constructible`
* `is_assignable`,`is_trivially_assignable`,`is_nothrow_assignable`
* `is_copy_assignable`,`is_trivially_copy_assignable`,`is_nothrow_copy_assignable`
* `is_move_assignable`,`is_trivially_move_assignable`,`is_nothrow_move_assignable`
* `is_destructible`,`is_trivially_destructible`,`is_nothrow_destructible`
* `has_virtual_destructor`
* `is_swappable_with`,`is_swappable`,`is_nothrow_swappable_with`,`is_nothrow_swappable`

## 检测两个类型之间的关系

本节是一些两个类型之间的关系的检测

* `is_same`,检查两个类型是否是相同类型，考虑`cv`限定符
* `is_base_of`
* `is_convertible`,`is_nothrow_convertible`
* `is_layout_compatible`
* `is_pointer_interconvertible_base_of`
* `is_invocable`,`is_invocable_r`,`is_nothrow_invocable`,`is_nothrow_invocable_r`
