#include <cmath>

template<typename Scalar>
inline Scalar constrain(const Scalar val,const Scalar min,const Scalar max)
{
    return val < min ? min : (val > max ? max : val);
}