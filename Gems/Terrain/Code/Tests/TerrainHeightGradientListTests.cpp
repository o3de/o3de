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
#include <Tests/Mocks/Terrain/MockTerrainDataRequestBus.h>

using ::testing::_;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;

class TerrainHeightGradientListComponentTest : public ::testing::Test
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

    Terrain::TerrainHeightGradientListComponent* AddHeightGradientListToEntity(AZ::Entity* entity)
    {
        // Create the TerrainHeightGradientListComponent with an entity in its configuration.
        Terrain::TerrainHeightGradientListConfig config;
        config.m_gradientEntities.push_back(entity->GetId());

        auto heightGradientListComponent = entity->CreateComponent<Terrain::TerrainHeightGradientListComponent>(config);
        m_app.RegisterComponentDescriptor(heightGradientListComponent->CreateDescriptor());

        return heightGradientListComponent;
    }

    void AddRequiredComponetsToEntity(AZ::Entity* entity)
    {
        // Create the required box component.
        UnitTest::MockAxisAlignedBoxShapeComponent* boxComponent = entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
        m_app.RegisterComponentDescriptor(boxComponent->CreateDescriptor());

        // Create a MockTerrainLayerSpawnerComponent to provide the required TerrainAreaService.
        UnitTest::MockTerrainLayerSpawnerComponent* layerSpawner = entity->CreateComponent<UnitTest::MockTerrainLayerSpawnerComponent>();
        m_app.RegisterComponentDescriptor(layerSpawner->CreateDescriptor());
    }
};

TEST_F(TerrainHeightGradientListComponentTest, MissingRequiredComponentsActivateFailure)
{
    auto entity = CreateEntity();

    AddHeightGradientListToEntity(entity.get());

    const AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
    EXPECT_FALSE(sortOutcome.IsSuccess());
}

TEST_F(TerrainHeightGradientListComponentTest, ActivateEntityActivateSuccess)
{
    // Check that the entity activates.
    auto entity = CreateEntity();

    AddHeightGradientListToEntity(entity.get());

    AddRequiredComponetsToEntity(entity.get());

    entity->Activate();
    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
}

TEST_F(TerrainHeightGradientListComponentTest, TerrainHeightGradientRefreshesTerrainSystem)
{
    // Check that the HeightGradientListComponent informs the TerrainSystem when the composition changes.
    auto entity = CreateEntity();

    AddHeightGradientListToEntity(entity.get());

    AddRequiredComponetsToEntity(entity.get());

    entity->Activate();

    NiceMock<UnitTest::MockTerrainSystemService> terrainSystem;

    // As the TerrainHeightGradientListComponent subscribes to the dependency monitor, RefreshArea will be called twice:
    // once due to OnCompositionChanged being picked up by the the dependency monitor and resending the notification,
    // and once when the HeightGradientListComponent gets the OnCompositionChanged directly through the DependencyNotificationBus.
    EXPECT_CALL(terrainSystem, RefreshArea(_, _)).Times(2);

    LmbrCentral::DependencyNotificationBus::Event(entity->GetId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

    // Stop the EXPECT_CALL check now, as OnCompositionChanged will get called twice again during the reset.
    Mock::VerifyAndClearExpectations(&terrainSystem);
}

TEST_F(TerrainHeightGradientListComponentTest, TerrainHeightGradientListReturnsHeights)
{
    // Check that the HeightGradientListComponent returns expected height values.
    auto entity = CreateEntity();

    AddHeightGradientListToEntity(entity.get());

    AddRequiredComponetsToEntity(entity.get());

    NiceMock<UnitTest::MockTerrainAreaHeightRequests> heightfieldRequestBus(entity->GetId());

    entity->Activate();

    const float mockGradientValue = 0.25f;
    NiceMock<UnitTest::MockGradientRequests> gradientRequests(entity->GetId());
    ON_CALL(gradientRequests, GetValue).WillByDefault(Return(mockGradientValue));

    // Setup a mock to provide the encompassing Aabb to the HeightGradientListComponent.
    const float min = 0.0f;
    const float max = 1000.0f;
    const AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(min), AZ::Vector3(max));
    NiceMock<UnitTest::MockShapeComponentRequests> mockShapeRequests(entity->GetId());
    ON_CALL(mockShapeRequests, GetEncompassingAabb).WillByDefault(Return(aabb));

    const float worldMax = 10000.0f;
    const AZ::Aabb worldAabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(min), AZ::Vector3(worldMax));
    NiceMock<UnitTest::MockTerrainDataRequests> mockterrainDataRequests;
    ON_CALL(mockterrainDataRequests, GetTerrainHeightQueryResolution).WillByDefault(Return(AZ::Vector2(1.0f)));
    ON_CALL(mockterrainDataRequests, GetTerrainAabb).WillByDefault(Return(worldAabb));

    // Ensure the cached values in the HeightGradientListComponent are up to date.
    LmbrCentral::DependencyNotificationBus::Event(entity->GetId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

    const AZ::Vector3 inPosition = AZ::Vector3::CreateZero();
    AZ::Vector3 outPosition = AZ::Vector3::CreateZero();
    bool terrainExists = false;
    Terrain::TerrainAreaHeightRequestBus::Event(
        entity->GetId(), &Terrain::TerrainAreaHeightRequestBus::Events::GetHeight, inPosition, outPosition, terrainExists);

    const float height = outPosition.GetZ();

    EXPECT_NEAR(height, mockGradientValue * max, 0.01f);
}

