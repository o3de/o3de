/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzTest/AzTest.h>

#include <Components/TerrainHeightGradientListComponent.h>

#include <MockAxisAlignedBoxShapeComponent.h>
#include <GradientSignal/Ebuses/MockGradientRequestBus.h>
#include <LmbrCentral/Shape/MockShapes.h>
#include <Terrain/MockTerrainLayerSpawner.h>
#include <Terrain/MockTerrain.h>
#include <Tests/Mocks/Terrain/MockTerrainDataRequestBus.h>
#include <TerrainTestFixtures.h>

using ::testing::_;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;

class TerrainHeightGradientListComponentTest
    : public UnitTest::TerrainTestFixture
{
protected:

    Terrain::TerrainHeightGradientListComponent* AddHeightGradientListToEntity(AZ::Entity* entity)
    {
        // Create the TerrainHeightGradientListComponent with an entity in its configuration.
        Terrain::TerrainHeightGradientListConfig config;
        config.m_gradientEntities.push_back(entity->GetId());

        auto heightGradientListComponent = entity->CreateComponent<Terrain::TerrainHeightGradientListComponent>(config);
        return heightGradientListComponent;
    }

    void AddRequiredComponentsToEntity(AZ::Entity* entity)
    {
        // Create the required box component.
        entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();

        // Create a MockTerrainLayerSpawnerComponent to provide the required TerrainAreaService.
        entity->CreateComponent<UnitTest::MockTerrainLayerSpawnerComponent>();
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
    AddRequiredComponentsToEntity(entity.get());
    ActivateEntity(entity.get());

    EXPECT_EQ(entity->GetState(), AZ::Entity::State::Active);
}

TEST_F(TerrainHeightGradientListComponentTest, TerrainHeightGradientRefreshesTerrainSystem)
{
    // Check that the HeightGradientListComponent informs the TerrainSystem when the composition changes.
    auto entity = CreateEntity();
    AddHeightGradientListToEntity(entity.get());
    AddRequiredComponentsToEntity(entity.get());
    ActivateEntity(entity.get());

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
    AddRequiredComponentsToEntity(entity.get());

    NiceMock<UnitTest::MockTerrainAreaHeightRequests> heightfieldRequestBus(entity->GetId());

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
    NiceMock<UnitTest::MockTerrainDataRequests> mockterrainDataRequests;
    ON_CALL(mockterrainDataRequests, GetTerrainHeightQueryResolution).WillByDefault(Return(1.0f));
    ON_CALL(mockterrainDataRequests, GetTerrainHeightBounds).WillByDefault(Return(AzFramework::Terrain::FloatRange({0.0f, worldMax})));

    ActivateEntity(entity.get());

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

TEST_F(TerrainHeightGradientListComponentTest, TerrainHeightGradientListGetHeightAndGetHeightsMatch)
{
    // Check that the HeightGradientListComponent returns the same height values from GetHeight as GetHeights.

    auto entity = CreateEntity();
    AddHeightGradientListToEntity(entity.get());
    AddRequiredComponentsToEntity(entity.get());

    NiceMock<UnitTest::MockTerrainAreaHeightRequests> heightfieldRequestBus(entity->GetId());

    // Create a deterministic but varying result for our mock gradient.
    NiceMock<UnitTest::MockGradientRequests> gradientRequests(entity->GetId());
    ON_CALL(gradientRequests, GetValue)
        .WillByDefault(
            [](const GradientSignal::GradientSampleParams& params) -> float
            {
                double intpart;
                return aznumeric_cast<float>(modf(params.m_position.GetX(), &intpart));
            });

    // Setup a mock to provide the encompassing Aabb to the HeightGradientListComponent.
    const float min = 0.0f;
    const float max = 1000.0f;
    const AZ::Aabb aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(min), AZ::Vector3(max));
    NiceMock<UnitTest::MockShapeComponentRequests> mockShapeRequests(entity->GetId());
    ON_CALL(mockShapeRequests, GetEncompassingAabb).WillByDefault(Return(aabb));

    NiceMock<UnitTest::MockTerrainDataRequests> mockterrainDataRequests;
    ON_CALL(mockterrainDataRequests, GetTerrainHeightQueryResolution).WillByDefault(Return(1.0f));

    ActivateEntity(entity.get());

    // Ensure the cached values in the HeightGradientListComponent are up to date.
    LmbrCentral::DependencyNotificationBus::Event(entity->GetId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

    AZStd::vector<AZ::Vector3> inOutPositions;
    AZStd::vector<bool> terrainExistsList;

    // Build up a list of input positions to query with.
    for (float y = 0.0f; y <= 10.0f; y += 0.1f)
    {
        for (float x = 0.0f; x <= 10.0f; x += 0.1f)
        {
            inOutPositions.emplace_back(x, y, 0.0f);
            terrainExistsList.emplace_back(false);
        }
    }

    // Get the values from GetHeights
    Terrain::TerrainAreaHeightRequestBus::Event(
        entity->GetId(), &Terrain::TerrainAreaHeightRequestBus::Events::GetHeights, inOutPositions, terrainExistsList);

    // For each result returned from GetHeights, verify that it matches the result from GetHeight
    for (size_t index = 0; index < inOutPositions.size(); index++)
    {
        AZ::Vector3 inPosition(inOutPositions[index].GetX(), inOutPositions[index].GetY(), 0.0f);
        AZ::Vector3 outPosition = AZ::Vector3(0.0f);
        bool terrainExists = false;
        Terrain::TerrainAreaHeightRequestBus::Event(
            entity->GetId(), &Terrain::TerrainAreaHeightRequestBus::Events::GetHeight, inPosition, outPosition, terrainExists);

        ASSERT_TRUE(inOutPositions[index].IsClose(outPosition));
        ASSERT_EQ(terrainExists, terrainExistsList[index]);
    }
}
