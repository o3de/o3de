/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/utils.h>
#include <NumericalMethods/Eigenanalysis.h>
#include <Eigenanalysis/Solver3x3.h>
#include <Eigenanalysis/Utilities.h>
#include <Tests/Environment.h>
#include <LinearAlgebra.h>

namespace NumericalMethods::Eigenanalysis
{
    // Structs for holding test cases. The allocator isn't ready when test cases are instantiated. The below structs do
    // not use any dynamic memory allocation.
    struct TestMatrix
    {
        const double m_00, m_01, m_02;
        const double       m_11, m_12;
        const double             m_22;
    };

    struct TestCase
    {
        TestMatrix m_matrix;                              // Test matrix.
        AZStd::array<Eigenpair<Real, 3>, 3> m_eigenpairs; // Expected eigenpairs.
    };

    // Small helper to allow creating an array without needing to specify its size.
    template <typename V, typename... T>
    constexpr auto CreateArrayOf(T&&... t) -> AZStd::array<V, sizeof...(T)>
    {
        return { { AZStd::forward<T>(t)... } };
    }

    // Test cases with unique eigenvalues. Eigenpairs must be sorted by eigenvalues in ascending order.
    auto testCasesUniqueEigenvalues = CreateArrayOf<TestCase>(
        TestCase{
            {
                    -11,   -3,   19,
                          -15,  -18,
                                 18
            },
            {{
                { -25.8595477937, { -0.505754443439, 0.698786152511, 0.505875830615 } },
                { -15.8674241728, { -0.771059353507, -0.629147018687, 0.098191151571 } },
                { 33.7269719665, { 0.386884887674, -0.340399679696, 0.856999499272 } }
            }}
        },
        TestCase{
            {
                    1,    0,    1,
                          1,    0,
                                1
            },
            {{
                { 0.0, { -0.707106781187, 0.0, 0.707106781187 } },
                { 1.0, { 0.0, 1.0, 0.0 } },
                { 2.0, { 0.707106781187, 0.0, 0.707106781187 } }
            }}
        },
        TestCase{
            {
                    5,   -2,  -17,
                         -4,  -20,
                               15
            },
            {{
                { -21.6701752837, { 0.420221510597, 0.700470257632, 0.576849460608 } },
                { 3.4584421978, { 0.793094051356, -0.592408347576, 0.141612765759 } },
                { 34.2117330859, { -0.440925966274, -0.397987145389, 0.804481525189 } }
            }}
        },
        TestCase{
            {
                    19,  -15,    6,
                         -10,   -6,
                                -5
            },
            {{
                { -17.2954822996, { 0.326496938489, 0.902445817476, 0.281053901729 } },
                { -5.99734083922, { -0.326080759975, -0.171551806039, 0.92964580127 } },
                { 27.2928231388, { 0.887170269526, -0.395172777863, 0.238259078535 } }
            }}
        },
        TestCase{
            {
                    -9,    4,  -11,
                           2,    5,
                               -17
            },
            {{
                { -26.1553450876, { 0.562432913498, -0.221379297992, 0.796655775247 } },
                { -1.32806716822, { -0.803682454314, 0.0800766638504, 0.589645860271 } },
                { 3.48341225584, { 0.19432892333, 0.971894507818, 0.132880906192 } }
            }}
        },
        TestCase{
            {
                    1,   10,   19,
                        -16,  -10,
                               18
            },
            {{
                { -27.7725713042, { -0.51831392659, 0.764992480984, 0.382278926363 } },
                { 0.250092094577, { -0.676678866396, -0.640202791396, 0.363656565542 } },
                { 30.5224792096, { 0.52293057405, -0.0701918081225, 0.849480267456 } }
            }}
        },
        TestCase{
            {
                    2,  -20,  -15,
                        -18,   18,
                               15
            },
            {{
                { -31.8837451646, { -0.430872006429, -0.879972750147, 0.199993182569 } },
                { -6.43875739702, { 0.716635511483, -0.198972078689, 0.668463653151 } },
                { 37.3225025616, { -0.548436739978, 0.431344492142, 0.716351220661 } }
            }}
        },
        TestCase{
            {
                    -9,   19,   15,
                          15,   16,
                                 6
            },
            {{
                { -21.1504502799, { -0.893560928596, 0.339760284078, 0.293448149167 } },
                { -6.10937620187, { 0.0235549957329, -0.617262260016, 0.786404771435 } },
                { 39.2598264818, { 0.448323576295, 0.709612747718, 0.543558386205 } }
            }}
        },
        TestCase{
            {
                    -7,   -3,   16,
                          12,   -9,
                                 3
            },
            {{
                { -19.059434343, { -0.785269804824, 0.101151484629, 0.610835256668 } },
                { 4.45691526016, { 0.464300105322, 0.748872986542, 0.472879120099 } },
                { 22.6025190828, { 0.409605597898, -0.654948568351, 0.635031988947 } }
            }}
        },
        TestCase{
            {
                    -9,  -14,   15,
                           6,    4,
                               -18
            },
            {{
                { -32.817456338, { -0.628492962187, -0.3005975342, 0.717382547121 } },
                { -3.26156142171, { 0.530405111203, 0.508970569411, 0.677952341601 } },
                { 15.0790177597, { 0.568917405684, -0.806591645076, 0.160445952281 } }
            }}
        }
    );

    // Test cases with unique eigenvalues. Eigenpairs must be sorted by eigenvalues in ascending order.
    auto testCasesRepeatedEigenvalues = CreateArrayOf<TestCase>(
        TestCase{
            {
                    1,    1,    1,
                          1,    1,
                                1
            },
            {{
                { 0.0, { -0.7071067811865475, 0.7071067811865475, 0.0 } },
                { 0.0, { 0.4082482904638630, 0.4082482904638630, -0.8164965809277260 } },
                { 3.0, { 0.5773502691896258, 0.5773502691896258, 0.5773502691896258 } }
            }}
        },
        TestCase{
            {
                    1.0,    0.0,    1.4142135623730950,
                            2.0,    0.0,
                                    0.0
            },
            {{
                { -1.0, { -0.5773502691896257, 0.0, 0.8164965809277261 } },
                { 2.0, { 0.0, 1.0, 0.0 } },
                { 2.0, { 0.816496580927726, 0, 0.5773502691896258 } }
            }}
        }
    );


    // Array to vector conversion.
    AZStd::vector<double> ArrayToVector(AZStd::array<double, 3> data)
    {
        return AZStd::vector<double>(data.begin(), data.end());
    }


    // Helper to compare actual and expected eigenvectors. Both must be unit length but they may point in opposite
    // directions.
    void ExpectParallelUnitVector(
        const AZStd::vector<double>& actual, const AZStd::vector<double>& expected, const double tolerance
    )
    {
        const VectorVariable actualVector = VectorVariable::CreateFromVector(actual);
        const VectorVariable expectedVector = VectorVariable::CreateFromVector(expected);

        // Check length of vector.
        EXPECT_NEAR(actualVector.Norm(), 1.0, tolerance);

        // Check direction of vector.
        AZStd::vector<double> corrected(3, 0.0);
        if (actualVector.Dot(expectedVector) >= 0)
        {
            corrected = expected;
        }
        else
        {
            corrected = (expectedVector * (-1.0)).GetValues();
        }
        ExpectClose(actual, corrected, tolerance);
    }


    // Helper to test that a unit vector is a circular combination of two other unit vectors. All three unit vectors
    // must lie in the same plane.
    void ExpectLinearlyDependentUnitVector(
        const AZStd::vector<double>& actual,
        const AZStd::vector<double>& baseOne,
        const AZStd::vector<double>& baseTwo,
        const double tolerance
    )
    {
        const VectorVariable actualVector = VectorVariable::CreateFromVector(actual);
        const VectorVariable baseOneVector = VectorVariable::CreateFromVector(baseOne);
        const VectorVariable baseTwoVector = VectorVariable::CreateFromVector(baseTwo);

        // Check length of vector.
        EXPECT_NEAR(actualVector.Norm(), 1.0, 1e-6);

        // Check whether the actual vector is a circular combination of the two base vectors.
        double componentOne = baseOneVector.Dot(actualVector);
        double componentTwo = baseTwoVector.Dot(actualVector);
        EXPECT_NEAR(componentOne * componentOne + componentTwo * componentTwo, 1.0, tolerance);

        const VectorVariable composedVector = baseOneVector * componentOne + baseTwoVector * componentTwo;
        ExpectClose(actualVector, composedVector, tolerance);
    }


    // Helper to check that the returned eigenvectors form a right handed basis.
    void ExpectRightHandedOrthogonalBasis(
        const AZStd::array<double, 3>& x,
        const AZStd::array<double, 3>& y,
        const AZStd::array<double, 3>& z,
        const double tolerance
    )
    {
        const VectorVariable xVector = VectorVariable::CreateFromVector(ArrayToVector(x));
        const VectorVariable yVector = VectorVariable::CreateFromVector(ArrayToVector(y));
        const VectorVariable zVector = VectorVariable::CreateFromVector(ArrayToVector(z));

        ExpectClose(zVector, CrossProduct(xVector, yVector), tolerance);

        EXPECT_NEAR(xVector.Dot(yVector), 0.0, tolerance);
        EXPECT_NEAR(xVector.Dot(zVector), 0.0, tolerance);
        EXPECT_NEAR(yVector.Dot(zVector), 0.0, tolerance);
    }


    TEST(CrossProductTest, CrossProduct_CorrectResults)
    {
        VectorVariable v1 = VectorVariable::CreateFromVector({ 6.0, -5.0, 8.0 });
        VectorVariable v2 = VectorVariable::CreateFromVector({ -7.0, -4.0, 2.0 });
        VectorVariable v3 = VectorVariable::CreateFromVector({ 3.0, 7.0, -5.0 });

        ExpectClose(CrossProduct(v1, v2).GetValues(), { 22.0, -68.0, -59.0 }, 1e-3);
        ExpectClose(CrossProduct(v2, v1).GetValues(), { -22.0, 68.0, 59.0 }, 1e-3);

        ExpectClose(CrossProduct(v1, v3).GetValues(), { -31.0, 54.0, 57.0 }, 1e-3);
        ExpectClose(CrossProduct(v3, v1).GetValues(), { 31.0, -54.0, -57.0 }, 1e-3);

        ExpectClose(CrossProduct(v2, v3).GetValues(), { 6.0, -29.0, -37.0 }, 1e-3);
        ExpectClose(CrossProduct(v3, v2).GetValues(), { -6.0, 29.0, 37.0 }, 1e-3);

        ExpectClose(CrossProduct(v1, v1).GetValues(), { 0.0, 0.0, 0.0 }, 1e-3);
        ExpectClose(CrossProduct(v2, v2).GetValues(), { 0.0, 0.0, 0.0 }, 1e-3);
        ExpectClose(CrossProduct(v3, v3).GetValues(), { 0.0, 0.0, 0.0 }, 1e-3);
    }


    class OrthogonalComplementParams
        : public ::testing::TestWithParam<AZStd::array<double, 3>>
    {
    };

    TEST_P(OrthogonalComplementParams, OrthogonalComplement_CorrectResult)
    {
        VectorVariable v1 = VectorVariable::CreateFromVector(ArrayToVector(GetParam()));

        VectorVariable v2(3);
        VectorVariable v3(3);

        ComputeOrthogonalComplement(v1, v2, v3);

        ExpectClose(CrossProduct(v2, v3), v1, 1e-6);
        EXPECT_NEAR(v2.Norm(), 1.0, 1e-6);
        EXPECT_NEAR(v3.Norm(), 1.0, 1e-6);
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        OrthogonalComplementParams,
        ::testing::Values(
            // The allocator isn't ready when the code below is executed. As a workaround, use a fixed size arrays.
            AZStd::array<double, 3>({ 1.0, 0.0, 0.0 }),
            AZStd::array<double, 3>({ 0.0, 1.0, 0.0 }),
            AZStd::array<double, 3>({ 0.0, 0.0, 1.0 }),
            AZStd::array<double, 3>({ 0.801784, 0.534522, -0.267261 })
        )
    );


    class ComputeEigenvector0Params
        : public ::testing::TestWithParam<TestCase>
    {
    };

    TEST_P(ComputeEigenvector0Params, ComputeEigenvector0_CorrectResult)
    {
        const TestCase& testCase = GetParam();

        // The eigenvalues of the test matrices are spaced sufficiently far apart, so we can use this algorithm to
        // compute all eigenvectors.
        for (const auto& expected : testCase.m_eigenpairs)
        {
            const VectorVariable actual = ComputeEigenvector0(
                testCase.m_matrix.m_00,
                testCase.m_matrix.m_01,
                testCase.m_matrix.m_02,
                testCase.m_matrix.m_11,
                testCase.m_matrix.m_12,
                testCase.m_matrix.m_22,
                expected.m_value
            );

            // The computed eigenvector must either be parallel or anti-parallel to the expected eigenvector.
            ExpectParallelUnitVector(actual.GetValues(), ArrayToVector(expected.m_vector), 1e-6);
        }
    }

    INSTANTIATE_TEST_CASE_P(All, ComputeEigenvector0Params, ::testing::ValuesIn(testCasesUniqueEigenvalues));


    class ComputeEigenvector1UniqueEigenvalueParams
        : public ::testing::TestWithParam<TestCase>
    {
    };

    TEST_P(ComputeEigenvector1UniqueEigenvalueParams, ComputeEigenvector1_CorrectResultForUniqueEigenvalue)
    {
        const TestCase& testCase = GetParam();

        // The algorithm to find the second eigenvector relies on one eigenvector being known already. To test this
        // function fully, each eigenvector plays the role of known and expected vector in turn until all combinations
        // are exhausted.
        for (auto i = testCase.m_eigenpairs.begin(); i != testCase.m_eigenpairs.end(); ++i)
        {
            for (auto j = testCase.m_eigenpairs.begin(); j != testCase.m_eigenpairs.end(); ++j)
            {
                if (i != j)
                {
                    const VectorVariable knownVec = VectorVariable::CreateFromVector(ArrayToVector(i->m_vector));
                    const double knownVal = j->m_value;

                    const VectorVariable actual = ComputeEigenvector1(
                        testCase.m_matrix.m_00,
                        testCase.m_matrix.m_01,
                        testCase.m_matrix.m_02,
                        testCase.m_matrix.m_11,
                        testCase.m_matrix.m_12,
                        testCase.m_matrix.m_22,
                        knownVal,
                        knownVec
                    );

                    // The computed eigenvector must either be parallel or anti-parallel to the expected eigenvector.
                    ExpectParallelUnitVector(actual.GetValues(), ArrayToVector(j->m_vector), 1e-6);
                }
            }
        }
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        ComputeEigenvector1UniqueEigenvalueParams,
        ::testing::ValuesIn(testCasesUniqueEigenvalues)
    );


    class ComputeEigenvector1RepeatedEigenvalueParams
        : public ::testing::TestWithParam<TestCase>
    {
    };

    TEST_P(ComputeEigenvector1RepeatedEigenvalueParams, ComputeEigenvector1_CorrectResultForRepeatedEigenvalue)
    {
        const TestCase& testCase = GetParam();

        // Get the index of the eigenpair with the unique eigenvalue.
        int uniqueIndex = testCase.m_eigenpairs[0].m_value == testCase.m_eigenpairs[1].m_value ? 2 : 0;

        const VectorVariable knownVec = VectorVariable::CreateFromVector(
            ArrayToVector(testCase.m_eigenpairs[uniqueIndex].m_vector)
        );
        const double knownVal = testCase.m_eigenpairs[(uniqueIndex + 1) % 3].m_value;

        const VectorVariable actual = ComputeEigenvector1(1.0, 1.0, 1.0, 1.0, 1.0, 1.0, knownVal, knownVec);

        // The computed eigenvalue must be a circular combination of the two expected eigenvectors corresponding to the
        // repeated eigenvalue.
        ExpectLinearlyDependentUnitVector(
            actual.GetValues(),
            ArrayToVector(testCase.m_eigenpairs[(uniqueIndex + 1) % 3].m_vector),
            ArrayToVector(testCase.m_eigenpairs[(uniqueIndex + 2) % 3].m_vector),
            1e-6
        );
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        ComputeEigenvector1RepeatedEigenvalueParams,
        ::testing::ValuesIn(testCasesRepeatedEigenvalues)
    );


    class ComputeEigenvector2Params
        : public ::testing::TestWithParam<TestCase>
    {
    };

    TEST_P(ComputeEigenvector2Params, ComputeEigenvector2_CorrectResult)
    {
        const TestCase& testCase = GetParam();

        // The third eigenvector is simply computed as the cross-product of the first two.
        for (int i = 0; i < 3; ++i)
        {
            const VectorVariable knownOne = VectorVariable::CreateFromVector(
                ArrayToVector(testCase.m_eigenpairs[(i + 0) % 3].m_vector)
            );

            const VectorVariable knownTwo = VectorVariable::CreateFromVector(
                ArrayToVector(testCase.m_eigenpairs[(i + 1) % 3].m_vector)
            );

            const VectorVariable expected = VectorVariable::CreateFromVector(
                ArrayToVector(testCase.m_eigenpairs[(i + 2) % 3].m_vector)
            );

            // The computed eigenvectors must either be parallel or anti-parallel to the expected eigenvectors.
            ExpectParallelUnitVector(ComputeEigenvector2(knownOne, knownTwo).GetValues(), expected.GetValues(), 1e-6);
            ExpectParallelUnitVector(ComputeEigenvector2(knownTwo, knownOne).GetValues(), expected.GetValues(), 1e-6);
        }
    }

    INSTANTIATE_TEST_CASE_P(Unique, ComputeEigenvector2Params, ::testing::ValuesIn(testCasesUniqueEigenvalues));
    INSTANTIATE_TEST_CASE_P(Repeated, ComputeEigenvector2Params, ::testing::ValuesIn(testCasesRepeatedEigenvalues));


    class NonIterativeSymmetricEigensolver3x3UniqueEigenvalueParams
        : public ::testing::TestWithParam<TestCase>
    {
    };

    TEST_P(
        NonIterativeSymmetricEigensolver3x3UniqueEigenvalueParams,
        NonIterativeSymmetricEigensolver3x3_CorrectResultForUniqueEigenvalue
    )
    {
        const TestCase& testCase = GetParam();

        SolverResult<Real, 3> result = NonIterativeSymmetricEigensolver3x3(
            testCase.m_matrix.m_00,
            testCase.m_matrix.m_01,
            testCase.m_matrix.m_02,
            testCase.m_matrix.m_11,
            testCase.m_matrix.m_12,
            testCase.m_matrix.m_22
        );

        // Must return exactly three eigenpairs.
        EXPECT_EQ(result.m_eigenpairs.size(), 3);

        // For non-diagonal matrices the eigenvalues will be sorted from smallest to largest.
        for (int i = 0; i < 3; ++i)
        {
            EXPECT_NEAR(result.m_eigenpairs[i].m_value, testCase.m_eigenpairs[i].m_value, 1e-6);
            ExpectParallelUnitVector(
                ArrayToVector(result.m_eigenpairs[i].m_vector), ArrayToVector(testCase.m_eigenpairs[i].m_vector), 1e-6
            );
        }

        ExpectRightHandedOrthogonalBasis(
            result.m_eigenpairs[0].m_vector, result.m_eigenpairs[1].m_vector, result.m_eigenpairs[2].m_vector, 1e-6
        );
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        NonIterativeSymmetricEigensolver3x3UniqueEigenvalueParams,
        ::testing::ValuesIn(testCasesUniqueEigenvalues)
    );


    class NonIterativeSymmetricEigensolver3x3RepeatedEigenvalueParams
        : public ::testing::TestWithParam<TestCase>
    {
    };

    TEST_P(
        NonIterativeSymmetricEigensolver3x3RepeatedEigenvalueParams,
        NonIterativeSymmetricEigensolver3x3_CorrectResultForRepeatedEigenvalue
    )
    {
        const TestCase& testCase = GetParam();

        // Get the index of the eigenpair with the unique eigenvalue.
        int uniqueIndex = testCase.m_eigenpairs[0].m_value == testCase.m_eigenpairs[1].m_value ? 2 : 0;

        SolverResult<Real, 3> result = NonIterativeSymmetricEigensolver3x3(
            testCase.m_matrix.m_00,
            testCase.m_matrix.m_01,
            testCase.m_matrix.m_02,
            testCase.m_matrix.m_11,
            testCase.m_matrix.m_12,
            testCase.m_matrix.m_22
        );

        // Must return exactly three eigenpairs.
        EXPECT_EQ(result.m_eigenpairs.size(), 3);

        // For non-diagonal matrices the eigenvalues will be sorted from smallest to largest.
        EXPECT_NEAR(result.m_eigenpairs[0].m_value, testCase.m_eigenpairs[0].m_value, 1e-6);
        EXPECT_NEAR(result.m_eigenpairs[1].m_value, testCase.m_eigenpairs[1].m_value, 1e-6);
        EXPECT_NEAR(result.m_eigenpairs[2].m_value, testCase.m_eigenpairs[2].m_value, 1e-6);

        for (int i = 0; i < 3; ++i)
        {
            if (i == uniqueIndex)
            {
                ExpectParallelUnitVector(
                    ArrayToVector(result.m_eigenpairs[i].m_vector),
                    ArrayToVector(testCase.m_eigenpairs[i].m_vector),
                    1e-6
                );
            }
            else
            {
                ExpectLinearlyDependentUnitVector(
                    ArrayToVector(result.m_eigenpairs[i].m_vector),
                    ArrayToVector(testCase.m_eigenpairs[(uniqueIndex + 1) % 3].m_vector),
                    ArrayToVector(testCase.m_eigenpairs[(uniqueIndex + 2) % 3].m_vector),
                    1e-6
                );
            }
        }

        ExpectRightHandedOrthogonalBasis(
            result.m_eigenpairs[0].m_vector, result.m_eigenpairs[1].m_vector, result.m_eigenpairs[2].m_vector, 1e-6
        );
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        NonIterativeSymmetricEigensolver3x3RepeatedEigenvalueParams,
        ::testing::ValuesIn(testCasesRepeatedEigenvalues)
    );


    class NonIterativeSymmetricEigensolver3x3DiagonalMatrixParams
        : public ::testing::TestWithParam<AZStd::array<double, 3>>
    {
    };

    TEST_P(
        NonIterativeSymmetricEigensolver3x3DiagonalMatrixParams,
        NonIterativeSymmetricEigensolver3x3_CorrectResultForDiagonalMatrix
    )
    {
        const AZStd::array<double, 3>& matrixDiagonal = GetParam();

        SolverResult<Real, 3> result = NonIterativeSymmetricEigensolver3x3(
            matrixDiagonal[0], 0.0, 0.0, matrixDiagonal[1], 0.0, matrixDiagonal[2]
        );

        // Must return exactly three eigenpairs.
        EXPECT_EQ(result.m_eigenpairs.size(), 3);

        // This is a special case in which the eigenvectors are the standard Cartesian basis vectors (returned in the
        // same order). This test also covers the zero matrix.
        EXPECT_NEAR(result.m_eigenpairs[0].m_value, matrixDiagonal[0], 1e-6);
        ExpectParallelUnitVector(ArrayToVector(result.m_eigenpairs[0].m_vector), { 1.0, 0.0, 0.0 }, 1e-6);

        EXPECT_NEAR(result.m_eigenpairs[1].m_value, matrixDiagonal[1], 1e-6);
        ExpectParallelUnitVector(ArrayToVector(result.m_eigenpairs[1].m_vector), { 0.0, 1.0, 0.0 }, 1e-6);

        EXPECT_NEAR(result.m_eigenpairs[2].m_value, matrixDiagonal[2], 1e-6);
        ExpectParallelUnitVector(ArrayToVector(result.m_eigenpairs[2].m_vector), { 0.0, 0.0, 1.0 }, 1e-6);
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        NonIterativeSymmetricEigensolver3x3DiagonalMatrixParams,
        ::testing::Values(
            AZStd::array<double, 3>({ 1.0, 1.0, 1.0 }),
            AZStd::array<double, 3>({ 1.0, 0.0, -1.0 }),
            AZStd::array<double, 3>({ 3.0, -8.0, 5.0 }),
            AZStd::array<double, 3>({ 0.0, 0.0, 0.0 })
        )
    );
} // namespace NumericalMethods::Eigenanalysis
