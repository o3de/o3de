/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/Math/MatrixMxN.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace Benchmark
{
    class BM_MathMatrixMxN
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            m_matrix1 = AZ::MatrixMxN::CreateRandom(500, 1000);
            m_matrix2 = AZ::MatrixMxN::CreateRandom(1000, 500);

            m_vector1 = AZ::VectorN::CreateRandom(1000);
            m_vector2 = AZ::VectorN::CreateRandom(500);
        }
        void internalTearDown()
        {
            m_matrix1 = AZ::MatrixMxN();
            m_matrix2 = AZ::MatrixMxN();
            m_vector1 = AZ::VectorN();
            m_vector2 = AZ::VectorN();
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

        AZ::MatrixMxN m_matrix1;
        AZ::MatrixMxN m_matrix2;
        AZ::VectorN m_vector1;
        AZ::VectorN m_vector2;

        AZ::MatrixMxN m_result1;
        AZ::MatrixMxN m_result2;
        AZ::VectorN m_result3;
        AZ::VectorN m_result4;
    };

    BENCHMARK_F(BM_MathMatrixMxN, VectorMatrixMultiply)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            AZ::VectorMatrixMultiply(m_matrix1, m_vector1, m_result3);
            benchmark::DoNotOptimize(m_result3);
        }
    }

    BENCHMARK_F(BM_MathMatrixMxN, VectorMatrixMultiplyLeft)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            AZ::VectorMatrixMultiplyLeft(m_vector2, m_matrix1, m_result4);
            benchmark::DoNotOptimize(m_result4);
        }
    }

    BENCHMARK_F(BM_MathMatrixMxN, MatrixMatrixMultiply)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            AZ::MatrixMatrixMultiply(m_matrix1, m_matrix2, m_result1);
            benchmark::DoNotOptimize(m_result1);
        }
    }

    BENCHMARK_F(BM_MathMatrixMxN, OuterProduct)(benchmark::State& state)
    {
        for ([[maybe_unused]] auto _ : state)
        {
            AZ::OuterProduct(m_vector1, m_vector2, m_result2);
            benchmark::DoNotOptimize(m_result2);
        }
    }
}

#endif
