/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Components/TerrainWorldComponent.h>
#include <Components/TerrainSystemComponent.h>

#include <AzTest/AzTest.h>

#include <Tests/Mocks/Terrain/MockTerrainDataRequestBus.h>
#include <Terrain/MockTerrain.h>
#include <TerrainTestFixtures.h>

class TerrainWorldComponentTest
    : public UnitTest::TerrainSystemTestFixture
{
protected:
    AZStd::unique_ptr<AZ::Entity> CreateAndActivateTerrainWorldComponent(const Terrain::TerrainWorldConfig& config)
    {
        auto entity = CreateEntity();
        entity->CreateComponent<Terrain::TerrainWorldComponent>(config);
        ActivateEntity(entity.get());

        // Run for one tick so that the terrain system has a chance to refresh all of its settings.
        AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.f, AZ::ScriptTimePoint{});
        return entity;
    }
};

TEST_F(TerrainWorldComponentTest, ComponentActivatesSuccessfully)
{
    auto entity = CreateAndActivateTerrainWorldComponent(Terrain::TerrainWorldConfig());
    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);

    entity.reset();
}

TEST_F(TerrainWorldComponentTest, ComponentCreatesAndActivatesTerrainSystem)
{
    // Verify that activation of the Terrain World component causes the Terrain System to get created/activated,
    // and deactivation of the Terrain World component causes the Terrain System to get destroyed/deactivated.

    using ::testing::NiceMock;
    using ::testing::AtLeast;

    NiceMock<UnitTest::MockTerrainDataNotificationListener> mockTerrainListener;
    EXPECT_CALL(mockTerrainListener, OnTerrainDataCreateBegin()).Times(AtLeast(1));
    EXPECT_CALL(mockTerrainListener, OnTerrainDataCreateEnd()).Times(AtLeast(1));
    EXPECT_CALL(mockTerrainListener, OnTerrainDataDestroyBegin()).Times(AtLeast(1));
    EXPECT_CALL(mockTerrainListener, OnTerrainDataDestroyEnd()).Times(AtLeast(1));

    auto entity = CreateAndActivateTerrainWorldComponent(Terrain::TerrainWorldConfig());
    entity.reset();
}

TEST_F(TerrainWorldComponentTest, WorldMinAndMaxAffectTerrainSystem)
{
    // Verify that the Z component of the Terrain World Component's World Min and World Max set the Terrain System's min/max.
    // The z min/max should be returned with GetTerrainHeightBounds, and since there are no terrain areas, the aabb returned
    // from worldBounds should be invalid.

    Terrain::TerrainWorldConfig config;
    config.m_minHeight = -345.0f;
    config.m_maxHeight = 678.0f;

    auto entity = CreateAndActivateTerrainWorldComponent(config);

    AzFramework::Terrain::FloatRange heightBounds;
    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
        heightBounds, &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainHeightBounds);

    AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
        worldBounds, &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainAabb);

    EXPECT_NEAR(config.m_minHeight, heightBounds.m_min, 0.001f);
    EXPECT_NEAR(config.m_maxHeight, heightBounds.m_max, 0.001f);
    EXPECT_FALSE(worldBounds.IsValid());

    entity.reset();
}

TEST_F(TerrainWorldComponentTest, QueryResolutionsAffectTerrainSystem)
{
    // Verify that the Height Query Resolution and Surface Data Query Resolution on the Terrain World Component set the query
    // resolutions in the Terrain System.

    Terrain::TerrainWorldConfig config;
    config.m_heightQueryResolution = 123.0f;
    config.m_surfaceDataQueryResolution = 456.0f;

    auto entity = CreateAndActivateTerrainWorldComponent(config);

    float heightQueryResolution = 0.0f;
    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
        heightQueryResolution, &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainHeightQueryResolution);

    float surfaceQueryResolution = 0.0f;
    AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(
        surfaceQueryResolution, &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainSurfaceDataQueryResolution);

    EXPECT_NEAR(config.m_heightQueryResolution, heightQueryResolution, 0.001f);
    EXPECT_NEAR(config.m_surfaceDataQueryResolution, surfaceQueryResolution, 0.001f);

    entity.reset();
}
