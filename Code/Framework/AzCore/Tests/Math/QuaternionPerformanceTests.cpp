/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/Math/Quaternion.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <random>

namespace Benchmark
{
    class BM_MathQuaternion
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            m_quatDataArray.resize(1000);

            const unsigned int seed = 1;
            std::mt19937_64 rng(seed);
            std::uniform_real_distribution<float> unif;

            std::generate(m_quatDataArray.begin(), m_quatDataArray.end(), [&unif, &rng]()
            {
                QuatData quatData;
                float angle = unif(rng);
                quatData.q1 = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(unif(rng), unif(rng), unif(rng)).GetNormalized(), angle);
                angle = unif(rng);
                quatData.q2 = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(unif(rng), unif(rng), unif(rng)).GetNormalized(), angle);
                angle = unif(rng);
                quatData.q3 = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(unif(rng), unif(rng), unif(rng)).GetNormalized(), angle);
                quatData.x = unif(rng);
                quatData.y = unif(rng);
                quatData.z = unif(rng);
                quatData.w = unif(rng);
                return quatData;
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

        struct QuatData
        {
            AZ::Quaternion q1;
            AZ::Quaternion q2;
            AZ::Quaternion q3;

            float x;
            float y;
            float z;
            float w;
        };

        std::vector<QuatData> m_quatDataArray;
    };

    BENCHMARK_F(BM_MathQuaternion, SplatFloatConstruction)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result(quatData.x);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, NonNormalized4FloatsConstruction)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result(quatData.x, quatData.y, quatData.z, quatData.w);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, CreateFromIdentity)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = AZ::Quaternion::CreateIdentity();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, CreateZero)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = AZ::Quaternion::CreateZero();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, CreateRotationX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = AZ::Quaternion::CreateRotationX(quatData.x);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, CreateRotationY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = AZ::Quaternion::CreateRotationY(quatData.x);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, CreateRotationZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = AZ::Quaternion::CreateRotationZ(quatData.x);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, CreateShortestArcSameHemisphere)(benchmark::State& state)
    {
        const AZ::Vector3 vec1 = AZ::Vector3(1.0f, 2.0f, 3.0f).GetNormalized();
        const AZ::Vector3 vec2 = AZ::Vector3(-2.0f, 7.0f, -1.0f).GetNormalized();

        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = AZ::Quaternion::CreateShortestArc(vec1, vec2); //result should transform vec1 into vec2
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, CreateShortestArcDifferentHemisphere)(benchmark::State& state)
    {
        const AZ::Vector3 vec1 = AZ::Vector3(1.0f, 2.0f, 3.0f).GetNormalized();
        const AZ::Vector3 vec2 = AZ::Vector3(-1.0f, -2.0f, -3.0f).GetNormalized();

        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = AZ::Quaternion::CreateShortestArc(vec1, vec2); //result should transform vec1 into vec2
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.GetX();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.GetY();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.GetZ();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetW)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.GetW();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, SetX)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion quat;
                quat.SetX(quatData.x);
                benchmark::DoNotOptimize(quat);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, SetY)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion quat;
                quat.SetY(quatData.y);
                benchmark::DoNotOptimize(quat);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, SetZ)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion quat;
                quat.SetZ(quatData.z);
                benchmark::DoNotOptimize(quat);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, SetW)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion quat;
                quat.SetW(quatData.w);
                benchmark::DoNotOptimize(quat);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, SetSplat)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion quat;
                quat.Set(quatData.x);
                benchmark::DoNotOptimize(quat);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, SetAll)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion quat;
                quat.Set(quatData.x, quatData.y, quatData.z, quatData.w);
                benchmark::DoNotOptimize(quat);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, SetVectorAndReal)(benchmark::State& state)
    {
        AZ::Vector3 vec(5.0f, 6.0f, 7.0f);

        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion quat;
                quat.Set(vec, 8.0f);
                benchmark::DoNotOptimize(quat);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, SetArray)(benchmark::State& state)
    {
        const float quatArray[4] = { 5.0f, 6.0f, 7.0f, 8.0f };
        for (auto _ : state)
        {
            for ([[maybe_unused]] auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion quat;
                quat.Set(quatArray);
                benchmark::DoNotOptimize(quat);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, SetElements)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion quat;
                quat.SetElement(0, quatData.x);
                quat.SetElement(1, quatData.y);
                quat.SetElement(2, quatData.z);
                quat.SetElement(3, quatData.w);
                benchmark::DoNotOptimize(quat);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetElement)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.GetElement(0);
                benchmark::DoNotOptimize(result);

                result = quatData.q1.GetElement(1);
                benchmark::DoNotOptimize(result);

                result = quatData.q1.GetElement(2);
                benchmark::DoNotOptimize(result);

                result = quatData.q1.GetElement(3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetConjugate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1.GetConjugate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetInverseFast)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1.GetInverseFast();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetInverseFull)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1.GetInverseFull();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, Dot)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.Dot(quatData.q2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetLength)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.GetLength();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetLengthEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.GetLengthEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetLengthReciprocal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.GetLengthReciprocal();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetLengthReciprocalEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.GetLengthReciprocalEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetNormalized)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1.GetNormalized();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetNormalizedEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1.GetNormalizedEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, NormalizeWithLength)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.NormalizeWithLength();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, NormalizeWithLengthEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                float result = quatData.q1.NormalizeWithLengthEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, Lerp)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1.Lerp(quatData.q2, quatData.x);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, NLerp)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1.NLerp(quatData.q2, quatData.x);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, Slerp)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1.Slerp(quatData.q2, quatData.x);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, OperatorEquality)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                bool result = (quatData.q1 == quatData.q2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, OperatorInequality)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                bool result = (quatData.q1 != quatData.q2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, OperatorNegate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = -quatData.q1;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, OperatorSum)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1 + quatData.q2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, OperatorSub)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1 - quatData.q2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, OperatorMultiply)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1 * quatData.q2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, OperatorMultiplyScalar)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q1 * quatData.x;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, TransformVector)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Vector3 vector(quatData.x, quatData.y, quatData.z);
                AZ::Vector3 result1 = quatData.q1.TransformVector(vector);
                AZ::Vector3 result2 = quatData.q2.TransformVector(vector);
                benchmark::DoNotOptimize(result1);
                benchmark::DoNotOptimize(result2);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, IsClose)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                bool result = quatData.q1.IsClose(quatData.q2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, IsIdentity)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                bool result = quatData.q1.IsIdentity();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetEulerDegrees)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Vector3 result = quatData.q1.GetEulerDegrees();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, GetEulerRadians)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Vector3 result = quatData.q1.GetEulerRadians();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, SetFromEulerRadians)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Vector3 value = AZ::Vector3(quatData.x, quatData.y, quatData.z);
                AZ::Quaternion result;
                result.SetFromEulerRadians(value);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, SetFromEulerDegrees)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Vector3 value = AZ::Vector3(quatData.x, quatData.y, quatData.z);
                AZ::Quaternion result;
                result.SetFromEulerDegrees(value);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, ConvertToAxisAngle)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Vector3 axis;
                float angle;
                quatData.q1.ConvertToAxisAngle(axis, angle);
                benchmark::DoNotOptimize(axis);
                benchmark::DoNotOptimize(angle);
            }
        }
    }

    BENCHMARK_F(BM_MathQuaternion, AggregateMultiply)(::benchmark::State& state)
    {
        for (auto _ : state)
        {
            AZ::Vector3 position = AZ::Vector3::CreateZero();
            for (auto& quatData : m_quatDataArray)
            {
                AZ::Quaternion result = quatData.q3 * quatData.q2 * quatData.q1;
                AZ::Quaternion normalizedResult = result.GetNormalized();

                AZ::Vector3 direction = normalizedResult.TransformVector(AZ::Vector3::CreateAxisX());
                position += direction;
            }
            benchmark::DoNotOptimize(position);
        }
    }
}

#endif
