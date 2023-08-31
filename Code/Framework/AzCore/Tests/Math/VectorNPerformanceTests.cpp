/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/VectorN.h>
#include <AzCore/UnitTest/TestTypes.h>

#if defined(HAVE_BENCHMARK)

#include <random>
#include <benchmark/benchmark.h>

namespace Benchmark
{
    class BM_MathVectorN
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            m_vector1 = AZ::VectorN::CreateRandom(4000);
            m_vector2 = AZ::VectorN::CreateRandom(4000);
            m_vector3 = AZ::VectorN::CreateRandom(4000);
            m_result1 = AZ::VectorN::CreateZero(4000);
        }
        void internalTearDown()
        {
            m_vector1 = AZ::VectorN();
            m_vector2 = AZ::VectorN();
            m_vector3 = AZ::VectorN();
            m_result1 = AZ::VectorN();
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
        void TearDown(const benchmark::State&) override
        {
            internalTearDown();
        }
        void TearDown(benchmark::State&) override
        {
            internalTearDown();
        }

        AZ::VectorN m_vector1;
        AZ::VectorN m_vector2;
        AZ::VectorN m_vector3;
        AZ::VectorN m_result1;
    };

    BENCHMARK_F(BM_MathVectorN, GetNormalized)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            m_result1 = m_vector1.GetNormalized();
            benchmark::DoNotOptimize(m_result1);
        }
    }

    BENCHMARK_F(BM_MathVectorN, Dot)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            float result = m_vector1.Dot(m_vector2);
            benchmark::DoNotOptimize(result);
        }
    }

    BENCHMARK_F(BM_MathVectorN, NaiveDotBaseline)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            float result = 0.0f;
            for (AZStd::size_t i = 0; i < m_vector1.GetVectorValues().size(); ++i)
            {
                result += m_vector1.GetVectorValues()[i].Dot(m_vector2.GetVectorValues()[i]);
            }
            benchmark::DoNotOptimize(result);
        }
    }

    BENCHMARK_F(BM_MathVectorN, GetMin)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            m_result1 = m_vector1.GetMin(m_vector2);
            benchmark::DoNotOptimize(m_result1);
        }
    }

    BENCHMARK_F(BM_MathVectorN, GetMax)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            m_result1 = m_vector1.GetMax(m_vector2);
            benchmark::DoNotOptimize(m_result1);
        }
    }

    BENCHMARK_F(BM_MathVectorN, GetClamp)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            m_result1 = m_vector1.GetClamp(m_vector2, m_vector3);
            benchmark::DoNotOptimize(m_result1);
        }
    }

    BENCHMARK_F(BM_MathVectorN, Sub)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            m_result1 = m_vector1 - m_vector2;
            benchmark::DoNotOptimize(m_result1);
        }
    }

    BENCHMARK_F(BM_MathVectorN, Sum)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            m_result1 = m_vector1 + m_vector2;
            benchmark::DoNotOptimize(m_result1);
        }
    }

    BENCHMARK_F(BM_MathVectorN, Mul)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            m_result1 = m_vector1 * m_vector2;
            benchmark::DoNotOptimize(m_result1);
        }
    }

    BENCHMARK_F(BM_MathVectorN, Div)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            m_result1 = m_vector1 / m_vector2;
            benchmark::DoNotOptimize(m_result1);
        }
    }

    BENCHMARK_F(BM_MathVectorN, GetAbs)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            m_result1 = m_vector1.GetAbs();
            benchmark::DoNotOptimize(m_result1);
        }
    }
}

#endif
