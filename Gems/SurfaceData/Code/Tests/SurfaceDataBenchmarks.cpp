/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef HAVE_BENCHMARK

#include <AzTest/AzTest.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzFramework/Components/TransformComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <SurfaceData/Components/SurfaceDataShapeComponent.h>

namespace UnitTest
{
    class SurfaceDataBenchmark : public ::benchmark::Fixture
    {
    public:
        void internalSetUp()
        {
            m_surfaceDataSystemEntity = AZStd::make_unique<AZ::Entity>();
            m_surfaceDataSystemEntity->CreateComponent<SurfaceData::SurfaceDataSystemComponent>();
            m_surfaceDataSystemEntity->Init();
            m_surfaceDataSystemEntity->Activate();
        }

        void internalTearDown()
        {
            m_surfaceDataSystemEntity.reset();
        }

        // Create an entity with a Transform component and a SurfaceDataShape component at the given position with the given tags.
        AZStd::unique_ptr<AZ::Entity> CreateBenchmarkEntity(
            AZ::Vector3 worldPos, AZStd::span<const char* const> providerTags, AZStd::span<const char* const> modifierTags)
        {
            AZStd::unique_ptr<AZ::Entity> entity = AZStd::make_unique<AZ::Entity>();

            auto transform = entity->CreateComponent<AzFramework::TransformComponent>();
            transform->SetWorldTM(AZ::Transform::CreateTranslation(worldPos));

            SurfaceData::SurfaceDataShapeConfig surfaceConfig;
            for (auto& providerTag : providerTags)
            {
                surfaceConfig.m_providerTags.push_back(SurfaceData::SurfaceTag(providerTag));
            }
            for (auto& modifierTag : modifierTags)
            {
                surfaceConfig.m_modifierTags.push_back(SurfaceData::SurfaceTag(modifierTag));
            }
            entity->CreateComponent<SurfaceData::SurfaceDataShapeComponent>(surfaceConfig);

            return entity;
        }

        /* Create a set of shape surfaces in the world that we can use for benchmarking.
           Each shape is centered in XY and is the XY size of the world, but with different Z heights and placements.
           There are two boxes and one cylinder, layered like this:

           Top:
           ---
           |O| <- two boxes of equal XY size with a cylinder face-up in the center
           ---

           Side:
           |-----------|
           |           |<- entity 3, box that contains the other shapes
           | |-------| | <- entity 2, cylinder inside entity 3 and intersecting entity 1
           | |       | | 
           |-----------|<- entity 1, thin box
           | |-------| |
           |           |
           |-----------|

           This will give us either 2 or 3 generated surface points at every query point. The entity 1 surface will get the entity 2 and 3
           modifier tags added to it. The entity 2 surface will get the entity 3 modifier tag added to it. The entity 3 surface won't get
           modified.
        */
        AZStd::vector<AZStd::unique_ptr<AZ::Entity>> CreateBenchmarkEntities(float worldSize)
        {
            AZStd::vector<AZStd::unique_ptr<AZ::Entity>> testEntities;
            float halfWorldSize = worldSize / 2.0f;

            // Create a large flat box with 1 provider tag.
            AZStd::unique_ptr<AZ::Entity> surface1 = CreateBenchmarkEntity(
                AZ::Vector3(halfWorldSize, halfWorldSize, 10.0f), AZStd::array{ "surface1" }, {});
            {
                LmbrCentral::BoxShapeConfig boxConfig(AZ::Vector3(worldSize, worldSize, 1.0f));
                auto shapeComponent = surface1->CreateComponent(LmbrCentral::BoxShapeComponentTypeId);
                shapeComponent->SetConfiguration(boxConfig);

                surface1->Init();
                surface1->Activate();
            }
            testEntities.push_back(AZStd::move(surface1));

            // Create a large cylinder with 1 provider tag and 1 modifier tag.
            AZStd::unique_ptr<AZ::Entity> surface2 = CreateBenchmarkEntity(
                AZ::Vector3(halfWorldSize, halfWorldSize, 20.0f), AZStd::array{ "surface2" }, AZStd::array{ "modifier2" });
            {
                LmbrCentral::CylinderShapeConfig cylinderConfig;
                cylinderConfig.m_height = 30.0f;
                cylinderConfig.m_radius = halfWorldSize;
                auto shapeComponent = surface2->CreateComponent(LmbrCentral::CylinderShapeComponentTypeId);
                shapeComponent->SetConfiguration(cylinderConfig);

                surface2->Init();
                surface2->Activate();
            }
            testEntities.push_back(AZStd::move(surface2));

            // Create a large box with 1 provider tag and 1 modifier tag.
            AZStd::unique_ptr<AZ::Entity> surface3 = CreateBenchmarkEntity(
                AZ::Vector3(halfWorldSize, halfWorldSize, 30.0f), AZStd::array{ "surface3" }, AZStd::array{ "modifier3" });
            {
                LmbrCentral::BoxShapeConfig boxConfig(AZ::Vector3(worldSize, worldSize, 100.0f));
                auto shapeComponent = surface3->CreateComponent(LmbrCentral::BoxShapeComponentTypeId);
                shapeComponent->SetConfiguration(boxConfig);

                surface3->Init();
                surface3->Activate();
            }
            testEntities.push_back(AZStd::move(surface3));

            return testEntities;
        }

        SurfaceData::SurfaceTagVector CreateBenchmarkTagFilterList()
        {
            SurfaceData::SurfaceTagVector tagFilterList;
            tagFilterList.emplace_back("surface1");
            tagFilterList.emplace_back("surface2");
            tagFilterList.emplace_back("surface3");
            tagFilterList.emplace_back("modifier2");
            tagFilterList.emplace_back("modifier3");
            return tagFilterList;
        }

    protected:
        void SetUp([[maybe_unused]] const benchmark::State& state) override
        {
            internalSetUp();
        }
        void SetUp([[maybe_unused]] benchmark::State& state) override
        {
            internalSetUp();
        }

        void TearDown([[maybe_unused]] const benchmark::State& state) override
        {
            internalTearDown();
        }
        void TearDown([[maybe_unused]] benchmark::State& state) override
        {
            internalTearDown();
        }

        AZStd::unique_ptr<AZ::Entity> m_surfaceDataSystemEntity;
    };

    BENCHMARK_DEFINE_F(SurfaceDataBenchmark, BM_GetSurfacePoints)(benchmark::State& state)
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

    BENCHMARK_DEFINE_F(SurfaceDataBenchmark, BM_GetSurfacePointsFromRegion)(benchmark::State& state)
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

    BENCHMARK_DEFINE_F(SurfaceDataBenchmark, BM_GetSurfacePointsFromList)(benchmark::State& state)
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

    BENCHMARK_REGISTER_F(SurfaceDataBenchmark, BM_GetSurfacePoints)
        ->Arg( 1024 )
        ->Arg( 2048 )
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_REGISTER_F(SurfaceDataBenchmark, BM_GetSurfacePointsFromRegion)
        ->Arg( 1024 )
        ->Arg( 2048 )
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_REGISTER_F(SurfaceDataBenchmark, BM_GetSurfacePointsFromList)
        ->Arg( 1024 )
        ->Arg( 2048 )
        ->Unit(::benchmark::kMillisecond);

    BENCHMARK_DEFINE_F(SurfaceDataBenchmark, BM_AddSurfaceTagWeight)(benchmark::State& state)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZ::Crc32 tags[AzFramework::SurfaceData::Constants::MaxSurfaceWeights];
        AZ::SimpleLcgRandom randomGenerator(1234567);

        // Declare this outside the loop so that we aren't benchmarking creation and destruction.
        SurfaceData::SurfaceTagWeights weights;

        bool clearEachTime = state.range(0) > 0;

        // Create a list of randomly-generated tag values.
        for (auto& tag : tags)
        {
            tag = randomGenerator.GetRandom();
        }

        for ([[maybe_unused]] auto _ : state)
        {
            // We'll benchmark this two ways:
            // 1. We clear each time, which means each AddSurfaceWeightIfGreater call will search the whole list then add.
            // 2. We don't clear, which means that after the first run, AddSurfaceWeightIfGreater will always try to replace values.
            if (clearEachTime)
            {
                weights.Clear();
            }

            // For each tag, try to add it with a random weight.
            for (auto& tag : tags)
            {
                weights.AddSurfaceTagWeight(tag, randomGenerator.GetRandomFloat());
            }
        }
    }

    BENCHMARK_REGISTER_F(SurfaceDataBenchmark, BM_AddSurfaceTagWeight)
        ->Arg(false)
        ->Arg(true)
        ->ArgName("ClearEachTime");

    BENCHMARK_DEFINE_F(SurfaceDataBenchmark, BM_HasAnyMatchingTags_NoMatches)(benchmark::State& state)
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZ::Crc32 tags[AzFramework::SurfaceData::Constants::MaxSurfaceWeights];
        AZ::SimpleLcgRandom randomGenerator(1234567);

        // Declare this outside the loop so that we aren't benchmarking creation and destruction.
        SurfaceData::SurfaceTagWeights weights;

        // Create a list of randomly-generated tag values.
        for (auto& tag : tags)
        {
            // Specifically always set the last bit so that we can create comparison tags that won't match.
            tag = randomGenerator.GetRandom() | 0x01;

            // Add the tag to our weights list with a random weight.
            weights.AddSurfaceTagWeight(tag, randomGenerator.GetRandomFloat());
        }

        // Create a set of similar comparison tags that won't match. We still want a random distribution of values though,
        // because the SurfaceTagWeights might behave differently with ordered lists.
        SurfaceData::SurfaceTagVector comparisonTags;
        for (auto& tag : tags)
        {
            comparisonTags.emplace_back(tag ^ 0x01);
        }

        for ([[maybe_unused]] auto _ : state)
        {
            // Test to see if any of our tags match.
            // All of comparison tags should get compared against all of the added tags.
            bool result = weights.HasAnyMatchingTags(comparisonTags);
            benchmark::DoNotOptimize(result);
        }
    }

    BENCHMARK_REGISTER_F(SurfaceDataBenchmark, BM_HasAnyMatchingTags_NoMatches);



#endif
}


