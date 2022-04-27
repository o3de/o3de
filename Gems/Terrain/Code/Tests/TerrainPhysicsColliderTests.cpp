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
#include <TerrainTestFixtures.h>

using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::_;
using ::testing::Return;

class TerrainPhysicsColliderComponentTest
    : public UnitTest::TerrainTestFixture
{
protected:
    AZStd::unique_ptr<AZ::Entity> m_entity;
    Terrain::TerrainPhysicsColliderComponent* m_colliderComponent;
    UnitTest::MockAxisAlignedBoxShapeComponent* m_boxComponent;

    void SetUp() override
    {
        UnitTest::TerrainTestFixture::SetUp();

        m_entity = CreateEntity();
        m_boxComponent = m_entity->CreateComponent<UnitTest::MockAxisAlignedBoxShapeComponent>();
    }

    void TearDown() override
    {
        m_entity.reset();
    }

    void AddTerrainPhysicsColliderToEntity(const Terrain::TerrainPhysicsColliderConfig& configuration)
    {
        m_colliderComponent = m_entity->CreateComponent<Terrain::TerrainPhysicsColliderComponent>(configuration);
    }

    void ProcessRegionLoop(const AZ::Aabb& inRegion, const AZ::Vector2& stepSize,
        AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
        AzFramework::SurfaceData::SurfaceTagWeightList* surfaceTags,
        const AZStd::function<float(size_t, size_t)>& heightGenerator)
    {
        if (!perPositionCallback)
        {
            return;
        }

        const size_t numSamplesX = aznumeric_cast<size_t>(ceil(inRegion.GetExtents().GetX() / stepSize.GetX()));
        const size_t numSamplesY = aznumeric_cast<size_t>(ceil(inRegion.GetExtents().GetY() / stepSize.GetY()));

        AzFramework::SurfaceData::SurfacePoint surfacePoint;
        for (size_t y = 0; y < numSamplesY; y++)
        {
            float fy = aznumeric_cast<float>(inRegion.GetMin().GetY() + (y * stepSize.GetY()));
            for (size_t x = 0; x < numSamplesX; x++)
            {
                bool terrainExists = false;
                float fx = aznumeric_cast<float>(inRegion.GetMin().GetX() + (x * stepSize.GetX()));
                surfacePoint.m_position.Set(fx, fy, heightGenerator(x, y));
                if (surfaceTags)
                {
                    surfacePoint.m_surfaceTags.clear();
                    if (fy < 128.0)
                    {
                        surfacePoint.m_surfaceTags.push_back(surfaceTags->at(0));
                    }
                    else
                    {
                        surfacePoint.m_surfaceTags.push_back(surfaceTags->at(1));
                    }
                }
                perPositionCallback(x, y, surfacePoint, terrainExists);
            }
        }
    }
};

TEST_F(TerrainPhysicsColliderComponentTest, ActivateEntityActivateSuccess)
{
    // Check that the entity activates with a collider and the required shape attached.
    AddTerrainPhysicsColliderToEntity(Terrain::TerrainPhysicsColliderConfig());
    ActivateEntity(m_entity.get());
    EXPECT_EQ(m_entity->GetState(), AZ::Entity::State::Active);
     
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderTransformChangedNotifiesHeightfieldBus)
{
    // Check that the HeightfieldBus is notified when the transform of the entity changes.
    AddTerrainPhysicsColliderToEntity(Terrain::TerrainPhysicsColliderConfig());
    ActivateEntity(m_entity.get());

    NiceMock<UnitTest::MockHeightfieldProviderNotificationBusListener> heightfieldListener(m_entity->GetId());
    EXPECT_CALL(heightfieldListener, OnHeightfieldDataChanged(_,_)).Times(1);

    // The component gets transform change notifications via the shape bus.
    LmbrCentral::ShapeComponentNotificationsBus::Event(
        m_entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderShapeChangedNotifiesHeightfieldBus)
{
    // Check that the Heightfield bus is notified when the shape component changes.
    AddTerrainPhysicsColliderToEntity(Terrain::TerrainPhysicsColliderConfig());
    ActivateEntity(m_entity.get());

    NiceMock<UnitTest::MockHeightfieldProviderNotificationBusListener> heightfieldListener(m_entity->GetId());
    EXPECT_CALL(heightfieldListener, OnHeightfieldDataChanged(_,_)).Times(1);

    LmbrCentral::ShapeComponentNotificationsBus::Event(
        m_entity->GetId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
        LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderReturnsAlignedRowBoundsCorrectly)
{
    // Check that the heightfield grid size is correct when the shape bounds match the grid resolution.
    AddTerrainPhysicsColliderToEntity(Terrain::TerrainPhysicsColliderConfig());

    const float boundsMin = 0.0f;
    const float boundsMax = 1024.0f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    float mockHeightResolution = 1.0f;
    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    ActivateEntity(m_entity.get());

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    // With the bounds set at 0-1024 and a resolution of 1.0, the heightfield grid should be 1024x1024.
    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderExpandsMinBoundsCorrectly)
{
    // Check that the heightfield grid is correctly expanded if the minimum value of the bounds needs expanding
    // to correctly encompass it.
    AddTerrainPhysicsColliderToEntity(Terrain::TerrainPhysicsColliderConfig());

    const float boundsMin = 0.1f;
    const float boundsMax = 1024.0f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    float mockHeightResolution = 1.0f;
    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    ActivateEntity(m_entity.get());

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    // If the heightfield is not expanded to ensure it encompasses the shape bounds,
    // the values returned would be 1023.
    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderExpandsMaxBoundsCorrectly)
{
    // Check that the heightfield grid is correctly expanded if the maximum value of the bounds needs expanding
    // to correctly encompass it.
    AddTerrainPhysicsColliderToEntity(Terrain::TerrainPhysicsColliderConfig());

    const float boundsMin = 0.0f;
    const float boundsMax = 1023.5f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    float mockHeightResolution = 1.0f;
    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));

    ActivateEntity(m_entity.get());

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    // If the heightfield is not expanded to ensure it encompasses the shape bounds,
    // the values returned would be 1023.
    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderGetHeightsReturnsHeights)
{
    // Check that the TerrainPhysicsCollider returns a heightfield of the expected size.
    AddTerrainPhysicsColliderToEntity(Terrain::TerrainPhysicsColliderConfig());

    const float boundsMin = 0.0f;
    const float boundsMax = 1024.0f;

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    float mockHeightResolution = 1.0f;
    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));
    ON_CALL(terrainListener, ProcessHeightsFromRegion).WillByDefault(
        [this](const AZ::Aabb& inRegion, const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            [[maybe_unused]] AzFramework::Terrain::TerrainDataRequests::Sampler sampleFilter)
        {
            ProcessRegionLoop(inRegion, stepSize, perPositionCallback, nullptr,
                []([[maybe_unused]]size_t x, [[maybe_unused]]size_t y){ return 0.0f; });
        }
    );

    ActivateEntity(m_entity.get());

    int32_t cols, rows;
    Physics::HeightfieldProviderRequestsBus::Event(
        m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, cols, rows);

    AZStd::vector<float> heights;

    Physics::HeightfieldProviderRequestsBus::EventResult(
        heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeights);

    EXPECT_EQ(cols, 1024);
    EXPECT_EQ(rows, 1024);
    EXPECT_EQ(heights.size(), cols * rows);
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderReturnsRelativeHeightsCorrectly)
{
    // Check that the values stored in the heightfield returned by the TerrainPhysicsCollider are correct.
    AddTerrainPhysicsColliderToEntity(Terrain::TerrainPhysicsColliderConfig());

    const AZ::Vector3 boundsMin = AZ::Vector3(0.0f);
    const AZ::Vector3 boundsMax = AZ::Vector3(256.0f, 256.0f, 32768.0f);

    const float mockHeight = 32768.0f;
    float mockHeightResolution = 1.0f;

    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));
    ON_CALL(terrainListener, ProcessHeightsFromRegion).WillByDefault(
        [this, mockHeight](const AZ::Aabb& inRegion, const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            [[maybe_unused]]  AzFramework::Terrain::TerrainDataRequests::Sampler sampleFilter)
        {
            ProcessRegionLoop(inRegion, stepSize, perPositionCallback, nullptr,
                [mockHeight]([[maybe_unused]]size_t x, [[maybe_unused]]size_t y){ return mockHeight; });
        }
    );

    ActivateEntity(m_entity.get());

    // Just return the bounds as setup. This is equivalent to the box being at the origin.
    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(boundsMin), AZ::Vector3(boundsMax));
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    AZStd::vector<float> heights;

    Physics::HeightfieldProviderRequestsBus::EventResult(heights, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeights);

    ASSERT_FALSE(heights.empty());

    const float expectedHeightValue = 16384.0f;
    EXPECT_NEAR(heights[0], expectedHeightValue, 0.01f);
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderReturnsMaterials)
{
    // Check that the TerrainPhysicsCollider returns all the assigned materials.
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

    AddTerrainPhysicsColliderToEntity(config);
    ActivateEntity(m_entity.get());

    AZStd::vector<Physics::MaterialId> materialList;
    Physics::HeightfieldProviderRequestsBus::EventResult(
        materialList, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetMaterialList);

    // The materialList should be 3 items long: the two materials we've added, plus a default material.
    EXPECT_EQ(materialList.size(), 3);

    Physics::MaterialId defaultMaterial = Physics::MaterialId();
    EXPECT_EQ(materialList[0], defaultMaterial);
    EXPECT_EQ(materialList[1], mat1);
    EXPECT_EQ(materialList[2], mat2);
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderReturnsMaterialsWhenNotMapped)
{
    // Check that the TerrainPhysicsCollider returns a default material when no surfaces are mapped.
    AddTerrainPhysicsColliderToEntity(Terrain::TerrainPhysicsColliderConfig());
    ActivateEntity(m_entity.get());

    AZStd::vector<Physics::MaterialId> materialList;
    Physics::HeightfieldProviderRequestsBus::EventResult(
        materialList, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetMaterialList);

    // The materialList should be 1 items long: which should be the default material.
    EXPECT_EQ(materialList.size(), 1);

    Physics::MaterialId defaultMaterial = Physics::MaterialId();
    EXPECT_EQ(materialList[0], defaultMaterial);
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderGetHeightsAndMaterialsReturnsCorrectly)
{
    // Check that the TerrainPhysicsCollider returns a heightfield of the expected size.
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

    AddTerrainPhysicsColliderToEntity(config);

    const AZ::Vector3 boundsMin = AZ::Vector3(0.0f);
    const AZ::Vector3 boundsMax = AZ::Vector3(256.0f, 256.0f, 32768.0f);

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(boundsMin, boundsMax);
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    const float mockHeight = 32768.0f;
    float mockHeightResolution = 1.0f;

    AzFramework::SurfaceData::SurfaceTagWeight tagWeight1(tag1, 1.0f);
    AzFramework::SurfaceData::SurfaceTagWeight tagWeight2(tag2, 1.0f);

    AzFramework::SurfaceData::SurfaceTagWeightList surfaceTags = { tagWeight1, tagWeight2 };

    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));
    ON_CALL(terrainListener, ProcessSurfacePointsFromRegion).WillByDefault(
        [this, mockHeight, &surfaceTags](const AZ::Aabb& inRegion, const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            [[maybe_unused]] AzFramework::Terrain::TerrainDataRequests::Sampler sampleFilter)
        {
            ProcessRegionLoop(inRegion, stepSize, perPositionCallback, &surfaceTags,
                [mockHeight]([[maybe_unused]]size_t x, [[maybe_unused]]size_t y){ return mockHeight; });
        }
    );

    ActivateEntity(m_entity.get());

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
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderDefaultMaterialAssignedWhenTagHasNoMapping)
{
    // Create two SurfaceTag/Material mappings and add them to the collider.
    Terrain::TerrainPhysicsColliderConfig config;

    const Physics::MaterialId defaultSurfaceMaterial = Physics::MaterialId::Create();
    const Physics::MaterialId mat1 = Physics::MaterialId::Create();

    const SurfaceData::SurfaceTag tag1 = SurfaceData::SurfaceTag("tag1");
    const SurfaceData::SurfaceTag tag2 = SurfaceData::SurfaceTag("tag2");

    Terrain::TerrainPhysicsSurfaceMaterialMapping mapping1;
    mapping1.m_materialId = mat1;
    mapping1.m_surfaceTag = tag1;
    config.m_surfaceMaterialMappings.emplace_back(mapping1);
    config.m_defaultMaterialSelection.SetMaterialId(defaultSurfaceMaterial);

    // Intentionally don't set the mapping for "tag2". It's expected the default material will substitute.
    AddTerrainPhysicsColliderToEntity(config);

    const AZ::Vector3 boundsMin = AZ::Vector3(0.0f);
    const AZ::Vector3 boundsMax = AZ::Vector3(256.0f, 256.0f, 32768.0f);

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(boundsMin, boundsMax);
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    const float mockHeight = 32768.0f;
    float mockHeightResolution = 1.0f;

    AzFramework::SurfaceData::SurfaceTagWeight tagWeight1(tag1, 1.0f);
    AzFramework::SurfaceData::SurfaceTagWeight tagWeight2(tag2, 1.0f);

    AzFramework::SurfaceData::SurfaceTagWeightList surfaceTags = { tagWeight1, tagWeight2 };

    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));
    ON_CALL(terrainListener, ProcessSurfacePointsFromRegion).WillByDefault(
        [this, mockHeight, &surfaceTags](const AZ::Aabb& inRegion, const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            [[maybe_unused]] AzFramework::Terrain::TerrainDataRequests::Sampler sampleFilter)
    {
        ProcessRegionLoop(inRegion, stepSize, perPositionCallback, &surfaceTags,
            [mockHeight]([[maybe_unused]]size_t x, [[maybe_unused]]size_t y){ return mockHeight; });
    }
    );

    ActivateEntity(m_entity.get());

    // Validate material list is generated with the default material
    {
        AZStd::vector<Physics::MaterialId> materialList;
        Physics::HeightfieldProviderRequestsBus::EventResult(
            materialList, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetMaterialList);

        // The materialList should be 2 items long: the default material and mat1.
        EXPECT_EQ(materialList.size(), 2);
        EXPECT_EQ(materialList[0], defaultSurfaceMaterial);
        EXPECT_EQ(materialList[1], mat1);
    }

    // Validate material indices 
    {
        AZStd::vector<Physics::HeightMaterialPoint> heightsAndMaterials;
        Physics::HeightfieldProviderRequestsBus::EventResult(
            heightsAndMaterials, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials);

        // We set the bounds to 256, so check that the correct number of entries are present.
        EXPECT_EQ(heightsAndMaterials.size(), 256 * 256);

        // Check an entry from the first half of the returned list.
        EXPECT_EQ(heightsAndMaterials[0].m_materialIndex, 1);

        // Check an entry from the second half of the list.
        // This should point to the default material (0) since we don't have a mapping for "tag2"
        EXPECT_EQ(heightsAndMaterials[256 * 128].m_materialIndex, 0);
    }
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderDefaultMaterialAssignedWhenNoMappingsExist)
{
    // Create only the default material with no mapping for the tags. It's expected the default material will be assigned to both tags.
    Terrain::TerrainPhysicsColliderConfig config;    
    const Physics::MaterialId defaultSurfaceMaterial = Physics::MaterialId::Create();
    config.m_defaultMaterialSelection.SetMaterialId(defaultSurfaceMaterial);
    AddTerrainPhysicsColliderToEntity(config);

    const AZ::Vector3 boundsMin = AZ::Vector3(0.0f);
    const AZ::Vector3 boundsMax = AZ::Vector3(256.0f, 256.0f, 32768.0f);

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(boundsMin, boundsMax);
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    const float mockHeight = 32768.0f;
    float mockHeightResolution = 1.0f;

    const SurfaceData::SurfaceTag tag1 = SurfaceData::SurfaceTag("tag1");
    AzFramework::SurfaceData::SurfaceTagWeight tagWeight1(tag1, 1.0f);

    const SurfaceData::SurfaceTag tag2 = SurfaceData::SurfaceTag("tag2");
    AzFramework::SurfaceData::SurfaceTagWeight tagWeight2(tag2, 1.0f);

    AzFramework::SurfaceData::SurfaceTagWeightList surfaceTags = { tagWeight1, tagWeight2 };

    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));
    ON_CALL(terrainListener, ProcessSurfacePointsFromRegion).WillByDefault(
        [this, mockHeight, &surfaceTags](const AZ::Aabb& inRegion, const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            [[maybe_unused]] AzFramework::Terrain::TerrainDataRequests::Sampler sampleFilter)
    {
        ProcessRegionLoop(inRegion, stepSize, perPositionCallback, &surfaceTags,
            [mockHeight]([[maybe_unused]]size_t x, [[maybe_unused]]size_t y){ return mockHeight; });
    }
    );

    ActivateEntity(m_entity.get());

    // Validate material list is generated with the default material
    {
        AZStd::vector<Physics::MaterialId> materialList;
        Physics::HeightfieldProviderRequestsBus::EventResult(
            materialList, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetMaterialList);

        EXPECT_EQ(materialList.size(), 1);
        EXPECT_EQ(materialList[0], defaultSurfaceMaterial);
    }

    // Validate material indices 
    {
        AZStd::vector<Physics::HeightMaterialPoint> heightsAndMaterials;
        Physics::HeightfieldProviderRequestsBus::EventResult(
            heightsAndMaterials, m_entity->GetId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials);

        // We set the bounds to 256, so check that the correct number of entries are present.
        EXPECT_EQ(heightsAndMaterials.size(), 256 * 256);

        // Check an entry from the first half of the returned list. Should be the default material index 0.
        EXPECT_EQ(heightsAndMaterials[0].m_materialIndex, 0);

        // Check an entry from the second half of the list. Should be the default material index 0.
        EXPECT_EQ(heightsAndMaterials[256 * 128].m_materialIndex, 0);
    }
}

TEST_F(TerrainPhysicsColliderComponentTest, TerrainPhysicsColliderRequestSubpartForDirtyRegion)
{
    // The test validates the requested sub-part of terrain collider matches the source data
    AddTerrainPhysicsColliderToEntity(Terrain::TerrainPhysicsColliderConfig());

    const int32_t terrainSize = 256;

    const AZ::Vector3 boundsMin = AZ::Vector3(0.0f);
    const AZ::Vector3 boundsMax = AZ::Vector3(terrainSize, terrainSize, 512.0f);

    NiceMock<UnitTest::MockShapeComponentRequests> boxShape(m_entity->GetId());
    const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(boundsMin, boundsMax);
    ON_CALL(boxShape, GetEncompassingAabb).WillByDefault(Return(bounds));

    AzFramework::SurfaceData::SurfaceTagWeight tagWeight1(AZ::Crc32("tag1"), 1.0f);
    AzFramework::SurfaceData::SurfaceTagWeight tagWeight2(AZ::Crc32("tag2"), 1.0f);

    AzFramework::SurfaceData::SurfaceTagWeightList surfaceTags = { tagWeight1, tagWeight2 };
    float mockHeightResolution = 1.0f;

    NiceMock<UnitTest::MockTerrainDataRequests> terrainListener;
    ON_CALL(terrainListener, GetTerrainHeightQueryResolution).WillByDefault(Return(mockHeightResolution));
    ON_CALL(terrainListener, ProcessSurfacePointsFromRegion).WillByDefault(
        [this, &surfaceTags](const AZ::Aabb& inRegion, const AZ::Vector2& stepSize,
            AzFramework::Terrain::SurfacePointRegionFillCallback perPositionCallback,
            [[maybe_unused]] AzFramework::Terrain::TerrainDataRequests::Sampler sampleFilter)
    {
        // Assign a variety of heights across the terrain
        ProcessRegionLoop(inRegion, stepSize, perPositionCallback, &surfaceTags,
            [](size_t x, size_t y){ return float(x + y); });
    }
    );

    ActivateEntity(m_entity.get());

    // Get the entire array of points
    AZStd::vector<Physics::HeightMaterialPoint> heightsMaterials = m_colliderComponent->GetHeightsAndMaterials();
    EXPECT_EQ(heightsMaterials.size(), terrainSize * terrainSize);

    // Request a sub-part of the terrain and validate the points match the original data
    int32_t callCounter = 0;
    Physics::UpdateHeightfieldSampleFunction validateDataCallback = [&callCounter, &heightsMaterials](int32_t row,
        int32_t column, const Physics::HeightMaterialPoint& dataPoint)
    {
        size_t lookUpIndex = row * terrainSize + column;
        EXPECT_LT(lookUpIndex, heightsMaterials.size());
        EXPECT_EQ(heightsMaterials[lookUpIndex].m_height, dataPoint.m_height);
        ++callCounter;
    };

    AZ::Vector3 regionMin(AZ::Vector3(10.0f));
    AZ::Vector3 regionMax(AZ::Vector3(200.0f));
    int32_t dx = int32_t(regionMax.GetX() - regionMin.GetX());
    int32_t dy = int32_t(regionMax.GetY() - regionMin.GetY());
            
    m_colliderComponent->UpdateHeightsAndMaterials(validateDataCallback, AZ::Aabb::CreateFromMinMax(regionMin, regionMax));

    // Validate update heightfield callback was called the exact amount of times required for the region
    EXPECT_EQ(dx * dy, callCounter);
}
