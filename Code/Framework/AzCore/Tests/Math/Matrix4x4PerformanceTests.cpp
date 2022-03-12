/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <random>

namespace Benchmark
{
    const static float s_mat4x4testArray[] = { 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f, 16.f };

    class BM_MathMatrix4x4
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
                testData.m1 = AZ::Matrix4x4::CreateFromQuaternionAndTranslation(testData.q1, AZ::Vector3(unif(rng), unif(rng), unif(rng)));
                testData.q1 = AZ::Quaternion(unif(rng), unif(rng), unif(rng), unif(rng)).GetNormalized();
                testData.m2 = AZ::Matrix4x4::CreateFromQuaternionAndTranslation(testData.q1, AZ::Vector3(unif(rng), unif(rng), unif(rng)));
                testData.v1 = AZ::Vector4(unif(rng), unif(rng), unif(rng), unif(rng));
                testData.v2 = AZ::Vector3(unif(rng), unif(rng), unif(rng));
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
            AZ::Matrix4x4 m1;
            AZ::Matrix4x4 m2;
            AZ::Quaternion q1;
            AZ::Vector4 v1;
            AZ::Vector3 v2;
        };

        std::vector<TestData> m_testDataArray;
    };

    BENCHMARK_F(BM_MathMatrix4x4, CreateIdentity)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateIdentity();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateZero)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateZero();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, GetRowX4)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector4 result = testData.m1.GetRow(0);
                benchmark::DoNotOptimize(result);
                result = testData.m1.GetRow(1);
                benchmark::DoNotOptimize(result);
                result = testData.m1.GetRow(2);
                benchmark::DoNotOptimize(result);
                result = testData.m1.GetRow(3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, GetColumnX3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector4 result = testData.m1.GetColumn(0);
                benchmark::DoNotOptimize(result);
                result = testData.m1.GetColumn(1);
                benchmark::DoNotOptimize(result);
                result = testData.m1.GetColumn(2);
                benchmark::DoNotOptimize(result);
                result = testData.m1.GetColumn(3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateFromValue)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateFromValue(1.0f);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateFromRowMajorFloat16)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateFromRowMajorFloat16(s_mat4x4testArray);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateFromColumnMajorFloat9)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateFromColumnMajorFloat16(s_mat4x4testArray);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateProjection)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateProjection(s_mat4x4testArray[0], s_mat4x4testArray[1], s_mat4x4testArray[2], s_mat4x4testArray[3]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateProjectionFov)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateProjectionFov(s_mat4x4testArray[0], s_mat4x4testArray[1], s_mat4x4testArray[2], s_mat4x4testArray[3]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateInterpolated)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateInterpolated(testData.m1, testData.m2, testData.v1.GetX());
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, StoreToRowMajorFloat16)(benchmark::State& state)
    {
        float storeValues[16];

        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m1.StoreToRowMajorFloat16(storeValues);
                benchmark::DoNotOptimize(storeValues);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, StoreToColumnMajorFloat16)(benchmark::State& state)
    {
        float storeValues[16];

        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m1.StoreToColumnMajorFloat16(storeValues);
                benchmark::DoNotOptimize(storeValues);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateRotationX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateRotationX(testData.v1.GetX());
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateRotationY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateRotationY(testData.v1.GetX());
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateRotationZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateRotationZ(testData.v1.GetX());
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateFromQuaternion)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateFromQuaternion(testData.q1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateScale(testData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateDiagonal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateDiagonal(testData.v1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, GetElement)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathMatrix4x4, SetElement)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 testMatrix = testData.m2;
                testMatrix.SetElement(1, 2, -5.0f);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, SetRowX3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 testMatrix = testData.m2;
                testMatrix.SetRow(0, testData.v1);
                testMatrix.SetRow(1, testData.v1);
                testMatrix.SetRow(2, testData.v1);
                testMatrix.SetRow(3, testData.v1);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, SetColumnX3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 testMatrix = testData.m2;
                testMatrix.SetColumn(0, testData.v1);
                testMatrix.SetColumn(1, testData.v1);
                testMatrix.SetColumn(2, testData.v1);
                testMatrix.SetColumn(3, testData.v1);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, GetBasisX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector4 result = testData.m1.GetBasisX();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, GetBasisY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector4 result = testData.m1.GetBasisY();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, GetBasisZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector4 result = testData.m1.GetBasisZ();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, CreateTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = AZ::Matrix4x4::CreateTranslation(testData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, SetTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 testMatrix = testData.m1;
                testMatrix.SetTranslation(testData.v2);
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, GetTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.GetTranslation();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, OperatorAssign)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = testData.m1;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, OperatorMultiply)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = testData.m1 * testData.m2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, OperatorMultiplyAssign)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 testMatrix = testData.m2;
                testMatrix *= testData.m1;
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, OperatorMultiplyVector3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1 * testData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, OperatorMultiplyVector4)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector4 result = testData.m1 * testData.v1;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, TransposedMultiply3x3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.TransposedMultiply3x3(testData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, Multiply3x3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.m1.Multiply3x3(testData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, Transpose)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 testMatrix = testData.m1;
                testMatrix.Transpose();
                benchmark::DoNotOptimize(testMatrix);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, GetTranspose)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = testData.m1.GetTranspose();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, GetInverseFast)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = testData.m1.GetInverseFast();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, GetInverseTransform)(benchmark::State& state)
    {
        // use a nonsingular matrix that has (0,0,0,1) for its final row to avoid asserts
        AZ::Matrix4x4 mat = AZ::Matrix4x4::CreateRotationX(1.0f);
        mat.SetElement(0, 1, 23.1234f);

        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = mat.GetInverseTransform();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, GetInverseFull)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Matrix4x4 result = testData.m1.GetInverseFull();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathMatrix4x4, SetRotationPartFromQuaternion)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                testData.m2.SetRotationPartFromQuaternion(testData.q1);
            }
        }
    }
}

#endif
