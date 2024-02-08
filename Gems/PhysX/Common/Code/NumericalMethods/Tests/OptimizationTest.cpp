/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <NumericalMethods/Optimization.h>
#include <Optimization/SolverBFGS.h>
#include <Optimization/LineSearch.h>
#include <Optimization/Constants.h>
#include <Optimization/Utilities.h>
#include <Tests/Environment.h>

namespace NumericalMethods::Optimization
{
    // The Rosenbrock function is a function commonly used to test optimization routines because it has a very long, narrow
    // valley
    struct
    {
        double a = 1.0;
        double b = 100.0;
    } RosenbrockConstants;

    class TestFunctionRosenbrock
        : public Optimization::Function
    {
    public:
        TestFunctionRosenbrock() = default;

    private:
        virtual AZ::u32 GetDimension() const override
        {
            return 2;
        }

        virtual AZ::Outcome<double, FunctionOutcome> ExecuteImpl(const AZStd::vector<double>& x) const override
        {
            return AZ::Success((RosenbrockConstants.a - x[0]) * (RosenbrockConstants.a - x[0]) +
                RosenbrockConstants.b * (x[1] - x[0] * x[0]) * (x[1] - x[0] * x[0]));
        }
    };

    VectorVariable TestFunctionRosenbrockGradient(const VectorVariable p)
    {
        double x = p[0];
        double y = p[1];

        VectorVariable gradient(2);
        gradient[0] = -2.0 * (RosenbrockConstants.a - x) - 4.0 * RosenbrockConstants.b * x * (y - x * x);
        gradient[1] = 2.0 * RosenbrockConstants.b * (y - x * x);
        return gradient;
    }

    TEST(OptimizationTest, FunctionValue_RosenbrockFunction_CorrectValues)
    {
        TestFunctionRosenbrock testFunctionRosenbrock;
        EXPECT_NEAR(FunctionValue(testFunctionRosenbrock, VectorVariable::CreateFromVector({ 1.0, 1.0 })), 0.0, 1e-3);
        EXPECT_NEAR(FunctionValue(testFunctionRosenbrock, VectorVariable::CreateFromVector({ 3.0, 5.0 })), 1604.0, 1e-3);
        EXPECT_NEAR(FunctionValue(testFunctionRosenbrock, VectorVariable::CreateFromVector({ -2.0, 4.0 })), 9.0, 1e-3);
        EXPECT_NEAR(FunctionValue(testFunctionRosenbrock, VectorVariable::CreateFromVector({ -3.0, 7.0 })), 416.0, 1e-3);
        EXPECT_NEAR(FunctionValue(testFunctionRosenbrock, VectorVariable::CreateFromVector({ 0.0, 5.0 })), 2501.0, 1e-3);
        EXPECT_NEAR(FunctionValue(testFunctionRosenbrock, VectorVariable::CreateFromVector({ 4.0, 0.0 })), 25609.0, 1e-3);
    }

    TEST(OptimizationTest, Gradient_RosenbrockFunction_CorrectGradient)
    {
        TestFunctionRosenbrock testFunctionRosenbrock;
        VectorVariable x(2);
        for (double x0 = -5.0; x0 < 6.0; x0 += 2.0)
        {
            for (double x1 = -5.0; x1 < 6.0; x1 += 2.0)
            {
                x[0] = x0;
                x[1] = x1;
                VectorVariable gradient = Gradient(testFunctionRosenbrock, x);
                VectorVariable expectedGradient = TestFunctionRosenbrockGradient(x);
                ExpectClose(gradient, expectedGradient, 1e-3);
            }
        }
    }

    TEST(OptimizationTest, DirectionalDerivative_RosenbrockFunction_CorrectDerivative)
    {
        TestFunctionRosenbrock testFunctionRosenbrock;
        VectorVariable x(2);
        x[0] = 3.0;
        x[1] = -4.0;
        VectorVariable direction(2);
        for (double d0 = -5.0; d0 < 6.0; d0 += 2.0)
        {
            for (double d1 = -5.0; d1 < 6.0; d1 += 2.0)
            {
                direction[0] = d0;
                direction[1] = d1;
                double directionalDerivative = DirectionalDerivative(testFunctionRosenbrock, x, direction);
                double expectedDirectionalDerivative = TestFunctionRosenbrockGradient(x).Dot(direction);
                EXPECT_NEAR(directionalDerivative, expectedDirectionalDerivative, 1e-3);
            }
        }
    }

    TEST(OptimizationTest, CubicMinimum_KnownCubic_CorrectMinimum)
    {
        double expectedMinimum = 3.0;
        double otherRoot = -7.0;
        double a = 0.0;
        double f_a = (a - expectedMinimum) * (a - expectedMinimum) * (a - otherRoot);
        double df_a = 2.0 * (a - expectedMinimum) * (a - otherRoot) + (a - expectedMinimum) * (a - expectedMinimum);
        double b = 5.0;
        double f_b = (b - expectedMinimum) * (b - expectedMinimum) * (b - otherRoot);
        double c = -3.0;
        double f_c = (c - expectedMinimum) * (c - expectedMinimum) * (c - otherRoot);
        double calculatedMinimum = CubicMinimum(a, f_a, df_a, b, f_b, c, f_c);
        EXPECT_NEAR(calculatedMinimum, expectedMinimum, 1e-3);
    }

    TEST(OptimizationTest, QuadraticMinimum_KnownQuadratic_CorrectMinimum)
    {
        double expectedMinimum = 2.0;
        double a = -1.0;
        double f_a = 5.0 * (a - expectedMinimum) * (a - expectedMinimum) + 7.0;
        double df_a = 10.0 * (a - expectedMinimum);
        double b = 1.0;
        double f_b = 5.0 * (b - expectedMinimum) * (b - expectedMinimum) + 7.0;
        double calculatedMinimum = QuadraticMinimum(a, f_a, df_a, b, f_b);
        EXPECT_NEAR(calculatedMinimum, expectedMinimum, 1e-3);
    }

    TEST(OptimizationTest, ValidateStepSize_ValidateStepSize_CorrectResult)
    {
        EXPECT_TRUE(ValidateStepSize(0.5, 0.0, 1.0, 0.1));
        EXPECT_FALSE(ValidateStepSize(0.05, 0.0, 1.0, 0.1));
        EXPECT_FALSE(ValidateStepSize(-0.5, 0.0, 1.0, 0.1));
        EXPECT_FALSE(ValidateStepSize(1.5, 0.0, 1.0, 0.1));
        EXPECT_TRUE(ValidateStepSize(1.5, 2.0, -1.0, 0.05));
        EXPECT_FALSE(ValidateStepSize(std::numeric_limits<double>::quiet_NaN(), 2.0, 0.0, 0.1));
        EXPECT_FALSE(ValidateStepSize(std::numeric_limits<double>::infinity(), -1.0, 3.0, 0.2));
    }

    TEST(OptimizationTest, LineSearch_SelectStepSizeFromInterval_SatisfiesWolfeConditions)
    {
        TestFunctionRosenbrock testFunctionRosenbrock;
        VectorVariable x0 = VectorVariable::CreateFromVector({ 7.0, 7.0 });
        VectorVariable searchDirection = VectorVariable::CreateFromVector({ -1.0, -1.0 });
        double alpha0 = 0.0;
        double alpha1 = 20.0;
        double f_alpha0 = FunctionValue(testFunctionRosenbrock, x0);
        double f_alpha1 = FunctionValue(testFunctionRosenbrock, x0 + alpha1 * searchDirection);
        double df_alpha0 = DirectionalDerivative(testFunctionRosenbrock, x0, searchDirection);
        double f_x0 = f_alpha0;
        double df_x0 = df_alpha0;
        LineSearchResult lineSearchResult = SelectStepSizeFromInterval(alpha0, alpha1, f_alpha0, f_alpha1, df_alpha0,
            testFunctionRosenbrock, x0, searchDirection, f_x0, df_x0, WolfeConditionsC1, WolfeConditionsC2);

        EXPECT_TRUE(lineSearchResult.m_outcome == LineSearchOutcome::Success);
        // check that the Wolfe conditions are satisfied by the returned step size
        EXPECT_TRUE(lineSearchResult.m_functionValue < f_x0 + WolfeConditionsC1 * df_x0 * lineSearchResult.m_stepSize);
        EXPECT_TRUE(fabs(lineSearchResult.m_derivativeValue) <= -WolfeConditionsC2 * df_x0);
    }

    TEST(OptimizationTest, LineSearch_VariousSearchDirections_SatisfiesWolfeCondition)
    {
        TestFunctionRosenbrock testFunctionRosenbrock;
        VectorVariable x0 = VectorVariable::CreateFromVector({ 7.0, 7.0 });
        AZStd::vector<AZStd::vector<double>> searchVectors = { { -1.0, -1.0 }, { -0.1, -0.2 }, { -8.0, -9.0 } };
        for (const auto& searchVector : searchVectors)
        {
            VectorVariable searchDirection = VectorVariable::CreateFromVector(searchVector);
            double f_x0 = FunctionValue(testFunctionRosenbrock, x0);
            double df_x0 = DirectionalDerivative(testFunctionRosenbrock, x0, searchDirection);
            LineSearchResult lineSearchResult = LineSearchWolfe(testFunctionRosenbrock, x0, f_x0, searchDirection);

            EXPECT_TRUE(lineSearchResult.m_outcome == LineSearchOutcome::Success);
            // check that the Wolfe conditions are satisfied by the returned step size
            EXPECT_TRUE(lineSearchResult.m_functionValue < f_x0 + WolfeConditionsC1 * df_x0 * lineSearchResult.m_stepSize);
            EXPECT_TRUE(fabs(lineSearchResult.m_derivativeValue) <= -WolfeConditionsC2 * df_x0);
        }
    }

    TEST(OptimizationTest, MinimizeBFGS_RosenbrockFunction_CorrectMinimum)
    {
        TestFunctionRosenbrock testFunctionRosenbrock;
        AZStd::vector<double> xInitial = { -7.0, 11.0 };
        SolverResult solverResult = MinimizeBFGS(testFunctionRosenbrock, xInitial);
        AZStd::vector<double> xExpected = { 1.0, 1.0 };
        ExpectClose(solverResult.m_xValues, xExpected, 1e-3);
    }
} // namespace NumericalMethods::Optimization
