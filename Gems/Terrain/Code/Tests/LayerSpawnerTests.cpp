/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/MemoryComponent.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <Components/TerrainLayerSpawnerComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <AzTest/AzTest.h>

#include <TerrainMocks.h>

class LayerSpawnerComponentTest
    : public ::testing::Test
{
protected:
    AZ::ComponentApplication m_app;

    AZStd::unique_ptr<AZ::Entity> m_entity;
    Terrain::TerrainLayerSpawnerComponent* m_layerSpawnerComponent;
    UnitTest::MockBoxShapeComponent* m_shapeComponent;
    AZStd::unique_ptr<UnitTest::MockTerrainSystem> m_terrainSystem;

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
        if (m_terrainSystem)
        {
            m_terrainSystem->Deactivate();
        }
        m_app.Destroy();
    }

    void CreateEntity()
    {
        m_entity = AZStd::make_unique<AZ::Entity>();
        m_entity->Init();

        ASSERT_TRUE(m_entity);
    }

    void AddLayerSpawnerAndShapeComponentToEntity()
    {
        AddLayerSpawnerAndShapeComponentToEntity(Terrain::TerrainLayerSpawnerConfig());
    }

    void AddLayerSpawnerAndShapeComponentToEntity(const Terrain::TerrainLayerSpawnerConfig& config)
    {
        m_layerSpawnerComponent = m_entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(config);
        m_app.RegisterComponentDescriptor(m_layerSpawnerComponent->CreateDescriptor());

        m_shapeComponent = m_entity->CreateComponent<UnitTest::MockBoxShapeComponent>();
        m_app.RegisterComponentDescriptor(m_shapeComponent->CreateDescriptor());

        ASSERT_TRUE(m_layerSpawnerComponent);
        ASSERT_TRUE(m_shapeComponent);
    }

    void ResetEntity()
    {
        m_entity->Deactivate();
        m_entity->Reset();
    }

    void CreateMockTerrainSystem()
    {
        m_terrainSystem = AZStd::make_unique<UnitTest::MockTerrainSystem>();
        m_terrainSystem->Activate();
    }
};

TEST_F(LayerSpawnerComponentTest, SanityTest)
{
    ASSERT_TRUE(true);
}

TEST_F(LayerSpawnerComponentTest, ActivatEntityActivateSuccess)
{
    CreateEntity();
    AddLayerSpawnerAndShapeComponentToEntity();

    m_entity->Activate();
    EXPECT_TRUE(m_entity->GetState() == AZ::Entity::State::Active);
     
    ResetEntity();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerDefaultValuesCorrect)
{
    CreateEntity();
    AddLayerSpawnerAndShapeComponentToEntity();

    m_entity->Activate();

    AZ::u32 priority = 999, layer = 999;
    Terrain::TerrainSpawnerRequestBus::Event(m_entity->GetId(), &Terrain::TerrainSpawnerRequestBus::Events::GetPriority, layer, priority);

    EXPECT_EQ(0, priority);
    EXPECT_EQ(1, layer);

    bool useGroundPlane = false;

    Terrain::TerrainSpawnerRequestBus::EventResult(useGroundPlane, m_entity->GetId(),  &Terrain::TerrainSpawnerRequestBus::Events::GetUseGroundPlane);

    EXPECT_EQ(true, useGroundPlane);

    ResetEntity();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerConfigValuesCorrect)
{
    CreateEntity();

    constexpr static AZ::u32 testPriority = 15;
    constexpr static AZ::u32 testLayer = 0;

    Terrain::TerrainLayerSpawnerConfig config;
    config.m_layer = testLayer;
    config.m_priority = testPriority;
    config.m_useGroundPlane = false;

    AddLayerSpawnerAndShapeComponentToEntity(config);

    m_entity->Activate();

    AZ::u32 priority = 999, layer = 999;
    Terrain::TerrainSpawnerRequestBus::Event(m_entity->GetId(), &Terrain::TerrainSpawnerRequestBus::Events::GetPriority, layer, priority);

    EXPECT_EQ(testPriority, priority);
    EXPECT_EQ(testLayer, layer);

    bool useGroundPlane = true;

    Terrain::TerrainSpawnerRequestBus::EventResult(
        useGroundPlane, m_entity->GetId(), &Terrain::TerrainSpawnerRequestBus::Events::GetUseGroundPlane);

    EXPECT_EQ(false, useGroundPlane);

    ResetEntity();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerRegisterAreaUpdatesTerrainSystem)
{
    CreateEntity();

    CreateMockTerrainSystem();

    AddLayerSpawnerAndShapeComponentToEntity();

    m_entity->Activate();

    // The Activate call should have registered the area.
    EXPECT_EQ(1, m_terrainSystem->m_registerAreaCalledCount);

    ResetEntity();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerUnregisterAreaUpdatesTerrainSystem)
{
    CreateEntity();

    CreateMockTerrainSystem();

    AddLayerSpawnerAndShapeComponentToEntity();

    m_entity->Activate();

    m_layerSpawnerComponent->Deactivate();

    // The Deactivate call should have unregistered the area.
    EXPECT_EQ(1, m_terrainSystem->m_unregisterAreaCalledCount);

    ResetEntity();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerTransformChangedUpdatesTerrainSystem)
{
    CreateEntity();

    CreateMockTerrainSystem();

    AddLayerSpawnerAndShapeComponentToEntity();

    m_entity->Activate();

    AZ::TransformNotificationBus::Event(
        m_entity->GetId(), &AZ::TransformNotificationBus::Events::OnTransformChanged, AZ::Transform(), AZ::Transform());

    EXPECT_EQ(1, m_terrainSystem->m_refreshAreaCalledCount);

    ResetEntity();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerShapeChangedUpdatesTerrainSystem)
{
    CreateEntity();

    CreateMockTerrainSystem();

    AddLayerSpawnerAndShapeComponentToEntity();

    m_entity->Activate();

   LmbrCentral::ShapeComponentNotificationsBus::Event(
        m_entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

    EXPECT_EQ(1, m_terrainSystem->m_refreshAreaCalledCount);

    ResetEntity();
}
