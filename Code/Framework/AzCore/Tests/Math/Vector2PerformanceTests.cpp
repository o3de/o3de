/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>
#include <AzCore/UnitTest/TestTypes.h>

#if defined(HAVE_BENCHMARK)

#include <random>
#include <benchmark/benchmark.h>

namespace Benchmark
{
    class BM_MathVector2
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
                vecData.v1 = AZ::Vector2(unif(rng), unif(rng));
                vecData.v2 = AZ::Vector2(unif(rng), unif(rng));
                vecData.v3 = AZ::Vector2(unif(rng), unif(rng));
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
            AZ::Vector2 v1;
            AZ::Vector2 v2;
            AZ::Vector2 v3;
        };

        std::vector<VecData> m_vecDataArray;
    };

    BENCHMARK_F(BM_MathVector2, GetSet)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            AZ::Vector2 v1, v2;
            float x = 0.0f, y = 0.0f;
            for (auto& vecData : m_vecDataArray)
            {
                x += vecData.v1.GetX();
                y += vecData.v2.GetY();

                x += vecData.v2.GetX();
                y += vecData.v1.GetY();

                v1.SetX(x);
                v2.SetX(x);

                v1.SetY(y);
                v2.SetY(y);
            }

            benchmark::DoNotOptimize(v1);
            benchmark::DoNotOptimize(v2);
        }
    }

    BENCHMARK_F(BM_MathVector2, ElementAccess)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            AZ::Vector2 v1, v2;
            float x = 0.0f, y = 0.0f;

            for (auto& vecData : m_vecDataArray)
            {
                x += vecData.v1.GetElement(0);
                y += vecData.v2.GetElement(1);

                x += vecData.v2.GetElement(0);
                y += vecData.v1.GetElement(1);

                v1.SetElement(0, x);
                v2.SetElement(0, x);

                v1.SetElement(1, y);
                v2.SetElement(1, y);
            }

            benchmark::DoNotOptimize(v1);
            benchmark::DoNotOptimize(v2);
        }
    }

    BENCHMARK_F(BM_MathVector2, CreateSelectCmpEqual)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = AZ::Vector2::CreateSelectCmpEqual(vecData.v1, vecData.v2, vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, CreateSelectCmpGreaterEqual)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = AZ::Vector2::CreateSelectCmpGreaterEqual(vecData.v1, vecData.v2, vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, CreateSelectCmpGreater)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = AZ::Vector2::CreateSelectCmpGreater(vecData.v1, vecData.v2, vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetNormalized)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetNormalized();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetNormalizedEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetNormalizedEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, NormalizeWithLength)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, NormalizeWithLengthEstimate)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, GetNormalizedSafe)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetNormalizedSafe();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetNormalizedSafeEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetNormalizedSafeEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetDistance)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, GetDistanceEstimate)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, Lerp)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v2.Lerp(vecData.v1, 0.0f);
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

    BENCHMARK_F(BM_MathVector2, Slerp)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v2.Slerp(vecData.v1, 0.0f);
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

    BENCHMARK_F(BM_MathVector2, Nlerp)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v2.Nlerp(vecData.v1, 0.0f);
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

    BENCHMARK_F(BM_MathVector2, Dot)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, Equality)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, Inequality)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, IsLessThan)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, IsLessEqualThan)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, IsGreaterThan)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, IsGreaterEqualThan)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, GetMin)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetMin(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetMax)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetMax(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetClamp)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetClamp(vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, Sub)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1 - vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, Sum)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1 + vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, Mul)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1 * vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, Div)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1 / vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetSin)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetSin();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetCos)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetCos();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetSinCos)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 sin, cos;
                vecData.v1.GetSinCos(sin, cos);
                benchmark::DoNotOptimize(sin);
                benchmark::DoNotOptimize(cos);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetAcos)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetAcos();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetAtan)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetAtan();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetAtan2)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.GetAtan2();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetAngleMod)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetAngleMod();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, Angle)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, AngleDeg)(benchmark::State& state)
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

    BENCHMARK_F(BM_MathVector2, GetAbs)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetAbs();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetReciprocal)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetReciprocal();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetReciprocalEstimate)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetReciprocalEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector2, GetProjected)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector2 result = vecData.v1.GetProjected(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }
}

#endif
