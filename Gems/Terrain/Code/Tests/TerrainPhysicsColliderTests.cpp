/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Memory/MemoryComponent.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include <AzFramework/Physics/Mocks/MockHeightfieldProviderBus.h>

#include <Components/TerrainPhysicsColliderComponent.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/MockShapes.h>
#include <AzTest/AzTest.h>

#include <MockAxisAlignedBoxShapeComponent.h>
#include <Terrain/MockTerrain.h>

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
};

TEST_F(TerrainPhysicsColliderComponentTest, ActivateEntityActivateSuccess)
{
    // Check that the entity activates with a collider and the required shape attached.
    CreateEntity();
    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();
    EXPECT_EQ(m_entity->GetState(), AZ::Entity::State::Active);
     
    m_entity.reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderTransformChangedNotifiesHeightfieldBus)
{
    // Check that the HeightfieldBus is notified when the transform of the entity changes.
    CreateEntity();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    NiceMock<UnitTest::MockHeightfieldProviderNotificationBusListener> heightfieldListener(m_entity->GetId());
    EXPECT_CALL(heightfieldListener, OnHeightfieldDataChanged(_)).Times(1);

    // The component gets transform change notifications via the shape bus.
    LmbrCentral::ShapeComponentNotificationsBus::Event(
        m_entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);

    m_entity.reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderShapeChangedNotifiesHeightfieldBus)
{
    // Check that the Heightfield bus is notified when the shape component changes.
    CreateEntity();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    NiceMock<UnitTest::MockHeightfieldProviderNotificationBusListener> heightfieldListener(m_entity->GetId());
    EXPECT_CALL(heightfieldListener, OnHeightfieldDataChanged(_)).Times(1);

    LmbrCentral::ShapeComponentNotificationsBus::Event(
        m_entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

    m_entity.reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderReturnsAlignedRowBoundsCorrectly)
{
    // Check that the heightfield grid size is correct when the shape bounds match the grid resolution.
    CreateEntity();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    const float boundsMin = 0.0f;
    const float boundsMax = 1024.0f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    const AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);
    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    // With the bounds set at 0-1024 and a resolution of 1.0, the heightfield grid should be 1024x1024.
    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);

    m_entity.reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderExpandsMinBoundsCorrectly)
{
    // Check that the heightfield grid is correctly expanded if the minimum value of the bounds needs expanding
    // to correctly encompass it.
    CreateEntity();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    const float boundsMin = 0.1f;
    const float boundsMax = 1024.0f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);
    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    // If the heightfield is not expanded to ensure it encompasses the shape bounds,
    // the values returned would be 1023.
    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);

    m_entity.reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderExpandsMaxBoundsCorrectly)
{
    // Check that the heightfield grid is correctly expanded if the maximum value of the bounds needs expanding
    // to correctly encompass it.
    CreateEntity();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    const float boundsMin = 0.0f;
    const float boundsMax = 1023.5f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);
    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    // If the heightfield is not expanded to ensure it encompasses the shape bounds,
    // the values returned would be 1023.
    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);

    m_entity.reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderGetHeightsReturnsHeights)
{
    // Check that the TerrainPhysicsCollider returns a heightfield of the expected size.
    CreateEntity();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    const float boundsMin = 0.0f;
    const float boundsMax = 1024.0f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);
    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    AZStd::vector<float> heights;

    Physics::HeightfieldProviderRequestsBus::EventResult(
        heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeights);

    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);
    EXPECT_EQ(heights.size(), cols * rows);
   
    m_entity.reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderReturnsRelativeHeightsCorrectly)
{
    // Check that the values stored in the heightfield returned by the TerrainPhysicsCollider are correct.
    CreateEntity();

    AddTerrainPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    const AZ::Vector3 boundsMin = AZ::Vector3(0.0f);
    const AZ::Vector3 boundsMax = AZ::Vector3(256.0f, 256.0f, 32768.0f);

    const float mockHeight = 32768.0f;
    AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);

    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetHeightFromFloats).WillByDefault(Return(mockHeight));
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    // Just return the bounds as setup. This is equivalent to the box being at the origin.
    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    AZStd::vector<float> heights;

    Physics::HeightfieldProviderRequestsBus::EventResult(heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeights);

    ASSERT_FALSE(heights.empty());

    const float expectedHeightValue = 16384.0f;
    EXPECT_NEAR(heights[0], expectedHeightValue, 0.01f);

    m_entity->Reset();
}
