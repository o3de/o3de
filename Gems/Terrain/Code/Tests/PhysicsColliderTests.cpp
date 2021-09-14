/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/MemoryComponent.h>

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include <Components/TerrainPhysicsColliderComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <AzTest/AzTest.h>

#include <TerrainMocks.h>

class PhysicsColliderComponentTest
    : public ::testing::Test
{
protected:
    AZ::ComponentApplication m_app;

    AZStd::unique_ptr<AZ::Entity> m_entity;
    Terrain::TerrainPhysicsColliderComponent* m_colliderComponent;
    UnitTest::MockBoxShapeComponent* m_shapeComponent;
    UnitTest::MockHeightfieldProviderNotificationBusListener* m_heightfieldBusListener;
    AZStd::unique_ptr<UnitTest::MockTerrainSystem> m_terrainSystem;

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
        m_terrainSystem = AZStd::make_unique<UnitTest::MockTerrainSystem>();
        m_terrainSystem->Activate();
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

    AZ::TransformNotificationBus::Event(
        m_entity->GetId(), &AZ::TransformNotificationBus::Events::OnTransformChanged, AZ::Transform(), AZ::Transform());

    EXPECT_EQ(1, m_heightfieldBusListener->m_onHeightfieldDataChangedCalledCount);

    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderShapeChangedNotifiesHeightfieldBus)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    m_entity->Activate();

   LmbrCentral::ShapeComponentNotificationsBus::Event(
        m_entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

    EXPECT_EQ(1, m_heightfieldBusListener->m_onHeightfieldDataChangedCalledCount);

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
        heightScale, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightScale);

    EXPECT_EQ(heightScale, 1.0f / 256.0f);

    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderGetHeightsReturnsHeights)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    CreateMockTerrainSystem();

    m_entity->Activate();

    AZStd::vector<int16_t> heights;

    Physics::HeightfieldProviderRequestsBus::EventResult(
        heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeights);

    EXPECT_TRUE(heights.size() != 0);
   
    ResetEntity();
}

TEST_F(PhysicsColliderComponentTest, PhysicsColliderReturnsRelativeHeights)
{
    CreateEntity();

    AddHeightfieldListener();

    AddPhysicsColliderAndShapeComponentToEntity();

    CreateMockTerrainSystem();

    m_entity->Activate();

    float min = 100;
    float max = 200;

    float mockHeight = 10.0f;

    m_terrainSystem->SetMockHeight(mockHeight);

    m_shapeComponent->SetAabbFromMinMax(AZ::Vector3(min), AZ::Vector3(max));

    AZStd::vector<int16_t> heights;

    Physics::HeightfieldProviderRequestsBus::EventResult(heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeights);

    ASSERT_TRUE(heights.size() != 0);

    float heightScale = 0.0f;

    Physics::HeightfieldProviderRequestsBus::EventResult(
        heightScale, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightScale);

    float aabbCenter = min + (max - min) / 2.0f;
    
    // The height returned by the terrain system(mockHeight) is absolute, the collider will return a relative value.
    int16_t expectedHeight = aznumeric_cast<int16_t>((mockHeight - aabbCenter) * heightScale);

    EXPECT_TRUE(heights[0] == expectedHeight);

    ResetEntity();
}
