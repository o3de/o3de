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
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <Tests/Viewport/ViewportUiManagerTests.cpp>
#include <EditorColliderComponent.h>

namespace UnitTest
{
    class PhysXColliderComponentModeTest
        : public ToolsApplicationFixture<false>
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

    class PhysXEditorColliderComponentFixture : public UnitTest::ToolsApplicationFixture<false>
    {
    public:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;
        void SetupTransform(const AZ::Quaternion& rotation, const AZ::Vector3& translation, float uniformScale);
        void SetupCollider(
            const Physics::ShapeConfiguration& shapeConfiguration,
            const AZ::Quaternion& colliderRotation,
            const AZ::Vector3& colliderOffset);
        void SetupNonUniformScale(const AZ::Vector3& nonUniformScale);
        void EnterColliderSubMode(PhysX::ColliderComponentModeRequests::SubMode subMode);

        AZ::Entity* m_entity = nullptr;
        AZ::EntityComponentIdPair m_idPair;
    };

    void PhysXEditorColliderComponentFixture::SetUpEditorFixtureImpl()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        UnitTest::CreateDefaultEditorEntity("EditorColliderComponentEntity", &m_entity);
    }

    void PhysXEditorColliderComponentFixture::TearDownEditorFixtureImpl()
    {
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_entity->GetId());
        m_entity = nullptr;
    }

    void PhysXEditorColliderComponentFixture::SetupTransform(
        const AZ::Quaternion& rotation, const AZ::Vector3& translation, float uniformScale)
    {
        const AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(rotation, translation);
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetLocalUniformScale, uniformScale);
    }

    void PhysXEditorColliderComponentFixture::SetupCollider(
        const Physics::ShapeConfiguration& shapeConfiguration, const AZ::Quaternion& colliderRotation, const AZ::Vector3& colliderOffset)
    {
        m_entity->Deactivate();
        auto* colliderComponent =
            m_entity->CreateComponent<PhysX::EditorColliderComponent>(Physics::ColliderConfiguration(), shapeConfiguration);
        m_entity->Activate();
        m_idPair = AZ::EntityComponentIdPair(m_entity->GetId(), colliderComponent->GetId());
        PhysX::EditorColliderComponentRequestBus::Event(
            m_idPair, &PhysX::EditorColliderComponentRequests::SetColliderOffset, colliderOffset);
        PhysX::EditorColliderComponentRequestBus::Event(
            m_idPair, &PhysX::EditorColliderComponentRequests::SetColliderRotation, colliderRotation);
    }

    void PhysXEditorColliderComponentFixture::SetupNonUniformScale(const AZ::Vector3& nonUniformScale)
    {
        m_entity->Deactivate();
        m_entity->CreateComponent(AzToolsFramework::Components::EditorNonUniformScaleComponent::RTTI_Type());
        m_entity->Activate();
        AZ::NonUniformScaleRequestBus::Event(m_entity->GetId(), &AZ::NonUniformScaleRequests::SetScale, nonUniformScale);
    }

    void PhysXEditorColliderComponentFixture::EnterColliderSubMode(PhysX::ColliderComponentModeRequests::SubMode subMode)
    {
        AzToolsFramework::SelectEntity(m_entity->GetId());
        EnterComponentMode<PhysX::EditorColliderComponent>();
        PhysX::ColliderComponentModeRequestBus::Broadcast(&PhysX::ColliderComponentModeRequests::SetCurrentMode, subMode);
    }

    using PhysXEditorColliderComponentManipulatorFixture =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<PhysXEditorColliderComponentFixture>;

    // use a reasonably large tolerance because manipulator precision is limited by viewport resolution
    static const float ManipulatorTolerance = 0.01f;

    TEST_F(PhysXEditorColliderComponentManipulatorFixture, OffsetManipulatorsCorrectlyLocatedRelativeToCollider)
    {
        const AZ::Vector3 boxDimensions(2.0f, 3.0f, 1.5f);
        const AZ::Quaternion boxRotation(0.1f, 0.1f, 0.7f, 0.7f);
        const AZ::Vector3 boxOffset(3.0f, 1.0f, 2.0f);
        SetupCollider(Physics::BoxShapeConfiguration(boxDimensions), boxRotation, boxOffset);
        const AZ::Quaternion entityRotation(0.8f, 0.2f, 0.4f, 0.4f);
        const AZ::Vector3 entityTranslation(2.0f, -3.0f, 0.5f);
        const float uniformScale = 2.0f;
        SetupTransform(entityRotation, entityTranslation, uniformScale);
        EnterColliderSubMode(PhysX::ColliderComponentModeRequests::SubMode::Offset);

        // the expected position of the collider centre based on the combination of entity transform and collider offset
        const AZ::Vector3 expectedColliderPosition(8.8f, -2.28f, 3.54f);

        // the expected world space direction of the collider offset x-axis based on the entity transform
        const AZ::Vector3 expectedXAxis(0.6f, 0.64f, 0.48f);

        // position the camera to look down at the collider from above
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationX(-AZ::Constants::HalfPi), expectedColliderPosition + AZ::Vector3::CreateAxisZ(10.0f)));

        // position in world space, slightly moved along the x-axis in order to grab the x translation manipulator
        const AZ::Vector3 worldStart = expectedColliderPosition + 0.5f * expectedXAxis;

        // position in world space to move to
        const AZ::Vector3 worldEnd = worldStart + 2.0f * expectedXAxis;

        const auto screenStart = AzFramework::WorldToScreen(worldStart, m_cameraState);
        const auto screenEnd = AzFramework::WorldToScreen(worldEnd, m_cameraState);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to the position of the x offset manipulator
            ->MousePosition(screenStart)
            // drag to move the manipulator
            ->MouseLButtonDown()
            ->MousePosition(screenEnd)
            ->MouseLButtonUp();

        AZ::Vector3 newColliderOffset = AZ::Vector3::CreateZero();
        PhysX::EditorColliderComponentRequestBus::EventResult(
            newColliderOffset, m_idPair, &PhysX::EditorColliderComponentRequests::GetColliderOffset);

        EXPECT_THAT(newColliderOffset, IsCloseTolerance(AZ::Vector3(4.0f, 1.0f, 2.0f), ManipulatorTolerance));
    }

    TEST_F(PhysXEditorColliderComponentManipulatorFixture, OffsetManipulatorsCorrectlyLocatedRelativeToColliderWithNonUniformScale)
    {
        const float capsuleRadius = 0.5f;
        const float capsuleHeight = 2.0f;
        const AZ::Quaternion capsuleRotation(0.2f, -0.4f, 0.8f, 0.4f);
        const AZ::Vector3 capsuleOffset(-2.0f, 3.0f, -1.0f);
        SetupCollider(Physics::CapsuleShapeConfiguration(capsuleHeight, capsuleRadius), capsuleRotation, capsuleOffset);
        const AZ::Quaternion entityRotation(-0.1f, 0.7f, -0.7f, 0.1f);
        const AZ::Vector3 entityTranslation(-1.0f, 1.0f, -2.5f);
        const float uniformScale = 1.5f;
        SetupTransform(entityRotation, entityTranslation, uniformScale);
        const AZ::Vector3 nonUniformScale(2.0f, 0.5f, 1.5f);
        SetupNonUniformScale(nonUniformScale);
        EnterColliderSubMode(PhysX::ColliderComponentModeRequests::SubMode::Offset);

        // the expected position of the collider centre based on the combination of entity transform, collider offset and non-uniform scale
        const AZ::Vector3 expectedColliderPosition(4.13f, 4.84f, -4.75f);

        // the expected world space direction of the collider offset z-axis based on the entity transform
        const AZ::Vector3 expectedZAxis(0.28f, -0.96f, 0.0f);

        // position the camera to look at the collider from underneath
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi), expectedColliderPosition - AZ::Vector3::CreateAxisZ(10.0f)));

        // position in world space, slightly moved along the z-axis in order to grab the z translation manipulator
        // need to go in the negative z direction because the camera angle causes the manipulator to flip
        const AZ::Vector3 worldStart = expectedColliderPosition - 0.5f * expectedZAxis;

        // position in world space to move to
        const AZ::Vector3 worldEnd = worldStart - 2.25f * expectedZAxis;

        const auto screenStart = AzFramework::WorldToScreen(worldStart, m_cameraState);
        const auto screenEnd = AzFramework::WorldToScreen(worldEnd, m_cameraState);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to the position of the z offset manipulator
            ->MousePosition(screenStart)
            // drag to move the manipulator
            ->MouseLButtonDown()
            ->MousePosition(screenEnd)
            ->MouseLButtonUp();

        AZ::Vector3 newColliderOffset = AZ::Vector3::CreateZero();
        PhysX::EditorColliderComponentRequestBus::EventResult(
            newColliderOffset, m_idPair, &PhysX::EditorColliderComponentRequests::GetColliderOffset);

        EXPECT_THAT(newColliderOffset, IsCloseTolerance(AZ::Vector3(-2.0f, 3.0f, -2.0f), ManipulatorTolerance));
    }

    TEST_F(PhysXEditorColliderComponentManipulatorFixture, BoxColliderScaleManipulatorsCorrectlyLocatedRelativeToColliderWithNonUniformScale)
    {
        const AZ::Vector3 boxDimensions(2.0f, 2.0f, 3.0f);
        const AZ::Quaternion boxRotation(0.7f, 0.7f, -0.1f, 0.1f);
        const AZ::Vector3 boxOffset(0.5f, 1.5f, 2.0f);
        SetupCollider(Physics::BoxShapeConfiguration(boxDimensions), boxRotation, boxOffset);
        const AZ::Quaternion entityRotation(0.2f, 0.4f, -0.4f, 0.8f);
        const AZ::Vector3 entityTranslation(2.0f, -3.0f, -2.0f);
        const float uniformScale = 0.5f;
        SetupTransform(entityRotation, entityTranslation, uniformScale);
        const AZ::Vector3 nonUniformScale(3.0f, 1.5f, 2.5f);
        SetupNonUniformScale(nonUniformScale);
        EnterColliderSubMode(PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // the expected position of the collider centre based on the combination of entity transform, collider offset and non-uniform scale
        const AZ::Vector3 expectedColliderPosition(4.37f, -4.285f, -1.1f);

        // the expected position of the y scale manipulator relative to the centre of the collider, based on collider
        // rotation, entity rotation and scale, and non-uniform scale
        const AZ::Vector3 scaleManipulatorYDelta(0.54f, -0.72f, -1.2f);

        // position the camera to look at the collider along the x-y diagonal
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationZ(-AZ::Constants::QuarterPi), expectedColliderPosition - AZ::Vector3(2.0f, 2.0f, 0.0f)));

        const AZ::Vector3 worldStart = expectedColliderPosition + scaleManipulatorYDelta;
        const AZ::Vector3 worldEnd = worldStart + 0.1f * scaleManipulatorYDelta;

        const auto screenStart = AzFramework::WorldToScreen(worldStart, m_cameraState);
        const auto screenEnd = AzFramework::WorldToScreen(worldEnd, m_cameraState);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to the position of the y scale manipulator
            ->MousePosition(screenStart)
            // drag to move the manipulator
            ->MouseLButtonDown()
            ->MousePosition(screenEnd)
            ->MouseLButtonUp();

        AZ::Vector3 newBoxDimensions = AZ::Vector3::CreateZero();
        AzToolsFramework::BoxManipulatorRequestBus::EventResult(
            newBoxDimensions, m_idPair, &AzToolsFramework::BoxManipulatorRequests::GetDimensions);

        EXPECT_THAT(newBoxDimensions, IsCloseTolerance(AZ::Vector3(2.0f, 2.2f, 3.0f), ManipulatorTolerance));
    }

    TEST_F(PhysXEditorColliderComponentManipulatorFixture, SphereColliderScaleManipulatorsCorrectlyLocatedRelativeToColliderWithNonUniformScale)
    {
        const float sphereRadius = 1.0f;
        const AZ::Quaternion sphereRotation(-0.1f, 0.7f, -0.7f, 0.1f);
        const AZ::Vector3 sphereOffset(-2.0f, 1.0f, -3.0f);
        SetupCollider(Physics::SphereShapeConfiguration(sphereRadius), sphereRotation, sphereOffset);
        const AZ::Quaternion entityRotation(-0.4f, -0.2f, 0.4f, 0.8f);
        const AZ::Vector3 entityTranslation(-1.0f, -3.0f, 3.0f);
        const float uniformScale = 1.5f;
        SetupTransform(entityRotation, entityTranslation, uniformScale);
        const AZ::Vector3 nonUniformScale(1.5f, 0.5f, 2.0f);
        SetupNonUniformScale(nonUniformScale);
        EnterColliderSubMode(PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // the expected position of the collider centre based on the combination of entity transform, collider offset and non-uniform scale
        const AZ::Vector3 expectedColliderPosition(1.7f, -10.65f, -3.0f);

        // position the camera to look at the collider along the y-axis
        AzFramework::SetCameraTransform(
            m_cameraState, AZ::Transform::CreateTranslation(expectedColliderPosition - AZ::Vector3(0.0f, 5.0f, 0.0f)));

        // the expected position of the scale manipulator relative to the centre of the collider, based on collider
        // rotation, entity scale, non-uniform scale and camera state
        const AZ::Vector3 scaleManipulatorDelta(-1.1952f, -1.8036f, 0.168f);

        const AZ::Vector3 worldStart = expectedColliderPosition + scaleManipulatorDelta;
        const AZ::Vector3 worldEnd = worldStart - 0.1f * scaleManipulatorDelta;

        const auto screenStart = AzFramework::WorldToScreen(worldStart, m_cameraState);
        const auto screenEnd = AzFramework::WorldToScreen(worldEnd, m_cameraState);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to the position of the y scale manipulator
            ->MousePosition(screenStart)
            // drag to move the manipulator
            ->MouseLButtonDown()
            ->MousePosition(screenEnd)
            ->MouseLButtonUp();

        float newSphereRadius = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(
            newSphereRadius, m_idPair, &PhysX::EditorColliderComponentRequests::GetSphereRadius);

        EXPECT_NEAR(newSphereRadius, 0.9f, ManipulatorTolerance);
    }

    TEST_F(PhysXEditorColliderComponentManipulatorFixture, CapsuleColliderScaleManipulatorsCorrectlyLocatedRelativeToColliderWithNonUniformScale)
    {
        const float capsuleRadius = 0.2f;
        const float capsuleHeight = 1.0f;
        const AZ::Quaternion capsuleRotation(-0.2f, -0.8f, -0.4f, 0.4f);
        const AZ::Vector3 capsuleOffset(1.0f, -2.0f, 1.0f);
        SetupCollider(Physics::CapsuleShapeConfiguration(capsuleHeight, capsuleRadius), capsuleRotation, capsuleOffset);
        const AZ::Quaternion entityRotation(0.7f, -0.1f, -0.1f, 0.7f);
        const AZ::Vector3 entityTranslation(-2.0f, 1.0f, -3.0f);
        const float uniformScale = 2.0f;
        SetupTransform(entityRotation, entityTranslation, uniformScale);
        const AZ::Vector3 nonUniformScale(1.0f, 0.5f, 1.5f);
        SetupNonUniformScale(nonUniformScale);
        EnterColliderSubMode(PhysX::ColliderComponentModeRequests::SubMode::Dimensions);

        // the expected position of the collider centre based on the combination of entity transform, collider offset and non-uniform scale
        const AZ::Vector3 expectedColliderPosition(-0.92f, -2.44f, -5.0f);

        // the expected position of the height manipulator relative to the centre of the collider, based on collider
        // rotation, entity scale and non-uniform scale
        const AZ::Vector3 heightManipulatorDelta(-0.3096f, 0.6528f, 0.4f);

        // position the camera to look at the collider along the y-z diagonal
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationX(-AZ::Constants::QuarterPi), expectedColliderPosition + AZ::Vector3(0.0f, -1.0f, 1.0f)));

        const AZ::Vector3 worldStart = expectedColliderPosition + heightManipulatorDelta;
        const AZ::Vector3 worldEnd = worldStart + 0.2f * heightManipulatorDelta;

        const auto screenStart = AzFramework::WorldToScreen(worldStart, m_cameraState);
        const auto screenEnd = AzFramework::WorldToScreen(worldEnd, m_cameraState);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to the position of the height manipulator
            ->MousePosition(screenStart)
            // drag to move the manipulator
            ->MouseLButtonDown()
            ->MousePosition(screenEnd)
            ->MouseLButtonUp();

        float newCapsuleHeight = 0.0f;
        PhysX::EditorColliderComponentRequestBus::EventResult(
            newCapsuleHeight, m_idPair, &PhysX::EditorColliderComponentRequests::GetCapsuleHeight);

        EXPECT_NEAR(newCapsuleHeight, 1.2f, ManipulatorTolerance);
    }

    TEST_F(PhysXEditorColliderComponentManipulatorFixture, ColliderRotationManipulatorsCorrectlyLocatedRelativeToColliderWithNonUniformScale)
    {
        const float capsuleRadius = 1.2f;
        const float capsuleHeight = 4.0f;
        const AZ::Quaternion capsuleRotation(0.7f, 0.7f, -0.1f, 0.1f);
        const AZ::Vector3 capsuleOffset(-2.0f, -2.0f, 1.0f);
        SetupCollider(Physics::CapsuleShapeConfiguration(capsuleHeight, capsuleRadius), capsuleRotation, capsuleOffset);
        const AZ::Quaternion entityRotation(0.8f, -0.4f, -0.4f, 0.2f);
        const AZ::Vector3 entityTranslation(1.0f, -1.5f, 2.0f);
        const float uniformScale = 1.5f;
        SetupTransform(entityRotation, entityTranslation, uniformScale);
        const AZ::Vector3 nonUniformScale(1.5f, 1.5f, 2.0f);
        SetupNonUniformScale(nonUniformScale);
        EnterColliderSubMode(PhysX::ColliderComponentModeRequests::SubMode::Rotation);

        // the expected position of the collider centre based on the combination of entity transform, collider offset and non-uniform scale
        const AZ::Vector3 expectedColliderPosition(-0.86f, 4.8f, -0.52f);

        // the y and z axes of the collider's frame in world space, used to locate points on the x rotation manipulator arc to interact with
        const AZ::Vector3 yDirection(0.36f, -0.8f, -0.48f);
        const AZ::Vector3 zDirection(0.9024f, 0.168f, 0.3968f);

        // position the camera to look at the collider along the world y axis
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateTranslation(expectedColliderPosition - AZ::Vector3(0.0f, 10.0f, 0.0f)));

        const float screenToWorldMultiplier = AzToolsFramework::CalculateScreenToWorldMultiplier(expectedColliderPosition, m_cameraState);
        const float manipulatorViewRadius = 2.0f;
        const AZ::Vector3 worldStart = expectedColliderPosition + screenToWorldMultiplier * manipulatorViewRadius * yDirection;
        const AZ::Vector3 worldEnd = expectedColliderPosition + screenToWorldMultiplier * manipulatorViewRadius * zDirection;

        const auto screenStart = AzFramework::WorldToScreen(worldStart, m_cameraState);
        const auto screenEnd = AzFramework::WorldToScreen(worldEnd, m_cameraState);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to a position on the angular manipulator
            ->MousePosition(screenStart)
            // drag to move the manipulator
            ->MouseLButtonDown()
            ->MousePosition(screenEnd)
            ->MouseLButtonUp();

        AZ::Quaternion newColliderRotation = AZ::Quaternion::CreateIdentity();
        PhysX::EditorColliderComponentRequestBus::EventResult(
            newColliderRotation, m_idPair, &PhysX::EditorColliderComponentRequests::GetColliderRotation);

        EXPECT_THAT(newColliderRotation, testing::Not(IsCloseTolerance(capsuleRotation, ManipulatorTolerance)));
    }
} // namespace UnitTest
