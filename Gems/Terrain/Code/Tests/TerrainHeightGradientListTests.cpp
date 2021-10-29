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

#include <Components/TerrainHeightGradientListComponent.h>

#include <MockAxisAlignedBoxShapeComponent.h>
#include <GradientSignal/Ebuses/MockGradientRequestBus.h>
#include <LmbrCentral/Shape/MockShapes.h>
#include <Terrain/MockTerrainLayerSpawner.h>
#include <Terrain/MockTerrain.h>

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;

class TerrainHeightGradientListComponentTest : public ::testing::Test
{
protected:
    AZ::ComponentApplication m_app;

    AZStd::unique_ptr<AZ::Entity> m_entity;

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

    void CreateEntity()
    {
        m_entity = AZStd::make_unique<AZ::Entity>();
        ASSERT_TRUE(m_entity);

        // Create the required box component.
        UnitTest::MockAxisAlignedBoxShapeComponent* boxComponent = m_entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
        m_app.RegisterComponentDescriptor(boxComponent->CreateDescriptor());

        // Create the TerrainHeightGradientListComponent with an entity in its configuration.
        Terrain::TerrainHeightGradientListConfig config;
        config.m_gradientEntities.push_back(m_entity->GetId());

        Terrain::TerrainHeightGradientListComponent* heightGradientListComponent = m_entity->CreateComponent<Terrain::TerrainHeightGradientListComponent>(config);
        m_app.RegisterComponentDescriptor(heightGradientListComponent->CreateDescriptor());

        // Create a MockTerrainLayerSpawnerComponent to provide the required TerrainAreaService.
        UnitTest::MockTerrainLayerSpawnerComponent* layerSpawner = m_entity->CreateComponent<UnitTest::MockTerrainLayerSpawnerComponent>();
        m_app.RegisterComponentDescriptor(layerSpawner->CreateDescriptor());

        m_entity->Init();
    }
};

TEST_F(TerrainHeightGradientListComponentTest, ActivateEntityActivateSuccess)
{
    // Check that the entity activates.
    CreateEntity();

    m_entity->Activate();
    EXPECT_EQ(m_entity->GetState(), AZ::Entity::State::Active);

    m_entity.reset();
}

TEST_F(TerrainHeightGradientListComponentTest, TerrainHeightGradientRefreshesTerrainSystem)
{
    // Check that the HeightGradientListComponent informs the TerrainSystem when the composition changes.
    CreateEntity();

    m_entity->Activate();

    NiceMock<UnitTest::MockTerrainSystemService> terrainSystem;

    // As the TerrainHeightGradientListComponent subscribes to the dependency monitor, RefreshArea will be called twice:
    // once due to OnCompositionChanged being picked up by the the dependency monitor and resending the notification,
    // and once when the HeightGradientListComponent gets the OnCompositionChanged directly through the DependencyNotificationBus.
    EXPECT_CALL(terrainSystem, RefreshArea(_, _)).Times(2);

    LmbrCentral::DependencyNotificationBus::Event(m_entity->GetId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

    // Stop the EXPECT_CALL check now, as OnCompositionChanged will get called twice again during the reset.
    Mock::VerifyAndClearExpectations(&terrainSystem);

    m_entity.reset();
}

TEST_F(TerrainHeightGradientListComponentTest, TerrainHeightGradientListReturnsHeights)
{
    // Check that the HeightGradientListComponent returns expected height values.
    CreateEntity();

    NiceMock<UnitTest::MockTerrainAreaHeightRequests> heightfieldRequestBus(m_entity->GetId());

    m_entity->Activate();

    const float mockGradientValue = 0.25f;
    NiceMock<UnitTest::MockGradientRequests> gradientRequests(m_entity->GetId());
    ON_CALL(gradientRequests, GetValue).WillByDefault(Return(mockGradientValue));

    // Setup a mock to provide the encompassing Aabb to the HeightGradientListComponent.
    const float min = 0.0f;
    const float max = 1000.0f;
    const AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(min), AZ::Vector3(max));
    NiceMock<UnitTest::MockShapeComponentRequests> mockShapeRequests(m_entity->GetId());
    ON_CALL(mockShapeRequests, GetEncompassingAabb).WillByDefault(Return(aabb));

    const float worldMax = 10000.0f;
    const AZ::Aabb worldAabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(min), AZ::Vector3(worldMax));
    NiceMock<UnitTest::MockTerrainDataRequests> mockterrainDataRequests;
    ON_CALL(mockterrainDataRequests, GetTerrainHeightQueryResolution).WillByDefault(Return(AZ::Vector2(1.0f)));
    ON_CALL(mockterrainDataRequests, GetTerrainAabb).WillByDefault(Return(worldAabb));

    // Ensure the cached values in the HeightGradientListComponent are up to date.
    LmbrCentral::DependencyNotificationBus::Event(m_entity->GetId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

    const AZ::Vector3 inPosition = AZ::Vector3::CreateZero();
    AZ::Vector3 outPosition = AZ::Vector3::CreateZero();
    bool terrainExists = false;
    Terrain::TerrainAreaHeightRequestBus::Event(m_entity->GetId(), &Terrain::TerrainAreaHeightRequestBus::Events::GetHeight, inPosition, outPosition, terrainExists);

    const float height = outPosition.GetZ();

    EXPECT_NEAR(height, mockGradientValue * max, 0.01f);

    m_entity.reset();
}

