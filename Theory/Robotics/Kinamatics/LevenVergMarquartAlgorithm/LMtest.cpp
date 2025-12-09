#include <iostream>
#include "LMAlgorithm.hpp"
#include "gtest/gtest.h"

constexpr size_t MAX_ITERATE = 1000;
using namespace Eigen;
// y = 2x + 5
Vector<float, 1> linear_f(const Vector<float, 1> &x)
{
    return Vector<float, 1>(x(0) * 2 + 5);
}
TEST(LinearZeroPointFind, SingleVarSingleF)
{
    LMAlgorithm<float, 1, 1> lm;
    Vector<float, 1> y_d(10);
    Matrix<float, 1, 1> jacobian(2);
    lm.init(linear_f, y_d);
    for (size_t i = 0; i < MAX_ITERATE; ++i)
    {
        lm.iterate(jacobian);
        if (lm.isInTolarence())
            break;
    }
    EXPECT_TRUE(lm.isInTolarence());
    std::cout << "loop count " << lm.getIterationCount() << std::endl;
    std::cout << "x value " << std::endl
              << lm.getxRef() << std::endl;
}

struct Quadratic_f
{
    Vector<double, 1> operator()(const Vector<double, 1> &x)
    {
        return Vector<double, 1>(2 * x(0) * x(0) + 3 * x(0) + 5);
    }
    Matrix<double, 1, 1> jacobian(const Vector<double, 1> &x)
    {
        return Matrix<double, 1, 1>(4 * x(0) + 3);
    }
};

TEST(QuadraticZeroPointFind, SingleVarSingleF)
{
    Quadratic_f quadf;
    LMAlgorithm<double, 1, 1> lm;
    Vector<double, 1> y_d(10);
    lm.init(quadf, y_d);
    for (size_t i = 0; i < MAX_ITERATE; ++i)
    {
        lm.iterate(quadf.jacobian(lm.getxRef()));
        if (lm.isInTolarence())
            break;
    }
    EXPECT_TRUE(lm.isInTolarence());
    std::cout << "loop count " << lm.getIterationCount() << std::endl;
    std::cout << "x value " << std::endl
              << lm.getxRef() << std::endl;
}
// 3*x^3 + 4*x^2 + 5*x + 7
struct CubicFunction
{
    Vector<double, 1> operator()(const Vector<double, 1> &x)
    {
        return Vector<double, 1>(3 * x(0) * x(0) * x(0) + 4 * x(0) * x(0) + 5 * x(0) + 7);
    }
    Matrix<double, 1, 1> jacobian(const Vector<double, 1> &x)
    {
        return Matrix<double, 1, 1>(9 * x(0) * x(0) + 8 * x(0) + 5);
    }
};
TEST(CubicZeroPointFind, SingleVarSingleF)
{
    CubicFunction cubicf;
    LMAlgorithm<double, 1, 1> lm;
    Vector<double, 1> y_d(10);
    lm.init(cubicf, y_d);
    for (size_t i = 0; i < MAX_ITERATE; ++i)
    {
        lm.iterate(cubicf.jacobian(lm.getxRef()));
        if (lm.isInTolarence())
            break;
    }
    EXPECT_TRUE(lm.isInTolarence());
    std::cout << "loop count " << lm.getIterationCount() << std::endl;
    std::cout << "x value " << std::endl
              << lm.getxRef() << std::endl;
}
// xe^x
struct TranscendentalFunction
{
    Vector<double, 1> operator()(const Vector<double, 1> &x)
    {
        return Vector<double, 1>(x(0) * std::exp(x(0)));
    }
    Matrix<double, 1, 1> jacobian(const Vector<double, 1> &x)
    {
        return Matrix<double, 1, 1>((x(0) + 1) * std::exp(x(0)));
    }
};
TEST(TranscendentalZeroPointFind, SingleVarSingleF)
{
    TranscendentalFunction cubicf;
    LMAlgorithm<double, 1, 1> lm;
    Vector<double, 1> y_d(1);
    lm.init(cubicf, y_d);
    for (size_t i = 0; i < MAX_ITERATE; ++i)
    {
        lm.iterate(cubicf.jacobian(lm.getxRef()));
        if (lm.isInTolarence())
            break;
    }
    EXPECT_TRUE(lm.isInTolarence());
    std::cout << "loop count " << lm.getIterationCount() << std::endl;
    std::cout << "x value " << std::endl
              << lm.getxRef() << std::endl;
}
// y = Ax
struct LinearMatrixFunction
{
    LinearMatrixFunction()
    {
        Matrix<double, 4, 4> A{{1, 3, 0, 0}, {2, 4, 0, 0}, {0, 0, 5, 6}, {0, 0, 7, 8}};
        _A = A;
    }
    Vector<double, 4> operator()(const Vector<double, 4> &x)
    {
        return _A * x;
    };
    Matrix<double, 4, 4> jacobian(const Vector<double, 4> &x)
    {
        (void)x;
        return _A;
    }
    Matrix<double, 4, 4> _A;
};
TEST(LinearMatrixZeroPointFind, VectorValueF)
{
    LinearMatrixFunction linear_matrix;
    LMAlgorithm<double, 4, 4> lm;
    Vector<double, 4> y_d(1, 2, 3, 4);
    lm.init(linear_matrix, y_d);
    for (size_t i = 0; i < MAX_ITERATE; ++i)
    {

        lm.iterate(linear_matrix.jacobian(lm.getxRef()));
        if (lm.isInTolarence())
            break;
    }
    EXPECT_TRUE(lm.isInTolarence());
    std::cout << "loop count " << lm.getIterationCount() << std::endl;
    std::cout << "x value " << std::endl
              << lm.getxRef() << std::endl;
}
int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}