/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Environment.h>
#include <AzTest/AzTest.h>
#include <LinearAlgebra.h>

namespace NumericalMethods
{
    TEST(LinearAlgebraTest, VectorVariableAccessors_AccessingElements_CorrectResults)
    {
        VectorVariable v1 = VectorVariable::CreateFromVector({ 2.0, 5.0, -3.0 });

        EXPECT_NEAR(v1[0], 2.0, 1e-3);
        EXPECT_NEAR(v1[2], -3.0, 1e-3);
        EXPECT_NEAR(v1[1], 5.0, 1e-3);
    }

    TEST(LinearAlgebraTest, VectorVariable_GetDimension_CorrectDimension)
    {
        VectorVariable v1 = VectorVariable::CreateFromVector({ 3.0, -4.0, 12.0 });
        VectorVariable v2 = VectorVariable::CreateFromVector({ -7.0, -24.0 });
        VectorVariable v3 = VectorVariable::CreateFromVector({ 17.0 });
        VectorVariable v4;
        VectorVariable v5 = VectorVariable::CreateFromVector({ 3.0, 10.0, -5.0, 7.0, -8.0, 3.0 });
        VectorVariable v6(5);

        EXPECT_EQ(v1.GetDimension(), 3);
        EXPECT_EQ(v2.GetDimension(), 2);
        EXPECT_EQ(v3.GetDimension(), 1);
        EXPECT_EQ(v4.GetDimension(), 0);
        EXPECT_EQ(v5.GetDimension(), 6);
        EXPECT_EQ(v6.GetDimension(), 5);
    }

    TEST(LinearAlgebraTest, VectorVariableArithmetic_AddingVectors_CorrectResults)
    {
        VectorVariable v1 = VectorVariable::CreateFromVector({ 1.0, 2.0, 3.0 });
        VectorVariable v2 = VectorVariable::CreateFromVector({ 2.0, 6.0, -4.0 });
        VectorVariable v3 = v1 + v2;
        ExpectClose(v3.GetValues(), { 3.0, 8.0, -1.0 }, 1e-3);
        v3 += v1;
        ExpectClose(v3.GetValues(), { 4.0, 10.0, 2.0 }, 1e-3);
    }

    TEST(LinearAlgebraTest, VectorVariableArithmetic_SubtractingVectors_CorrectResults)
    {
        VectorVariable v1 = VectorVariable::CreateFromVector({ 1.0, 2.0, 3.0 });
        VectorVariable v2 = VectorVariable::CreateFromVector({ 2.0, 6.0, -4.0 });
        VectorVariable v3 = v1 - v2;
        ExpectClose(v3.GetValues(), { -1.0, -4.0, 7.0 }, 1e-3);
        v3 = v2 - v1;
        ExpectClose(v3.GetValues(), { 1.0, 4.0, -7.0 }, 1e-3);
        VectorVariable v4 = -v3;
        ExpectClose(v4.GetValues(), { -1.0, -4.0, 7.0 }, 1e-3);
        v4 -= v2;
        ExpectClose(v4.GetValues(), { -3.0, -10.0, 11.0 }, 1e-3);
    }

    TEST(LinearAlgebraTest, VectorVariableArithmetic_ScalarVectorMultiplication_CorrectResults)
    {
        VectorVariable v1 = VectorVariable::CreateFromVector({ 1.0, 2.0, 3.0 });
        VectorVariable v2 = VectorVariable::CreateFromVector({ 2.0, 6.0, -4.0 });
        VectorVariable v3 = 3.0 * v1;
        ExpectClose(v3.GetValues(), { 3.0, 6.0, 9.0 }, 1e-3);
        v3 = v2 * 0.5;
        ExpectClose(v3.GetValues(), { 1.0, 3.0, -2.0 }, 1e-3);
    }

    TEST(LinearAlgebraTest, VectorVariableArithmetic_Norm_CorrectResults)
    {
        VectorVariable v1 = VectorVariable::CreateFromVector({ 3.0, -4.0, 12.0 });
        VectorVariable v2 = VectorVariable::CreateFromVector({ -7.0, -24.0 });
        VectorVariable v3 = VectorVariable::CreateFromVector({ 17.0 });
        VectorVariable v4;
        VectorVariable v5 = VectorVariable::CreateFromVector({ 3.0, 10.0, -5.0, 7.0, -8.0, 3.0 });
        VectorVariable v6(5);

        EXPECT_NEAR(v1.Norm(), 13.0, 1e-3);
        EXPECT_NEAR(v2.Norm(), 25.0, 1e-3);
        EXPECT_NEAR(v3.Norm(), 17.0, 1e-3);
        EXPECT_NEAR(v4.Norm(), 0, 1e-3);
        EXPECT_NEAR(v5.Norm(), 16, 1e-3);
        EXPECT_NEAR(v6.Norm(), 0, 1e-3);
    }

    TEST(LinearAlgebraTest, VectorVariableArithmetic_DotProduct_CorrectResults)
    {
        VectorVariable v1 = VectorVariable::CreateFromVector({ 6.0, -5.0, 8.0 });
        VectorVariable v2 = VectorVariable::CreateFromVector({ -7.0, -4.0, 2.0 });
        VectorVariable v3 = VectorVariable::CreateFromVector({ 3.0, 7.0, -5.0 });

        EXPECT_NEAR(v1.Dot(v2), -6.0, 1e-3);
        EXPECT_NEAR(v2.Dot(v1), -6.0, 1e-3);
        EXPECT_NEAR(v1.Dot(v3), -57.0, 1e-3);
        EXPECT_NEAR(v2.Dot(v3), -59.0, 1e-3);
    }

    TEST(LinearAlgebraTest, MatrixArithmetic_AddingMatrices_CorrectResults)
    {
        MatrixVariable m1(2, 3);
        MatrixVariable m2(2, 3);
        for (AZ::u32 row = 0; row < 2; row++)
        {
            for (AZ::u32 col = 0; col < 3; col++)
            {
                m1.Element(row, col) = row * row + col; // [[0, 1, 2], [1, 2, 3]]
                m2.Element(row, col) = row + col * 2.0; // [[0, 2, 4], [1, 3, 5]]
            }
        }

        MatrixVariable m3 = m1 + m2; // [[0, 3, 6], [2, 5, 8]]
        EXPECT_EQ(m3.GetNumRows(), 2);
        EXPECT_EQ(m3.GetNumColumns(), 3);
        EXPECT_NEAR(m3.Element(0, 0), 0.0, 1e-3);
        EXPECT_NEAR(m3.Element(1, 2), 8.0, 1e-3);
        EXPECT_NEAR(m3.Element(0, 1), 3.0, 1e-3);
        m3 += m1; // [[0, 4, 8], [3, 7, 11]]
        EXPECT_NEAR(m3.Element(0, 2), 8.0, 1e-3);
        EXPECT_NEAR(m3.Element(1, 1), 7.0, 1e-3);
    }

    TEST(LinearAlgebraTest, MatrixArithmetic_SubtractingMatrices_CorrectResults)
    {
        MatrixVariable m1(2, 3);
        MatrixVariable m2(2, 3);
        for (AZ::u32 row = 0; row < 2; row++)
        {
            for (AZ::u32 col = 0; col < 3; col++)
            {
                m1.Element(row, col) = row * row + col; // [[0, 1, 2], [1, 2, 3]]
                m2.Element(row, col) = row + col * 2.0; // [[0, 2, 4], [1, 3, 5]]
            }
        }

        MatrixVariable m3 = m2 - m1; // [[0, 1, 2], [0, 1, 2]]
        EXPECT_EQ(m3.GetNumRows(), 2);
        EXPECT_EQ(m3.GetNumColumns(), 3);
        EXPECT_NEAR(m3.Element(0, 2), 2.0, 1e-3);
        EXPECT_NEAR(m3.Element(1, 1), 1.0, 1e-3);
        EXPECT_NEAR(m3.Element(0, 1), 1.0, 1e-3);
        m3 = m1 - m3; // [[0, 0, 0], [1, 1, 1]]
        EXPECT_NEAR(m3.Element(0, 1), 0.0, 1e-3);
        EXPECT_NEAR(m3.Element(0, 0), 0.0, 1e-3);
        EXPECT_NEAR(m3.Element(1, 2), 1.0, 1e-3);
    }

    TEST(LinearAlgebraTest, MatrixArithmetic_MatrixScalarDivision_CorrectResults)
    {
        MatrixVariable m1(2, 2);
        m1.Element(0, 0) = 3.0;
        m1.Element(0, 1) = 9.0;
        m1.Element(1, 0) = -6.0;
        m1.Element(1, 1) = 3.0;
        MatrixVariable m2 = m1 / 3.0;
        EXPECT_NEAR(m2.Element(0, 0), 1.0, 1e-3);
        EXPECT_NEAR(m2.Element(1, 0), -2.0, 1e-3);
        MatrixVariable m3 = m2 / 0.5;
        EXPECT_NEAR(m3.Element(1, 1), 2.0, 1e-3);
        EXPECT_NEAR(m3.Element(1, 0), -4.0, 1e-3);
    }

    TEST(LinearAlgebraTest, MatrixArithmetic_MatrixScalarMultiplication_CorrectResults)
    {
        MatrixVariable m1(3, 2);
        m1.Element(0, 0) = 7.0;
        m1.Element(0, 1) = 5.0;
        m1.Element(1, 0) = -3.0;
        m1.Element(1, 1) = 4.0;
        m1.Element(2, 0) = 6.0;
        m1.Element(2, 1) = -2.0;
        MatrixVariable m2 = 4.0 * m1;
        EXPECT_NEAR(m2.Element(2, 0), 24.0, 1e-3);
        EXPECT_NEAR(m2.Element(1, 0), -12.0, 1e-3);
        EXPECT_NEAR(m2.Element(0, 1), 20.0, 1e-3);
        MatrixVariable m3 = 0.5 * m2;
        EXPECT_NEAR(m3.Element(1, 1), 8.0, 1e-3);
        EXPECT_NEAR(m3.Element(2, 1), -4.0, 1e-3);
        EXPECT_NEAR(m3.Element(0, 0), 14.0, 1e-3);
    }

    TEST(LinearAlgebraTest, MatrixArithmetic_MatrixMatrixMultiplication_CorrectResults)
    {
        MatrixVariable m1(3, 2);
        m1.Element(0, 0) = 1.0;
        m1.Element(0, 1) = 7.0;
        m1.Element(1, 0) = -2.0;
        m1.Element(1, 1) = -4.0;
        m1.Element(2, 0) = -3.0;
        m1.Element(2, 1) = 5.0;
        MatrixVariable m2(2, 2);
        m2.Element(0, 0) = 4.0;
        m2.Element(0, 1) = -3.0;
        m2.Element(1, 0) = 5.0;
        m2.Element(1, 1) = 2.0;
        MatrixVariable m3 = m1 * m2;

        EXPECT_EQ(m3.GetNumRows(), 3);
        EXPECT_EQ(m3.GetNumColumns(), 2);
        EXPECT_NEAR(m3.Element(0, 0), 39.0, 1e-3);
        EXPECT_NEAR(m3.Element(0, 1), 11.0, 1e-3);
        EXPECT_NEAR(m3.Element(1, 0), -28.0, 1e-3);
        EXPECT_NEAR(m3.Element(1, 1), -2.0, 1e-3);
        EXPECT_NEAR(m3.Element(2, 0), 13.0, 1e-3);
        EXPECT_NEAR(m3.Element(2, 1), 19.0, 1e-3);
    }

    TEST(LinearAlgebraTest, MatrixArithmetic_MatrixVectorMultiplication_CorrectResults)
    {
        MatrixVariable m(3, 2);
        m.Element(0, 0) = 1.0;
        m.Element(0, 1) = 7.0;
        m.Element(1, 0) = -2.0;
        m.Element(1, 1) = -4.0;
        m.Element(2, 0) = -3.0;
        m.Element(2, 1) = 5.0;
        VectorVariable v1 = VectorVariable::CreateFromVector({ 2.0, -3.0 });
        VectorVariable v2 = m * v1;

        EXPECT_EQ(v2.GetDimension(), 3);
        ExpectClose(v2.GetValues(), { -19.0, 8.0, -21.0 }, 1e-3);
    }

    TEST(LinearAlgebraTest, MatrixArithmetic_OuterProduct_CorrectResults)
    {
        VectorVariable v1 = VectorVariable::CreateFromVector({ 1.0, -2.0, 2.0 });
        VectorVariable v2 = VectorVariable::CreateFromVector({ -2.0, 3.0, 1.0 });
        MatrixVariable m = OuterProduct(v1, v2);

        EXPECT_NEAR(m.Element(0, 1), 3.0, 1e-3);
        EXPECT_NEAR(m.Element(1, 2), -2.0, 1e-3);
        EXPECT_NEAR(m.Element(1, 1), -6.0, 1e-3);
        EXPECT_NEAR(m.Element(0, 2), 1.0, 1e-3);
        EXPECT_NEAR(m.Element(2, 1), 6.0, 1e-3);
    }
} // namespace NumericalMethods::Optimization
