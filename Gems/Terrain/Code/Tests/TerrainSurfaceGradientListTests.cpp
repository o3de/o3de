/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainSurfaceGradientListComponent.h>
#include <Terrain/MockTerrainLayerSpawner.h>
#include <GradientSignal/Ebuses/MockGradientRequestBus.h>

using ::testing::NiceMock;
using ::testing::Return;

namespace UnitTest
{
    class TerrainSurfaceGradientListTest : public ::testing::Test
    {
    protected:
        AZ::ComponentApplication m_app;

        const AZStd::string surfaceTag1 = "testtag1";
        const AZStd::string surfaceTag2 = "testtag2";

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 20 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_NO_RECORDS;
            appDesc.m_stackRecordLevels = 20;

            m_app.Create(appDesc);
        }

        void TearDown() override
        {
            m_app.Destroy();
        }

        AZStd::unique_ptr<AZ::Entity> CreateEntity()
        {
            auto entity = AZStd::make_unique<AZ::Entity>();
            entity->Init();
            return entity;
        }

        UnitTest::MockTerrainLayerSpawnerComponent* AddRequiredComponentsToEntity(AZ::Entity* entity)
        {
            auto layerSpawnerComponent = entity->CreateComponent<UnitTest::MockTerrainLayerSpawnerComponent>();
            m_app.RegisterComponentDescriptor(layerSpawnerComponent->CreateDescriptor());

            return layerSpawnerComponent;
        }
    };

    TEST_F(TerrainSurfaceGradientListTest, SurfaceGradientMissingRequirementsActivateFails)
    {
        auto entity = CreateEntity();

        auto terrainSurfaceGradientListComponent = entity->CreateComponent<Terrain::TerrainSurfaceGradientListComponent>();
        m_app.RegisterComponentDescriptor(terrainSurfaceGradientListComponent->CreateDescriptor());

        const AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
    }

    TEST_F(TerrainSurfaceGradientListTest, SurfaceGradientActivateSuccess)
    {
        auto entity = CreateEntity();

        AddRequiredComponentsToEntity(entity.get());

        auto terrainSurfaceGradientListComponent = entity->CreateComponent<Terrain::TerrainSurfaceGradientListComponent>();
        m_app.RegisterComponentDescriptor(terrainSurfaceGradientListComponent->CreateDescriptor());

        entity->Activate();

        EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
    }

    TEST_F(TerrainSurfaceGradientListTest, SurfaceGradientReturnsSurfaceWeights)
    {
        // When there is more than one surface/weight defined and added to the component, they should all
        // be returned.  The component isn't required to return them in descending order.
        auto entity = CreateEntity();

        AddRequiredComponentsToEntity(entity.get());

        auto gradientEntity1 = CreateEntity();
        auto gradientEntity2 = CreateEntity();

        const float gradient1Value = 0.3f;
        NiceMock<UnitTest::MockGradientRequests> mockGradientRequests1(gradientEntity1->GetId());
        ON_CALL(mockGradientRequests1, GetValue).WillByDefault(Return(gradient1Value));

        const float gradient2Value = 1.0f;
        NiceMock<UnitTest::MockGradientRequests> mockGradientRequests2(gradientEntity2->GetId());
        ON_CALL(mockGradientRequests2, GetValue).WillByDefault(Return(gradient2Value));

        AzFramework::SurfaceData::SurfaceTagWeightList weightList;
        Terrain::TerrainAreaSurfaceRequestBus::Event(
            entity->GetId(), &Terrain::TerrainAreaSurfaceRequestBus::Events::GetSurfaceWeights, AZ::Vector3::CreateZero(), weightList);

        AZ::Crc32 expectedCrcList[] = { AZ::Crc32(surfaceTag1), AZ::Crc32(surfaceTag2) };
        const float expectedWeightList[] = { gradient1Value, gradient2Value };

        int index = 0;
        for (const auto& surfaceWeight : weightList)
        {
            EXPECT_EQ(surfaceWeight.m_surfaceType, expectedCrcList[index]);
            EXPECT_NEAR(surfaceWeight.m_weight, expectedWeightList[index], 0.01f);
            index++;
        }
    }

    TEST_F(TerrainSurfaceGradientListTest, SurfaceGradientGetSurfaceWeightsAndGetSurfaceWeightsFromListMatch)
    {
        // The GetSurfaceWeights and GetSurfaceWeightsFromList APIs should return the same values for the given inputs.

        auto entity = CreateEntity();
        AddRequiredComponentsToEntity(entity.get());

        // Create a deterministic but varying result for our mock gradient - return the fractional part of the X position.
        auto gradientEntity1 = CreateEntity();
        NiceMock<UnitTest::MockGradientRequests> mockGradientRequests1(gradientEntity1->GetId());
        ON_CALL(mockGradientRequests1, GetValue)
            .WillByDefault(
                [](const GradientSignal::GradientSampleParams& params) -> float
                {
                    double intpart;
                    return aznumeric_cast<float>(modf(params.m_position.GetX(), &intpart));
                });

        // Return varying result for this mock too, but this time return the Y position fraction.
        auto gradientEntity2 = CreateEntity();
        NiceMock<UnitTest::MockGradientRequests> mockGradientRequests2(gradientEntity2->GetId());
        ON_CALL(mockGradientRequests2, GetValue)
            .WillByDefault(
                [](const GradientSignal::GradientSampleParams& params) -> float
                {
                    double intpart;
                    return aznumeric_cast<float>(modf(params.m_position.GetY(), &intpart));
                });


        // Build up a list of input positions to query with.
        AZStd::vector<AZ::Vector3> inPositions;
        for (float y = 0.0f; y <= 10.0f; y += 0.1f)
        {
            for (float x = 0.0f; x <= 10.0f; x += 0.1f)
            {
                inPositions.emplace_back(x, y, 0.0f);
            }
        }

        // Call GetSurfaceWeightsFromList to get the set of output SurfaceWeightList values
        AZStd::vector<AzFramework::SurfaceData::SurfaceTagWeightList> weightsList(inPositions.size());
        Terrain::TerrainAreaSurfaceRequestBus::Event(
            entity->GetId(), &Terrain::TerrainAreaSurfaceRequestBus::Events::GetSurfaceWeightsFromList, inPositions, weightsList);

        // For each result returned from GetSurfaceWeightsFromList, verify that it matches the result from GetSurfaceWeights
        for (size_t index = 0; index < inPositions.size(); index++)
        {
            AzFramework::SurfaceData::SurfaceTagWeightList weightList;
            Terrain::TerrainAreaSurfaceRequestBus::Event(
                entity->GetId(), &Terrain::TerrainAreaSurfaceRequestBus::Events::GetSurfaceWeights, inPositions[index], weightList);

            // Verify that we're returning the same values in the same order.
            ASSERT_EQ(weightsList[index].size(), weightList.size());
            for (size_t weightIndex = 0; weightIndex < weightsList[index].size(); weightIndex++)
            {
                ASSERT_EQ(weightsList[index][weightIndex].m_surfaceType, weightList[weightIndex].m_surfaceType);
                ASSERT_EQ(weightsList[index][weightIndex].m_weight, weightList[weightIndex].m_weight);
            }
        }
    }
} // namespace UnitTest


