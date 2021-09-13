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
#include <TerrainMocks.h>

using ::testing::AtLeast;

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

    void CreateEntity()
    {
        m_entity = AZStd::make_unique<AZ::Entity>();
        m_entity->Init();

        ASSERT_TRUE(m_entity);
    }

    void ResetEntity()
    {
        m_entity->Deactivate();
        m_entity->Reset();
    }
};

TEST_F(TerrainSystemTest, TrivialCreateDestroy)
{
    m_terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
}

TEST_F(TerrainSystemTest, TrivialActivateDeactivate)
{
    m_terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
    m_terrainSystem->Activate();
    m_terrainSystem->Deactivate();
}

TEST_F(TerrainSystemTest, CreateEventsCalledOnActivation)
{
    UnitTest::MockTerrainListener mockTerrainListener;
    EXPECT_CALL(mockTerrainListener, OnTerrainDataCreateBegin()).Times(AtLeast(1));
    EXPECT_CALL(mockTerrainListener, OnTerrainDataCreateEnd()).Times(AtLeast(1));

    m_terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
    m_terrainSystem->Activate();
}

TEST_F(TerrainSystemTest, DestroyEventsCalledOnDeactivation)
{
    UnitTest::MockTerrainListener mockTerrainListener;
    EXPECT_CALL(mockTerrainListener, OnTerrainDataCreateBegin()).Times(AtLeast(1));
    EXPECT_CALL(mockTerrainListener, OnTerrainDataCreateEnd()).Times(AtLeast(1));
    EXPECT_CALL(mockTerrainListener, OnTerrainDataDestroyBegin()).Times(AtLeast(1));
    EXPECT_CALL(mockTerrainListener, OnTerrainDataDestroyEnd()).Times(AtLeast(1));

    m_terrainSystem = AZStd::make_unique<Terrain::TerrainSystem>();
    m_terrainSystem->Activate();
    m_terrainSystem->Deactivate();
}
