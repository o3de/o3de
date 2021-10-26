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

using ::testing::NiceMock;
using ::testing::Return;

namespace PhysXEditorTests
{
    AZStd::vector<Physics::HeightMaterialPoint> GetSamples()
    {
        AZStd::vector<Physics::HeightMaterialPoint> samples{ { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                             { 2.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                             { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                             { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                             { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                             { -1.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                             { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                             { 0.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight },
                                                             { 3.0, Physics::QuadMeshType::SubdivideUpperLeftToBottomRight } };
        return samples;
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

    EntityPtr TestCreateActiveGameEntityFromEditorEntity(AZ::Entity* editorEntity)
    {
        EntityPtr gameEntity = AZStd::make_unique<AZ::Entity>();
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::PreExportEntity, *editorEntity, *gameEntity);
        gameEntity->Init();
        NiceMock<UnitTest::MockPhysXHeightfieldProvider> mockShapeRequests2(gameEntity->GetId());
        ON_CALL(mockShapeRequests2, GetHeightfieldTransform).WillByDefault(Return(AZ::Transform::CreateTranslation({ 1, 2, 0 })));
        ON_CALL(mockShapeRequests2, GetHeightfieldGridSpacing).WillByDefault(Return(AZ::Vector2(1, 1)));
        ON_CALL(mockShapeRequests2, GetHeightsAndMaterials).WillByDefault(Return(GetSamples()));
        ON_CALL(mockShapeRequests2, GetHeightfieldGridSize)
            .WillByDefault(
                [](int32_t& numColumns, int32_t& numRows)
                {
                    numColumns = 3;
                    numRows = 3;
                });
        ON_CALL(mockShapeRequests2, GetHeightfieldHeightBounds)
            .WillByDefault(
                [](float& x, float& y)
                {
                    x = 0.0f;
                    y = 0.0f;
                });
        gameEntity->Activate();
        return gameEntity;
    }


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
                    x = 0.0f;
                    y = 0.0f;
                });
        editorEntity->Activate();

        EntityPtr gameEntity = TestCreateActiveGameEntityFromEditorEntity(editorEntity.get());

        // check that the runtime entity has the expected components
        EXPECT_TRUE(gameEntity->FindComponent<UnitTest::MockPhysXHeightfieldProviderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent<PhysX::HeightfieldColliderComponent>() != nullptr);
        EXPECT_TRUE(gameEntity->FindComponent(LmbrCentral::AxisAlignedBoxShapeComponentTypeId) != nullptr);

        CleanupHeightfieldComponent();
    }

    TEST_F(PhysXEditorFixture, EditorHeightfieldColliderComponentHeightfieldColliderWithAABoxCorrectRuntimeGeometry)
    {
        EntityPtr editorEntity = SetupHeightfieldComponent();
        NiceMock<UnitTest::MockPhysXHeightfieldProvider> mockShapeRequests(editorEntity->GetId());
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
                    x = 0.0f;
                    y = 0.0f;
                });
        editorEntity->Activate();

        EntityPtr gameEntity = TestCreateActiveGameEntityFromEditorEntity(editorEntity.get());

        AzPhysics::SimulatedBody* staticBody = nullptr;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(
            staticBody, gameEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequests::GetSimulatedBody);
        const auto* pxRigidStatic = static_cast<const physx::PxRigidStatic*>(staticBody->GetNativePointer());

        PHYSX_SCENE_READ_LOCK(pxRigidStatic->getScene());

        // there should be a single shape on the rigid body and it should be a heightfield
        EXPECT_EQ(pxRigidStatic->getNbShapes(), 1);

        physx::PxShape* shape = nullptr;
        pxRigidStatic->getShapes(&shape, 1, 0);
        EXPECT_EQ(shape->getGeometryType(), physx::PxGeometryType::eHEIGHTFIELD);

        CleanupHeightfieldComponent();
    }

} // namespace PhysXEditorTests

