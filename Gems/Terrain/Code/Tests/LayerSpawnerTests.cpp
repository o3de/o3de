/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Memory/MemoryComponent.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <Components/TerrainLayerSpawnerComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <AzTest/AzTest.h>

#include <Terrain/MockTerrain.h>
#include <MockAxisAlignedBoxShapeComponent.h>

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;

class LayerSpawnerComponentTest
    : public ::testing::Test
{
protected:
    AZ::ComponentApplication m_app;

    AZStd::unique_ptr<AZ::Entity> m_entity;
    Terrain::TerrainLayerSpawnerComponent* m_layerSpawnerComponent;
    UnitTest::MockAxisAlignedBoxShapeComponent* m_shapeComponent;
    AZStd::unique_ptr<NiceMock<UnitTest::MockTerrainSystemService>> m_terrainSystem;

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
        m_entity.reset();
        m_terrainSystem.reset();
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

        m_shapeComponent = m_entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
        m_app.RegisterComponentDescriptor(m_shapeComponent->CreateDescriptor());

        ASSERT_TRUE(m_layerSpawnerComponent);
        ASSERT_TRUE(m_shapeComponent);
    }

    void CreateMockTerrainSystem()
    {
        m_terrainSystem = AZStd::make_unique<NiceMock<UnitTest::MockTerrainSystemService>>();
    }
};

TEST_F(LayerSpawnerComponentTest, ActivatEntityActivateSuccess)
{
    CreateEntity();
    AddLayerSpawnerAndShapeComponentToEntity();

    m_entity->Activate();
    EXPECT_EQ(m_entity->GetState(), AZ::Entity::State::Active);
     
    m_entity->Deactivate();
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

    EXPECT_TRUE(useGroundPlane);

    m_entity->Deactivate();
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

    EXPECT_FALSE(useGroundPlane);

    m_entity->Deactivate();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerRegisterAreaUpdatesTerrainSystem)
{
    CreateEntity();

    CreateMockTerrainSystem();

    // The Activate call should register the area.
    EXPECT_CALL(*m_terrainSystem, RegisterArea(_)).Times(1);

    AddLayerSpawnerAndShapeComponentToEntity();

    m_entity->Activate();

    m_entity->Deactivate();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerUnregisterAreaUpdatesTerrainSystem)
{
    CreateEntity();

    CreateMockTerrainSystem();

    // The Deactivate call should unregister the area.
    EXPECT_CALL(*m_terrainSystem, UnregisterArea(_)).Times(1);

    AddLayerSpawnerAndShapeComponentToEntity();

    m_entity->Activate();

    m_entity->Deactivate();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerTransformChangedUpdatesTerrainSystem)
{
    CreateEntity();

    CreateMockTerrainSystem();

    // The TransformChanged call should refresh the area.
    EXPECT_CALL(*m_terrainSystem, RefreshArea(_, _)).Times(1);

    AddLayerSpawnerAndShapeComponentToEntity();

    m_entity->Activate();

    // The component gets transform change notifications via the shape bus.
    LmbrCentral::ShapeComponentNotificationsBus::Event(
        m_entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);

    m_entity->Deactivate();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerShapeChangedUpdatesTerrainSystem)
{
    CreateEntity();

    CreateMockTerrainSystem();

    // The ShapeChanged call should refresh the area.
    EXPECT_CALL(*m_terrainSystem, RefreshArea(_, _)).Times(1);

    AddLayerSpawnerAndShapeComponentToEntity();

    m_entity->Activate();

   LmbrCentral::ShapeComponentNotificationsBus::Event(
        m_entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

    m_entity->Deactivate();
}
