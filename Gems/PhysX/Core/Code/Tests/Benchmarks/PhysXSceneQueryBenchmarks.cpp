/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef HAVE_BENCHMARK
#include <vector>

#include <AzCore/Math/Random.h>
#include <AzTest/AzTest.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <Tests/PhysXGenericTestFixture.h>
#include <Tests/PhysXTestCommon.h>
#include <Benchmarks/PhysXBenchmarksCommon.h>
#include <Benchmarks/PhysXBenchmarksUtilities.h>

#include <PhysX/PhysXLocks.h>
#include <Scene/PhysXScene.h>

namespace PhysX::Benchmarks
{
    namespace SceneQueryConstants
    {
        const AZ::Vector3 BoxDimensions = AZ::Vector3::CreateOne();
        static const float SphereShapeRadius = 2.0f;
        static const AZ::u32 MinRadius = 2u;
        static const int Seed = 100;

        static const std::vector<std::vector<std::pair<int64_t, int64_t>>> BenchmarkConfigs =
        {
            // {{a, b}, {c, d}} means generate all pairs of parameters {x, y} for the benchmark
            // such as x = a * 2 ^ k && x <= b and y = c * 2 ^ l && y <= d
            // We have several configurations here because number of box entities might become
            // larger than possible number of box locations if the maxRadius is small
            {{4, 16}, {8, 512}},
            {{32, 256}, {16, 512}},
            {{512, 1024}, {32, 512}},
            {{2048, 4096}, {64, 512}}
        };
    }

    class PhysXSceneQueryBenchmarkFixture
        : public benchmark::Fixture
        , public PhysX::GenericPhysicsFixture

    {
        //! Spawns box entities in unique locations in 1/8 of sphere with all non-negative dimensions between radii[2, max radius].
        //! Accepts 2 parameters from \state.
        //!
        //! \state.range(0) - number of box entities to spawn
        //! \state.range(1) - max radius
        void internalSetUp(const ::benchmark::State& state)
        {
            PhysX::GenericPhysicsFixture::SetUpInternal();

            m_random = AZ::SimpleLcgRandom(SceneQueryConstants::Seed);
            m_numBoxes = aznumeric_cast<AZ::u32>(state.range(0));
            const auto minRadius = SceneQueryConstants::MinRadius;
            const auto maxRadius = aznumeric_cast<AZ::u32>(state.range(1));

            std::vector<AZ::Vector3> possibleBoxes;
            AZ::u32 possibleBoxesCount = 0;

            // Generate all possible boxes in radii [minRadius, maxRadius] where all the dimensions are positive.
            for (auto x = 0u; x <= maxRadius; ++x)
            {
                for (auto y = 0u; y * y <= maxRadius * maxRadius - x * x; ++y)
                {
                    for (auto z = aznumeric_cast<AZ::u32>(sqrt(std::max(x * x - y * y - minRadius * minRadius, 0u))); z * z <= maxRadius * maxRadius - x * x - y * y; ++z)
                    {
                        const auto sqDist = x * x + y * y + z * z;
                        if (minRadius * minRadius <= sqDist && sqDist <= maxRadius * maxRadius)
                        {
                            auto boxPosition = AZ::Vector3(aznumeric_cast<float>(x), aznumeric_cast<float>(y), aznumeric_cast<float>(z));
                            possibleBoxes.push_back(boxPosition);
                            possibleBoxesCount++;
                        }
                    }
                }
            }
            AZ_Assert(m_numBoxes <= possibleBoxesCount, "Number of supplied boxes should be less than or equal to possible positions for boxes.");

            // Shuffle to pick first m_numBoxes
            for (AZ::u32 i = 1; i < possibleBoxesCount; ++i)
            {
                std::swap(possibleBoxes[i], possibleBoxes[m_random.GetRandom() % (i + 1)]);
            }

            m_boxes = std::vector<AZ::Vector3>(possibleBoxes.begin(), possibleBoxes.begin() + m_numBoxes);
            for (auto box : m_boxes)
            {
                auto newEntity = TestUtils::CreateBoxEntity(m_testSceneHandle, box, SceneQueryConstants::BoxDimensions);
                Physics::RigidBodyRequestBus::Event(newEntity->GetId(), &Physics::RigidBodyRequestBus::Events::SetGravityEnabled, false);
                m_entities.push_back(newEntity);
            }
        }

        void internalTearDown()
        {
            m_boxes.clear();
            m_entities.clear();
            PhysX::GenericPhysicsFixture::TearDownInternal();
        }

    public:
        void SetUp(const benchmark::State& state) override
        {
            internalSetUp(state);
        }
        void SetUp(benchmark::State& state) override
        {
            internalSetUp(state);
        }

        void TearDown(const benchmark::State&) override
        {
            internalTearDown();
        }
        void TearDown(benchmark::State&) override
        {
            internalTearDown();
        }

    protected:
        std::vector<EntityPtr> m_entities;
        std::vector<AZ::Vector3> m_boxes;
        AZ::u32 m_numBoxes = 0;
        AZ::SimpleLcgRandom m_random;
    };

    BENCHMARK_DEFINE_F(PhysXSceneQueryBenchmarkFixture, BM_RaycastRandomBoxes)(benchmark::State& state)
    {
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3::CreateZero();
        request.m_distance = 2000.0f;

        AZStd::vector<int64_t> executionTimes;
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        auto next = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            request.m_direction = m_boxes[next].GetNormalized();

            auto start = AZStd::chrono::steady_clock::now();

            AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

            auto timeElasped = AZStd::chrono::duration_cast<AZStd::chrono::nanoseconds>(AZStd::chrono::steady_clock::now() - start);
            executionTimes.emplace_back(timeElasped.count());

            benchmark::DoNotOptimize(result);
            next = (next + 1) % m_numBoxes;
        }

        // get the P50, P90, P99 percentiles of each call and the standard deviation and mean
        Utils::ReportPercentiles(state, executionTimes);
        Utils::ReportStandardDeviationAndMeanCounters(state, executionTimes);
    }

    BENCHMARK_DEFINE_F(PhysXSceneQueryBenchmarkFixture, BM_RaycastRandomBoxesParallel)(benchmark::State& state)
    {
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3::CreateZero();
        request.m_distance = 2000.0f;

        AZStd::vector<int64_t> executionTimes;
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        auto next = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            request.m_direction = m_boxes[next].GetNormalized();

            auto start = AZStd::chrono::steady_clock::now();

            const size_t numThreads = 4;

            AZStd::vector<AZStd::thread> threads;
            threads.reserve(numThreads);
            AzPhysics::SceneHandle testSceneHandle = m_testSceneHandle;

            for (size_t i = 0; i < numThreads; ++i)
            {
                threads.emplace_back(
                    [&testSceneHandle, &request, sceneInterface]()
                    {
                        PhysXScene* azScene = static_cast<PhysXScene*>(sceneInterface->GetScene(testSceneHandle));
                        physx::PxScene* pxScene = static_cast<physx::PxScene*>(azScene->GetNativePointer());

                        PHYSX_SCENE_READ_LOCK(pxScene);

                        const size_t iterationsNum = 1000000;

                        AzPhysics::SceneQueryHits result;
                        for (size_t k = 0; k < iterationsNum; ++k)
                        {
                            sceneInterface->QueryScene(testSceneHandle, &request, result);

                            result.m_hits.clear();
                        }
                        benchmark::DoNotOptimize(result);
                    });
            }

            for (AZStd::thread& thread : threads)
            {
                thread.join();
            }

            auto timeElasped = AZStd::chrono::duration_cast<AZStd::chrono::nanoseconds>(AZStd::chrono::steady_clock::now() - start);

            executionTimes.emplace_back(timeElasped.count());

            next = (next + 1) % m_numBoxes;
        }

        // get the P50, P90, P99 percentiles of each call and the standard deviation and mean
        Utils::ReportPercentiles(state, executionTimes);
        Utils::ReportStandardDeviationAndMeanCounters(state, executionTimes);
    }

    BENCHMARK_DEFINE_F(PhysXSceneQueryBenchmarkFixture, BM_ShapecastRandomBoxes)(benchmark::State& state)
    {
        AzPhysics::ShapeCastRequest request = AzPhysics::ShapeCastRequestHelpers::CreateSphereCastRequest(
            SceneQueryConstants::SphereShapeRadius,
            AZ::Transform::CreateIdentity(),
            AZ::Vector3::CreateOne(),
            2000.0f
        );


        AZStd::vector<int64_t> executionTimes;
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        auto next = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            request.m_direction = m_boxes[next].GetNormalized();

            auto start = AZStd::chrono::steady_clock::now(); 

            AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

            auto timeElasped = AZStd::chrono::duration_cast<AZStd::chrono::nanoseconds>(AZStd::chrono::steady_clock::now() - start);
            executionTimes.emplace_back(timeElasped.count());

            benchmark::DoNotOptimize(result);
            next = (next + 1) % m_numBoxes;
        }

        //get the P50, P90, P99 percentiles of each call and the standard deviation and mean
        Utils::ReportPercentiles(state, executionTimes);
        Utils::ReportStandardDeviationAndMeanCounters(state, executionTimes);
    }

    BENCHMARK_DEFINE_F(PhysXSceneQueryBenchmarkFixture, BM_OverlapRandomBoxes)(benchmark::State& state)
    {
        AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateSphereOverlapRequest(
            SceneQueryConstants::SphereShapeRadius,
            AZ::Transform::CreateIdentity()
        );

        AZStd::vector<int64_t> executionTimes;
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        auto next = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            request.m_pose = AZ::Transform::CreateTranslation(m_boxes[next]);

            auto start = AZStd::chrono::steady_clock::now();

            AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);

            auto timeElasped = AZStd::chrono::duration_cast<AZStd::chrono::nanoseconds>(AZStd::chrono::steady_clock::now() - start);
            executionTimes.emplace_back(timeElasped.count());

            benchmark::DoNotOptimize(result);
            next = (next + 1) % m_numBoxes;
        }

        //get the P50, P90, P99 percentiles of each call and the standard deviation and mean
        Utils::ReportPercentiles(state, executionTimes);
        Utils::ReportStandardDeviationAndMeanCounters(state, executionTimes);
    }

    BENCHMARK_REGISTER_F(PhysXSceneQueryBenchmarkFixture, BM_RaycastRandomBoxes)
        ->RangeMultiplier(2)
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[0])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[1])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[2])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[3])
        ->Unit(::benchmark::kNanosecond)
        ;

    BENCHMARK_REGISTER_F(PhysXSceneQueryBenchmarkFixture, BM_RaycastRandomBoxesParallel)
        ->RangeMultiplier(2)
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[0])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[1])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[2])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[3])
        ->Unit(::benchmark::kNanosecond);

    BENCHMARK_REGISTER_F(PhysXSceneQueryBenchmarkFixture, BM_ShapecastRandomBoxes)
        ->RangeMultiplier(2)
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[0])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[1])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[2])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[3])
        ->Unit(::benchmark::kNanosecond)
        ;
    BENCHMARK_REGISTER_F(PhysXSceneQueryBenchmarkFixture, BM_OverlapRandomBoxes)
        ->RangeMultiplier(2)
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[0])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[1])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[2])
        ->Ranges(SceneQueryConstants::BenchmarkConfigs[3])
        ->Unit(::benchmark::kNanosecond)
        ;
}
#endif
