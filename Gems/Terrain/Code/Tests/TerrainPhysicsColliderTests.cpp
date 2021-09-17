/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/MemoryComponent.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <Components/TerrainPhysicsColliderComponent.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/MockShapes.h>
#include <AzTest/AzTest.h>

#include <MockAxisAlignedBoxShapeComponent.h>
#include <TerrainMocks.h>

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;
using ::testing::Return;

class TerrainPhysicsColliderComponentTest
    : public ::testing::Test
{
protected:
    AZ::ComponentApplication m_app;

    AZStd::unique_ptr<AZ::Entity> m_entity;
    Terrain::TerrainPhysicsColliderComponent* m_colliderComponent;
    AZStd::unique_ptr<NiceMock<UnitTest::MockHeightfieldProviderNotificationBusListener>> m_heightfieldBusListener;
    UnitTest::MockAxisAlignedBoxShapeComponent* m_boxComponent;

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

        m_entity->Init();
    }

    void AddTerrainPhysicsColliderAndShapeComponentToEntity()
    {
        m_boxComponent = m_entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
        m_app.RegisterComponentDescriptor(m_boxComponent->CreateDescriptor());

        m_colliderComponent = m_entity->CreateComponent<Terrain::TerrainPhysicsColliderComponent>(Terrain::TerrainPhysicsColliderConfig());
        m_app.RegisterComponentDescriptor(m_colliderComponent->CreateDescriptor());
    }

    void AddHeightfieldListener()
    {
        m_heightfieldBusListener = AZStd::make_unique<NiceMock<UnitTest::MockHeightfieldProviderNotificationBusListener>>();

        ASSERT_TRUE(m_heightfieldBusListener);
    }
};

TEST_F(TerrainPhysicsColliderComponentTest, ActivateEntityActivateSuccess)
{
    // Check that the entity activates with a collider and the required shape attached.
    CreateEntity();
    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();
    EXPECT_EQ(m_entity->GetState(), AZ::Entity::State::Active);
     
    m_entity->Reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderTransformChangedNotifiesHeightfieldBus)
{
    // Check that the HeightfieldBus is notified when the transform of the entity changes.
    CreateEntity();

    AddHeightfieldListener();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    EXPECT_CALL(*m_heightfieldBusListener, OnHeightfieldDataChanged(_)).Times(1);

    AZ::TransformNotificationBus::Event(
        m_entity->GetId(), &AZ::TransformNotificationBus::Events::OnTransformChanged, AZ::Transform(), AZ::Transform());

    m_entity->Reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderShapeChangedNotifiesHeightfieldBus)
{
    // Check that the Heightfield bus is notified when the shape component changes.
    CreateEntity();

    AddHeightfieldListener();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    EXPECT_CALL(*m_heightfieldBusListener, OnHeightfieldDataChanged(_)).Times(1);

    LmbrCentral::ShapeComponentNotificationsBus::Event(
        m_entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

    m_entity->Reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderHeightScaleReturnsCorrectly)
{
    // Check that the default scale is as expected.
    CreateEntity();

    AddHeightfieldListener();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    float heightScale = 0.0f;

    Physics::HeightfieldProviderRequestsBus::EventResult(
        heightScale, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetScale);

    const float expectedScale = 1.0f / 256.0f;
    const float nearTolerance = 0.01f;
    EXPECT_NEAR(heightScale, expectedScale, nearTolerance);

    m_entity->Reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderReturnsAlignedRowBoundsCorrectly)
{
    // Check that the heightfield grid size is correct when the shape bounds match the grid resolution.
    CreateEntity();

    AddHeightfieldListener();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    const float boundsMin = 0.0f;
    const float boundsMax = 1024.0f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    const AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);
    NiceMock<UnitTest::MockTerrainDataRequestsListener> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);

    m_entity->Reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderExpandsMinBoundsCorrectly)
{
    // Check that the heightfield grid is correctly expanded if the minimum value of the bounds needs expanding
    // to correctly encompass it.
    CreateEntity();

    AddHeightfieldListener();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    const float boundsMin = 0.1f;
    const float boundsMax = 1024.0f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);
    NiceMock<UnitTest::MockTerrainDataRequestsListener> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);

    m_entity->Reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderExpandsMaxBoundsCorrectly)
{
    // Check that the heightfield grid is correctly expanded if the maximum value of the bounds needs expanding
    // to correctly encompass it.
    CreateEntity();

    AddHeightfieldListener();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    const float boundsMin = 0.0f;
    const float boundsMax = 1023.5f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);
    NiceMock<UnitTest::MockTerrainDataRequestsListener> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);

    m_entity->Reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderGetHeightsReturnsHeights)
{
    // Check that the TerrainPhysicsCollider returns a heightfield of the expected size.
    CreateEntity();

    AddHeightfieldListener();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    const float boundsMin = 0.0f;
    const float boundsMax = 1024.0f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);
    NiceMock<UnitTest::MockTerrainDataRequestsListener> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    AZStd::vector<int16_t> heights;

    Physics::HeightfieldProviderRequestsBus::EventResult(
        heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeights);

    const int expectedHeightsSize = 1024 * 1024;
    EXPECT_EQ(heights.size(), cols * rows);
    EXPECT_EQ(heights.size(), expectedHeightsSize);
   
    m_entity->Reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderUpdateHeightsReturnsHeightsInRegion)
{
    // Check that the TerrainPhysicsCollider returns a heightfield of the correct size when asked for a subregion.
    CreateEntity();

    AddHeightfieldListener();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    const float boundsMin = 0.0f;
    const float boundsMax = 1024.0f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);
    NiceMock<UnitTest::MockTerrainDataRequestsListener> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    const float regionMax = 512.0f;
    const AZ::Aabb dirtyRegion = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(regionMax));

    AZStd::vector<int16_t> heights;
    Physics::HeightfieldProviderRequestsBus::EventResult(
        heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::UpdateHeights, dirtyRegion);

    EXPECT_EQ(heights.size(), regionMax * regionMax);

    m_entity->Reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderReturnsRelativeHeightsCorrectly)
{
    // Check that the values stored in the heightfield returned by the TerrainPhysicsCollider are correct.
    CreateEntity();

    AddHeightfieldListener();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    const float boundsMin = 0.0f;
    const float boundsMax = 256;

    const float mockHeight = 32768.0f;
    AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);

    NiceMock<UnitTest::MockTerrainDataRequestsListener> terrainListener;
    ON_CALL(terrainListener, GetHeightFromFloats).WillByDefault(Return(mockHeight));
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    AZStd::vector<int16_t> heights;

    Physics::HeightfieldProviderRequestsBus::EventResult(heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeights);

    ASSERT_FALSE(heights.empty());

    float heightScale = 0.0f;

    Physics::HeightfieldProviderRequestsBus::EventResult(
        heightScale, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetScale);

    float aabbCenter = boundsMin + (boundsMax - boundsMin) / 2.0f;

    // The expected height is the offset of the height from the centre of the bounding box multiplied by the scale value and then cast to an
    // int16_t
    int16_t expectedHeight = azlossy_cast<int16_t>((mockHeight - aabbCenter) * heightScale);

    EXPECT_EQ(heights[0], expectedHeight);

    const int16_t expectedHeightValue = 127;
    EXPECT_EQ(heights[0], expectedHeightValue);

    m_entity->Reset();
}
