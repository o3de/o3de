/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector4.h>
#include <AzCore/UnitTest/TestTypes.h>

#if defined(HAVE_BENCHMARK)

#include <random>
#include <benchmark/benchmark.h>

namespace Benchmark
{
    class BM_MathVector4
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
                vecData.v1 = AZ::Vector4(unif(rng), unif(rng), unif(rng), unif(rng));
                vecData.v2 = AZ::Vector4(unif(rng), unif(rng), unif(rng), unif(rng));
                vecData.v3 = AZ::Vector4(unif(rng), unif(rng), unif(rng), unif(rng));
                vecData.v4 = AZ::Vector4(unif(rng), unif(rng), unif(rng), unif(rng));
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
            AZ::Vector4 v1;
            AZ::Vector4 v2;
            AZ::Vector4 v3;
            AZ::Vector4 v4;
        };

        std::vector<VecData> m_vecDataArray;
    };

    BENCHMARK_F(BM_MathVector4, GetSet)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            AZ::Vector4 v1, v2, v3, v4;
            float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

            for (auto& vecData : m_vecDataArray)
            {
                x += vecData.v1.GetX();
                y += vecData.v2.GetY();
                z += vecData.v3.GetZ();
                w += vecData.v4.GetW();

                x += vecData.v2.GetX();
                y += vecData.v3.GetY();
                z += vecData.v4.GetZ();
                w += vecData.v1.GetW();

                x += vecData.v3.GetX();
                y += vecData.v4.GetY();
                z += vecData.v1.GetZ();
                w += vecData.v2.GetW();

                x += vecData.v4.GetX();
                y += vecData.v1.GetY();
                z += vecData.v2.GetZ();
                w += vecData.v3.GetW();

                v1.SetX(x);
                v2.SetX(x);
                v3.SetX(x);
                v4.SetX(x);

                v1.SetY(y);
                v2.SetY(y);
                v3.SetY(y);
                v4.SetY(y);

                v1.SetZ(z);
                v2.SetZ(z);
                v3.SetZ(z);
                v4.SetZ(z);

                v1.SetW(w);
                v2.SetW(w);
                v3.SetW(w);
                v4.SetW(w);
            }

            benchmark::DoNotOptimize(v1);
            benchmark::DoNotOptimize(v2);
            benchmark::DoNotOptimize(v3);
            benchmark::DoNotOptimize(v4);
        }
    }

    BENCHMARK_F(BM_MathVector4, ElementAccess)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            AZ::Vector4 v1, v2, v3, v4;
            float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

            for (auto& vecData : m_vecDataArray)
            {
                x += vecData.v1.GetElement(0);
                y += vecData.v2.GetElement(1);
                z += vecData.v3.GetElement(2);
                w += vecData.v4.GetElement(3);

                x += vecData.v2.GetElement(0);
                y += vecData.v3.GetElement(1);
                z += vecData.v4.GetElement(2);
                w += vecData.v1.GetElement(3);

                x += vecData.v3.GetElement(0);
                y += vecData.v4.GetElement(1);
                z += vecData.v1.GetElement(2);
                w += vecData.v2.GetElement(3);

                x += vecData.v4.GetElement(0);
                y += vecData.v1.GetElement(1);
                z += vecData.v2.GetElement(2);
                w += vecData.v3.GetElement(3);

                v1.SetElement(0, x);
                v2.SetElement(0, x);
                v3.SetElement(0, x);
                v4.SetElement(0, x);

                v1.SetElement(1, y);
                v2.SetElement(1, y);
                v3.SetElement(1, y);
                v4.SetElement(1, y);

                v1.SetElement(2, z);
                v2.SetElement(2, z);
                v3.SetElement(2, z);
                v4.SetElement(2, z);

                v1.SetElement(3, w);
                v2.SetElement(3, w);
                v3.SetElement(3, w);
                v4.SetElement(3, w);
            }

            benchmark::DoNotOptimize(v1);
            benchmark::DoNotOptimize(v2);
            benchmark::DoNotOptimize(v3);
            benchmark::DoNotOptimize(v4);
        }
    }

    BENCHMARK_F(BM_MathVector4, CreateSelectCmpEqual)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = AZ::Vector4::CreateSelectCmpEqual(vecData.v1, vecData.v2, vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, CreateSelectCmpGreaterEqual)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = AZ::Vector4::CreateSelectCmpGreaterEqual(vecData.v1, vecData.v2, vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, CreateSelectCmpGreater)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = AZ::Vector4::CreateSelectCmpGreater(vecData.v1, vecData.v2, vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetNormalized)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetNormalized();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetNormalizedEstimate)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetNormalizedEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, NormalizeWithLength)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.NormalizeWithLength();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, NormalizeWithLengthEstimate)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.NormalizeWithLengthEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetNormalizedSafe)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetNormalizedSafe();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetNormalizedSafeEstimate)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetNormalizedSafeEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetDistance)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v2.GetDistance(vecData.v1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetDistanceEstimate)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v2.GetDistanceEstimate(vecData.v1);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, Lerp)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v2.Lerp(vecData.v1, 0.0f);
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

    BENCHMARK_F(BM_MathVector4, Slerp)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v2.Slerp(vecData.v1, 0.0f);
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

    BENCHMARK_F(BM_MathVector4, Nlerp)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v2.Nlerp(vecData.v1, 0.0f);
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

    BENCHMARK_F(BM_MathVector4, Dot)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.Dot(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, Dot3)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.Dot3(vecData.v2.GetAsVector3());
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetHomogenized)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector3 result = vecData.v1.GetHomogenized();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, Equality)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1 == vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, Inequality)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1 != vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, IsLessThan)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1.IsLessThan(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, IsLessEqualThan)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1.IsLessEqualThan(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, IsGreaterThan)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1.IsGreaterThan(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, IsGreaterEqualThan)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                bool result = vecData.v1.IsGreaterEqualThan(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetMin)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetMin(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetMax)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetMax(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetClamp)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetClamp(vecData.v2, vecData.v3);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, Sub)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1 - vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, Sum)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1 + vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, Mul)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1 * vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, Div)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1 / vecData.v2;
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetSin)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetSin();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetCos)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetCos();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetSinCos)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 sin, cos;
                vecData.v1.GetSinCos(sin, cos);
                benchmark::DoNotOptimize(sin);
                benchmark::DoNotOptimize(cos);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetAcos)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetAcos();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetAtan)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetAtan();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetExpEstimate)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetExpEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, ScalarExpBaseline)(benchmark::State& state)
    {
        // This is just a comparison benchmark, so we can easily validate that our ExpEstimate is a performance improvement over simply calling exp()
        // If ScalarExpBaseline is ever the same speed or faster than GetExpEstimate then we have an issue with our estimate function
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float r0 = exp(vecData.v1.GetX());
                float r1 = exp(vecData.v1.GetY());
                float r2 = exp(vecData.v1.GetZ());
                float r3 = exp(vecData.v1.GetW());
                benchmark::DoNotOptimize(r0);
                benchmark::DoNotOptimize(r1);
                benchmark::DoNotOptimize(r2);
                benchmark::DoNotOptimize(r3);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetAngleMod)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetAngleMod();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, Angle)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.Angle(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, AngleDeg)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                float result = vecData.v1.AngleDeg(vecData.v2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetAbs)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetAbs();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetReciprocal)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetReciprocal();
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathVector4, GetReciprocalEstimate)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& vecData : m_vecDataArray)
            {
                AZ::Vector4 result = vecData.v1.GetReciprocalEstimate();
                benchmark::DoNotOptimize(result);
            }
        }
    }
}

#endif
