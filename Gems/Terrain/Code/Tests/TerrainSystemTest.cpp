/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/MemoryComponent.h>

#include <AzTest/AzTest.h>

#include <TerrainSystem/TerrainSystem.h>
#include <Components/TerrainLayerSpawnerComponent.h>
#include <Components/TerrainHeightGradientListComponent.h>

#include <Terrain/MockTerrain.h>
#include <MockAxisAlignedBoxShapeComponent.h>

using ::testing::AtLeast;
using ::testing::NiceMock;
using ::testing::Return;

class TerrainSystemTest : public ::testing::Test
{
protected:
    AZ::ComponentApplication m_app;
    AZStd::unique_ptr<Terrain::TerrainSystem> m_terrainSystem;

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
        m_terrainSystem.reset();
        m_app.Destroy();
    }

    AZStd::unique_ptr<AZ::Entity> CreateEntity()
    {
        return AZStd::make_unique<AZ::Entity>();
    }

    void ActivateEntity(AZ::Entity* entity)
    {
        entity->Init();
        EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());

        entity->Activate();
        EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());
    }

    template<typename Component, typename Configuration>
    AZ::Component* CreateComponent(AZ::Entity* entity, const Configuration& config)
    {
        m_app.RegisterComponentDescriptor(Component::CreateDescriptor());
        return entity->CreateComponent<Component>(config);
    }

    template<typename Component>
    AZ::Component* CreateComponent(AZ::Entity* entity)
    {
        m_app.RegisterComponentDescriptor(Component::CreateDescriptor());
        return entity->CreateComponent<Component>();
    }
};

TEST_F(TerrainSystemTest, TrivialCreateDestroy)
{
    // Trivially verify that the terrain system can successfully be constructed and destructed without errors.

    m_terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
}

TEST_F(TerrainSystemTest, TrivialActivateDeactivate)
{
    // Verify that the terrain system can be activated and deactivated without errors.

    m_terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
    m_terrainSystem->Activate();
    m_terrainSystem->Deactivate();
}

TEST_F(TerrainSystemTest, CreateEventsCalledOnActivation)
{
    // Verify that when the terrain system is activated, the OnTerrainDataCreate* ebus notifications are generated.

    NiceMock<UnitTest::MockTerrainDataNotificationListener> mockTerrainListener;
    EXPECT_CALL(mockTerrainListener, OnTerrainDataCreateBegin()).Times(AtLeast(1));
    EXPECT_CALL(mockTerrainListener, OnTerrainDataCreateEnd()).Times(AtLeast(1));

    m_terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
    m_terrainSystem->Activate();
}

TEST_F(TerrainSystemTest, DestroyEventsCalledOnDeactivation)
{
    // Verify that when the terrain system is deactivated, the OnTerrainDataDestroy* ebus notifications are generated.

    NiceMock<UnitTest::MockTerrainDataNotificationListener> mockTerrainListener;
    EXPECT_CALL(mockTerrainListener, OnTerrainDataDestroyBegin()).Times(AtLeast(1));
    EXPECT_CALL(mockTerrainListener, OnTerrainDataDestroyEnd()).Times(AtLeast(1));

    m_terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
    m_terrainSystem->Activate();
    m_terrainSystem->Deactivate();
}

TEST_F(TerrainSystemTest, TerrainDoesNotExistWhenNoTerrainLayerSpawnersAreRegistered)
{
    // For the terrain system, terrain should only exist where terrain layer spawners are present.

    // Verify that in the active terrain system, if there are no terrain layer spawners, any arbitrary point
    // will return false for terrainExists, returns a height equal to the min world bounds of the terrain system, and returns
    // a normal facing up the Z axis.

    // Create the terrain system and give it one tick to fully initialize itself.
    m_terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
    m_terrainSystem->Activate();
    AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.f, AZ::ScriptTimePoint{});

    AZ::Aabb worldBounds = m_terrainSystem->GetTerrainAabb();

    // Loop through several points within the world bounds, including on the edges, and verify that they all return false for
    // terrainExists with default heights and normals.
    for (float y = worldBounds.GetMin().GetY(); y <= worldBounds.GetMax().GetY(); y += (worldBounds.GetExtents().GetY() / 4.0f))
    {
        for (float x = worldBounds.GetMin().GetX(); x <= worldBounds.GetMax().GetX(); x += (worldBounds.GetExtents().GetX() / 4.0f))
        {
            AZ::Vector3 position(x, y, 0.0f);
            bool terrainExists = true;
            float height = m_terrainSystem->GetHeight(position, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &terrainExists);
            EXPECT_FALSE(terrainExists);
            EXPECT_EQ(height, worldBounds.GetMin().GetZ());

            terrainExists = true;
            AZ::Vector3 normal = m_terrainSystem->GetNormal(
                position, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &terrainExists);
            EXPECT_FALSE(terrainExists);
            EXPECT_EQ(normal, AZ::Vector3::CreateAxisZ());

            bool isHole = m_terrainSystem->GetIsHoleFromFloats(
                position.GetX(), position.GetY(), AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT);
            EXPECT_TRUE(isHole);
        }
    }
}

TEST_F(TerrainSystemTest, TerrainExistsOnlyWithinTerrainLayerSpawnerBounds)
{
    // Verify that the presence of a TerrainLayerSpawner causes terrain to exist in (and *only* in) the box where the TerrainLayerSpawner
    // is defined.

    // The terrain system should only query Heights and Normals from the TerrainAreaHeightRequest bus within the
    // TerrainLayerSpawner region, and so those values should only get returned from GetHeight/GetNormal for queries inside that region.

    // Create the base entity with a mock Box Shape and a Terrain Layer Spawner.
    auto entity = CreateEntity();
    CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>(entity.get());
    CreateComponent<Terrain::TerrainLayerSpawnerComponent>(entity.get());

    // Set up the box shape to return a box from (0,0,5) to (10, 10, 15)
    AZ::Aabb spawnerBox = AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, 5.0f, 10.0f, 10.0f, 15.0f);
    NiceMock<UnitTest::MockBoxShapeComponentRequests> boxShapeRequests(entity->GetId());
    NiceMock<UnitTest::MockShapeComponentRequests> shapeRequests(entity->GetId());
    ON_CALL(shapeRequests, GetEncompassingAabb).WillByDefault(Return(spawnerBox));

    // Set up a mock height provider that always returns 5.0 and a normal of Y-up.
    const float spawnerHeight = 5.0f;
    const AZ::Vector3 spawnerNormal = AZ::Vector3::CreateAxisY();
    NiceMock<UnitTest::MockTerrainAreaHeightRequests> terrainAreaHeightRequests(entity->GetId());
    ON_CALL(terrainAreaHeightRequests, GetHeight)
        .WillByDefault(
            [spawnerHeight](const AZ::Vector3& inPosition, AZ::Vector3& outPosition, bool& terrainExists)
            {
                outPosition = inPosition;
                outPosition.SetZ(spawnerHeight);
                terrainExists = true;
            });
    ON_CALL(terrainAreaHeightRequests, GetNormal)
        .WillByDefault(
            [spawnerNormal](
                [[maybe_unused]] const AZ::Vector3& inPosition, AZ::Vector3& outNormal, bool& terrainExists)
            {
                outNormal = spawnerNormal;
                terrainExists = true;
            });

    ActivateEntity(entity.get());

    // Verify that terrain exists within the layer spawner bounds, and doesn't exist outside of it.

    // Create the terrain system and give it one tick to fully initialize itself.
    m_terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
    m_terrainSystem->Activate();
    AZ::TickBus::Broadcast(&AZ::TickBus::Events::OnTick, 0.f, AZ::ScriptTimePoint{});

    AZ::Aabb worldBounds = m_terrainSystem->GetTerrainAabb();

    // Create a box that's twice as big as the layer spawner box.  Loop through it and verify that points within the layer box contain
    // terrain and the expected height & normal values, and points outside the layer box don't contain terrain.
    const AZ::Aabb encompassingBox =
        AZ::Aabb::CreateFromMinMax(spawnerBox.GetMin() - (spawnerBox.GetExtents() / 2.0f),
            spawnerBox.GetMax() + (spawnerBox.GetExtents() / 2.0f));

    for (float y = encompassingBox.GetMin().GetY(); y < encompassingBox.GetMax().GetY(); y += 1.0f)  
    {
        for (float x = encompassingBox.GetMin().GetX(); x < encompassingBox.GetMax().GetX(); x += 1.0f)
        {
            AZ::Vector3 position(x, y, 0.0f);
            bool heightQueryTerrainExists = false;
            bool normalQueryTerrainExists = false;
            float height =
                m_terrainSystem->GetHeight(position, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &heightQueryTerrainExists);
            AZ::Vector3 normal =
                m_terrainSystem->GetNormal(position, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &normalQueryTerrainExists);
            bool isHole = m_terrainSystem->GetIsHoleFromFloats(
                position.GetX(), position.GetY(), AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT);

            if (spawnerBox.Contains(AZ::Vector3(position.GetX(), position.GetY(), spawnerBox.GetMin().GetZ())))
            {
                EXPECT_TRUE(heightQueryTerrainExists);
                EXPECT_TRUE(normalQueryTerrainExists);
                EXPECT_FALSE(isHole);
                EXPECT_EQ(height, spawnerHeight);
                EXPECT_EQ(normal, spawnerNormal);
            }
            else
            {
                EXPECT_FALSE(heightQueryTerrainExists);
                EXPECT_FALSE(normalQueryTerrainExists);
                EXPECT_TRUE(isHole);
            }
        }
    }
}

