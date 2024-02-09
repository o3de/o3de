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
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>
#include <StaticRigidBodyComponent.h>
#include <RigidBodyStatic.h>
#include <PhysX/PhysXLocks.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>
#include <PhysX/MockPhysXHeightfieldProviderComponent.h>
#include <PhysX/Material/PhysXMaterial.h>
#include <PhysX/Material/PhysXMaterialConfiguration.h>
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

    AZ::Data::Asset<Physics::MaterialAsset> FindOrCreateMaterialAsset(AZ::Data::AssetId assetId)
    {
        AZ::Data::Asset<Physics::MaterialAsset> materialAsset =
            AZ::Data::AssetManager::Instance().FindAsset<Physics::MaterialAsset>(assetId , AZ::Data::AssetLoadBehavior::Default);

        if (!materialAsset)
        {
            const PhysX::MaterialConfiguration defaultMaterialConfiguration;
            const Physics::MaterialAsset::MaterialProperties materialProperties =
            {
                {PhysX::MaterialConstants::DynamicFrictionName, defaultMaterialConfiguration.m_dynamicFriction},
                {PhysX::MaterialConstants::StaticFrictionName, defaultMaterialConfiguration.m_staticFriction},
                {PhysX::MaterialConstants::RestitutionName, defaultMaterialConfiguration.m_restitution},
                {PhysX::MaterialConstants::DensityName, defaultMaterialConfiguration.m_density},
                {PhysX::MaterialConstants::RestitutionCombineModeName, static_cast<AZ::u32>(defaultMaterialConfiguration.m_restitutionCombine)},
                {PhysX::MaterialConstants::FrictionCombineModeName, static_cast<AZ::u32>(defaultMaterialConfiguration.m_frictionCombine)},
                {PhysX::MaterialConstants::CompliantContactModeEnabledName, defaultMaterialConfiguration.m_compliantContactMode.m_enabled},
                {PhysX::MaterialConstants::CompliantContactModeDampingName, defaultMaterialConfiguration.m_compliantContactMode.m_damping},
                {PhysX::MaterialConstants::CompliantContactModeStiffnessName, defaultMaterialConfiguration.m_compliantContactMode.m_stiffness},
                {PhysX::MaterialConstants::DebugColorName, defaultMaterialConfiguration.m_debugColor}
            };

            materialAsset =
                AZ::Data::AssetManager::Instance().CreateAsset<Physics::MaterialAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);

            if (!materialAsset)
            {
                AZ_Error("PhysXEditorTests", false, "Failed to create material asset with id '%s'", assetId.ToString<AZStd::string>().c_str());
                return {};
            }

            materialAsset->SetData(
                PhysX::MaterialConstants::MaterialAssetType,
                PhysX::MaterialConstants::MaterialAssetVersion,
                materialProperties);
        }

        return materialAsset;
    }

    AZStd::vector<AZ::Data::Asset<Physics::MaterialAsset>> GetMaterialList()
    {
        AZ::Data::Asset<Physics::MaterialAsset> materialAsset1 =
            FindOrCreateMaterialAsset(AZ::Uuid::CreateString("{EC976D51-2C26-4C1E-BBF2-75BAAAFA162C}"));

        AZ::Data::Asset<Physics::MaterialAsset> materialAsset2 =
            FindOrCreateMaterialAsset(AZ::Uuid::CreateString("{B9836F51-A235-4781-95E3-A6302BEE9EFF}"));

        AZ::Data::Asset<Physics::MaterialAsset> materialAsset3 =
            FindOrCreateMaterialAsset(AZ::Uuid::CreateString("{7E060707-BB03-47EB-B046-4503C7145B6E}"));

        return {
            materialAsset1,
            materialAsset2,
            materialAsset3
        };
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
        ON_CALL(mockShapeRequests, GetHeightfieldAabb).WillByDefault(
            Return(AZ::Aabb::CreateFromMinMaxValues(0.0f, 0.0f, -3.0f, 3.0f, 3.0f, 3.0f)));
        ON_CALL(mockShapeRequests, GetHeightfieldGridSize)
            .WillByDefault(
                [](size_t& numColumns, size_t& numRows)
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

        ON_CALL(mockShapeRequests, GetHeightfieldIndicesFromRegion)
            .WillByDefault(
                []([[maybe_unused]] const AZ::Aabb& region, size_t& startColumn, size_t& startRow, size_t& numColumns, size_t& numRows)
                {
                    startColumn = 0;
                    startRow = 0;
                    numColumns = 3;
                    numRows = 3;
                });


        ON_CALL(mockShapeRequests, UpdateHeightsAndMaterials)
            .WillByDefault(
                [](const Physics::UpdateHeightfieldSampleFunction& updateHeightsMaterialsCallback, [[maybe_unused]] size_t startColumn,
                   [[maybe_unused]] size_t startRow, [[maybe_unused]] size_t numColumns, [[maybe_unused]] size_t numRows)
                {
                    auto samples = GetSamples();
                    for (size_t row = 0; row < 3; row++)
                    {
                        for (size_t col = 0; col < 3; col++)
                        {
                            updateHeightsMaterialsCallback(col, row, samples[(row * 3) + col]);
                        }
                    }
                });

        ON_CALL(mockShapeRequests, UpdateHeightsAndMaterialsAsync)
            .WillByDefault(
                [](const Physics::UpdateHeightfieldSampleFunction& updateHeightsMaterialsCallback,
                   const Physics::UpdateHeightfieldCompleteFunction& updateHeightsMaterialsCompleteCallback,
                   [[maybe_unused]] size_t startColumn,
                   [[maybe_unused]] size_t startRow,
                   [[maybe_unused]] size_t numColumns,
                   [[maybe_unused]] size_t numRows)
                {
                    auto samples = GetSamples();
                    for (size_t row = 0; row < 3; row++)
                    {
                        for (size_t col = 0; col < 3; col++)
                        {
                            updateHeightsMaterialsCallback(col, row, samples[(row * 3) + col]);
                        }
                    }

                    updateHeightsMaterialsCompleteCallback();
                });
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

            m_editorEntity = SetupHeightfieldComponent();
            m_editorMockShapeRequests = AZStd::make_unique<NiceMock<UnitTest::MockPhysXHeightfieldProvider>>(m_editorEntity->GetId());
            SetupMockMethods(*m_editorMockShapeRequests.get());
            m_editorEntity->Activate();

            // Notify the Editor entity that the heightfield data changed so that it refreshes itself before we build
            // the corresponding game entity.
            Physics::HeightfieldProviderNotificationBus::Event(
                m_editorEntity->GetId(),
                &Physics::HeightfieldProviderNotificationBus::Events::OnHeightfieldDataChanged, AZ::Aabb::CreateNull(),
                Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::Settings);

            m_gameEntity = TestCreateActiveGameEntityFromEditorEntity(m_editorEntity.get());
            m_gameMockShapeRequests = AZStd::make_unique<NiceMock<UnitTest::MockPhysXHeightfieldProvider>>(m_gameEntity->GetId());
            SetupMockMethods(*m_gameMockShapeRequests.get());
            m_gameEntity->Activate();

            // Send the notification a second time so that the game entity gets refreshed as well.
            Physics::HeightfieldProviderNotificationBus::Event(
                m_gameEntity->GetId(),
                &Physics::HeightfieldProviderNotificationBus::Events::OnHeightfieldDataChanged, AZ::Aabb::CreateNull(),
                Physics::HeightfieldProviderNotifications::HeightfieldChangeMask::Settings);

            // The updates are performed asynchronously, so block on the jobs until they're completed.
            auto editorHeightfieldComponent = m_editorEntity->FindComponent<PhysX::EditorHeightfieldColliderComponent>();
            editorHeightfieldComponent->BlockOnPendingJobs();
            auto runtimeHeightfieldComponent = m_gameEntity->FindComponent<PhysX::HeightfieldColliderComponent>();
            runtimeHeightfieldComponent->BlockOnPendingJobs();
        }

        void TearDown() override
        {
            if (auto* materialManager = AZ::Interface<Physics::MaterialManager>::Get())
            {
                materialManager->DeleteAllMaterials();
            }

            CleanupHeightfieldComponent();

            m_editorEntity = nullptr;
            m_gameEntity = nullptr;
            m_editorMockShapeRequests = nullptr;
            m_gameMockShapeRequests = nullptr;

            PhysXEditorFixture::TearDown();
        }

        Physics::MaterialId GetMaterialFromRaycast(float x, float y)
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
                return result.m_hits[0].m_physicsMaterialId;
            }

            return {};
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

        // Make sure to deactivate the entities before destroying the mocks, or else it's possible to get deadlocked.
        gameEntity->Deactivate();
        editorEntity->Deactivate();

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

        size_t numRows{ 0 };
        size_t numColumns{ 0 };
        Physics::HeightfieldProviderRequestsBus::Event(
            gameEntityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, numColumns, numRows);
        EXPECT_EQ(numColumns, heightfield->getNbColumns());
        EXPECT_EQ(numRows, heightfield->getNbRows());

        AZStd::vector<Physics::HeightMaterialPoint> samples;
        Physics::HeightfieldProviderRequestsBus::EventResult(
            samples, gameEntityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials);

        float minHeightBounds{ 0.0f };
        float maxHeightBounds{ 0.0f };
        Physics::HeightfieldProviderRequestsBus::Event(
            gameEntityId, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldHeightBounds, minHeightBounds, maxHeightBounds);

        const float halfBounds{ (maxHeightBounds - minHeightBounds) / 2.0f };
        const float scaleFactor = (maxHeightBounds <= minHeightBounds) ? 1.0f : AZStd::numeric_limits<int16_t>::max() / halfBounds;

        for (int sampleRow = 0; sampleRow < numRows; ++sampleRow)
        {
            for (int sampleColumn = 0; sampleColumn < numColumns; ++sampleColumn)
            {
                physx::PxHeightFieldSample samplePhysX = heightfield->getSample(sampleRow, sampleColumn);
                Physics::HeightMaterialPoint samplePhysics = samples[sampleRow * numColumns + sampleColumn];
                EXPECT_EQ(samplePhysX.height, azlossy_cast<physx::PxI16>(samplePhysics.m_height * scaleFactor));
            }
        }
    }

    TEST_F(PhysXEditorHeightfieldFixture, EditorHeightfieldColliderComponentHeightfieldColliderCorrectMaterials)
    {
        AZ::EntityId gameEntityId = m_gameEntity->GetId();

        size_t numRows{ 0 };
        size_t numColumns{ 0 };
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

        const AZStd::vector<AZ::Data::Asset<Physics::MaterialAsset>> physicsMaterialAssets = GetMaterialList();

        // Our heightfield is located in the world as follows:
        // - entity center is (0, 0)
        // - mocked heightfield is 3 samples spaced at 1 m intervals, so it's a heightfield size of (2, 2)
        // - mocked heightfield transform returns the heightfield center at (1, 2)
        // - final heightfield goes from (0, 1) - (2, 3)
        // Note: entity also has a box of size (1, 1) on it, but since we've mocked the heightfield provider, the box is ignored
        const float heightfieldMinCornerX = 0.0f;
        const float heightfieldMinCornerY = 1.0f;

        // PhysX Heightfield cooking doesn't map 1-1 sample material indices to triangle material indices 
        // Hence hardcoding the expected material indices in the test 
        const AZStd::array<int, 4> physicsMaterialsValidationDataIndex = {0, 2, 1, 1};

        for (size_t sampleRow = 0; sampleRow < numRows; ++sampleRow)
        {
            for (size_t sampleColumn = 0; sampleColumn < numColumns; ++sampleColumn)
            {
                physx::PxHeightFieldSample samplePhysX = heightfield->getSample(static_cast<physx::PxU32>(sampleRow), static_cast<physx::PxU32>(sampleColumn));

                auto [materialIndex0, materialIndex1] =
                    PhysX::Utils::GetPhysXMaterialIndicesFromHeightfieldSamples(samples, sampleColumn, sampleRow, numColumns, numRows);
                EXPECT_EQ(samplePhysX.materialIndex0, materialIndex0);
                EXPECT_EQ(samplePhysX.materialIndex1, materialIndex1);

                if (sampleRow != numRows - 1 && sampleColumn != numColumns - 1)
                {
                    // There are two materials per quad, so we'll perform 1 raycast per triangle per quad.
                    // Our quads are 1 m in size, so a ray at (1/4 m, 1/4 m) and (3/4 m, 3/4 m) in each quad should hit the two triangles.
                    const float x_offset = heightfieldMinCornerX + 0.25f;
                    const float y_offset = heightfieldMinCornerY + 0.25f;
                    const float secondRayOffset = 0.5f;

                    float rayX = x_offset + sampleColumn;
                    float rayY = y_offset + sampleRow;

                    Physics::MaterialId matId1 = GetMaterialFromRaycast(rayX, rayY);
                    EXPECT_TRUE(matId1.IsValid());

                    Physics::MaterialId matId2 = GetMaterialFromRaycast(rayX + secondRayOffset, rayY + secondRayOffset);
                    EXPECT_TRUE(matId2.IsValid());

                    if (matId1.IsValid())
                    {
                        const AZ::Data::Asset<Physics::MaterialAsset> expectedMaterialAsset = physicsMaterialAssets[physicsMaterialsValidationDataIndex[sampleRow * 2 + sampleColumn]];

                        AZStd::shared_ptr<Physics::Material> mat1 = AZ::Interface<Physics::MaterialManager>::Get()->GetMaterial(matId1);

                        EXPECT_TRUE(mat1.get() != nullptr);
                        if (mat1)
                        {
                            EXPECT_EQ(mat1->GetMaterialAsset(), expectedMaterialAsset);
                        }
                    }
                }
            }
        }
    }

} // namespace PhysXEditorTests

