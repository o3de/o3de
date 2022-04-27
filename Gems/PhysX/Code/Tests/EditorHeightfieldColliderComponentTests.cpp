/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <Tests/EditorTestUtilities.h>
#include <EditorHeightfieldColliderComponent.h>
#include <HeightfieldColliderComponent.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>
#include <StaticRigidBodyComponent.h>
#include <RigidBodyStatic.h>
#include <PhysX/PhysXLocks.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <PhysX/MockPhysXHeightfieldProviderComponent.h>
#include <AzCore/Casting/lossy_cast.h>
#include <Utils.h>

using ::testing::NiceMock;
using ::testing::Return;

namespace PhysXEditorTests
{
    AZStd::vector<Physics::HeightMaterialPoint> GetSamples()
    {
        AZStd::vector<Physics::HeightMaterialPoint> samples{ { 3.0f, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight, 0 },
                                                             { 2.0f, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight, 1 },
                                                             { 1.5f, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight, 2 },
                                                             { 1.0f, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight, 0 },
                                                             { 3.0f, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight, 1 },
                                                             { 1.0f, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight, 2 },
                                                             { 3.0f, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight, 0 },
                                                             { 0.0f, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight, 1 },
                                                             { 3.0f, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight, 2 } };
        return samples;
    }

    AZStd::vector<Physics::MaterialId> GetMaterialList()
    {
        AZStd::vector<Physics::MaterialId> materials{
            {Physics::MaterialId::FromUUID("{EC976D51-2C26-4C1E-BBF2-75BAAAFA162C}")},
            {Physics::MaterialId::FromUUID("{B9836F51-A235-4781-95E3-A6302BEE9EFF}")},
            {Physics::MaterialId::FromUUID("{7E060707-BB03-47EB-B046-4503C7145B6E}")}
        };
        return materials;
    }

    EntityPtr SetupHeightfieldComponent()
    {
        // create an editor entity with a shape collider component and a box shape component
        EntityPtr editorEntity = CreateInactiveEditorEntity("HeightfieldColliderComponentEditorEntity");
        editorEntity->CreateComponent<UnitTest::MockPhysXHeightfieldProviderComponent>();
        editorEntity->CreateComponent(LmbrCentral::EditorAxisAlignedBoxShapeComponentTypeId);
        editorEntity->CreateComponent<PhysX::EditorHeightfieldColliderComponent>();
        AZ::ComponentApplicationBus::Broadcast(
            &AZ::ComponentApplicationRequests::RegisterComponentDescriptor,
            UnitTest::MockPhysXHeightfieldProviderComponent::CreateDescriptor());
        return editorEntity;
    }

    void CleanupHeightfieldComponent()
    {
        AZ::ComponentApplicationBus::Broadcast(
            &AZ::ComponentApplicationRequests::UnregisterComponentDescriptor,
            UnitTest::MockPhysXHeightfieldProviderComponent::CreateDescriptor());
    }

    void SetupMockMethods(NiceMock<UnitTest::MockPhysXHeightfieldProvider>& mockShapeRequests)
    {
        ON_CALL(mockShapeRequests, GetHeightfieldTransform).WillByDefault(Return(AZ::Transform::CreateTranslation({ 1, 2, 0 })));
        ON_CALL(mockShapeRequests, GetHeightfieldGridSpacing).WillByDefault(Return(AZ::Vector2(1, 1)));
        ON_CALL(mockShapeRequests, GetHeightsAndMaterials).WillByDefault(Return(GetSamples()));
        ON_CALL(mockShapeRequests, GetHeightfieldGridSize)
            .WillByDefault(
                [](int32_t& numColumns, int32_t& numRows)
                {
                    numColumns = 3;
                    numRows = 3;
                });
        ON_CALL(mockShapeRequests, GetHeightfieldHeightBounds)
            .WillByDefault(
                [](float& x, float& y)
                {
                    x = -3.0f;
                    y = 3.0f;
                });
        ON_CALL(mockShapeRequests, GetMaterialList).WillByDefault(Return(GetMaterialList()));
    }

    EntityPtr TestCreateActiveGameEntityFromEditorEntity(AZ::Entity* editorEntity)
    {
        EntityPtr gameEntity = AZStd::make_unique<AZ::Entity>();
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::PreExportEntity, *editorEntity, *gameEntity);
        gameEntity->Init();
        return gameEntity;
    }

    class PhysXEditorHeightfieldFixture : public PhysXEditorFixture
    {
    public:
        void SetUp() override
        {
            PhysXEditorFixture::SetUp();
            PopulateDefaultMaterialLibrary();

            m_editorEntity = SetupHeightfieldComponent();
            m_editorMockShapeRequests = AZStd::make_unique<NiceMock<UnitTest::MockPhysXHeightfieldProvider>>(m_editorEntity->GetId());
            SetupMockMethods(*m_editorMockShapeRequests.get());
            m_editorEntity->Activate();

            m_gameEntity = TestCreateActiveGameEntityFromEditorEntity(m_editorEntity.get());
            m_gameMockShapeRequests = AZStd::make_unique<NiceMock<UnitTest::MockPhysXHeightfieldProvider>>(m_gameEntity->GetId());
            SetupMockMethods(*m_gameMockShapeRequests.get());
            m_gameEntity->Activate();

            Physics::HeightfieldProviderNotificationBus::Broadcast(
                &Physics::HeightfieldProviderNotificationBus::Events::OnHeightfieldDataChanged, AZ::Aabb::CreateNull(),
                Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::CreateEnd);
        }

        void TearDown() override
        {
            CleanupHeightfieldComponent();

            m_editorEntity = nullptr;
            m_gameEntity = nullptr;
            m_editorMockShapeRequests = nullptr;
            m_gameMockShapeRequests = nullptr;

            PhysXEditorFixture::TearDown();
        }

        void PopulateDefaultMaterialLibrary()
        {
            AZ::Data::AssetId assetId = AZ::Data::AssetId(AZ::Uuid::Create());

            // Create an asset out of our Script Event
            Physics::MaterialLibraryAsset* matLibAsset = aznew Physics::MaterialLibraryAsset;
            {
                const AZStd::vector<Physics::MaterialId> matIds = GetMaterialList();

                for (const Physics::MaterialId& matId : matIds)
                {
                    Physics::MaterialFromAssetConfiguration matConfig;
                    matConfig.m_id = matId;
                    matConfig.m_configuration.m_surfaceType = matId.GetUuid().ToString<AZStd::string>();
                    matLibAsset->AddMaterialData(matConfig);
                }
            }

            // Note: There is no interface to simply update material library asset. It has to go via updating the entire configuration which causes assets reloading.
            // It makes sense as a safety mechanism in the Editor but makes it harder to write tests.
            // Hence have to work around it via const_cast here to be able to simply set the generated asset into configuration.
            AzPhysics::SystemConfiguration* sysConfig = const_cast<AzPhysics::SystemConfiguration*>(AZ::Interface<AzPhysics::SystemInterface>::Get()->GetConfiguration());

            AZ::Data::Asset<Physics::MaterialLibraryAsset> assetData(assetId, matLibAsset, AZ::Data::AssetLoadBehavior::Default);
            sysConfig->m_materialLibraryAsset = assetData;
        }

        Physics::Material* GetMaterialFromRaycast(float x, float y)
        {
            AzPhysics::RayCastRequest request;
            request.m_start = AZ::Vector3(x, y, 5.0f);
            request.m_direction = AZ::Vector3(0.0f, 0.0f, -1.0f);
            request.m_distance = 10.0f;

            //query the scene
            auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
            AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_defaultSceneHandle, &request);
            EXPECT_EQ(result.m_hits.size(), 1);

            if (result)
            {
                return result.m_hits[0].m_material;
            }

            return nullptr;
        };

        EntityPtr m_editorEntity;
        EntityPtr m_gameEntity;
        AZStd::unique_ptr<NiceMock<UnitTest::MockPhysXHeightfieldProvider>> m_editorMockShapeRequests;
        AZStd::unique_ptr<NiceMock<UnitTest::MockPhysXHeightfieldProvider>> m_gameMockShapeRequests;
    };

    TEST_F(PhysXEditorFixture, EditorHeightfieldColliderComponentDependenciesSatisfiedEntityIsValid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("HeightfieldColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorHeightfieldColliderComponent>();
        entity->CreateComponent(LmbrCentral::EditorAxisAlignedBoxShapeComponentTypeId);
        entity->CreateComponent<UnitTest::MockPhysXHeightfieldProviderComponent>()->CreateDescriptor();

        // the entity should be in a valid state because the shape component and
        // the Terrain Physics Collider Component requirement is satisfied.
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_TRUE(sortOutcome.IsSuccess());
    }

    TEST_F(PhysXEditorFixture, EditorHeightfieldColliderComponentDependenciesMissingEntityIsInvalid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("HeightfieldColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorHeightfieldColliderComponent>();

        // the entity should not be in a valid state because the heightfield collider component requires
        // a shape component and the Terrain Physics Collider Component
        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::MissingRequiredService);
    }

    TEST_F(PhysXEditorFixture, EditorHeightfieldColliderComponentMultipleHeightfieldColliderComponentsEntityIsInvalid)
    {
        EntityPtr entity = CreateInactiveEditorEntity("HeightfieldColliderComponentEditorEntity");
        entity->CreateComponent<PhysX::EditorHeightfieldColliderComponent>();
        entity->CreateComponent(LmbrCentral::EditorAxisAlignedBoxShapeComponentTypeId);

        // adding a second heightfield collider component should make the entity invalid
        entity->CreateComponent<PhysX::EditorHeightfieldColliderComponent>();

        AZ::Entity::DependencySortOutcome sortOutcome = entity->EvaluateDependenciesGetDetails();
        EXPECT_FALSE(sortOutcome.IsSuccess());
        EXPECT_TRUE(sortOutcome.GetError().m_code == AZ::Entity::DependencySortResult::HasIncompatibleServices);
    }

    TEST_F(PhysXEditorFixture, EditorHeightfieldColliderComponentHeightfieldColliderWithCorrectComponentsCorrectRuntimeComponents)
    {
        EntityPtr editorEntity = SetupHeightfieldComponent();
        NiceMock<UnitTest::MockPhysXHeightfieldProvider> mockShapeRequests(editorEntity->GetId());
        SetupMockMethods(mockShapeRequests);
        editorEntity->Activate();

        EntityPtr gameEntity = TestCreateActiveGameEntityFromEditorEntity(editorEntity.get());
        NiceMock<UnitTest::MockPhysXHeightfieldProvider> mockShapeRequests2(gameEntity->GetId());
        SetupMockMethods(mockShapeRequests2);
        gameEntity->Activate();

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<UnitTest::MockPhysXHeightfieldProviderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::HeightfieldColliderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent(LmbrCentral::AxisAlignedBoxShapeComponentTypeId) != nullptr);

        CleanupHeightfieldComponent();
    }

    TEST_F(PhysXEditorHeightfieldFixture, EditorHeightfieldColliderComponentHeightfieldColliderWithAABoxCorrectRuntimeGeometry)
    {
        AZ::EntityId gameEntityId = m_gameEntity->GetId();

        AzPhysics::SimulatedBody* staticBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            staticBody, gameEntityId, &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a heightfield
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);

        physx::PxShape* shape = nullptr;
        pxRigidStatic->getShapes(&shape, 1, 0);
        EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eHEIGHTFIELD);

        physx::PxHeightFieldGeometry heightfieldGeometry;
        shape->getHeightFieldGeometry(heightfieldGeometry);

        physx::PxHeightField* heightfield = heightfieldGeometry.heightField;

        int32_t numRows{ 0 };
        int32_t numColumns{ 0 };
        Physics::HeightfieldProviderRequestsBus::Event(
            gameEntityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, numColumns, numRows);
        EXPECT_EQ(numColumns, heightfield->getNbColumns());
        EXPECT_EQ(numRows, heightfield->getNbRows());

        AZStd::vector<Physics::HeightMaterialPoint> samples;
        Physics::HeightfieldProviderRequestsBus::EventResult(
            samples, gameEntityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials);

        for (int sampleRow = 0; sampleRow < numRows; ++sampleRow)
        {
            for (int sampleColumn = 0; sampleColumn < numColumns; ++sampleColumn)
            {
                float minHeightBounds{ 0.0f };
                float maxHeightBounds{ 0.0f };
                Physics::HeightfieldProviderRequestsBus::Event(
                    gameEntityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldHeightBounds, minHeightBounds,
                    maxHeightBounds);

                const float halfBounds{ (maxHeightBounds - minHeightBounds) / 2.0f };
                const float scaleFactor = (maxHeightBounds <= minHeightBounds) ? 1.0f : AZStd::numeric_limits<int16_t>::max() / halfBounds;

                physx::PxHeightFieldSample samplePhysX = heightfield->getSample(sampleRow, sampleColumn);
                Physics::HeightMaterialPoint samplePhysics = samples[sampleRow * numColumns + sampleColumn];
                EXPECT_EQ(samplePhysX.height, azlossy_cast<physx::PxI16>(samplePhysics.m_height * scaleFactor));
            }
        }
    }

    TEST_F(PhysXEditorHeightfieldFixture, EditorHeightfieldColliderComponentHeightfieldColliderCorrectMaterials)
    {
        AZ::EntityId gameEntityId = m_gameEntity->GetId();

        int32_t numRows{ 0 };
        int32_t numColumns{ 0 };
        Physics::HeightfieldProviderRequestsBus::Event(
            gameEntityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, numColumns, numRows);

        EXPECT_EQ(numRows, 3);
        EXPECT_EQ(numColumns, 3);

        AZStd::vector<Physics::HeightMaterialPoint> samples;
        Physics::HeightfieldProviderRequestsBus::EventResult(
            samples, gameEntityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials);

        AzPhysics::SimulatedBody* staticBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            staticBody, gameEntityId, &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);

        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());
        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        physx::PxShape* shape = nullptr;
        pxRigidStatic->getShapes(&shape, 1, 0);

        physx::PxHeightFieldGeometry heightfieldGeometry;
        shape->getHeightFieldGeometry(heightfieldGeometry);

        physx::PxHeightField* heightfield = heightfieldGeometry.heightField;

        AZStd::vector<AZStd::string> physicsSurfaceTypes;
        for (Physics::MaterialId materialId : GetMaterialList())
        {
            physicsSurfaceTypes.emplace_back(materialId.GetUuid().ToString<AZStd::string>());
        }

        // PhysX Heightfield cooking doesn't map 1-1 sample material indices to triangle material indices 
        // Hence hardcoding the expected material indices in the test 
        const AZStd::array<int, 4> physicsMaterialsValidationDataIndex = {0, 2, 1, 1};

        for (int sampleRow = 0; sampleRow < numRows; ++sampleRow)
        {
            for (int sampleColumn = 0; sampleColumn < numColumns; ++sampleColumn)
            {
                physx::PxHeightFieldSample samplePhysX = heightfield->getSample(sampleRow, sampleColumn);

                auto [materialIndex0, materialIndex1] = PhysX::Utils::GetPhysXMaterialIndicesFromHeightfieldSamples(samples, sampleRow, sampleColumn, numRows, numColumns);
                EXPECT_EQ(samplePhysX.materialIndex0, materialIndex0);
                EXPECT_EQ(samplePhysX.materialIndex1, materialIndex1);

                if (sampleRow != numRows - 1 && sampleColumn != numColumns - 1)
                {
                    const float x_offset = -0.25f;
                    const float y_offset = 0.75f;
                    const float secondRayOffset = 0.5f;

                    float rayX = x_offset + sampleColumn;
                    float rayY = y_offset + sampleRow;

                    Physics::Material* mat1 = GetMaterialFromRaycast(rayX, rayY);
                    EXPECT_NE(mat1, nullptr);

                    Physics::Material* mat2 = GetMaterialFromRaycast(rayX + secondRayOffset, rayY + secondRayOffset);
                    EXPECT_NE(mat2, nullptr);

                    if (mat1)
                    {
                        AZStd::string expectedMaterialName = physicsSurfaceTypes[physicsMaterialsValidationDataIndex[sampleRow * 2 + sampleColumn]];
                        EXPECT_EQ(mat1->GetSurfaceTypeName(), expectedMaterialName);
                    }
                }
            }
        }
    }

} // namespace PhysXEditorTests

