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
using ::testing::AtLeast;
using ::testing::_;
using ::testing::Return;

namespace UnitTest
{
    class TerrainSurfaceGradientListTest : public ::testing::Test
    {
    protected:
        AZ::ComponentApplication m_app;

        AZStd::unique_ptr<AZ::Entity> m_entity;
        UnitTest::MockTerrainLayerSpawnerComponent* m_layerSpawnerComponent = nullptr;
        AZStd::unique_ptr<AZ::Entity> m_gradientEntity1, m_gradientEntity2;

        const AZStd::string surfaceTag1 = "testtag1";
        const AZStd::string surfaceTag2 = "testtag2";

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 20 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_NO_RECORDS;
            appDesc.m_stackRecordLevels = 20;

            m_app.Create(appDesc);

            CreateEntities();
        }

        void TearDown() override
        {
            m_gradientEntity2.reset();
            m_gradientEntity1.reset();
            m_entity.reset();

            m_app.Destroy();
        }

        void CreateEntities()
        {
            m_entity = AZStd::make_unique<AZ::Entity>();
            ASSERT_TRUE(m_entity);

            m_entity->Init();

            m_gradientEntity1 = AZStd::make_unique<AZ::Entity>();
            ASSERT_TRUE(m_gradientEntity1);

            m_gradientEntity1->Init();

            m_gradientEntity2 = AZStd::make_unique<AZ::Entity>();
            ASSERT_TRUE(m_gradientEntity2);

            m_gradientEntity2->Init();
        }

        void AddSurfaceGradientListToEntities()
        {
            m_layerSpawnerComponent = m_entity->CreateComponent<UnitTest::MockTerrainLayerSpawnerComponent>();
            m_app.RegisterComponentDescriptor(m_layerSpawnerComponent->CreateDescriptor());

            Terrain::TerrainSurfaceGradientListConfig config;

            Terrain::TerrainSurfaceGradientMapping mapping1;
            mapping1.m_gradientEntityId = m_gradientEntity1->GetId();
            mapping1.m_surfaceTag = SurfaceData::SurfaceTag(surfaceTag1);
            config.m_gradientSurfaceMappings.emplace_back(mapping1);

            Terrain::TerrainSurfaceGradientMapping mapping2;
            mapping2.m_gradientEntityId = m_gradientEntity2->GetId();
            mapping2.m_surfaceTag = SurfaceData::SurfaceTag(surfaceTag2);
            config.m_gradientSurfaceMappings.emplace_back(mapping2);

            Terrain::TerrainSurfaceGradientListComponent* terrainSurfaceGradientListComponent =
                m_entity->CreateComponent<Terrain::TerrainSurfaceGradientListComponent>(config);
            m_app.RegisterComponentDescriptor(terrainSurfaceGradientListComponent->CreateDescriptor());
        }
    };

    TEST_F(TerrainSurfaceGradientListTest, SurfaceGradientReturnsSurfaceWeights)
    {
        // When there is more than one surface/weight defined and added to the component, they should all
        // be returned.  The component isn't required to return them in descending order.
        AddSurfaceGradientListToEntities();

        m_entity->Activate();
        m_gradientEntity1->Activate();
        m_gradientEntity2->Activate();

        const float gradient1Value = 0.3f;
        NiceMock<UnitTest::MockGradientRequests> mockGradientRequests1(m_gradientEntity1->GetId());
        ON_CALL(mockGradientRequests1, GetValue).WillByDefault(Return(gradient1Value));

        const float gradient2Value = 1.0f;
        NiceMock<UnitTest::MockGradientRequests> mockGradientRequests2(m_gradientEntity2->GetId());
        ON_CALL(mockGradientRequests2, GetValue).WillByDefault(Return(gradient2Value));

        AzFramework::SurfaceData::SurfaceTagWeightList weightList;
        Terrain::TerrainAreaSurfaceRequestBus::Event(
            m_entity->GetId(), &Terrain::TerrainAreaSurfaceRequestBus::Events::GetSurfaceWeights, AZ::Vector3::CreateZero(), weightList);

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


