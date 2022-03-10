/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef HAVE_BENCHMARK

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzTest/AzTest.h>

#include <GradientSignal/Components/RandomGradientComponent.h>
#include <GradientSignal/Components/GradientTransformComponent.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>

#include <SurfaceData/SurfaceDataTypes.h>

#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <MockAxisAlignedBoxShapeComponent.h>

#include <TerrainSystem/TerrainSystem.h>
#include <TerrainTestFixtures.h>
#include <benchmark/benchmark.h>

namespace UnitTest
{
    using ::testing::NiceMock;
    using ::testing::Return;

    class TerrainSystemBenchmarkFixture
        : public TerrainBenchmarkFixture
    {
    public:
        void RunTerrainApiBenchmark(
            benchmark::State& state,
            AZStd::function<void(
                float queryResolution,
                const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)> ApiCaller)
        {
            // Get the ranges for querying from our benchmark parameters
            float boundsRange = aznumeric_cast<float>(state.range(0));
            uint32_t numSurfaces = aznumeric_cast<uint32_t>(state.range(1));
            AzFramework::Terrain::TerrainDataRequests::Sampler sampler =
                static_cast<AzFramework::Terrain::TerrainDataRequests::Sampler>(state.range(2));

            // Set up our world bounds and query resolution
            AZ::Aabb worldBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-boundsRange / 2.0f), AZ::Vector3(boundsRange / 2.0f));
            float queryResolution = 1.0f;

            // Create a Random Gradient to use as our height provider
            const uint32_t heightRandomSeed = 12345;
            auto heightGradientEntity = CreateAndActivateTestRandomGradient(worldBounds, heightRandomSeed);

            // Create a set of Random Gradients to use as our surface providers
            Terrain::TerrainSurfaceGradientListConfig surfaceConfig;
            AZStd::vector<AZStd::unique_ptr<AZ::Entity>> surfaceGradientEntities;
            for (uint32_t surfaces = 0; surfaces < numSurfaces; surfaces++)
            {
                const uint32_t surfaceRandomSeed = 23456 + surfaces;
                auto surfaceGradientEntity = CreateAndActivateTestRandomGradient(worldBounds, surfaceRandomSeed);

                // Give each gradient a new surface tag
                surfaceConfig.m_gradientSurfaceMappings.emplace_back(
                    surfaceGradientEntity->GetId(), SurfaceData::SurfaceTag(AZStd::string::format("test%u", surfaces)));

                surfaceGradientEntities.emplace_back(AZStd::move(surfaceGradientEntity));
            }

            // Create a single Terrain Layer Spawner that covers the entire terrain world bounds
            // (Do this *after* creating and activating the height and surface gradients)
            auto testLayerSpawnerEntity = CreateTestLayerSpawnerEntity(worldBounds, heightGradientEntity->GetId(), surfaceConfig);
            ActivateEntity(testLayerSpawnerEntity.get());

            // Create the terrain system (do this after creating the terrain layer entity to ensure that we don't need any data refreshes)
            // Also ensure to do this after creating the global JobManager.
            auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution, worldBounds);

            // Call the terrain API we're testing for every height and width in our ranges.
            for ([[maybe_unused]] auto stateIterator : state)
            {
                ApiCaller(queryResolution, worldBounds, sampler);
            }

            testLayerSpawnerEntity.reset();

            heightGradientEntity.reset();

            surfaceGradientEntities.clear();
        }

        void GenerateInputPositionsList(float queryResolution, const AZ::Aabb& worldBounds, AZStd::vector<AZ::Vector3>& positions)
        {
            const size_t numSamplesX = aznumeric_cast<size_t>(ceil(worldBounds.GetExtents().GetX() / queryResolution));
            const size_t numSamplesY = aznumeric_cast<size_t>(ceil(worldBounds.GetExtents().GetY() / queryResolution));

            positions.clear();
            positions.reserve(numSamplesX * numSamplesY);

            for (size_t y = 0; y < numSamplesY; y++)
            {
                float fy = aznumeric_cast<float>(worldBounds.GetMin().GetY() + (y * queryResolution));
                for (size_t x = 0; x < numSamplesX; x++)
                {
                    float fx = aznumeric_cast<float>(worldBounds.GetMin().GetX() + (x * queryResolution));
                    positions.emplace_back(fx, fy, 0.0f);
                }
            }
        }
    };


    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_GetHeight)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                float worldMinZ = worldBounds.GetMin().GetZ();

                for (float y = worldBounds.GetMin().GetY(); y < worldBounds.GetMax().GetY(); y += 1.0f)
                {
                    for (float x = worldBounds.GetMin().GetX(); x < worldBounds.GetMax().GetX(); x += 1.0f)
                    {
                        float terrainHeight = worldMinZ;
                        bool terrainExists = false;
                        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                            terrainHeight, &AzFramework::Terrain::TerrainDataRequests::GetHeightFromFloats, x, y, sampler, &terrainExists);
                        benchmark::DoNotOptimize(terrainHeight);
                    }
                }
            });
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_GetHeight)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessHeightsRegion)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                auto perPositionCallback = []([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex, 
                    const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_position.GetZ());
                };

                AZ::Vector2 stepSize = AZ::Vector2(queryResolution);
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessHeightsFromRegion, worldBounds, stepSize, perPositionCallback, sampler);
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessHeightsRegion)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessHeightsRegionAsync)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                auto perPositionCallback = []([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex, 
                    const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_position.GetZ());
                };

                AZStd::semaphore completionEvent;
                auto completionCallback = [&completionEvent](AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext>)
                {
                    completionEvent.release();
                };

                AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams> asyncParams
                    = AZStd::make_shared<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams>();
                asyncParams->m_completionCallback = completionCallback;

                AZ::Vector2 stepSize = AZ::Vector2(queryResolution);
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessHeightsFromRegionAsync, worldBounds, stepSize, perPositionCallback, sampler, asyncParams);

                completionEvent.acquire();
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessHeightsRegionAsync)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessHeightsList)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            [this]([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                AZStd::vector<AZ::Vector3> inPositions;
                GenerateInputPositionsList(queryResolution, worldBounds, inPositions);

                auto perPositionCallback = [](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_position.GetZ());
                };
                
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessHeightsFromList, inPositions, perPositionCallback, sampler);
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessHeightsList)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessHeightsListAsync)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            [this]([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                AZStd::vector<AZ::Vector3> inPositions;
                GenerateInputPositionsList(queryResolution, worldBounds, inPositions);

                auto perPositionCallback = [](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_position.GetZ());
                };

                AZStd::semaphore completionEvent;
                auto completionCallback = [&completionEvent](AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext>)
                {
                    completionEvent.release();
                };

                AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams> asyncParams
                    = AZStd::make_shared<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams>();
                asyncParams->m_completionCallback = completionCallback;
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessHeightsFromListAsync, inPositions, perPositionCallback, sampler, asyncParams);

                completionEvent.acquire();
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessHeightsListAsync)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_GetNormal)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
               AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                for (float y = worldBounds.GetMin().GetY(); y < worldBounds.GetMax().GetY(); y += 1.0f)
                {
                    for (float x = worldBounds.GetMin().GetX(); x < worldBounds.GetMax().GetX(); x += 1.0f)
                    {
                        AZ::Vector3 terrainNormal;
                        bool terrainExists = false;
                        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                            terrainNormal, &AzFramework::Terrain::TerrainDataRequests::GetNormalFromFloats, x, y, sampler, &terrainExists);
                        benchmark::DoNotOptimize(terrainNormal);
                    }
                }
            });
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_GetNormal)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessNormalsRegion)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                auto perPositionCallback = []([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex, 
                    const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_normal);
                };
                
                AZ::Vector2 stepSize = AZ::Vector2(queryResolution);
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessNormalsFromRegion, worldBounds, stepSize, perPositionCallback, sampler);
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessNormalsRegion)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessNormalsRegionAsync)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                auto perPositionCallback = []([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex, 
                    const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_normal);
                };

                AZStd::semaphore completionEvent;
                auto completionCallback = [&completionEvent](AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext>)
                {
                    completionEvent.release();
                };

                AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams> asyncParams
                    = AZStd::make_shared<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams>();
                asyncParams->m_completionCallback = completionCallback;

                AZ::Vector2 stepSize = AZ::Vector2(queryResolution);
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessNormalsFromRegionAsync, worldBounds, stepSize, perPositionCallback, sampler, asyncParams);

                completionEvent.acquire();
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessNormalsRegionAsync)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessNormalsList)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            [this]([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                AZStd::vector<AZ::Vector3> inPositions;
                GenerateInputPositionsList(queryResolution, worldBounds, inPositions);

                auto perPositionCallback = [](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_normal);
                };
                
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessNormalsFromList, inPositions, perPositionCallback, sampler);
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessNormalsList)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessNormalsListAsync)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            [this]([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                AZStd::vector<AZ::Vector3> inPositions;
                GenerateInputPositionsList(queryResolution, worldBounds, inPositions);

                auto perPositionCallback = [](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_normal);
                };

                AZStd::semaphore completionEvent;
                auto completionCallback = [&completionEvent](AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext>)
                {
                    completionEvent.release();
                };

                AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams> asyncParams
                    = AZStd::make_shared<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams>();
                asyncParams->m_completionCallback = completionCallback;
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessNormalsFromListAsync, inPositions, perPositionCallback, sampler, asyncParams);

                completionEvent.acquire();
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessNormalsListAsync)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_GetSurfaceWeights)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
               AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                AzFramework::SurfaceData::SurfaceTagWeightList surfaceWeights;
                for (float y = worldBounds.GetMin().GetY(); y < worldBounds.GetMax().GetY(); y += 1.0f)
                {
                    for (float x = worldBounds.GetMin().GetX(); x < worldBounds.GetMax().GetX(); x += 1.0f)
                    {
                        bool terrainExists = false;
                        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                            &AzFramework::Terrain::TerrainDataRequests::GetSurfaceWeightsFromFloats, x, y, surfaceWeights, sampler,
                            &terrainExists);
                        benchmark::DoNotOptimize(surfaceWeights);
                    }
                }
            });
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_GetSurfaceWeights)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 2, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 2, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 4, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 4, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfaceWeightsRegion)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                auto perPositionCallback = []([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex, 
                    const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_surfaceTags);
                };
                
                AZ::Vector2 stepSize = AZ::Vector2(queryResolution);
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessSurfaceWeightsFromRegion, worldBounds, stepSize, perPositionCallback, sampler);
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfaceWeightsRegion)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 2, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 2, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 4, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 4, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfaceWeightsRegionAsync)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                auto perPositionCallback = []([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex, 
                    const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_surfaceTags);
                };

                AZStd::semaphore completionEvent;
                auto completionCallback = [&completionEvent](AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext>)
                {
                    completionEvent.release();
                };

                AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams> asyncParams
                    = AZStd::make_shared<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams>();
                asyncParams->m_completionCallback = completionCallback;

                AZ::Vector2 stepSize = AZ::Vector2(queryResolution);
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessSurfaceWeightsFromRegionAsync, worldBounds, stepSize, perPositionCallback, sampler, asyncParams);

                completionEvent.acquire();
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfaceWeightsRegionAsync)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 2, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 2, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 4, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 4, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfaceWeightsList)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            [this]([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                AZStd::vector<AZ::Vector3> inPositions;
                GenerateInputPositionsList(queryResolution, worldBounds, inPositions);

                auto perPositionCallback = [](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_surfaceTags);
                };
                
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessSurfaceWeightsFromList, inPositions, perPositionCallback, sampler);
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfaceWeightsList)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 2, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 2, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 4, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 4, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfaceWeightsListAsync)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            [this]([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                AZStd::vector<AZ::Vector3> inPositions;
                GenerateInputPositionsList(queryResolution, worldBounds, inPositions);

                auto perPositionCallback = [](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint.m_surfaceTags);
                };

                AZStd::semaphore completionEvent;
                auto completionCallback = [&completionEvent](AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext>)
                {
                    completionEvent.release();
                };

                AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams> asyncParams
                    = AZStd::make_shared<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams>();
                asyncParams->m_completionCallback = completionCallback;
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessSurfaceWeightsFromListAsync, inPositions, perPositionCallback, sampler, asyncParams);

                completionEvent.acquire();
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfaceWeightsListAsync)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 2, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 2, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 4, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 4, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_GetSurfacePoints)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
               AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                AzFramework::SurfaceData::SurfacePoint surfacePoint;
                for (float y = worldBounds.GetMin().GetY(); y < worldBounds.GetMax().GetY(); y += 1.0f)
                {
                    for (float x = worldBounds.GetMin().GetX(); x < worldBounds.GetMax().GetX(); x += 1.0f)
                    {
                        bool terrainExists = false;
                        AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                            &AzFramework::Terrain::TerrainDataRequests::GetSurfacePointFromFloats, x, y, surfacePoint, sampler,
                            &terrainExists);
                        benchmark::DoNotOptimize(surfacePoint);
                    }
                }
            });
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_GetSurfacePoints)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfacePointsRegion)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                auto perPositionCallback = []([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex, 
                    const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint);
                };
                
                AZ::Vector2 stepSize = AZ::Vector2(queryResolution);
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessSurfacePointsFromRegion, worldBounds, stepSize, perPositionCallback, sampler);
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfacePointsRegion)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfacePointsRegionAsync)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            []([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                auto perPositionCallback = []([[maybe_unused]] size_t xIndex, [[maybe_unused]] size_t yIndex, 
                    const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint);
                };

                AZStd::semaphore completionEvent;
                auto completionCallback = [&completionEvent](AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext>)
                {
                    completionEvent.release();
                };

                AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams> asyncParams
                    = AZStd::make_shared<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams>();
                asyncParams->m_completionCallback = completionCallback;

                AZ::Vector2 stepSize = AZ::Vector2(queryResolution);
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessSurfacePointsFromRegionAsync, worldBounds, stepSize, perPositionCallback, sampler, asyncParams);

                completionEvent.acquire();
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfacePointsRegionAsync)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfacePointsList)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            [this]([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                AZStd::vector<AZ::Vector3> inPositions;
                GenerateInputPositionsList(queryResolution, worldBounds, inPositions);

                auto perPositionCallback = [](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint);
                };
                
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessSurfacePointsFromList, inPositions, perPositionCallback, sampler);
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfacePointsList)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfacePointsListAsync)(benchmark::State& state)
    {
        // Run the benchmark
        RunTerrainApiBenchmark(
            state,
            [this]([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                AZStd::vector<AZ::Vector3> inPositions;
                GenerateInputPositionsList(queryResolution, worldBounds, inPositions);

                auto perPositionCallback = [](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, [[maybe_unused]] bool terrainExists)
                {
                    benchmark::DoNotOptimize(surfacePoint);
                };

                AZStd::semaphore completionEvent;
                auto completionCallback = [&completionEvent](AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::TerrainJobContext>)
                {
                    completionEvent.release();
                };

                AZStd::shared_ptr<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams> asyncParams
                    = AZStd::make_shared<AzFramework::Terrain::TerrainDataRequests::ProcessAsyncParams>();
                asyncParams->m_completionCallback = completionCallback;
                AzFramework::Terrain::TerrainDataRequestBus::Broadcast(
                    &AzFramework::Terrain::TerrainDataRequests::ProcessSurfacePointsFromListAsync, inPositions, perPositionCallback, sampler, asyncParams);

                completionEvent.acquire();
            }
        );
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_ProcessSurfacePointsListAsync)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::CLAMP) })
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_GetClosestIntersectionRandom)(benchmark::State& state)
    {
        // Run the benchmark
        const uint32_t numRays = aznumeric_cast<uint32_t>(state.range(1));
        RunTerrainApiBenchmark(
            state,
            [numRays]([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                [[maybe_unused]] AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                // Cast rays starting at random positions above the terrain,
                // and ending at a random positions below the terrain.
                AZ::SimpleLcgRandom random;
                AzFramework::RenderGeometry::RayRequest ray;
                AzFramework::RenderGeometry::RayResult result;
                for (uint32_t i = 0; i < numRays; ++i)
                {
                    ray.m_startWorldPosition.SetX(worldBounds.GetMin().GetX() + (random.GetRandomFloat() * worldBounds.GetXExtent()));
                    ray.m_startWorldPosition.SetY(worldBounds.GetMin().GetY() + (random.GetRandomFloat() * worldBounds.GetYExtent()));
                    ray.m_startWorldPosition.SetZ(worldBounds.GetMax().GetZ());
                    ray.m_endWorldPosition.SetX(worldBounds.GetMin().GetX() + (random.GetRandomFloat() * worldBounds.GetXExtent()));
                    ray.m_endWorldPosition.SetY(worldBounds.GetMin().GetY() + (random.GetRandomFloat() * worldBounds.GetYExtent()));
                    ray.m_endWorldPosition.SetZ(worldBounds.GetMin().GetZ());
                    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                        result, &AzFramework::Terrain::TerrainDataRequests::GetClosestIntersection, ray);
                }
            });
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_GetClosestIntersectionRandom)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 10, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 10, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 10, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 100, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 100, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 100, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 1000, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1000, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 1000, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(TerrainSystemBenchmarkFixture, BM_GetClosestIntersectionWorstCase)(benchmark::State& state)
    {
        // Run the benchmark
        const uint32_t numRays = aznumeric_cast<uint32_t>(state.range(1));
        RunTerrainApiBenchmark(
            state,
            [numRays]([[maybe_unused]] float queryResolution, const AZ::Aabb& worldBounds,
                [[maybe_unused]] AzFramework::Terrain::TerrainDataRequests::Sampler sampler)
            {
                // Cast rays starting at an upper corner of the terrain world,
                // and ending at the opposite top corner of the terrain world,
                // traversing the entire grid without finding an intersection.
                AzFramework::RenderGeometry::RayRequest ray;
                AzFramework::RenderGeometry::RayResult result;
                ray.m_startWorldPosition = worldBounds.GetMax();
                ray.m_endWorldPosition = worldBounds.GetMin();
                ray.m_endWorldPosition.SetZ(ray.m_startWorldPosition.GetZ());
                for (uint32_t i = 0; i < numRays; ++i)
                {
                    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
                        result, &AzFramework::Terrain::TerrainDataRequests::GetClosestIntersection, ray);
                }
            });
    }

    BENCHMARK_REGISTER_F(TerrainSystemBenchmarkFixture, BM_GetClosestIntersectionWorstCase)
        ->Args({ 1024, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 1, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 10, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 10, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 10, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 100, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 100, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 100, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 1024, 1000, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 2048, 1000, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Args({ 4096, 1000, static_cast<int>(AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT) })
        ->Unit(::benchmark::kMillisecond);
#endif

}
