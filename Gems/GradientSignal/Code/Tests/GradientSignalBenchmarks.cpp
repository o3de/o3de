/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef HAVE_BENCHMARK

#include <Tests/GradientSignalTestFixtures.h>
#include <Tests/GradientSignalTestHelpers.h>

#include <AzTest/AzTest.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <AzFramework/Components/TransformComponent.h>
#include <GradientSignal/Components/ConstantGradientComponent.h>
#include <GradientSignal/Components/GradientSurfaceDataComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <SurfaceData/Components/SurfaceDataShapeComponent.h>
#include <SurfaceData/Components/SurfaceDataSystemComponent.h>

namespace UnitTest
{
    class GradientGetValues : public GradientSignalBenchmarkFixture
    {
    public:
        // Create an arbitrary size shape for creating our gradients for benchmark runs.
        const float TestShapeHalfBounds = 128.0f;
    };

    // --------------------------------------------------------------------------------------
    // Base Gradients

    BENCHMARK_DEFINE_F(GradientGetValues, BM_ConstantGradient)(benchmark::State& state)
    {
        auto entity = BuildTestConstantGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_ImageGradient)(benchmark::State& state)
    {
        auto entity = BuildTestImageGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }


    BENCHMARK_DEFINE_F(GradientGetValues, BM_PerlinGradient)(benchmark::State& state)
    {
        auto entity = BuildTestPerlinGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_RandomGradient)(benchmark::State& state)
    {
        auto entity = BuildTestRandomGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_ShapeAreaFalloffGradient)(benchmark::State& state)
    {
        auto entity = BuildTestShapeAreaFalloffGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_ConstantGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_ImageGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_PerlinGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_RandomGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_ShapeAreaFalloffGradient);

    // --------------------------------------------------------------------------------------
    // Gradient Modifiers

    BENCHMARK_DEFINE_F(GradientGetValues, BM_DitherGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestDitherGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_InvertGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestDitherGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_LevelsGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestLevelsGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_MixedGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto mixedEntity = BuildTestConstantGradient(TestShapeHalfBounds);
        auto entity = BuildTestMixedGradient(TestShapeHalfBounds, baseEntity->GetId(), mixedEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_PosterizeGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestPosterizeGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_ReferenceGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestReferenceGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_SmoothStepGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestSmoothStepGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_ThresholdGradient)(benchmark::State& state)
    {
        auto baseEntity = BuildTestRandomGradient(TestShapeHalfBounds);
        auto entity = BuildTestThresholdGradient(TestShapeHalfBounds, baseEntity->GetId());
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_DitherGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_InvertGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_LevelsGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_MixedGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_PosterizeGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_ReferenceGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_SmoothStepGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_ThresholdGradient);

    // --------------------------------------------------------------------------------------
    // Surface Gradients

    BENCHMARK_DEFINE_F(GradientGetValues, BM_SurfaceAltitudeGradient)(benchmark::State& state)
    {
        auto entity = BuildTestSurfaceAltitudeGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_SurfaceMaskGradient)(benchmark::State& state)
    {
        auto entity = BuildTestSurfaceMaskGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    BENCHMARK_DEFINE_F(GradientGetValues, BM_SurfaceSlopeGradient)(benchmark::State& state)
    {
        auto entity = BuildTestSurfaceSlopeGradient(TestShapeHalfBounds);
        GradientSignalTestHelpers::RunGetValueOrGetValuesBenchmark(state, entity->GetId());
    }

    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_SurfaceAltitudeGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_SurfaceMaskGradient);
    GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(GradientGetValues, BM_SurfaceSlopeGradient);

    // --------------------------------------------------------------------------------------
    // Gradient Surface Data


    class GradientSurfaceData : public GradientSignalBenchmarkFixture
    {
    public:
        /* To benchmark the GradientSurfaceDataComponent, we need to create a surface provider in the world, then use
           the GradientSurfaceDataComponent to modify the surface points.

           For the surface provider, we create a flat box centered in XY that's the XY size of the world.
           For the GradientSurfaceDataComponent, we'll use a constant gradient as its input, and a sphere centered in XY that's
           the XY size of the world as its constrained bounds.

           Every surface point within the sphere will have the tags from the provider and the modifier, and every point outside the
           sphere will only have the provider tags.
        */
        AZStd::vector<AZStd::unique_ptr<AZ::Entity>> CreateBenchmarkEntities(float worldSize)
        {
            AZStd::vector<AZStd::unique_ptr<AZ::Entity>> testEntities;
            float halfWorldSize = worldSize / 2.0f;

            // Create a large flat box with 2 provider tags.
            {
                AZStd::unique_ptr<AZ::Entity> surface = AZStd::make_unique<AZ::Entity>();
                AZ::Vector3 worldPos(halfWorldSize, halfWorldSize, 10.0f);

                auto transform = surface->CreateComponent<AzFramework::TransformComponent>();
                transform->SetWorldTM(AZ::Transform::CreateTranslation(worldPos));

                LmbrCentral::BoxShapeConfig boxConfig(AZ::Vector3(worldSize, worldSize, 1.0f));
                auto shapeComponent = surface->CreateComponent(LmbrCentral::BoxShapeComponentTypeId);
                shapeComponent->SetConfiguration(boxConfig);

                SurfaceData::SurfaceDataShapeConfig surfaceConfig;
                surfaceConfig.m_providerTags.push_back(SurfaceData::SurfaceTag("surface1"));
                surfaceConfig.m_providerTags.push_back(SurfaceData::SurfaceTag("surface2"));
                surface->CreateComponent<SurfaceData::SurfaceDataShapeComponent>(surfaceConfig);

                surface->Init();
                surface->Activate();

                testEntities.push_back(AZStd::move(surface));
            }

            // Create a large sphere with a constant gradient and a GradientSurfaceDataComponent.
            {
                AZStd::unique_ptr<AZ::Entity> modifier = AZStd::make_unique<AZ::Entity>();
                AZ::Vector3 worldPos(halfWorldSize, halfWorldSize, 10.0f);

                auto transform = modifier->CreateComponent<AzFramework::TransformComponent>();
                transform->SetWorldTM(AZ::Transform::CreateTranslation(worldPos));

                GradientSignal::ConstantGradientConfig gradientConfig;
                gradientConfig.m_value = 0.75f;
                modifier->CreateComponent<GradientSignal::ConstantGradientComponent>(gradientConfig);

                LmbrCentral::SphereShapeConfig sphereConfig;
                sphereConfig.m_radius = halfWorldSize;
                auto shapeComponent = modifier->CreateComponent(LmbrCentral::SphereShapeComponentTypeId);
                shapeComponent->SetConfiguration(sphereConfig);

                GradientSignal::GradientSurfaceDataConfig modifierConfig;
                modifierConfig.m_shapeConstraintEntityId = modifier->GetId();
                modifierConfig.m_modifierTags.push_back(SurfaceData::SurfaceTag("modifier1"));
                modifierConfig.m_modifierTags.push_back(SurfaceData::SurfaceTag("modifier2"));
                modifier->CreateComponent<GradientSignal::GradientSurfaceDataComponent>(modifierConfig);

                modifier->Init();
                modifier->Activate();

                testEntities.push_back(AZStd::move(modifier));
            }

            return testEntities;
        }

        SurfaceData::SurfaceTagVector CreateBenchmarkTagFilterList()
        {
            SurfaceData::SurfaceTagVector tagFilterList;
            tagFilterList.emplace_back("surface1");
            tagFilterList.emplace_back("surface2");
            tagFilterList.emplace_back("modifier1");
            tagFilterList.emplace_back("modifier2");
            return tagFilterList;
        }
    };
    BENCHMARK_DEFINE_F(GradientSurfaceData, BM_GetSurfacePoints)(benchmark::State& state)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // Create our benchmark world
        const float worldSize = aznumeric_cast<float>(state.range(0));
        AZStd::vector<AZStd::unique_ptr<AZ::Entity>> benchmarkEntities = CreateBenchmarkEntities(worldSize);
        SurfaceData::SurfaceTagVector filterTags = CreateBenchmarkTagFilterList();

        // Query every point in our world at 1 meter intervals.
        for ([[maybe_unused]] auto _ : state)
        {
            // This is declared outside the loop so that the list of points doesn't fully reallocate on every query.
            SurfaceData::SurfacePointList points;

            for (float y = 0.0f; y < worldSize; y += 1.0f)
            {
                for (float x = 0.0f; x < worldSize; x += 1.0f)
                {
                    AZ::Vector3 queryPosition(x, y, 0.0f);
                    points.Clear();

                    AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePoints(queryPosition, filterTags, points);
                    benchmark::DoNotOptimize(points);
                }
            }
        }
    }

    BENCHMARK_DEFINE_F(GradientSurfaceData, BM_GetSurfacePointsFromRegion)(benchmark::State& state)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // Create our benchmark world
        float worldSize = aznumeric_cast<float>(state.range(0));
        AZStd::vector<AZStd::unique_ptr<AZ::Entity>> benchmarkEntities = CreateBenchmarkEntities(worldSize);
        SurfaceData::SurfaceTagVector filterTags = CreateBenchmarkTagFilterList();

        // Query every point in our world at 1 meter intervals.
        for ([[maybe_unused]] auto _ : state)
        {
            SurfaceData::SurfacePointList points;

            AZ::Aabb inRegion = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.0f), AZ::Vector3(worldSize));
            AZ::Vector2 stepSize(1.0f);
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromRegion(inRegion, stepSize, filterTags, points);
            benchmark::DoNotOptimize(points);
        }
    }

    BENCHMARK_DEFINE_F(GradientSurfaceData, BM_GetSurfacePointsFromList)(benchmark::State& state)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // Create our benchmark world
        const float worldSize = aznumeric_cast<float>(state.range(0));
        const int64_t worldSizeInt = state.range(0);
        AZStd::vector<AZStd::unique_ptr<AZ::Entity>> benchmarkEntities = CreateBenchmarkEntities(worldSize);
        SurfaceData::SurfaceTagVector filterTags = CreateBenchmarkTagFilterList();

        // Query every point in our world at 1 meter intervals.
        for ([[maybe_unused]] auto _ : state)
        {
            AZStd::vector<AZ::Vector3> queryPositions;
            queryPositions.reserve(worldSizeInt * worldSizeInt);

            for (float y = 0.0f; y < worldSize; y += 1.0f)
            {
                for (float x = 0.0f; x < worldSize; x += 1.0f)
                {
                    queryPositions.emplace_back(x, y, 0.0f);
                }
            }

            SurfaceData::SurfacePointList points;

            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromList(queryPositions, filterTags, points);
            benchmark::DoNotOptimize(points);
        }
    }

    BENCHMARK_REGISTER_F(GradientSurfaceData, BM_GetSurfacePoints)
        ->Arg(1024)
        ->Arg(2048)
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_REGISTER_F(GradientSurfaceData, BM_GetSurfacePointsFromRegion)
        ->Arg(1024)
        ->Arg(2048)
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_REGISTER_F(GradientSurfaceData, BM_GetSurfacePointsFromList)
        ->Arg(1024)
        ->Arg(2048)
        ->Unit(::benchmark::kMillisecond);


#endif
}


