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
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <AzTest/AzTest.h>

#include <TerrainMocks.h>

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;

class PhysicsColliderComponentTest
    : public ::testing::Test
{
protected:
    AZ::ComponentApplication m_app;

    AZStd::unique_ptr<AZ::Entity> m_entity;
    Terrain::TerrainPhysicsColliderComponent* m_colliderComponent;
    UnitTest::MockBoxShapeComponent* m_shapeComponent;
    UnitTest::MockHeightfieldProviderNotificationBusListener* m_heightfieldBusListener;
    AZStd::unique_ptr<UnitTest::MockTerrainSystemService> m_terrainSystem;
    AZStd::unique_ptr<UnitTest::MockTerrainDataRequestsListener> m_terrainDataRequestListener;
    
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
        if (m_terrainSystem)
        {
            m_terrainSystem->Deactivate();
        }

        if (m_terrainDataRequestListener)
        {
            m_terrainDataRequestListener->Deactivate();
        }
        m_app.Destroy();
    }

    void CreateEntity()
    {
        m_entity = AZStd::make_unique<AZ::Entity>();
        m_entity->Init();

        ASSERT_TRUE(m_entity);
    }

    void AddPhysicsColliderAndShapeComponentToEntity()
    {
        AddPhysicsColliderAndShapeComponentToEntity(Terrain::TerrainPhysicsColliderConfig());
    }

    void AddPhysicsColliderAndShapeComponentToEntity(const Terrain::TerrainPhysicsColliderConfig& config)
    {
        m_colliderComponent = m_entity->CreateComponent<Terrain::TerrainPhysicsColliderComponent>(config);
        m_app.RegisterComponentDescriptor(m_colliderComponent->CreateDescriptor());

        m_shapeComponent = m_entity->CreateComponent<UnitTest::MockBoxShapeComponent>();
        m_app.RegisterComponentDescriptor(m_shapeComponent->CreateDescriptor());

        ASSERT_TRUE(m_colliderComponent);
        ASSERT_TRUE(m_shapeComponent);
    }

    void AddHeightfieldListener()
    {
        m_heightfieldBusListener = m_entity->CreateComponent<UnitTest::MockHeightfieldProviderNotificationBusListener>();
        m_app.RegisterComponentDescriptor(m_heightfieldBusListener->CreateDescriptor());

        ASSERT_TRUE(m_heightfieldBusListener);
    }

    void CreateMockTerrainSystem()
    {
        m_terrainSystem = AZStd::make_unique<UnitTest::MockTerrainSystemService>();
        m_terrainSystem->Activate();
    }

    void CreateTerrainDataListener()
    {
        m_terrainDataRequestListener = AZStd::make_unique<UnitTest::MockTerrainDataRequestsListener>();
        m_terrainDataRequestListener->Activate();
    }

    void ResetEntity()
    {
        m_entity->Deactivate();
        m_entity->Reset();
    }
};

TEST_F(PhysicsColliderComponentTest, ActivateEntityActivateSuccess)
{
    CreateEntity();
    AddPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();
    EXPECT_EQ(m_entity->GetState(), AZ::Entity::State::Active);
     
    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderTransformChangedNotifiesHeightfieldBus)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    EXPECT_CALL(*m_heightfieldBusListener, OnHeightfieldDataChanged(_)).Times(1);

    AZ::TransformNotificationBus::Event(
        m_entity->GetId(), &AZ::TransformNotificationBus::Events::OnTransformChanged, AZ::Transform(), AZ::Transform());

    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderShapeChangedNotifiesHeightfieldBus)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    EXPECT_CALL(*m_heightfieldBusListener, OnHeightfieldDataChanged(_)).Times(1);

    LmbrCentral::ShapeComponentNotificationsBus::Event(
        m_entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderHeightScaleReturnsCorrectly)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

    float heightScale = 0.0f;

    Physics::HeightfieldProviderRequestsBus::EventResult(
        heightScale, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetScale);

    EXPECT_NEAR(heightScale, 1.0f / 256.0f, 0.1f);

    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderReturnsAlignedRowBoundsCorrectly)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    CreateMockTerrainSystem();

    CreateTerrainDataListener();

    m_entity->Activate();

    float min = 0.0f;
    float max = 1024.0f;

    m_shapeComponent->SetAabbFromMinMax(AZ::Vector3(min), AZ::Vector3(max));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);

    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderExpandsMinBoundsCorrectly)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    CreateMockTerrainSystem();

    CreateTerrainDataListener();

    m_entity->Activate();

    float min = 0.1f;
    float max = 1024.0f;

    m_shapeComponent->SetAabbFromMinMax(AZ::Vector3(min), AZ::Vector3(max));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);

    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderExpandsMaxBoundsCorrectly)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    CreateMockTerrainSystem();

    CreateTerrainDataListener();

    m_entity->Activate();

    float min = 0.0f;
    float max = 1023.5f;

    m_shapeComponent->SetAabbFromMinMax(AZ::Vector3(min), AZ::Vector3(max));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);

    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderGetHeightsReturnsHeights)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    CreateMockTerrainSystem();

    m_entity->Activate();

    float min = 0.0f;
    float max = 1024.0f;

    m_shapeComponent->SetAabbFromMinMax(AZ::Vector3(min), AZ::Vector3(max));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    AZStd::vector<int16_t> heights;

    Physics::HeightfieldProviderRequestsBus::EventResult(
        heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeights);

    EXPECT_EQ(heights.size(), cols * rows);
   
    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderUpdateHeightsReturnsHeightsInRegion)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    CreateMockTerrainSystem();

    m_entity->Activate();

    float min = 0.0f;
    float max = 1024.0f;

    m_shapeComponent->SetAabbFromMinMax(AZ::Vector3(min), AZ::Vector3(max));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    float regionMax = 512.0f;
    AZ::Aabb dirtyRegion = AZ::Aabb::CreateFromMinMax(AZ::Vector3(min), AZ::Vector3(regionMax));

    AZStd::vector<int16_t> heights;
    Physics::HeightfieldProviderRequestsBus::EventResult(
        heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::UpdateHeights, dirtyRegion);

    EXPECT_EQ(heights.size(), regionMax * regionMax);

    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderReturnsRelativeHeightsCorrectly)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    CreateMockTerrainSystem();

    CreateTerrainDataListener();

    m_entity->Activate();

    float min = 0.0f;
    float max = 256.0f;

    float mockHeight = 32768.0f;

    m_terrainDataRequestListener->m_mockHeight = mockHeight;

    m_shapeComponent->SetAabbFromMinMax(AZ::Vector3(min), AZ::Vector3(max));

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    AZStd::vector<int16_t> heights;

    Physics::HeightfieldProviderRequestsBus::EventResult(heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeights);

    ASSERT_TRUE(heights.size() != 0);

    float heightScale = 0.0f;

    Physics::HeightfieldProviderRequestsBus::EventResult(
        heightScale, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetScale);

    float aabbCenter = min + (max - min) / 2.0f;
    
    // The height returned by the terrain system(mockHeight) is absolute, the collider will return a relative value.
    int16_t expectedHeight = azlossy_cast<int16_t>((mockHeight - aabbCenter) * heightScale);

    EXPECT_EQ(heights[0], expectedHeight);

    ResetEntity();
}
