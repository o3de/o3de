/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <random>

namespace Benchmark
{
    const static float s_mat3x3testArray[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };

    class BM_MathMatrix3x3
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            m_testDataArray.resize(1000);

            const unsigned int seed = 1;
            std::mt19937_64 rng(seed);
            std::uniform_real_distribution<float> unif;

            std::generate(m_testDataArray.begin(), m_testDataArray.end(), [&unif, &rng]()
            {
                TestData testData;
                testData.q1 = AZ::Quaternion(unif(rng), unif(rng), unif(rng), unif(rng)).GetNormalized();
                testData.m1 = AZ::Matrix3x3::CreateFromQuaternion(testData.q1);
                testData.m2 = AZ::Matrix3x3::CreateFromQuaternion(testData.q1);
                testData.m3 = AZ::Matrix4x4::CreateFromQuaternionAndTranslation(testData.q1, AZ::Vector3(unif(rng), unif(rng), unif(rng)));
                testData.t1 = AZ::Transform::CreateFromQuaternionAndTranslation(testData.q1, AZ::Vector3(unif(rng), unif(rng), unif(rng)));
                testData.v1 = AZ::Vector3(unif(rng), unif(rng), unif(rng));
                return testData;
            });
        }
    public:
        void SetUp(const benchmark::State&) override
        {
            internalSetUp();
        }
        void SetUp(benchmark::State&) override
        {
            internalSetUp();
        }

        struct TestData
        {
            AZ::Matrix3x3 m1;
            AZ::Matrix3x3 m2;
            AZ::Matrix4x4 m3;
            AZ::Transform t1;
            AZ::Quaternion q1;
            AZ::Vector3 v1;
        };

        std::vector<TestData> m_testDataArray;
    };

    BENCHMARK_F(BM_MathMatrix3x3, CreateIdentity)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateIdentity();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateZero)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateZero();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetRowX3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.GetRow(0);
                benchmark::DoNotOptimize(result);
                result = testData.m1.GetRow(1);
                benchmark::DoNotOptimize(result);
                result = testData.m1.GetRow(2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetColumnX3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.GetColumn(0);
                benchmark::DoNotOptimize(result);
                result = testData.m1.GetColumn(1);
                benchmark::DoNotOptimize(result);
                result = testData.m1.GetColumn(2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateFromValue)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateFromValue(testData.v1.GetX());
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateFromRowMajorFloat9)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateFromRowMajorFloat9(s_mat3x3testArray);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateFromColumnMajorFloat9)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateFromColumnMajorFloat9(s_mat3x3testArray);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, StoreToRowMajorFloat9)(benchmark::State& state)
    {
        float storeValues[9];

        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m1.StoreToRowMajorFloat9(storeValues);
                benchmark::DoNotOptimize(storeValues);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, StoreToColumnMajorFloat9)(benchmark::State& state)
    {
        float storeValues[9];

        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m1.StoreToColumnMajorFloat9(storeValues);
                benchmark::DoNotOptimize(storeValues);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateRotationX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateRotationX(testData.v1.GetX());
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateRotationY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateRotationY(testData.v1.GetY());
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateRotationZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateRotationZ(testData.v1.GetZ());
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateFromTransform)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateFromTransform(testData.t1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateFromMatrix4x4)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateFromMatrix4x4(testData.m3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateFromQuaternion)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateFromQuaternion(testData.q1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateScale(testData.v1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateDiagonal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateDiagonal(testData.v1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, CreateCrossProduct)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = AZ::Matrix3x3::CreateCrossProduct(testData.v1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetElement)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                float result = testData.m1.GetElement(1, 2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, SetElement)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 testMatrix = testData.m2;
                testMatrix.SetElement(1, 2, -5.0f);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, SetRowX3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 testMatrix = testData.m2;
                testMatrix.SetRow(0, testData.v1);
                testMatrix.SetRow(1, testData.v1);
                testMatrix.SetRow(2, testData.v1);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, SetColumnX3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 testMatrix = testData.m2;
                testMatrix.SetColumn(0, testData.v1);
                testMatrix.SetColumn(1, testData.v1);
                testMatrix.SetColumn(2, testData.v1);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetBasisX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.GetBasisX();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetBasisY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.GetBasisY();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetBasisZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.GetBasisZ();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, OperatorAssign)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, OperatorMultiply)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1 * testData.m2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, TransposedMultiply)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1.TransposedMultiply(testData.m2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, MultiplyVector)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1 * testData.v1;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, OperatorSum)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1 + testData.m2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, OperatorDifference)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1 - testData.m2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, OperatorMultiplyScalar)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1 * testData.v1.GetX();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, OperatorDivideScalar)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1 / testData.v1.GetY();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, Transpose)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m1.Transpose();
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetTranspose)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1.GetTranspose();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, RetrieveScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.RetrieveScale();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, ExtractScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.ExtractScale();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, MultiplyByScaleX2)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m1.MultiplyByScale(testData.v1);
                testData.m1.MultiplyByScale(testData.v1.GetReciprocal());
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetPolarDecomposition)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1.GetPolarDecomposition();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetPolarDecomposition2)(benchmark::State& state)
    {
        AZ::Matrix3x3 orthogonalOut;
        AZ::Matrix3x3 symmetricOut;

        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m1.GetPolarDecomposition(&orthogonalOut, &symmetricOut);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetInverseFast)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1.GetInverseFast();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetInverseFull)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1.GetInverseFull();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, Orthogonalize)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m1.Orthogonalize();
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetOrthogonalized)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1.GetOrthogonalized();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, IsOrthogonal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result = testData.m1.IsOrthogonal();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetDiagonal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.GetDiagonal();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetDeterminant)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                float result = testData.m1.GetDeterminant();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix3x3, GetAdjugate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix3x3 result = testData.m1.GetAdjugate();
                benchmark::DoNotOptimize(result);
            }
        }
    }
}

#endif
