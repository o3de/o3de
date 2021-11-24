/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TestColliderComponent.h"

#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>
#include <Tests/Viewport/ViewportUiManagerTests.cpp>

namespace UnitTest
{
    class PhysXColliderComponentModeTest
        : public ToolsApplicationFixture
    {
    protected:
        using EntityPtr = AZ::Entity*;

        AZ::ComponentId m_colliderComponentId;

        EntityPtr CreateColliderComponent()
        {
            AZ::Entity* entity = nullptr;
            AZ::EntityId entityId = CreateDefaultEditorEntity("ComponentModeEntity", &entity);

            entity->Deactivate();

            // Add placeholder component which implements component mode.
            auto colliderComponent = entity->CreateComponent<TestColliderComponentMode>();

            m_colliderComponentId = colliderComponent->GetId();

            entity->Activate();

            AzToolsFramework::SelectEntity(entityId);

            return entity;
        }

        // Needed to support ViewportUi request calls.
        ViewportManagerWrapper m_viewportManagerWrapper;

        void SetUpEditorFixtureImpl() override
        {
            m_viewportManagerWrapper.Create();
        }
        void TearDownEditorFixtureImpl() override
        {
            m_viewportManagerWrapper.Destroy();
        }
    };

    TEST_F(PhysXColliderComponentModeTest, MouseWheelUpShouldSetNextMode)
    {
        // Given there is a collider component in component mode.
        CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);

        // When the mouse wheel is scrolled while holding ctrl
        AzToolsFramework::ViewportInteraction::MouseInteractionEvent
            interactionEvent(AzToolsFramework::ViewportInteraction::MouseInteraction(), 1.0f);
        interactionEvent.m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Wheel;
        interactionEvent.m_mouseInteraction.m_keyboardModifiers.m_keyModifiers =
            static_cast<AZ::u32>(AzToolsFramework::ViewportInteraction::KeyboardModifier::Ctrl);

        using MouseInteractionResult = AzToolsFramework::ViewportInteraction::MouseInteractionResult;
        MouseInteractionResult handled = MouseInteractionResult::None;
        AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::BroadcastResult(handled,
            &AzToolsFramework::ViewportInteraction::InternalMouseViewportRequests::InternalHandleAllMouseInteractions,
            interactionEvent);

        // Then the component mode is cycled.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(handled, MouseInteractionResult::Viewport);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Dimensions, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, MouseWheelDownShouldSetPreviousMode)
    {
        // Given there is a collider component in component mode.
        CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);

        // When the mouse wheel is scrolled while holding ctrl
        AzToolsFramework::ViewportInteraction::MouseInteractionEvent
            interactionEvent(AzToolsFramework::ViewportInteraction::MouseInteraction(), -1.0f);
        interactionEvent.m_mouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent::Wheel;
        interactionEvent.m_mouseInteraction.m_keyboardModifiers.m_keyModifiers =
            static_cast<AZ::u32>(AzToolsFramework::ViewportInteraction::KeyboardModifier::Ctrl);

        using MouseInteractionResult = AzToolsFramework::ViewportInteraction::MouseInteractionResult;
        MouseInteractionResult handled = MouseInteractionResult::None;
        AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::BroadcastResult(handled,
            &AzToolsFramework::ViewportInteraction::InternalMouseViewportRequests::InternalHandleAllMouseInteractions,
            interactionEvent);

        // Then the component mode is cycled.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(handled, MouseInteractionResult::Viewport);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Rotation, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKey1ShouldSetOffsetMode)
    {
        // Given there is a collider component in component mode.
        CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);

        // When the '1' key is pressed
        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_1);

        // Then the component mode is set to Offset.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKey2ShouldSetRotationMode)
    {
        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        AzToolsFramework::SelectEntity(colliderEntity->GetId());
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);

        // When the '2' key is pressed
        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_2);

        // Then the component mode is set to Rotation.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Rotation, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKey3ShouldSetSizeMode)
    {
        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        AzToolsFramework::SelectEntity(colliderEntity->GetId());
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);

        // When the '3' key is pressed
        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_3);

        // Then the component mode is set to Size.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Dimensions, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetSphereRadius)
    {
        // Given there is a sphere collider in component mode.
        auto colliderEntity = CreateColliderComponent();
        float radius = 5.0f;
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetShapeType(Physics::ShapeType::Sphere);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetSphereRadius(radius);

        AzToolsFramework::SelectEntity(colliderEntity->GetId());
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_R);

        // The the sphere radius should be reset.
        radius = colliderEntity->FindComponent<TestColliderComponentMode>()->GetSphereRadius();
        EXPECT_FLOAT_EQ(0.5f, radius);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetCapsuleSize)
    {
        // Given there is a capsule collider in component mode.
        auto colliderEntity = CreateColliderComponent();
        float height = 10.0f;
        float radius = 2.5f;
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetShapeType(Physics::ShapeType::Capsule);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetCapsuleHeight(height);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetCapsuleRadius(radius);

        AzToolsFramework::SelectEntity(colliderEntity->GetId());
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_R);

        // Then the capsule size should be reset.
        height = colliderEntity->FindComponent<TestColliderComponentMode>()->GetCapsuleHeight();
        radius = colliderEntity->FindComponent<TestColliderComponentMode>()->GetCapsuleRadius();
        EXPECT_FLOAT_EQ(1.0f, height);
        EXPECT_FLOAT_EQ(0.25f, radius);
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetAssetScale)
    {
        // Given there is a sphere collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        AZ::Vector3 assetScale(10.0f, 10.0f, 10.0f);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetShapeType(Physics::ShapeType::PhysicsAsset);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetAssetScale(assetScale);

        AzToolsFramework::SelectEntity(colliderEntity->GetId());
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_R);

        // Then the asset scale should be reset.
        assetScale = colliderEntity->FindComponent<TestColliderComponentMode>()->GetAssetScale();
        EXPECT_THAT(assetScale, IsClose(AZ::Vector3::CreateOne()));
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetOffset)
    {
        // Given there is a sphere collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        AZ::Vector3 offset(5.0f, 6.0f, 7.0f);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetColliderOffset(offset);
        AzToolsFramework::SelectEntity(colliderEntity->GetId());
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Offset);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_R);

        // Then the collider offset should be reset.
        offset = colliderEntity->FindComponent<TestColliderComponentMode>()->GetColliderOffset();
        EXPECT_THAT(offset, IsClose(AZ::Vector3::CreateZero()));
    }

    TEST_F(PhysXColliderComponentModeTest, PressingKeyRShouldResetRotation)
    {
        // Given there is a sphere collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        AZ::Quaternion rotation = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), 45.0f);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetColliderRotation(rotation);
        AzToolsFramework::SelectEntity(colliderEntity->GetId());
        EnterComponentMode<TestColliderComponentMode>();

        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Rotation);

        // When the 'R' key is pressed
        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_R);

        // Then the collider rotation should be reset.
        rotation = colliderEntity->FindComponent<TestColliderComponentMode>()->GetColliderRotation();
        EXPECT_THAT(rotation, IsClose(AZ::Quaternion::CreateIdentity()));
    }

    TEST_F(PhysXColliderComponentModeTest, ClickingOffsetButtonShouldSetOffsetMode)
    {
        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        // Given
        // Check preconditions
        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);

        // Get the cluster and button Ids
        AzToolsFramework::ViewportUi::ClusterId modeSelectionClusterId;
        AzToolsFramework::ViewportUi::ButtonId offsetModeButtonId;

        AZ::EntityId entityId = colliderEntity->GetId();

        PhysX::ColliderComponentModeUiRequestBus::EventResult(
            modeSelectionClusterId, AZ::EntityComponentIdPair(entityId, m_colliderComponentId),
            &PhysX::ColliderComponentModeUiRequests::GetClusterId);
        PhysX::ColliderComponentModeUiRequestBus::EventResult(
            offsetModeButtonId, AZ::EntityComponentIdPair(entityId, m_colliderComponentId),
            &PhysX::ColliderComponentModeUiRequests::GetOffsetButtonId);

        // When
        // Trigger the button
        m_viewportManagerWrapper.GetViewportManager()->PressButton(modeSelectionClusterId, offsetModeButtonId);

        // Then
        // Then the component mode is set to Offset.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, ClickingRotationButtonShouldSetRotationMode)
    {
        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        // Given
        // Check preconditions
        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);

        // Get the cluster and button Ids
        AzToolsFramework::ViewportUi::ClusterId modeSelectionClusterId;
        AzToolsFramework::ViewportUi::ButtonId offsetModeButtonId;

        AZ::EntityId entityId = colliderEntity->GetId();

        PhysX::ColliderComponentModeUiRequestBus::EventResult(
            modeSelectionClusterId, AZ::EntityComponentIdPair(entityId, m_colliderComponentId),
            &PhysX::ColliderComponentModeUiRequests::GetClusterId);
        PhysX::ColliderComponentModeUiRequestBus::EventResult(
            offsetModeButtonId, AZ::EntityComponentIdPair(entityId, m_colliderComponentId),
            &PhysX::ColliderComponentModeUiRequests::GetRotationButtonId);

        // When
        // Trigger the button
        m_viewportManagerWrapper.GetViewportManager()->PressButton(modeSelectionClusterId, offsetModeButtonId);

        // Then
        // Then the component mode is set to Offset.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Rotation, subMode);
    }

    TEST_F(PhysXColliderComponentModeTest, ClickingDimensionsButtonShouldSetDimensionsMode)
    {
        // Given there is a collider component in component mode.
        auto colliderEntity = CreateColliderComponent();
        EnterComponentMode<TestColliderComponentMode>();

        // Given
        // Check preconditions
        PhysX::ColliderComponentModeRequests::SubMode subMode = PhysX::ColliderComponentModeRequests::SubMode::NumModes;
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Offset, subMode);

        // Get the cluster and button Ids
        AzToolsFramework::ViewportUi::ClusterId modeSelectionClusterId;
        AzToolsFramework::ViewportUi::ButtonId offsetModeButtonId;

        AZ::EntityId entityId = colliderEntity->GetId();

        PhysX::ColliderComponentModeUiRequestBus::EventResult(
            modeSelectionClusterId, AZ::EntityComponentIdPair(entityId, m_colliderComponentId),
            &PhysX::ColliderComponentModeUiRequests::GetClusterId);
        PhysX::ColliderComponentModeUiRequestBus::EventResult(
            offsetModeButtonId, AZ::EntityComponentIdPair(entityId, m_colliderComponentId),
            &PhysX::ColliderComponentModeUiRequests::GetDimensionsButtonId);

        // When
        // Trigger the button
        m_viewportManagerWrapper.GetViewportManager()->PressButton(modeSelectionClusterId, offsetModeButtonId);

        // Then
        // Then the component mode is set to Offset.
        PhysX::ColliderComponentModeRequestBus::BroadcastResult(subMode, &PhysX::ColliderComponentModeRequests::GetCurrentMode);
        EXPECT_EQ(PhysX::ColliderComponentModeRequests::SubMode::Dimensions, subMode);
    }

    using PhysXColliderComponentModeManipulatorTest =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<PhysXColliderComponentModeTest>;

    TEST_F(PhysXColliderComponentModeManipulatorTest, AssetScaleManipulatorsScaleInCorrectDirection)
    {
        auto colliderEntity = CreateColliderComponent();
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetShapeType(Physics::ShapeType::PhysicsAsset);
        colliderEntity->FindComponent<TestColliderComponentMode>()->SetAssetScale(AZ::Vector3::CreateOne());
        EnterComponentMode<TestColliderComponentMode>();
        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode,
            PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // position the camera so the X axis manipulator will be flipped
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationZ(-AZ::Constants::QuarterPi), AZ::Vector3(-5.0f, -5.0f, 0.0f)));

        // select a point in world space slightly displaced from the position of the entity in the negative x direction
        // in order to grab the X manipulator
        const float x = 0.1f;
        const float xDelta = 0.1f;
        const AZ::Vector3 worldStart(-x, 0.0f, 0.0f);

        // position in world space to drag to
        const AZ::Vector3 worldEnd(-(x + xDelta), 0.0f, 0.0f);

        const auto screenStart = AzFramework::WorldToScreen(worldStart, m_cameraState);
        const auto screenEnd = AzFramework::WorldToScreen(worldEnd, m_cameraState);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to interact with the x scale manipulator
            ->MousePosition(screenStart)
            // drag to move the manipulator
            ->MouseLButtonDown()
            ->MousePosition(screenEnd)
            ->MouseLButtonUp();

        const auto worldToScreenMultiplier = 1.0f / AzToolsFramework::CalculateScreenToWorldMultiplier(worldStart, m_cameraState);
        const auto assetScale = colliderEntity->FindComponent<TestColliderComponentMode>()->GetAssetScale();
        // need quite a large tolerance because using screen co-ordinates limits precision
        const float tolerance = 0.01f;
        EXPECT_NEAR(assetScale.GetX(), 1.0f + xDelta * worldToScreenMultiplier, tolerance);
        EXPECT_NEAR(assetScale.GetY(), 1.0f, tolerance);
        EXPECT_NEAR(assetScale.GetZ(), 1.0f, tolerance);
    }
} // namespace UnitTest
