/*
Implement levenverg-marquartdt algorithm to solve unlinear curve fitting or zero point findding
*/
#pragma once
#include <Eigen/Core>
#include "HelperFunction.hpp"
#include <functional>
#include <Eigen/LU>
#include <Eigen/Cholesky>
// given vector function y = f(x), x in R^n , y in R^m
// find x_k in R^n, minimize norm 2 object function
// F(x) = ||y_d - f(x)||^2
// f(x) need to its jacobian matrix
template <typename Scalar, int VarNum, int FuncNum>
class LMAlgorithm
{
public:
    static constexpr Scalar DampingMax = static_cast<Scalar>(1e7);
    static constexpr Scalar DampingMin = static_cast<Scalar>(1e-7);
    static constexpr Scalar ErrorReductionRatioSmall = static_cast<Scalar>(0.25);
    static constexpr Scalar ErrorReductionRatioLarge = static_cast<Scalar>(0.75);
    static constexpr Scalar LambdaUpFactor = static_cast<Scalar>(11);
    static constexpr Scalar LambdaDownFactor = static_cast<Scalar>(7);
    LMAlgorithm(const Scalar tolarence = static_cast<Scalar>(1e-4), const Scalar lambda = static_cast<Scalar>(1e-3)) : _tolarence(tolarence), _lambda(lambda) {};
    ~LMAlgorithm() = default;
    void init(
        std::function<Eigen::Vector<Scalar, FuncNum>(const Eigen::Vector<Scalar, VarNum> &)> f,
        const Eigen::Vector<Scalar, FuncNum> &y_d,
        const Eigen::Vector<Scalar, VarNum> &x_0 = Eigen::Vector<Scalar, VarNum>::Zero())
    {
        _iter = 0;
        _f = f;
        _x = x_0;
        _y_d = y_d;
        _f_value = _f(_x);
    }
    void setDampingRatio(const Eigen::Matrix<Scalar, FuncNum, VarNum> &jacobian, const Scalar tau = static_cast<Scalar>(1e-2))
    {
        Scalar max = 0;
        // max(diag(JTJ))
        for (int i = 0; i < VarNum; ++i)
        {
            Scalar tmp = 0;
            for (int j = 0; j < FuncNum; ++j)
            {
                tmp += jacobian(j, i) * jacobian(j, i);
            }
            if (tmp > max)
            {
                max = tmp;
            }
        }
        _lambda = constrain(tau * max, DampingMin, DampingMax);
    }
    void iterate(const Eigen::Matrix<Scalar, FuncNum, VarNum> &jacobian)
    {
        _iter++;

        Eigen::Matrix<Scalar, VarNum, VarNum> JTJ = jacobian.transpose() * jacobian;
        Eigen::Matrix<Scalar, VarNum, VarNum> diagJTJ = Eigen::DiagonalMatrix<Scalar, VarNum, VarNum>(JTJ.diagonal());
        diagJTJ = _lambda * diagJTJ;

        Eigen::Vector<Scalar, FuncNum> y_now = _f(_x);
        Eigen::Vector<Scalar, VarNum> JTdy = jacobian.transpose() * (_y_d - y_now);
        // solve positive semidefinite linear function
        // (JTJ + lambda * diag(JTJ))dx = JTdy
        Eigen::LDLT<Eigen::Matrix<Scalar, VarNum, VarNum>> ldlt(JTJ + diagJTJ);
        Eigen::Vector<Scalar, VarNum> dx = ldlt.solve(JTdy);

        Scalar rho = calErrorReductionRatio(dx, diagJTJ, jacobian);
        changeLambdaAndx(dx, rho);
    }
    inline const Eigen::Vector<Scalar, VarNum> &getxRef()
    {
        return _x;
    }
    Scalar getErrorNorm()
    {
        return (_y_d - _f(_x)).norm();
    }
    bool isInTolarence()
    {
        return (getErrorNorm() < _tolarence);
    }
    Scalar getIterationCount()
    {
        return _iter;
    }

private:
    Scalar calErrorReductionRatio(const Eigen::Vector<Scalar, VarNum> &dx, const Eigen::Matrix<Scalar, VarNum, VarNum> &lambda_JTJ, const Eigen::Matrix<Scalar, FuncNum, VarNum> &jacobian)
    {
        Eigen::Vector<Scalar, FuncNum> error_now = _y_d - _f(_x);
        Eigen::Vector<Scalar, FuncNum> error_next = _y_d - _f(_x + dx);
        Scalar error_linear = std::abs(dx.transpose() * (lambda_JTJ * dx + jacobian.transpose() * (error_now)));
        Scalar rho = (error_now.dot(error_now) - error_next.dot(error_next)) / error_linear;
        return rho;
    }
    void changeLambdaAndx(const Eigen::Vector<Scalar, VarNum> &dx, const Scalar rho)
    {
        if (rho < ErrorReductionRatioSmall)
        {
            _lambda = constrain(_lambda * LambdaUpFactor, DampingMin, DampingMax);
        }
        else if (rho < ErrorReductionRatioLarge)
        {
            _x += dx;
        }
        else
        {
            _lambda = constrain(_lambda / LambdaDownFactor, DampingMin, DampingMax);
            _x += dx;
        }
    }

private:
    Scalar _tolarence;
    Scalar _lambda;
    std::function<Eigen::Vector<Scalar, FuncNum>(const Eigen::Vector<Scalar, VarNum> &)> _f;
    Eigen::Vector<Scalar, FuncNum> _y_d;
    Eigen::Vector<Scalar, VarNum> _x;
    Eigen::Vector<Scalar, FuncNum> _f_value;
    size_t _iter{0};
};