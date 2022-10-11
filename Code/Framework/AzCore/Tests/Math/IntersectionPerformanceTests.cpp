/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Geometry3DUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <random>
#include <Tests/Math/IntersectionTestHelpers.h>

namespace Benchmark
{
    class BM_IntersectSegmentTriangleRandom
        : public benchmark::Fixture
    {
    public:
        struct TestRay
        {
            AZ::Vector3 rayStart;
            AZ::Vector3 rayEnd;
        };

        struct TestTriangle
        {
            AZ::Vector3 triVerts[3];
        };

        TestRay GetRandomTestRay()
        {
            TestRay testRay;
            testRay.rayStart = AZ::Vector3(m_distribution(m_rng), m_distribution(m_rng), m_distribution(m_rng));
            testRay.rayEnd = AZ::Vector3(m_distribution(m_rng), m_distribution(m_rng), m_distribution(m_rng));
            return testRay;
        }

        AZStd::vector<TestTriangle> GetRandomTestTriangles(int64_t triangleCount)
        {
            AZStd::vector<TestTriangle> triangles;
            triangles.resize(triangleCount);

            std::generate(
                triangles.begin(), triangles.end(),
                [this]()
                {
                    TestTriangle testTriangle;
                    testTriangle.triVerts[0] = AZ::Vector3(m_distribution(m_rng), m_distribution(m_rng), m_distribution(m_rng));
                    testTriangle.triVerts[1] = AZ::Vector3(m_distribution(m_rng), m_distribution(m_rng), m_distribution(m_rng));
                    testTriangle.triVerts[2] = AZ::Vector3(m_distribution(m_rng), m_distribution(m_rng), m_distribution(m_rng));
                    return testTriangle;
                });

            return triangles;
        }

        // Output variables, defined here for convenience so that we don't have to define them in each benchmark.
        bool m_result;
        AZ::Vector3 m_outNormal;
        float m_outDistance;

        // Random number generator and distributions
        static constexpr inline unsigned int RandomSeed = 1;
        std::mt19937_64 m_rng{ RandomSeed };
        std::uniform_real_distribution<float> m_distribution{ -10.0f, 10.0f };
    };

    // Note that for the following benchmarks, the intersection implementations that live in IntersectionTestHelpers will have an
    // unfair timing advantage since the compiler/linker optimizers are able to do more to optimize them in this context than they
    // are able to do for AZ::Intersect::IntersectSegment*. Moving the alternate implementations into the AZCore library causes them
    // to slow down considerably in the benchmarks.

    BENCHMARK_DEFINE_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangleCCW)(benchmark::State& state)
    {
        AZStd::vector<TestTriangle> testTriangles = GetRandomTestTriangles(state.range(0));

        for ([[maybe_unused]] auto _ : state)
        {
            TestRay testRay = GetRandomTestRay();

            for (auto& testTriangle : testTriangles)
            {
                m_result = AZ::Intersect::IntersectSegmentTriangleCCW(
                    testRay.rayStart, testRay.rayEnd, testTriangle.triVerts[0], testTriangle.triVerts[1], testTriangle.triVerts[2],
                    m_outNormal, m_outDistance);
            }
        }
    }


    BENCHMARK_DEFINE_F(BM_IntersectSegmentTriangleRandom, ArenbergIntersectSegmentTriangleCCW)(benchmark::State& state)
    {
        AZStd::vector<TestTriangle> testTriangles = GetRandomTestTriangles(state.range(0));

        for ([[maybe_unused]] auto _ : state)
        {
            TestRay testRay = GetRandomTestRay();

            for (auto& testTriangle : testTriangles)
            {
                m_result = UnitTest::IntersectTest::ArenbergIntersectSegmentTriangleCCW(
                    testRay.rayStart, testRay.rayEnd, testTriangle.triVerts[0], testTriangle.triVerts[1], testTriangle.triVerts[2],
                    m_outNormal, m_outDistance);
            }
        }
    }

    BENCHMARK_DEFINE_F(BM_IntersectSegmentTriangleRandom, MollerTrumboreIntersectSegmentTriangleCCW)(benchmark::State& state)
    {
        AZStd::vector<TestTriangle> testTriangles = GetRandomTestTriangles(state.range(0));

        for ([[maybe_unused]] auto _ : state)
        {
            TestRay testRay = GetRandomTestRay();

            for (auto& testTriangle : testTriangles)
            {
                m_result = UnitTest::IntersectTest::MollerTrumboreIntersectSegmentTriangleCCW(
                    testRay.rayStart, testRay.rayEnd, testTriangle.triVerts[0], testTriangle.triVerts[1], testTriangle.triVerts[2],
                    m_outNormal, m_outDistance);
            }
        }
    }

    BENCHMARK_DEFINE_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangleCCWHitTester)(benchmark::State& state)
    {
        AZStd::vector<TestTriangle> testTriangles = GetRandomTestTriangles(state.range(0));

        for ([[maybe_unused]] auto _ : state)
        {
            TestRay testRay = GetRandomTestRay();
            AZ::Intersect::SegmentTriangleHitTester hitTester(testRay.rayStart, testRay.rayEnd);

            for (auto& testTriangle : testTriangles)
            {
                m_result = hitTester.IntersectSegmentTriangleCCW(
                    testTriangle.triVerts[0], testTriangle.triVerts[1], testTriangle.triVerts[2], m_outNormal, m_outDistance);
            }
        }
    }


    BENCHMARK_DEFINE_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangle)(benchmark::State& state)
    {
        AZStd::vector<TestTriangle> testTriangles = GetRandomTestTriangles(state.range(0));

        for ([[maybe_unused]] auto _ : state)
        {
            TestRay testRay = GetRandomTestRay();

            for (auto& testTriangle : testTriangles)
            {
                m_result = AZ::Intersect::IntersectSegmentTriangle(
                    testRay.rayStart, testRay.rayEnd, testTriangle.triVerts[0], testTriangle.triVerts[1], testTriangle.triVerts[2],
                    m_outNormal, m_outDistance);
            }
        }
    }

    BENCHMARK_DEFINE_F(BM_IntersectSegmentTriangleRandom, ArenbergIntersectSegmentTriangle)(benchmark::State& state)
    {
        AZStd::vector<TestTriangle> testTriangles = GetRandomTestTriangles(state.range(0));

        for ([[maybe_unused]] auto _ : state)
        {
            TestRay testRay = GetRandomTestRay();

            for (auto& testTriangle : testTriangles)
            {
                m_result = UnitTest::IntersectTest::ArenbergIntersectSegmentTriangle(
                    testRay.rayStart, testRay.rayEnd, testTriangle.triVerts[0], testTriangle.triVerts[1], testTriangle.triVerts[2],
                    m_outNormal, m_outDistance);
            }
        }
    }

    BENCHMARK_DEFINE_F(BM_IntersectSegmentTriangleRandom, MollerTrumboreIntersectSegmentTriangle)(benchmark::State& state)
    {
        AZStd::vector<TestTriangle> testTriangles = GetRandomTestTriangles(state.range(0));

        for ([[maybe_unused]] auto _ : state)
        {
            TestRay testRay = GetRandomTestRay();

            for (auto& testTriangle : testTriangles)
            {
                m_result = UnitTest::IntersectTest::MollerTrumboreIntersectSegmentTriangle(
                    testRay.rayStart, testRay.rayEnd, testTriangle.triVerts[0], testTriangle.triVerts[1], testTriangle.triVerts[2],
                    m_outNormal, m_outDistance);
            }
        }
    }

    BENCHMARK_DEFINE_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangleHitTester)(benchmark::State& state)
    {
        AZStd::vector<TestTriangle> testTriangles = GetRandomTestTriangles(state.range(0));

        for ([[maybe_unused]] auto _ : state)
        {
            TestRay testRay = GetRandomTestRay();
            AZ::Intersect::SegmentTriangleHitTester hitTester(testRay.rayStart, testRay.rayEnd);

            for (auto& testTriangle : testTriangles)
            {
                m_result = hitTester.IntersectSegmentTriangle(
                    testTriangle.triVerts[0], testTriangle.triVerts[1], testTriangle.triVerts[2], m_outNormal, m_outDistance);
            }
        }
    }

    // Test each random ray against 1 triangle.
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangleCCW)->Args({ 1 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, ArenbergIntersectSegmentTriangleCCW)->Args({ 1 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, MollerTrumboreIntersectSegmentTriangleCCW)->Args({ 1 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangleCCWHitTester)->Args({ 1 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangle)->Args({ 1 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, ArenbergIntersectSegmentTriangle)->Args({ 1 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, MollerTrumboreIntersectSegmentTriangle)->Args({ 1 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangleHitTester)->Args({ 1 });

    // Test each random ray against 1000 triangles.
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangleCCW)->Args({ 1000 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, ArenbergIntersectSegmentTriangleCCW)->Args({ 1000 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, MollerTrumboreIntersectSegmentTriangleCCW)->Args({ 1000 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangleCCWHitTester)->Args({ 1000 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangle)->Args({ 1000 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, ArenbergIntersectSegmentTriangle)->Args({ 1000 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, MollerTrumboreIntersectSegmentTriangle)->Args({ 1000 });
    BENCHMARK_REGISTER_F(BM_IntersectSegmentTriangleRandom, AzIntersectSegmentTriangleHitTester)->Args({ 1000 });


    class BM_IntersectSegmentTriangleIcoSphere
        : public ::benchmark::Fixture
    {
    public:
        AZ::Vector3 GetRandomTestRayStart()
        {
            // When getting the random number, return numbers in the [-50, -2] or [2, 50] ranges so that we cover both negative and
            // positive positional values, but they're all outside the sphere bounds.
            auto GetRandomPosition = [this]() -> float
            {
                float num = m_distribution(m_rng);
                return (rand() & 0x01) ? num : (num * -1.0f);
            };

            return AZ::Vector3(GetRandomPosition(), GetRandomPosition(), GetRandomPosition());
        }

        // Output variables, defined here for convenience so that we don't have to define them in each benchmark.
        bool m_result;
        AZ::Vector3 m_outNormal;
        float m_outDistance;

        // Random number generator and distributions
        // Create random number generators in the positive 2-50 range so that we're outside the sphere bounds.
        static constexpr inline unsigned int RandomSeed = 1;
        std::mt19937_64 m_rng{ RandomSeed };
        std::uniform_real_distribution<float> m_distribution{ 2.0f, 50.0f };

        // Generate an IcoSphere with ~20k triangles.
        static constexpr inline unsigned int SubdivisionDepth = 5;
    };

    // Note that for the following benchmarks, the intersection implementations that live in IntersectionTestHelpers will have an
    // unfair timing advantage since the compiler/linker optimizers are able to do more to optimize them in this context than they
    // are able to do for AZ::Intersect::IntersectSegment*. Moving the alternate implementations into the AZCore library causes them
    // to slow down considerably in the benchmarks.

    BENCHMARK_F(BM_IntersectSegmentTriangleIcoSphere, AzIntersectSegmentTriangleCCW)(benchmark::State& state)
    {
        AZStd::vector<AZ::Vector3> sphereGeometry = AZ::Geometry3dUtils::GenerateIcoSphere(SubdivisionDepth);

        for ([[maybe_unused]] auto _ : state)
        {
            AZ::Vector3 rayStart = GetRandomTestRayStart();

            for (int triangle = 0; triangle < sphereGeometry.size(); triangle += 3)
            {
                m_result = AZ::Intersect::IntersectSegmentTriangleCCW(
                    rayStart, -rayStart, sphereGeometry[triangle], sphereGeometry[triangle + 1], sphereGeometry[triangle + 2],
                    m_outNormal, m_outDistance);
            }
        }
    }

    BENCHMARK_F(BM_IntersectSegmentTriangleIcoSphere, ArenbergIntersectSegmentTriangleCCW)(benchmark::State& state)
    {
        AZStd::vector<AZ::Vector3> sphereGeometry = AZ::Geometry3dUtils::GenerateIcoSphere(SubdivisionDepth);

        for ([[maybe_unused]] auto _ : state)
        {
            AZ::Vector3 rayStart = GetRandomTestRayStart();

            for (int triangle = 0; triangle < sphereGeometry.size(); triangle += 3)
            {
                m_result = UnitTest::IntersectTest::ArenbergIntersectSegmentTriangleCCW(
                    rayStart, -rayStart, sphereGeometry[triangle], sphereGeometry[triangle + 1], sphereGeometry[triangle + 2],
                    m_outNormal, m_outDistance);
            }
        }
    }

    BENCHMARK_F(BM_IntersectSegmentTriangleIcoSphere, MollerTrumboreIntersectSegmentTriangleCCW)(benchmark::State& state)
    {
        AZStd::vector<AZ::Vector3> sphereGeometry = AZ::Geometry3dUtils::GenerateIcoSphere(SubdivisionDepth);

        for ([[maybe_unused]] auto _ : state)
        {
            AZ::Vector3 rayStart = GetRandomTestRayStart();

            for (int triangle = 0; triangle < sphereGeometry.size(); triangle += 3)
            {
                m_result = UnitTest::IntersectTest::MollerTrumboreIntersectSegmentTriangleCCW(
                    rayStart, -rayStart, sphereGeometry[triangle], sphereGeometry[triangle + 1], sphereGeometry[triangle + 2],
                    m_outNormal, m_outDistance);
            }
        }
    }

    BENCHMARK_F(BM_IntersectSegmentTriangleIcoSphere, AzIntersectSegmentTriangleCCWHitTester)(benchmark::State& state)
    {
        AZStd::vector<AZ::Vector3> sphereGeometry = AZ::Geometry3dUtils::GenerateIcoSphere(SubdivisionDepth);

        for ([[maybe_unused]] auto _ : state)
        {
            AZ::Vector3 rayStart = GetRandomTestRayStart();
            AZ::Intersect::SegmentTriangleHitTester hitTester(rayStart, -rayStart);

            for (int triangle = 0; triangle < sphereGeometry.size(); triangle += 3)
            {
                m_result = hitTester.IntersectSegmentTriangleCCW(
                    sphereGeometry[triangle], sphereGeometry[triangle + 1], sphereGeometry[triangle + 2], m_outNormal, m_outDistance);
            }
        }
    }

    BENCHMARK_F(BM_IntersectSegmentTriangleIcoSphere, AzIntersectSegmentTriangle)(benchmark::State& state)
    {
        AZStd::vector<AZ::Vector3> sphereGeometry = AZ::Geometry3dUtils::GenerateIcoSphere(SubdivisionDepth);

        for ([[maybe_unused]] auto _ : state)
        {
            AZ::Vector3 rayStart = GetRandomTestRayStart();

            for (int triangle = 0; triangle < sphereGeometry.size(); triangle += 3)
            {
                m_result = AZ::Intersect::IntersectSegmentTriangle(
                    rayStart, -rayStart, sphereGeometry[triangle], sphereGeometry[triangle + 1], sphereGeometry[triangle + 2], m_outNormal,
                    m_outDistance);
            }
        }
    }

    BENCHMARK_F(BM_IntersectSegmentTriangleIcoSphere, ArenbergIntersectSegmentTriangle)(benchmark::State& state)
    {
        AZStd::vector<AZ::Vector3> sphereGeometry = AZ::Geometry3dUtils::GenerateIcoSphere(SubdivisionDepth);

        for ([[maybe_unused]] auto _ : state)
        {
            AZ::Vector3 rayStart = GetRandomTestRayStart();

            for (int triangle = 0; triangle < sphereGeometry.size(); triangle += 3)
            {
                m_result = UnitTest::IntersectTest::ArenbergIntersectSegmentTriangle(
                    rayStart, -rayStart, sphereGeometry[triangle], sphereGeometry[triangle + 1], sphereGeometry[triangle + 2], m_outNormal,
                    m_outDistance);
            }
        }
    }

    BENCHMARK_F(BM_IntersectSegmentTriangleIcoSphere, MollerTrumboreIntersectSegmentTriangle)(benchmark::State& state)
    {
        AZStd::vector<AZ::Vector3> sphereGeometry = AZ::Geometry3dUtils::GenerateIcoSphere(SubdivisionDepth);

        for ([[maybe_unused]] auto _ : state)
        {
            AZ::Vector3 rayStart = GetRandomTestRayStart();

            for (int triangle = 0; triangle < sphereGeometry.size(); triangle += 3)
            {
                m_result = UnitTest::IntersectTest::MollerTrumboreIntersectSegmentTriangle(
                    rayStart, -rayStart, sphereGeometry[triangle], sphereGeometry[triangle + 1], sphereGeometry[triangle + 2], m_outNormal,
                    m_outDistance);
            }
        }
    }

    BENCHMARK_F(BM_IntersectSegmentTriangleIcoSphere, AzIntersectSegmentTriangleHitTester)(benchmark::State& state)
    {
        AZStd::vector<AZ::Vector3> sphereGeometry = AZ::Geometry3dUtils::GenerateIcoSphere(SubdivisionDepth);

        for ([[maybe_unused]] auto _ : state)
        {
            AZ::Vector3 rayStart = GetRandomTestRayStart();
            AZ::Intersect::SegmentTriangleHitTester hitTester(rayStart, -rayStart);

            for (int triangle = 0; triangle < sphereGeometry.size(); triangle += 3)
            {
                m_result = hitTester.IntersectSegmentTriangle(
                    sphereGeometry[triangle], sphereGeometry[triangle + 1], sphereGeometry[triangle + 2], m_outNormal, m_outDistance);
            }
        }
    }


} // namespace Benchmark

#endif
