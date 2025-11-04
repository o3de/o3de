/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/MatrixMxN.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>

namespace UnitTest
{
    class Math_MatrixMxN
        : public UnitTest::LeakDetectionFixture
    {
    };

    TEST_F(Math_MatrixMxN, TestConstructor)
    {
        AZ::MatrixMxN m1(3, 2, 5.0f);
        EXPECT_EQ(m1.GetRowCount(), 3);
        EXPECT_EQ(m1.GetColumnCount(), 2);
        for (AZStd::size_t rowIter = 0; rowIter < m1.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < m1.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(m1.GetElement(rowIter, colIter), 5.0f);
            }
        }
    }

    TEST_F(Math_MatrixMxN, TestCreate)
    {
        AZ::MatrixMxN zeros = AZ::MatrixMxN::CreateZero(8, 3);
        EXPECT_EQ(zeros.GetRowCount(), 8);
        EXPECT_EQ(zeros.GetColumnCount(), 3);
        for (AZStd::size_t rowIter = 0; rowIter < zeros.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < zeros.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(zeros.GetElement(rowIter, colIter), 0.0f);
            }
        }

        AZ::MatrixMxN random = AZ::MatrixMxN::CreateRandom(7, 6);
        EXPECT_EQ(random.GetRowCount(), 7);
        EXPECT_EQ(random.GetColumnCount(), 6);
        for (AZStd::size_t rowIter = 0; rowIter < random.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < random.GetColumnCount(); ++colIter)
            {
                ASSERT_GE(random.GetElement(rowIter, colIter), 0.0f);
                ASSERT_LE(random.GetElement(rowIter, colIter), 1.0f);
            }
        }
    }

    TEST_F(Math_MatrixMxN, TestGetSetElement)
    {
        AZ::MatrixMxN testMatrix(15, 7);
        EXPECT_EQ(testMatrix.GetRowCount(), 15);
        EXPECT_EQ(testMatrix.GetColumnCount(), 7);

        // Initialize each element to be the sum of the indices for that element
        for (AZStd::size_t rowIter = 0; rowIter < testMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < testMatrix.GetColumnCount(); ++colIter)
            {
                testMatrix.SetElement(rowIter, colIter, static_cast<float>(2.0f * rowIter + colIter));
            }
        }

        // Validate that each element was stored and can be retrieved correctly
        for (AZStd::size_t rowIter = 0; rowIter < testMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < testMatrix.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(testMatrix.GetElement(rowIter, colIter), static_cast<float>(2.0f * rowIter + colIter));
                EXPECT_FLOAT_EQ(testMatrix(rowIter, colIter), static_cast<float>(2.0f * rowIter + colIter));
            }
        }
    }

    TEST_F(Math_MatrixMxN, TestTranspose)
    {
        AZ::MatrixMxN testMatrix(15, 7);
        EXPECT_EQ(testMatrix.GetRowCount(), 15);
        EXPECT_EQ(testMatrix.GetColumnCount(), 7);
        for (AZStd::size_t rowIter = 0; rowIter < testMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < testMatrix.GetColumnCount(); ++colIter)
            {
                testMatrix.SetElement(rowIter, colIter, static_cast<float>(2.0f * rowIter + colIter));
            }
        }

        // Validate that the result has the correct dimensionality and every element was correctly transposed
        AZ::MatrixMxN resultMatrix = testMatrix.GetTranspose();
        EXPECT_EQ(resultMatrix.GetRowCount(), 7);
        EXPECT_EQ(resultMatrix.GetColumnCount(), 15);
        for (AZStd::size_t rowIter = 0; rowIter < resultMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < resultMatrix.GetColumnCount(); ++colIter)
            {
                // Originally we stored 2 times the row index + the column index
                // The transpose should therefore be storing values of 2 times the column index + the row index
                EXPECT_FLOAT_EQ(resultMatrix.GetElement(rowIter, colIter), static_cast<float>(2.0f * colIter + rowIter));
            }
        }
    }

    TEST_F(Math_MatrixMxN, TestOperators)
    {
        AZ::MatrixMxN oneMatrix(5, 3, 1.0f);
        AZ::MatrixMxN testMatrix = oneMatrix;
        for (AZStd::size_t rowIter = 0; rowIter < oneMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < oneMatrix.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(oneMatrix.GetElement(rowIter, colIter), 1.0f);
            }
        }

        testMatrix /= 2.0f;
        for (AZStd::size_t rowIter = 0; rowIter < testMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < testMatrix.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(testMatrix.GetElement(rowIter, colIter), 0.5f);
            }
        }

        testMatrix *= 2.0f;
        for (AZStd::size_t rowIter = 0; rowIter < testMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < testMatrix.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(testMatrix.GetElement(rowIter, colIter), 1.0f);
            }
        }

        testMatrix *= 2.0f;
        testMatrix += oneMatrix;
        for (AZStd::size_t rowIter = 0; rowIter < testMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < testMatrix.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(testMatrix.GetElement(rowIter, colIter), 3.0f);
            }
        }

        testMatrix -= oneMatrix;
        for (AZStd::size_t rowIter = 0; rowIter < testMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < testMatrix.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(testMatrix.GetElement(rowIter, colIter), 2.0f);
            }
        }

        testMatrix *= 2.0f;
        for (AZStd::size_t rowIter = 0; rowIter < testMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < testMatrix.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(testMatrix.GetElement(rowIter, colIter), 4.0f);
            }
        }

        testMatrix += 7.0f;
        for (AZStd::size_t rowIter = 0; rowIter < testMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < testMatrix.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(testMatrix.GetElement(rowIter, colIter), 11.0f);
            }
        }

        testMatrix -= 12.0f;
        for (AZStd::size_t rowIter = 0; rowIter < testMatrix.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < testMatrix.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(testMatrix.GetElement(rowIter, colIter), -1.0f);
            }
        }
    }

    TEST_F(Math_MatrixMxN, TestFloorCeilRound)
    {
        AZ::MatrixMxN mat1 = AZ::MatrixMxN(7, 3, 4.1f);
        AZ::MatrixMxN floor = mat1.GetFloor();
        for (AZStd::size_t rowIter = 0; rowIter < floor.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < floor.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(floor.GetElement(rowIter, colIter), 4.0f);
            }
        }
        
        AZ::MatrixMxN ceil = mat1.GetCeil();
        for (AZStd::size_t rowIter = 0; rowIter < floor.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < floor.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(ceil.GetElement(rowIter, colIter), 5.0f);
            }
        }
        
        AZ::MatrixMxN round = mat1.GetRound();
        for (AZStd::size_t rowIter = 0; rowIter < floor.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < floor.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(round.GetElement(rowIter, colIter), 4.0f);
            }
        }
        
        AZ::MatrixMxN vec2 = AZ::MatrixMxN(7, 3, 4.5f);
        AZ::MatrixMxN tie = vec2.GetRound();
        for (AZStd::size_t rowIter = 0; rowIter < floor.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < floor.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(tie.GetElement(rowIter, colIter), 4.0f);
            }
        }
    }

    TEST_F(Math_MatrixMxN, TestMinMaxClamp)
    {
        AZ::MatrixMxN random = AZ::MatrixMxN::CreateRandom(7, 3); // random matrix with elements between 0 and 1
        
        // min should be between 0.0 and 0.5
        AZ::MatrixMxN min = random.GetMin(AZ::MatrixMxN(7, 3, 0.5f));
        for (AZStd::size_t rowIter = 0; rowIter < min.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < min.GetColumnCount(); ++colIter)
            {
                EXPECT_LE(min.GetElement(rowIter, colIter), 0.5f);
            }
        }
        
        // max should be between 0.5 and 1.0
        AZ::MatrixMxN max = random.GetMax(AZ::MatrixMxN(7, 3, 0.5f));
        for (AZStd::size_t rowIter = 0; rowIter < max.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < max.GetColumnCount(); ++colIter)
            {
                EXPECT_GE(max.GetElement(rowIter, colIter), 0.5f);
            }
        }
        
        // clamp should be between 0.1 and 0.9
        AZ::MatrixMxN clamp = random.GetClamp(AZ::MatrixMxN(7,3,  0.1f), AZ::MatrixMxN(7, 3, 0.9f));
        for (AZStd::size_t rowIter = 0; rowIter < clamp.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < clamp.GetColumnCount(); ++colIter)
            {
                EXPECT_GE(clamp.GetElement(rowIter, colIter), 0.1f);
                EXPECT_LE(clamp.GetElement(rowIter, colIter), 0.9f);
            }
        }
    }

    TEST_F(Math_MatrixMxN, TestSquareAbs)
    {
        AZ::MatrixMxN random = AZ::MatrixMxN::CreateRandom(200, 100); // random vector between 0 and 1
        random -= 0.5f; // random vector between -0.5 and 0.5

        // absRand should be between 0.0 and 0.5
        AZ::MatrixMxN absRand = random.GetAbs();
        for (AZStd::size_t rowIter = 0; rowIter < absRand.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < absRand.GetColumnCount(); ++colIter)
            {
                EXPECT_GE(absRand.GetElement(rowIter, colIter), 0.0f);
                EXPECT_LE(absRand.GetElement(rowIter, colIter), 0.5f);
            }
        }

        // sqRand should be between -0.25 and 0.25
        AZ::MatrixMxN sqRand = random.GetSquare();
        for (AZStd::size_t rowIter = 0; rowIter < sqRand.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < sqRand.GetColumnCount(); ++colIter)
            {
                EXPECT_GE(sqRand.GetElement(rowIter, colIter), -0.25f);
                EXPECT_LE(sqRand.GetElement(rowIter, colIter),  0.25f);
            }
        }
    }

    TEST_F(Math_MatrixMxN, TestOuterProduct)
    {
        const float lhsElements[] =
        {
            0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f
        };

        const float rhsElements[] =
        {
            4.0f, 3.0f, 2.0f, 1.0f, 0.0f
        };

        const AZ::VectorN lhsVector = AZ::VectorN::CreateFromFloats(7, lhsElements);
        const AZ::VectorN rhsVector = AZ::VectorN::CreateFromFloats(5, rhsElements);

        AZ::MatrixMxN output = AZ::MatrixMxN::CreateZero(7, 5);
        AZ::OuterProduct(lhsVector, rhsVector, output);
        for (AZStd::size_t rowIter = 0; rowIter < output.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < output.GetColumnCount(); ++colIter)
            {
                const float expectedOutput = float(rowIter) * (4.0f - float(colIter));
                const float actualOutput = output.GetElement(rowIter, colIter);
                EXPECT_FLOAT_EQ(actualOutput, expectedOutput);
            }
        }
    }

    TEST_F(Math_MatrixMxN, TestVectorMatrixMultiply)
    {
        const float matrixElements[] =
        {
            1.0f, 0.0f, 2.0f, 0.0f, 1.0f, 0.0f, 3.0f,
            0.0f, 1.0f, 0.0f, 2.0f, 0.0f, 1.0f, 3.0f,
            0.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f, 3.0f,
            1.0f, 0.0f, 2.0f, 0.0f, 1.0f, 0.0f, 3.0f,
            0.0f, 1.0f, 0.0f, 2.0f, 0.0f, 1.0f, 3.0f,
            0.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f, 3.0f,
        };

        const float vectorElements[] =
        {
            1.0f, 0.0f, 2.0f, 0.0f, 3.0f, 0.0f, 1.0f
        };

        const float outputElements[] =
        {
            1.0f + 0.0f + 4.0f + 0.0f + 3.0f + 0.0f + 3.0f,
            0.0f + 0.0f + 0.0f + 0.0f + 0.0f + 0.0f + 3.0f,
            0.0f + 0.0f + 2.0f + 0.0f + 6.0f + 0.0f + 3.0f,
            1.0f + 0.0f + 4.0f + 0.0f + 3.0f + 0.0f + 3.0f,
            0.0f + 0.0f + 0.0f + 0.0f + 0.0f + 0.0f + 3.0f,
            0.0f + 0.0f + 2.0f + 0.0f + 6.0f + 0.0f + 3.0f,
        };

        const AZ::MatrixMxN matrix = AZ::MatrixMxN::CreateFromPackedFloats(6, 7, matrixElements);
        const AZ::VectorN vector = AZ::VectorN::CreateFromFloats(7, vectorElements);

        AZ::VectorN output = AZ::VectorN::CreateZero(6);
        VectorMatrixMultiply(matrix, vector, output);
        for (AZStd::size_t iter = 0; iter < output.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(output.GetElement(iter), outputElements[iter]);
        }
    }

    TEST_F(Math_MatrixMxN, TestVectorMatrixLeftMultiply)
    {
        const float matrixElements[] =
        {
          1.0f, 0.0f, 2.0f, 0.0f, 1.0f, 0.0f, 3.0f,
          0.0f, 1.0f, 0.0f, 2.0f, 0.0f, 1.0f, 3.0f,
          0.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f, 3.0f,
          1.0f, 0.0f, 2.0f, 0.0f, 1.0f, 0.0f, 3.0f,
          0.0f, 1.0f, 0.0f, 2.0f, 0.0f, 1.0f, 3.0f,
          0.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f, 3.0f,
        };

        const float vectorElements[] =
        {
            1.0f, 0.0f, 2.0f, 0.0f, 3.0f, 0.0f
        };

        const float outputElements[] =
        {
            1.0f + 0.0f + 0.0f + 0.0f + 0.0f + 0.0f,
            0.0f + 0.0f + 0.0f + 0.0f + 3.0f + 0.0f,
            2.0f + 0.0f + 2.0f + 0.0f + 0.0f + 0.0f,
            0.0f + 0.0f + 0.0f + 0.0f + 6.0f + 0.0f,
            1.0f + 0.0f + 4.0f + 0.0f + 0.0f + 0.0f,
            0.0f + 0.0f + 0.0f + 0.0f + 3.0f + 0.0f,
            3.0f + 0.0f + 6.0f + 0.0f + 9.0f + 0.0f,
        };

        const AZ::MatrixMxN matrix = AZ::MatrixMxN::CreateFromPackedFloats(6, 7, matrixElements);
        const AZ::VectorN vector = AZ::VectorN::CreateFromFloats(6, vectorElements);

        AZ::VectorN output = AZ::VectorN::CreateZero(7);
        VectorMatrixMultiplyLeft(vector, matrix, output);
        for (AZStd::size_t iter = 0; iter < output.GetDimensionality(); ++iter)
        {
            EXPECT_FLOAT_EQ(output.GetElement(iter), outputElements[iter]);
        }
    }

    TEST_F(Math_MatrixMxN, TestMatrixMatrixMultiplySingleBlock)
    {
        // 3 x 4
        const float lhsElements[] =
        {
            1.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 0.0f, 1.0f, 2.0f,
        };

        // 4 x 3
        const float rhsElements[] =
        {
            1.0f, 2.0f, 3.0f,
            4.0f, 5.0f, 6.0f,
            7.0f, 8.0f, 9.0f,
            0.0f, 1.0f, 2.0f,
        };

        // 3 x 3
        const float outputElements[] =
        {
             30.0f,  40.0f,  50.0f,
             78.0f, 104.0f, 130.0f,
             16.0f,  28.0f,  40.0f,
        };

        const AZ::MatrixMxN lhs = AZ::MatrixMxN::CreateFromPackedFloats(3, 4, lhsElements);
        const AZ::MatrixMxN rhs = AZ::MatrixMxN::CreateFromPackedFloats(4, 3, rhsElements);

        AZ::MatrixMxN output = AZ::MatrixMxN::CreateZero(3, 3);
        AZ::MatrixMatrixMultiply(lhs, rhs, output);

        const float* checkResult = outputElements;
        for (AZStd::size_t rowIter = 0; rowIter < output.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < output.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(output.GetElement(rowIter, colIter), *checkResult);
                ++checkResult;
            }
        }
    }

    TEST_F(Math_MatrixMxN, TestMatrixMatrixMultiply)
    {
        // 5 x 7
        const float lhsElements[] =
        {
            1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f,
            8.0f, 9.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 0.0f, 1.0f,
            2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
        };

        // 7 x 6
        const float rhsElements[] =
        {
            1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f,
            7.0f, 8.0f, 9.0f, 0.0f, 1.0f, 2.0f,
            3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f,
            5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 0.0f,
            1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f,
            7.0f, 8.0f, 9.0f, 0.0f, 1.0f, 2.0f,
        };

        // 5 x 6
        const float outputElements[] =
        {
            140.0f, 128.0f, 156.0f,  94.0f, 122.0f, 100.0f,
            121.0f, 138.0f, 165.0f,  62.0f,  89.0f,  96.0f,
            192.0f, 148.0f, 184.0f, 150.0f, 186.0f, 132.0f,
            173.0f, 158.0f, 193.0f, 118.0f, 153.0f, 128.0f,
             84.0f,  88.0f, 112.0f,  86.0f, 110.0f, 104.0f,
        };

        AZ::MatrixMxN lhs = AZ::MatrixMxN::CreateFromPackedFloats(5, 7, lhsElements);
        AZ::MatrixMxN rhs = AZ::MatrixMxN::CreateFromPackedFloats(7, 6, rhsElements);

        AZ::MatrixMxN output = AZ::MatrixMxN::CreateZero(5, 6);
        AZ::MatrixMatrixMultiply(lhs, rhs, output);

        const float* checkResult = outputElements;
        for (AZStd::size_t rowIter = 0; rowIter < output.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < output.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(output.GetElement(rowIter, colIter), *checkResult);
                ++checkResult;
            }
        }

        AZ::MatrixMxN result = lhs * rhs;
        checkResult = outputElements;
        for (AZStd::size_t rowIter = 0; rowIter < result.GetRowCount(); ++rowIter)
        {
            for (AZStd::size_t colIter = 0; colIter < result.GetColumnCount(); ++colIter)
            {
                EXPECT_FLOAT_EQ(result.GetElement(rowIter, colIter), *checkResult);
                ++checkResult;
            }
        }
    }
}
