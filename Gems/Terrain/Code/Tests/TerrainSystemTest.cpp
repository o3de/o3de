/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/MemoryComponent.h>

#include <TerrainSystem/TerrainSystem.h>

#include <AzTest/AzTest.h>
#include <Terrain/MockTerrain.h>

using ::testing::AtLeast;
using ::testing::NiceMock;

class TerrainSystemTest : public ::testing::Test
{
protected:
    AZ::ComponentApplication m_app;

    AZStd::unique_ptr<AZ::Entity> m_entity;
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

            AZ::Vector3 normal = m_terrainSystem->GetNormal(
                position, AzFramework::Terrain::TerrainDataRequests::Sampler::EXACT, &terrainExists);
            EXPECT_FALSE(terrainExists);
            EXPECT_EQ(normal, AZ::Vector3::CreateAxisZ());
        }
    }
}

