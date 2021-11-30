/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Sphere.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <random>

namespace Benchmark
{
    AZ::Plane near1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 1.f, 0.f), AZ::Vector3(0.f, -5.f, 0.f));
    AZ::Plane far1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, -1.f, 0.f), AZ::Vector3(0.f, 5.f, 0.f));
    AZ::Plane left1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(1.f, 0.f, 0.f), AZ::Vector3(-5.f, 0.f, 0.f));
    AZ::Plane right1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(-1.f, 0.f, 0.f), AZ::Vector3(5.f, 0.f, 0.f));
    AZ::Plane top1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, -1.f), AZ::Vector3(0.f, 0.f, 5.f));
    AZ::Plane bottom1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, 1.f), AZ::Vector3(0.f, 0.f, -5.f));
    AZ::Frustum frustum1 = AZ::Frustum(near1, far1, left1, right1, top1, bottom1);

    AZ::Plane near2 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 1.f, 0.f), AZ::Vector3(0.f, -2.f, 0.f));
    AZ::Plane far2 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, -1.f, 0.f), AZ::Vector3(0.f, 2.f, 0.f));
    AZ::Plane left2 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(1.f, 0.f, 0.f), AZ::Vector3(-2.f, 0.f, 0.f));
    AZ::Plane right2 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(-1.f, 0.f, 0.f), AZ::Vector3(2.f, 0.f, 0.f));
    AZ::Plane top2 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, -1.f), AZ::Vector3(0.f, 0.f, 2.f));
    AZ::Plane bottom2 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, 1.f), AZ::Vector3(0.f, 0.f, -2.f));
    AZ::Frustum frustum2 = AZ::Frustum(near2, far2, left2, right2, top2, bottom2);

    class BM_MathShapeIntersection
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            m_testDataArray.resize(1000);

            const unsigned int seed = 1;
            std::mt19937_64 rng(seed);
            std::uniform_real_distribution<float> unif(-100.0f, 100.0f);

            std::generate(m_testDataArray.begin(), m_testDataArray.end(), [&unif, &rng]()
            {
                AZ::Vector3 vector1 = AZ::Vector3(unif(rng), unif(rng), unif(rng));
                AZ::Vector3 vector2 = AZ::Vector3(unif(rng), unif(rng), unif(rng));
                AZ::Vector3 vector3 = AZ::Vector3(unif(rng), unif(rng), unif(rng));

                TestData testData;
                testData.sphere = AZ::Sphere(vector1, unif(rng));
                testData.aabb = AZ::Aabb::CreateCenterHalfExtents(vector2, vector3.GetAbs());
                testData.vector1 = AZ::Vector3(unif(rng), unif(rng), unif(rng));
                testData.vector2 = AZ::Vector3(unif(rng), unif(rng), unif(rng));
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
            AZ::Sphere sphere;
            AZ::Aabb aabb;
            AZ::Vector3 vector1;
            AZ::Vector3 vector2;
        };

        std::vector<TestData> m_testDataArray;
    };

    BENCHMARK_F(BM_MathShapeIntersection, ContainsFrustumPoint)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result;

                result = AZ::ShapeIntersection::Contains(frustum1, testData.vector1);
                benchmark::DoNotOptimize(result);

                result = AZ::ShapeIntersection::Contains(frustum1, testData.vector2);
                benchmark::DoNotOptimize(result);

                result = AZ::ShapeIntersection::Contains(frustum2, testData.vector1);
                benchmark::DoNotOptimize(result);

                result = AZ::ShapeIntersection::Contains(frustum2, testData.vector2);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathShapeIntersection, OverlapsFrustumSphere)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result;

                result = AZ::ShapeIntersection::Overlaps(frustum1, testData.sphere);
                benchmark::DoNotOptimize(result);

                result = AZ::ShapeIntersection::Overlaps(frustum2, testData.sphere);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathShapeIntersection, ContainsFrustumSphere)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result;

                result = AZ::ShapeIntersection::Contains(frustum1, testData.sphere);
                benchmark::DoNotOptimize(result);

                result = AZ::ShapeIntersection::Contains(frustum2, testData.sphere);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathShapeIntersection, OverlapsFrustumAabb)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result;

                result = AZ::ShapeIntersection::Overlaps(frustum1, testData.aabb);
                benchmark::DoNotOptimize(result);

                result = AZ::ShapeIntersection::Overlaps(frustum2, testData.aabb);
                benchmark::DoNotOptimize(result);
            }
        }
    }

    BENCHMARK_F(BM_MathShapeIntersection, ContainsFrustumAabb)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            for (auto& testData : m_testDataArray)
            {
                bool result;

                result = AZ::ShapeIntersection::Contains(frustum1, testData.aabb);
                benchmark::DoNotOptimize(result);

                result = AZ::ShapeIntersection::Contains(frustum2, testData.aabb);
                benchmark::DoNotOptimize(result);
            }
        }
    }
}

#endif
