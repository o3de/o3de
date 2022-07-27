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
#include <TerrainTestFixtures.h>

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;

class LayerSpawnerComponentTest
    : public UnitTest::TerrainTestFixture
{
protected:
    constexpr static inline float TestBoxHalfBounds = 128.0f;
};

TEST_F(LayerSpawnerComponentTest, ActivateEntityWithoutShapeFails)
{
    auto entity = CreateEntity();
    entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(Terrain::TerrainLayerSpawnerConfig());

    const AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
    EXPECT_FALSE(sortOutcome.IsSuccess());

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, ActivateEntityActivateSuccess)
{
    auto entity = CreateTestBoxEntity(TestBoxHalfBounds);
    entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(Terrain::TerrainLayerSpawnerConfig());

    ActivateEntity(entity.get());
    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerDefaultValuesCorrect)
{
    auto entity = CreateTestBoxEntity(TestBoxHalfBounds);
    entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(Terrain::TerrainLayerSpawnerConfig());

    ActivateEntity(entity.get());

    int32_t priority = 999;
    uint32_t layer = 999;
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
    auto entity = CreateTestBoxEntity(TestBoxHalfBounds);

    constexpr static AZ::u32 testPriority = 15;
    constexpr static AZ::u32 testLayer = 0;

    Terrain::TerrainLayerSpawnerConfig config;
    config.m_layer = testLayer;
    config.m_priority = testPriority;
    config.m_useGroundPlane = false;

    entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(config);

    ActivateEntity(entity.get());

    int32_t priority = 999;
    uint32_t layer = 999;
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
    auto entity = CreateTestBoxEntity(TestBoxHalfBounds);

    NiceMock<UnitTest::MockTerrainSystemService> terrainSystem;

    // The Activate call should register the area.
    EXPECT_CALL(terrainSystem, RegisterArea(_)).Times(1);

    entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(Terrain::TerrainLayerSpawnerConfig());

    ActivateEntity(entity.get());

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerUnregisterAreaUpdatesTerrainSystem)
{
    auto entity = CreateTestBoxEntity(TestBoxHalfBounds);

    NiceMock<UnitTest::MockTerrainSystemService> terrainSystem;

    // The Deactivate call should unregister the area.
    EXPECT_CALL(terrainSystem, UnregisterArea(_)).Times(1);

    entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(Terrain::TerrainLayerSpawnerConfig());

    ActivateEntity(entity.get());

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerTransformChangedUpdatesTerrainSystem)
{
    auto entity = CreateTestBoxEntity(TestBoxHalfBounds);

    NiceMock<UnitTest::MockTerrainSystemService> terrainSystem;

    // The TransformChanged call should refresh the area.
    EXPECT_CALL(terrainSystem, RefreshArea(_, _)).Times(1);

    entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(Terrain::TerrainLayerSpawnerConfig());

    ActivateEntity(entity.get());

    // The component gets transform change notifications via the shape bus.
    LmbrCentral::ShapeComponentNotificationsBus::Event(
        entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);

    entity.reset();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerShapeChangedUpdatesTerrainSystem)
{
    auto entity = CreateTestBoxEntity(TestBoxHalfBounds);

    NiceMock<UnitTest::MockTerrainSystemService> terrainSystem;

    // The ShapeChanged call should refresh the area.
    EXPECT_CALL(terrainSystem, RefreshArea(_, _)).Times(1);

    entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(Terrain::TerrainLayerSpawnerConfig());

    ActivateEntity(entity.get());

    LmbrCentral::ShapeComponentNotificationsBus::Event(
        entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

    entity.reset();
}
