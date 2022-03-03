/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Memory/MemoryComponent.h>

#include <Components/TerrainLayerSpawnerComponent.h>

#include <Terrain/MockTerrain.h>
#include <MockAxisAlignedBoxShapeComponent.h>

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;

class LayerSpawnerComponentTest
    : public ::testing::Test
{
protected:
    AZ::ComponentApplication m_app;

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

    Terrain::TerrainLayerSpawnerComponent* AddLayerSpawnerToEntity(AZ::Entity* entity, const Terrain::TerrainLayerSpawnerConfig& config)
    {
        auto layerSpawnerComponent = entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(config);
        m_app.RegisterComponentDescriptor(layerSpawnerComponent->CreateDescriptor());

        return layerSpawnerComponent;
    }

    UnitTest::MockAxisAlignedBoxShapeComponent* AddShapeComponentToEntity(AZ::Entity* entity)
    {
        UnitTest::MockAxisAlignedBoxShapeComponent* shapeComponent = entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
        m_app.RegisterComponentDescriptor(shapeComponent->CreateDescriptor());

        return shapeComponent;
    }
};

TEST_F(LayerSpawnerComponentTest, ActivateEntityWithoutShapeFails)
{
    auto entity = CreateEntity();

    AddLayerSpawnerToEntity(entity.get(), Terrain::TerrainLayerSpawnerConfig());

    const AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
    EXPECT_FALSE(sortOutcome.IsSuccess());

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, ActivateEntityActivateSuccess)
{
    auto entity = CreateEntity();

    AddLayerSpawnerToEntity(entity.get(), Terrain::TerrainLayerSpawnerConfig());
    AddShapeComponentToEntity(entity.get());

    entity->Activate();
    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerDefaultValuesCorrect)
{
    auto entity = CreateEntity();
    AddLayerSpawnerToEntity(entity.get(), Terrain::TerrainLayerSpawnerConfig());
    AddShapeComponentToEntity(entity.get());

    entity->Activate();

    AZ::u32 priority = 999, layer = 999;
    Terrain::TerrainSpawnerRequestBus::Event(entity->GetId(), &Terrain::TerrainSpawnerRequestBus::Events::GetPriority, layer, priority);

    EXPECT_EQ(0, priority);
    EXPECT_EQ(1, layer);

    bool useGroundPlane = false;

    Terrain::TerrainSpawnerRequestBus::EventResult(
        useGroundPlane, entity->GetId(), &Terrain::TerrainSpawnerRequestBus::Events::GetUseGroundPlane);

    EXPECT_TRUE(useGroundPlane);

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerConfigValuesCorrect)
{
    auto entity = CreateEntity();

    constexpr static AZ::u32 testPriority = 15;
    constexpr static AZ::u32 testLayer = 0;

    Terrain::TerrainLayerSpawnerConfig config;
    config.m_layer = testLayer;
    config.m_priority = testPriority;
    config.m_useGroundPlane = false;

    AddLayerSpawnerToEntity(entity.get(), config);
    AddShapeComponentToEntity(entity.get());

    entity->Activate();

    AZ::u32 priority = 999, layer = 999;
    Terrain::TerrainSpawnerRequestBus::Event(entity->GetId(), &Terrain::TerrainSpawnerRequestBus::Events::GetPriority, layer, priority);

    EXPECT_EQ(testPriority, priority);
    EXPECT_EQ(testLayer, layer);

    bool useGroundPlane = true;

    Terrain::TerrainSpawnerRequestBus::EventResult(
        useGroundPlane, entity->GetId(), &Terrain::TerrainSpawnerRequestBus::Events::GetUseGroundPlane);

    EXPECT_FALSE(useGroundPlane);

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerRegisterAreaUpdatesTerrainSystem)
{
    auto entity = CreateEntity();

    NiceMock<UnitTest::MockTerrainSystemService> terrainSystem;

    // The Activate call should register the area.
    EXPECT_CALL(terrainSystem, RegisterArea(_)).Times(1);

    AddLayerSpawnerToEntity(entity.get(), Terrain::TerrainLayerSpawnerConfig());
    AddShapeComponentToEntity(entity.get());

    entity->Activate();

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerUnregisterAreaUpdatesTerrainSystem)
{
    auto entity = CreateEntity();

    NiceMock<UnitTest::MockTerrainSystemService> terrainSystem;

    // The Deactivate call should unregister the area.
    EXPECT_CALL(terrainSystem, UnregisterArea(_)).Times(1);

    AddLayerSpawnerToEntity(entity.get(), Terrain::TerrainLayerSpawnerConfig());
    AddShapeComponentToEntity(entity.get());

    entity->Activate();

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerTransformChangedUpdatesTerrainSystem)
{
    auto entity = CreateEntity();

    NiceMock<UnitTest::MockTerrainSystemService> terrainSystem;

    // The TransformChanged call should refresh the area.
    EXPECT_CALL(terrainSystem, RefreshArea(_, _)).Times(1);

    AddLayerSpawnerToEntity(entity.get(), Terrain::TerrainLayerSpawnerConfig());
    AddShapeComponentToEntity(entity.get());

    entity->Activate();

    // The component gets transform change notifications via the shape bus.
    LmbrCentral::ShapeComponentNotificationsBus::Event(
        entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerShapeChangedUpdatesTerrainSystem)
{
    auto entity = CreateEntity();

    NiceMock<UnitTest::MockTerrainSystemService> terrainSystem;

    // The ShapeChanged call should refresh the area.
    EXPECT_CALL(terrainSystem, RefreshArea(_, _)).Times(1);

    AddLayerSpawnerToEntity(entity.get(), Terrain::TerrainLayerSpawnerConfig());
    AddShapeComponentToEntity(entity.get());

    entity->Activate();

    LmbrCentral::ShapeComponentNotificationsBus::Event(
        entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

    entity.reset();
}
