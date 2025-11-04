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

TEST_F(LayerSpawnerComponentTest, LayerSpawnerCreatesGroundPlaneWhenUseGroundPlaneSet)
{
    // Create a terrain world with height bounds from -128 to 128.
    const float queryResolution = 1.0f;
    const AzFramework::Terrain::FloatRange heightBounds = { -128.0f, 128.0f };
    auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution, heightBounds);

    // Create a terrain spawner with useGroundPlane enabled and a box from 0 to 32.
    Terrain::TerrainLayerSpawnerConfig config;
    config.m_useGroundPlane = true;
    const float SpawnerBoxHalfBounds = 16.0f;
    auto entity = CreateTestBoxEntity(SpawnerBoxHalfBounds);
    entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(config);
    ActivateEntity(entity.get());

    // Querying for terrain heights at the center of the spawner box should give us a valid point with a height equal to the min
    // height of the spawner box, not the min height of the terrain world.
    bool terrainExists = false;
    AZStd::array<AZ::Vector3, 1> positionList = { AZ::Vector3(16.0f, 16.0f, 16.0f) };
    float height = terrainSystem->GetHeight(
        positionList[0], AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &terrainExists);

    EXPECT_TRUE(terrainExists);
    EXPECT_EQ(height, 0.0f);

    // Verify that the results from QueryList also use the "useGroundPlane" setting.
    terrainSystem->QueryList(
        positionList,
        AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights,
        [](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
        {
            EXPECT_TRUE(terrainExists);
            EXPECT_EQ(surfacePoint.m_position.GetZ(), 0.0f);
        },
        AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT);


    entity.reset();
    terrainSystem.reset();
}

TEST_F(LayerSpawnerComponentTest, LayerSpawnerDoesNotCreateGroundPlaneWhenUseGroundPlaneNotSet)
{
    // Create a terrain world with height bounds from -128 to 128.
    const float queryResolution = 1.0f;
    const AzFramework::Terrain::FloatRange heightBounds = { -128.0f, 128.0f };
    auto terrainSystem = CreateAndActivateTerrainSystem(queryResolution, heightBounds);

    // Create a terrain spawner with useGroundPlane disabled and a box from 0 to 32.
    Terrain::TerrainLayerSpawnerConfig config;
    config.m_useGroundPlane = false;
    const float SpawnerBoxHalfBounds = 16.0f;
    auto entity = CreateTestBoxEntity(SpawnerBoxHalfBounds);
    entity->CreateComponent<Terrain::TerrainLayerSpawnerComponent>(config);
    ActivateEntity(entity.get());

    // Querying for terrain heights at the center of the spawner box should give us a invalid point because useGroundPlane isn't enabled.
    bool terrainExists = false;
    AZStd::array<AZ::Vector3, 1> positionList = { AZ::Vector3(16.0f, 16.0f, 16.0f) };
    float height = terrainSystem->GetHeight(positionList[0], AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &terrainExists);

    EXPECT_FALSE(terrainExists);
    EXPECT_EQ(height, -128.0f);

    // Verify that the results from QueryList also use the "useGroundPlane" setting.
    terrainSystem->QueryList(
        positionList,
        AzFramework::Terrain::TerrainDataRequests::TerrainDataMask::Heights,
        [](const AzFramework::SurfaceData::SurfacePoint& surfacePoint, bool terrainExists)
        {
            EXPECT_FALSE(terrainExists);
            EXPECT_EQ(surfacePoint.m_position.GetZ(), -128.0f);
        },
        AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT);


    entity.reset();
    terrainSystem.reset();
}
