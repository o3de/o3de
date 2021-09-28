/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Frustum.h>
#include <AzCore/UnitTest/TestTypes.h>

#if defined(HAVE_BENCHMARK)

#include <random>
#include <benchmark/benchmark.h>

namespace Benchmark
{
    class BM_MathFrustum
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            m_testFrustum = AZ::Frustum(AZ::ViewFrustumAttributes(AZ::Transform::CreateIdentity(), 1.0f, 2.0f * atanf(0.5f), 10.0f, 90.0f));

            m_dataArray.resize(1000);

            const unsigned int seed = 1;
            std::mt19937_64 rng(seed);
            std::uniform_real_distribution<float> unif;

            std::generate(m_dataArray.begin(), m_dataArray.end(), [&unif, &rng]()
            {
                Data data;
                data.sphereCenter = AZ::Vector3(unif(rng), unif(rng), unif(rng)) * 100.0f;
                data.sphereRadius = unif(rng) * 10.0f;
                data.aabbMin = AZ::Vector3(unif(rng), unif(rng), unif(rng)) * 100.0f;
                data.aabbMax = AZ::Vector3(unif(rng), unif(rng), unif(rng)).GetAbs() * 10.0f + data.aabbMin;
                return data;
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

        struct Data
        {
            AZ::Vector3 sphereCenter;
            float sphereRadius;
            AZ::Vector3 aabbMin;
            AZ::Vector3 aabbMax;
        };

        std::vector<Data> m_dataArray;
        AZ::Frustum m_testFrustum;
    };

    BENCHMARK_F(BM_MathFrustum, SphereIntersect)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& data : m_dataArray)
            {
                AZ::IntersectResult result = m_testFrustum.IntersectSphere(data.sphereCenter, data.sphereRadius);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathFrustum, AabbIntersect)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& data : m_dataArray)
            {
                AZ::IntersectResult result = m_testFrustum.IntersectAabb(data.aabbMin, data.aabbMax);
                benchmark::DoNotOptimize(result);
            }
        }
    }
}

#endif
