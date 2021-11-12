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
} // namespace UnitTest


