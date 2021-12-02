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
#include <Tests/Mocks/Terrain/MockTerrainDataRequestBus.h>

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

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderReturnsMaterials)
{
    // Check that the TerrainPhysicsCollider returns all the assigned materials.
    CreateEntity();

    m_boxComponent = m_entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
    m_app.RegisterComponentDescriptor(m_boxComponent->CreateDescriptor());

    // Create two SurfaceTag/Material mappings and add them to the collider.
    Terrain::TerrainPhysicsColliderConfig config;

    const Physics::MaterialId mat1 = Physics::MaterialId::Create();
    const Physics::MaterialId mat2 = Physics::MaterialId::Create();

    const SurfaceData::SurfaceTag tag1 = SurfaceData::SurfaceTag("tag1");
    const SurfaceData::SurfaceTag tag2 = SurfaceData::SurfaceTag("tag2");

    Terrain::TerrainPhysicsSurfaceMaterialMapping mapping1;
    mapping1.m_materialId = mat1;
    mapping1.m_surfaceTag = tag1;
    config.m_surfaceMaterialMappings.emplace_back(mapping1);

    Terrain::TerrainPhysicsSurfaceMaterialMapping mapping2;
    mapping2.m_materialId = mat2;
    mapping2.m_surfaceTag = tag2;
    config.m_surfaceMaterialMappings.emplace_back(mapping2);

    m_colliderComponent = m_entity->CreateComponent<Terrain::TerrainPhysicsColliderComponent>(config);
    m_app.RegisterComponentDescriptor(m_colliderComponent->CreateDescriptor());

    m_entity->Activate();

    AZStd::vector<Physics::MaterialId> materialList;
    Physics::HeightfieldProviderRequestsBus::EventResult(
        materialList, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetMaterialList);

    // The materialList should be 3 items long: the two materials we've added, plus a default material.
    EXPECT_EQ(materialList.size(), 3);

    Physics::MaterialId defaultMaterial = Physics::MaterialId();
    EXPECT_EQ(materialList[0], defaultMaterial);
    EXPECT_EQ(materialList[1], mat1);
    EXPECT_EQ(materialList[2], mat2);

    m_entity.reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderReturnsMaterialsWhenNotMapped)
{
    // Check that the TerrainPhysicsCollider returns a default material when no surfaces are mapped.
    CreateEntity();

    m_boxComponent = m_entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
    m_app.RegisterComponentDescriptor(m_boxComponent->CreateDescriptor());

    m_colliderComponent = m_entity->CreateComponent<Terrain::TerrainPhysicsColliderComponent>();
    m_app.RegisterComponentDescriptor(m_colliderComponent->CreateDescriptor());

    m_entity->Activate();

    AZStd::vector<Physics::MaterialId> materialList;
    Physics::HeightfieldProviderRequestsBus::EventResult(
        materialList, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetMaterialList);

    // The materialList should be 1 items long: which should be the default material.
    EXPECT_EQ(materialList.size(), 1);

    Physics::MaterialId defaultMaterial = Physics::MaterialId();
    EXPECT_EQ(materialList[0], defaultMaterial);

    m_entity.reset();
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderGetHeightsAndMaterialsReturnsCorrectly)
{
    // Check that the TerrainPhysicsCollider returns a heightfield of the expected size.
    CreateEntity();

    m_boxComponent = m_entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
    m_app.RegisterComponentDescriptor(m_boxComponent->CreateDescriptor());

    // Create two SurfaceTag/Material mappings and add them to the collider.
    Terrain::TerrainPhysicsColliderConfig config;

    const Physics::MaterialId mat1 = Physics::MaterialId::Create();
    const Physics::MaterialId mat2 = Physics::MaterialId::Create();

    const SurfaceData::SurfaceTag tag1 = SurfaceData::SurfaceTag("tag1");
    const SurfaceData::SurfaceTag tag2 = SurfaceData::SurfaceTag("tag2");

    Terrain::TerrainPhysicsSurfaceMaterialMapping mapping1;
    mapping1.m_materialId = mat1;
    mapping1.m_surfaceTag = tag1;
    config.m_surfaceMaterialMappings.emplace_back(mapping1);

    Terrain::TerrainPhysicsSurfaceMaterialMapping mapping2;
    mapping2.m_materialId = mat2;
    mapping2.m_surfaceTag = tag2;
    config.m_surfaceMaterialMappings.emplace_back(mapping2);

    m_colliderComponent = m_entity->CreateComponent<Terrain::TerrainPhysicsColliderComponent>(config);
    m_app.RegisterComponentDescriptor(m_colliderComponent->CreateDescriptor());

    m_entity->Activate();

    const AZ::Vector3 boundsMin = AZ::Vector3(0.0f);
    const AZ::Vector3 boundsMax = AZ::Vector3(256.0f, 256.0f, 32768.0f);

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(boundsMin, boundsMax);
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    const float mockHeight = 32768.0f;
    AZ::Vector2 mockHeightResolution = AZ::Vector2(1.0f);

    AzFramework::SurfaceData::SurfaceTagWeight return1;
    return1.m_surfaceType = tag1;
    return1.m_weight = 1.0f;

    AzFramework::SurfaceData::SurfaceTagWeight return2;
    return2.m_surfaceType = tag2;
    return2.m_weight = 1.0f;

    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));
    ON_CALL(terrainListener, GetHeightFromFloats).WillByDefault(Return(mockHeight));
    ON_CALL(terrainListener, GetMaxSurfaceWeightFromFloats)
        .WillByDefault(
            [return1, return2](
                [[maybe_unused]] float x, [[maybe_unused]] float y,
                [[maybe_unused]] AzFramework::Terrain::TerrainDataRequests::Sampler sampleFilter, [[maybe_unused]] bool* terrainExistsPtr)
            {
                // return tag1 for the first half of the rows, tag2 for the rest.
                if (y < 128.0)
                {
                    return return1;
                }
                return return2;
            });

    AZStd::vector<Physics::HeightMaterialPoint> heightsAndMaterials;

    Physics::HeightfieldProviderRequestsBus::EventResult(
        heightsAndMaterials, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials);

    // We set the bounds to 256, so check that the correct number of entries are present.
    EXPECT_EQ(heightsAndMaterials.size(), 256 * 256);

    const float expectedHeightValue = 16384.0f;

    // 
    // Check an entry from the first half of the returned list.
    EXPECT_EQ(heightsAndMaterials[0].m_materialIndex, 1);
    EXPECT_NEAR(heightsAndMaterials[0].m_height, expectedHeightValue, 0.01f);

    // Check an entry from the second half of the list
    EXPECT_EQ(heightsAndMaterials[256 * 128].m_materialIndex, 2);
    EXPECT_NEAR(heightsAndMaterials[256 * 128].m_height, expectedHeightValue, 0.01f);

    m_entity.reset();
}
