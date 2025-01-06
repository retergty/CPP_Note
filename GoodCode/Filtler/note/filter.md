# 滤波器

本文总结了常用的滤波器与代码实现

## Alpha Filter

阿尔法滤波器，也叫做一阶低通滤波器.

$$
H(s) = \frac1{s+\alpha}
$$

### 设置截止频率

```CPP
bool setCutoffFreq(float sample_freq, float cutoff_freq)
```

设置阿尔法滤波器的截止频率.

## Filtered Derivative

具有截止频率的微分器，实际上是一个微分器串联上阿尔法滤波器.

$$
G(s) = \frac{Ks}{Ts+1}
$$

### 设置时间常数

```CPP
void setParameters(float sample_interval, float time_constant)
```

## second order low pass filter

二阶低通滤波器

$$
G(s) = \frac{s^2}{s^2+2\zeta{w_n}s+w^2_n}
$$

### 设置截止频率

```CPP
void set_cutoff_frequency(float sample_freq, float cutoff_freq)
```

## Median Filter

中值滤波器

```CPP
template<typename T, int WINDOW = 3>
class MedianFilter
```

* `WINDOW`是窗口宽度,必须大于3且为奇数

### 应用中值滤波器

```CPP
T apply(const T &sample)
```

## Notch Filter

陷波滤波器

```CPP
template<typename T>
class NotchFilter
```

### 设置带宽于陷波频率

```CPP
bool setParameters(float sample_freq, float notch_freq, float bandwidth);
```
