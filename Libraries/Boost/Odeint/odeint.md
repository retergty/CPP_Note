# odeint

`odeint`是使用数值方法求解标准微分方程的库.

$$
x\prime = f(x,t)
$$

参考文档

* [odeint](https://headmyshoulder.github.io/odeint-v2/doc/boost_numeric_odeint/getting_started/overview.html)

## 使用方法

`odeint`是`boost`库的一部分.

```CPP
#include <boost/numeric/odeint.hpp>
```

## 常用API

### 微分方程

```CPP
void system( const state_type &x , state_type &dxdt , const double /* t */ )
```

`odeint`接受的微分方程组的形式如上.可以是任何的可调用对象.

`state_type`可以接受任何重载了运算符`[]`的类.

### integrate

```CPP
template< class System , class State , class Time , class Observer >
typename boost::enable_if< typename has_value_type<State>::type , size_t >::type
integrate( System system , State &start_state , Time start_time , Time end_time , Time dt , Observer observer )
{
    typedef controlled_runge_kutta< runge_kutta_dopri5< State , typename State::value_type , State , Time > > stepper_type;
    return integrate_adaptive( stepper_type() , system , start_state , start_time , end_time , dt , observer );
}

template< class Value , class System , class State , class Time , class Observer >
size_t 
integrate( System system , State &start_state , Time start_time , Time end_time , Time dt , Observer observer )
{
    typedef controlled_runge_kutta< runge_kutta_dopri5< State , Value , State , Time > > stepper_type;
    return integrate_adaptive( stepper_type() , system , start_state , start_time , end_time , dt , observer );
}
```

使用经典的`runge_kutta_cash_karp54`的数值积分算法，计算`[start,end_time]`内的`system`表示的方程的结果。`observer`会在每次步长过后调用.

* `system`是系统微分方程
* `start_state`是状态向量，可以指定初始值，运算后会返回最终值.
* `start_time`,`end_time`开始时间，结束时间
* `dt`初始步长.
* `observer`观测器，用于获取开始时间与结束时间之中的值.

`observer`是以下类型

```CPP
void observer( const state_type &x , double t );
```

其中`x`就是本次步长后的值，`t`是本次所在时间.

### integrate_const

使用固定步长

```CPP
runge_kutta4< state_type > stepper;
integrate_const( stepper , harmonic_oscillator , x , 0.0 , 10.0 , 0.01 );
```

### integrate_adaptive

使用自适应误差估计步长
