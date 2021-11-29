/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector3.h>
#include <AzCore/UnitTest/TestTypes.h>

#if defined(HAVE_BENCHMARK)

#include <random>
#include <benchmark/benchmark.h>

namespace Benchmark
{
    class BM_MathVector3
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            m_vecDataArray.resize(1000);

            const unsigned int seed = 1;
            std::mt19937_64 rng(seed);
            std::uniform_real_distribution<float> unif;

            std::generate(m_vecDataArray.begin(), m_vecDataArray.end(), [&unif, &rng]()
            {
                VecData vecData;
                vecData.v1 = AZ::Vector3(unif(rng), unif(rng), unif(rng));
                vecData.v2 = AZ::Vector3(unif(rng), unif(rng), unif(rng));
                vecData.v3 = AZ::Vector3(unif(rng), unif(rng), unif(rng));
                return vecData;
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

        struct VecData
        {
            AZ::Vector3 v1;
            AZ::Vector3 v2;
            AZ::Vector3 v3;
        };

        std::vector<VecData> m_vecDataArray;
    };

    BENCHMARK_F(BM_MathVector3, GetSet)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            AZ::Vector3 v1, v2, v3;
            float x = 0.0f, y = 0.0f, z = 0.0f;

            for (auto& vecData : m_vecDataArray)
            {
                x += vecData.v1.GetX();
                y += vecData.v2.GetY();
                z += vecData.v3.GetZ();

                x += vecData.v2.GetX();
                y += vecData.v3.GetY();
                z += vecData.v1.GetZ();

                x += vecData.v3.GetX();
                y += vecData.v1.GetY();
                z += vecData.v2.GetZ();

                v1.SetX(x);
                v2.SetX(x);
                v3.SetX(x);

                v1.SetY(y);
                v2.SetY(y);
                v3.SetY(y);

                v1.SetZ(z);
                v2.SetZ(z);
                v3.SetZ(z);
            }

            benchmark::DoNotOptimize(v1);
            benchmark::DoNotOptimize(v2);
            benchmark::DoNotOptimize(v3);
        }
    }

    BENCHMARK_F(BM_MathVector3, ElementAccess)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            AZ::Vector3 v1, v2, v3;
            float x = 0.0f, y = 0.0f, z = 0.0f;

            for (auto& vecData : m_vecDataArray)
            {
                x += vecData.v1.GetElement(0);
                y += vecData.v2.GetElement(1);
                z += vecData.v3.GetElement(2);

                x += vecData.v2.GetElement(0);
                y += vecData.v3.GetElement(1);
                z += vecData.v1.GetElement(2);

                x += vecData.v3.GetElement(0);
                y += vecData.v1.GetElement(1);
                z += vecData.v2.GetElement(2);

                v1.SetElement(0, x);
                v2.SetElement(0, x);
                v3.SetElement(0, x);

                v1.SetElement(1, y);
                v2.SetElement(1, y);
                v3.SetElement(1, y);

                v1.SetElement(2, z);
                v2.SetElement(2, z);
                v3.SetElement(2, z);
            }

            benchmark::DoNotOptimize(v1);
            benchmark::DoNotOptimize(v2);
            benchmark::DoNotOptimize(v3);
        }
    }

    BENCHMARK_F(BM_MathVector3, CreateSelectCmpEqual)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = AZ::Vector3::CreateSelectCmpEqual(vecData.v1, vecData.v2, vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, CreateSelectCmpGreaterEqual)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = AZ::Vector3::CreateSelectCmpGreaterEqual(vecData.v1, vecData.v2, vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, CreateSelectCmpGreater)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = AZ::Vector3::CreateSelectCmpGreater(vecData.v1, vecData.v2, vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetNormalized)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetNormalized();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetNormalizedEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetNormalizedEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, NormalizeWithLength)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.NormalizeWithLength();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, NormalizeWithLengthEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.NormalizeWithLengthEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetNormalizedSafe)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetNormalizedSafe();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetNormalizedSafeEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetNormalizedSafeEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetDistance)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v2.GetDistance(vecData.v1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetDistanceEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v2.GetDistanceEstimate(vecData.v1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Lerp)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v2.Lerp(vecData.v1, 0.0f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Lerp(vecData.v1, 0.25f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Lerp(vecData.v1, 0.5f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Lerp(vecData.v1, 0.75f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Lerp(vecData.v1, 1.0f);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Slerp)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v2.Slerp(vecData.v1, 0.0f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Slerp(vecData.v1, 0.25f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Slerp(vecData.v1, 0.5f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Slerp(vecData.v1, 0.75f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Slerp(vecData.v1, 1.0f);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Nlerp)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v2.Nlerp(vecData.v1, 0.0f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Nlerp(vecData.v1, 0.25f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Nlerp(vecData.v1, 0.5f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Nlerp(vecData.v1, 0.75f);
                benchmark::DoNotOptimize(result);

                result = vecData.v2.Nlerp(vecData.v1, 1.0f);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Dot)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.Dot(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Cross)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.Cross(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Equality)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1 == vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Inequality)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1 != vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, IsLessThan)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1.IsLessThan(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, IsLessEqualThan)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1.IsLessEqualThan(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, IsGreaterThan)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1.IsGreaterThan(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, IsGreaterEqualThan)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1.IsGreaterEqualThan(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetMin)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetMin(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetMax)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetMax(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetClamp)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetClamp(vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Sub)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1 - vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Sum)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1 + vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Mul)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1 * vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Div)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1 / vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetSin)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetSin();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetCos)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetCos();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetSinCos)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 sin, cos;
                vecData.v1.GetSinCos(sin, cos);
                benchmark::DoNotOptimize(sin);
                benchmark::DoNotOptimize(cos);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetAcos)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetAcos();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetAtan)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetAtan();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetAngleMod)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetAngleMod();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, Angle)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.Angle(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, AngleDeg)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.AngleDeg(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetAbs)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetAbs();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetReciprocal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetReciprocal();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetReciprocalEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetReciprocalEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, IsPerpendicular)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1.IsPerpendicular(vecData.v2, 0.01f);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetOrthogonalVector)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetOrthogonalVector();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector3, GetProjected)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetProjected(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }
}

#endif
