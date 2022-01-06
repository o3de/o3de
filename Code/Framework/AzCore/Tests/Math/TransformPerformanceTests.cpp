/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <random>
#include <benchmark/benchmark.h>

namespace Benchmark
{
    class BM_MathTransform
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            m_testDataArray.resize(1000);

            const unsigned int seed = 1;
            std::mt19937_64 rng(seed);
            std::uniform_real_distribution<float> distFloat;
            std::uniform_int_distribution<AZ::u32> distInt(0, std::numeric_limits<AZ::u32>::max());

            std::generate(m_testDataArray.begin(), m_testDataArray.end(), [&distFloat, &distInt, &rng]()
            {
                TestData testData;
                const AZ::Quaternion q1 = AZ::Quaternion(distFloat(rng), distFloat(rng), distFloat(rng), distFloat(rng)).GetNormalized();
                testData.t1 = AZ::Transform::CreateFromQuaternionAndTranslation(q1, AZ::Vector3(distFloat(rng), distFloat(rng), distFloat(rng)));
                const AZ::Quaternion q2 = AZ::Quaternion(distFloat(rng), distFloat(rng), distFloat(rng), distFloat(rng)).GetNormalized();
                testData.t2 = AZ::Transform::CreateFromQuaternionAndTranslation(q2, AZ::Vector3(distFloat(rng), distFloat(rng), distFloat(rng)));
                testData.m3x3 = AZ::Matrix3x3::CreateFromQuaternion(q1);
                testData.m3x4 = AZ::Matrix3x4::CreateFromQuaternionAndTranslation(q1, AZ::Vector3(distFloat(rng), distFloat(rng), distFloat(rng)));
                testData.q = q1;
                testData.v3 = AZ::Vector3(distFloat(rng), distFloat(rng), distFloat(rng));
                testData.v4 = AZ::Vector4(distFloat(rng), distFloat(rng), distFloat(rng), distFloat(rng));
                testData.value[0] = distFloat(rng);
                testData.value[1] = distFloat(rng);
                testData.value[2] = distFloat(rng);
                testData.index = distInt(rng) % 3;
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
            AZ::Transform t1;
            AZ::Transform t2;
            AZ::Matrix3x3 m3x3;
            AZ::Matrix3x4 m3x4;
            AZ::Quaternion q;
            AZ::Vector3 v3;
            AZ::Vector4 v4;
            float value[3];
            AZ::u32 index;
        };

        std::vector<TestData> m_testDataArray;
    };

    BENCHMARK_F(BM_MathTransform, CreateIdentity)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& testData : m_testDataArray)
            {
                AZ::Transform result = AZ::Transform::CreateIdentity();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, CreateRotationX)(benchmark::State& state)
    {
        for (auto _ : state)
        {   
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = AZ::Transform::CreateRotationX(testData.value[0]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, CreateRotationY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = AZ::Transform::CreateRotationY(testData.value[0]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, CreateRotationZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = AZ::Transform::CreateRotationZ(testData.value[0]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, CreateFromQuaternion)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = AZ::Transform::CreateFromQuaternion(testData.q);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, CreateFromQuaternionAndTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = AZ::Transform::CreateFromQuaternionAndTranslation(testData.q, testData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, CreateFromMatrix3x3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = AZ::Transform::CreateFromMatrix3x3(testData.m3x3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, CreateFromMatrix3x3AndTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = AZ::Transform::CreateFromMatrix3x3AndTranslation(testData.m3x3, testData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, CreateFromMatrix3x4)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = AZ::Transform::CreateFromMatrix3x4(testData.m3x4);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, CreateUniformScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = AZ::Transform::CreateUniformScale(testData.value[0]);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, CreateFromTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = AZ::Transform::CreateTranslation(testData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, CreateLookAt)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 vector = AZ::Vector3(testData.value[0], testData.value[1], testData.value[2]);
                AZ::Transform result = AZ::Transform::CreateLookAt(vector, -vector);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetBasis)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.t1.GetBasis(testData.index);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetBasisX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.t1.GetBasisX();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetBasisY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.t1.GetBasisY();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetBasisZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.t1.GetBasisZ();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetBasisAndTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 basis[4];
                testData.t1.GetBasisAndTranslation(&basis[0], &basis[1], &basis[2], &basis[3]);
                benchmark::DoNotOptimize(basis[0]);
                benchmark::DoNotOptimize(basis[1]);
                benchmark::DoNotOptimize(basis[2]);
                benchmark::DoNotOptimize(basis[3]);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetTranslation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.t1.GetTranslation();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, SetTranslationWithFloats)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform testTransform = testData.t2;
                testTransform.SetTranslation(testData.value[0], testData.value[1], testData.value[2]);
                benchmark::DoNotOptimize(testTransform);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, SetTranslationWithVector3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform testTransform = testData.t2;
                testTransform.SetTranslation(testData.v3);
                benchmark::DoNotOptimize(testTransform);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetRotation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                const AZ::Quaternion& result = testData.t1.GetRotation();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, SetRotation)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform testTransform = testData.t1;
                testTransform.SetRotation(testData.q);
                benchmark::DoNotOptimize(testTransform);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetUniformScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                float result = testData.t1.GetUniformScale();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, SetUniformScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform testTransform = testData.t2;
                testTransform.SetUniformScale(testData.value[0]);
                benchmark::DoNotOptimize(testTransform);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, ExtractUniformScale)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform testTransform = testData.t2;
                float result = testTransform.ExtractUniformScale();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, OperatorMultiplyTransform)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = testData.t1 * testData.t2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, OperatorMultiplyEqualsTransform)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform testTransform = testData.t2;
                testTransform *= testData.t1;
                benchmark::DoNotOptimize(testTransform);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, TransformPointVector3)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.t1.TransformPoint(testData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, TransformPointVector4)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector4 result = testData.t1.TransformPoint(testData.v4);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, TransformVector)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.t1.TransformVector(testData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetInverse)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = testData.t1.GetInverse();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, Invert)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform testTransform = testData.t2;
                testTransform.Invert();
                benchmark::DoNotOptimize(testTransform);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, IsOrthogonal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result = testData.t1.IsOrthogonal();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetOrthogonalized)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform result = testData.t1.GetOrthogonalized();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, Orthogonalize)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform testTransform = testData.t1;
                testTransform.Orthogonalize();
                benchmark::DoNotOptimize(testTransform);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, IsCloseExactAndDifferent)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result = testData.t1.IsClose(testData.t1);
                benchmark::DoNotOptimize(result);

                result = testData.t2.IsClose(testData.t1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, OperatorEqualEqual)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result = (testData.t1 == testData.t2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, OperatorNotEqual)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result = (testData.t1 != testData.t2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetEulerDegrees)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.t1.GetEulerDegrees();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, GetEulerRadians)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Vector3 result = testData.t1.GetEulerRadians();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, SetFromEulerDegrees)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform testTransform = testData.t1;
                testTransform.SetFromEulerDegrees(testData.v3);
                benchmark::DoNotOptimize(testTransform);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, SetFromEulerRadians)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                AZ::Transform testTransform = testData.t1;
                testTransform.SetFromEulerRadians(testData.v3);
                benchmark::DoNotOptimize(testTransform);
            }
        }
    }

    BENCHMARK_F(BM_MathTransform, IsFinite)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result = testData.t1.IsFinite();
                benchmark::DoNotOptimize(result);
            }
        }
    }
}

#endif
