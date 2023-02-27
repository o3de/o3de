/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysXColliderComponentModeTests.h>
#include <Source/EditorRigidBodyComponent.h>
#include <EditorColliderComponent.h>

namespace UnitTest
{
    static const float uniformScale = 1.0f;
    static const AZ::Quaternion ShapeRotation = AZ::Quaternion(0, 0, 0, 1);
    static const AZ::Quaternion EntityRotation = AZ::Quaternion(0, 0, 0, 1);
    static const AZ::Vector3 ShapeOffset = AZ::Vector3(0, 0, 0);
    static const AZ::Vector3 EntityTranslation = AZ::Vector3(5.0f, 15.0f, 10.0f);

    class ColliderPickingFixture : public PhysXEditorColliderComponentManipulatorFixture
    {
    public:
        void SetUpEditorFixtureImpl() override;
        //! Clicks at a given position and returns the entities that are selected.
        AzToolsFramework::EntityIdList ClickAndGetSelectedEntities(AzFramework::ScreenPoint screenPoint);
    };

    void ColliderPickingFixture::SetUpEditorFixtureImpl()
    {
        PhysXEditorColliderComponentManipulatorFixture::SetUpEditorFixtureImpl(); 

        m_cameraState.m_viewportSize = AzFramework::ScreenSize(1920, 1080);
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromEulerAnglesDegrees(AZ::Vector3(0.0f, 0.0f, 90.0f)), AZ::Vector3(20.0f, 15.0f, 10.0f)));

         m_actionDispatcher->CameraState(m_cameraState);
    }

    AzToolsFramework::EntityIdList ColliderPickingFixture::ClickAndGetSelectedEntities(AzFramework::ScreenPoint screenPoint)
    {
        // click the entity in the viewport
        m_actionDispatcher->SetStickySelect(true)
            ->CameraState(m_cameraState)
            ->MousePosition(screenPoint)
            ->MouseLButtonDown()
            ->MouseLButtonUp();

        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntities, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetSelectedEntities);
        return selectedEntities;
    }

    TEST_F(ColliderPickingFixture, ColliderPickingWithBoxShape)
    {
         // Given the setup conditions
         const AZ::Vector3 boxDimensions(5.0f, 5.0f, 5.0f);
         SetupCollider(Physics::BoxShapeConfiguration(boxDimensions), ShapeRotation, ShapeOffset);
         SetupTransform(EntityRotation, EntityTranslation, uniformScale);

         // When a user clicks just outside the collider it should not be selected
         auto clickPos1 = AzFramework::WorldToScreen(AZ::Vector3(7.5f, 12.4f, 10.0f), m_cameraState);
         auto selectedEntities = ClickAndGetSelectedEntities(clickPos1);

         EXPECT_THAT(selectedEntities.size(), testing::Eq(0));

         // Then when a user clicks inside the collider it should be selected
         auto clickPos2 = AzFramework::WorldToScreen(AZ::Vector3(7.5f, 12.6f, 10.0f), m_cameraState);
         selectedEntities = ClickAndGetSelectedEntities(clickPos2);

         EXPECT_THAT(selectedEntities.size(), testing::Eq(1));
         EXPECT_THAT(selectedEntities.front(), ::testing::Eq(m_entity->GetId()));
    }

    TEST_F(ColliderPickingFixture, ColliderPickingWithBoxShapeAndRigidBodyComponent)
    {
         // Given the setup conditions
         const AZ::Vector3 boxDimensions(5.0f, 5.0f, 5.0f);
         SetupTransform(EntityRotation, EntityTranslation, uniformScale);

         // The collider should be selectable with a collider and rigid body component
         m_entity->Deactivate();
         m_entity->CreateComponent<PhysX::EditorColliderComponent>(Physics::ColliderConfiguration(), Physics::BoxShapeConfiguration(boxDimensions));
         m_entity->CreateComponent<PhysX::EditorRigidBodyComponent>();
         m_entity->Activate();

         // When a user clicks just outside the collider it should not be picked
         auto clickPos1 = AzFramework::WorldToScreen(AZ::Vector3(7.5f, 12.4f, 10.0f), m_cameraState);
         auto selectedEntities = ClickAndGetSelectedEntities(clickPos1);

         EXPECT_THAT(selectedEntities.size(), testing::Eq(0));

         // Then when a user clicks inside the collider it should be selected
         auto clickPos2 = AzFramework::WorldToScreen(AZ::Vector3(7.5f, 12.6f, 10.0f), m_cameraState);
         selectedEntities = ClickAndGetSelectedEntities(clickPos2);

         EXPECT_THAT(selectedEntities.size(), testing::Eq(1));
         EXPECT_THAT(selectedEntities.front(), ::testing::Eq(m_entity->GetId()));
    }

    TEST_F(ColliderPickingFixture, ColliderPickingWithSphereShape)
    {
         // Given the setup conditions
         SetupCollider(Physics::SphereShapeConfiguration(2.5f), ShapeRotation, ShapeOffset);
         SetupTransform(EntityRotation, EntityTranslation, uniformScale);

         // When a user clicks just outside the collider it should not be picked
         auto clickPos1 = AzFramework::WorldToScreen(AZ::Vector3(5.0f, 12.4f, 10.0f), m_cameraState);
         auto selectedEntities = ClickAndGetSelectedEntities(clickPos1);

         EXPECT_THAT(selectedEntities.size(), testing::Eq(0));

         // Then when a user clicks inside the collider it should be selected
         auto clickPos2 = AzFramework::WorldToScreen(AZ::Vector3(5.0f, 12.6f, 10.0f), m_cameraState);
         selectedEntities = ClickAndGetSelectedEntities(clickPos2);

         EXPECT_THAT(selectedEntities.size(), testing::Eq(1));
         EXPECT_THAT(selectedEntities.front(), ::testing::Eq(m_entity->GetId()));
    }

    TEST_F(ColliderPickingFixture, ColliderPickingWithCapsuleShape)
    {
         // Given the setup conditions
         SetupCollider(Physics::CapsuleShapeConfiguration(5.0f, 2.5f), ShapeRotation, ShapeOffset);
         SetupTransform(EntityRotation, EntityTranslation, uniformScale);

         // When a user clicks just outside the collider it should not be picked
         auto clickPos1 = AzFramework::WorldToScreen(AZ::Vector3(5.0f, 12.4f, 10.0f), m_cameraState);
         auto selectedEntities = ClickAndGetSelectedEntities(clickPos1);

         EXPECT_THAT(selectedEntities.size(), testing::Eq(0));

         // Then when a user clicks inside the collider it should be selected
         auto clickPos2 = AzFramework::WorldToScreen(AZ::Vector3(5.0f, 12.6f, 10.0f), m_cameraState);
         selectedEntities = ClickAndGetSelectedEntities(clickPos2);

         EXPECT_THAT(selectedEntities.size(), testing::Eq(1));
         EXPECT_THAT(selectedEntities.front(), ::testing::Eq(m_entity->GetId()));
    }
} // namespace UnitTest
