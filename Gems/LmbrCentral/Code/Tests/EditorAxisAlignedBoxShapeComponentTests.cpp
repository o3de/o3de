/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Shape/EditorAxisAlignedBoxShapeComponent.h"
#include "Shape/EditorSphereShapeComponent.h"
#include <AZTestShared/Utils/Utils.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <EditorShapeTestUtils.h>

namespace LmbrCentral
{
    class EditorAxisAlignedBoxShapeComponentFixture
        : public UnitTest::ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorAxisAlignedBoxShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorSphereShapeComponentDescriptor;

        AZ::Entity* m_entity = nullptr;
        AZ::EntityId m_entityId;
        AZ::EntityComponentIdPair m_entityComponentIdPair;
    };

    void EditorAxisAlignedBoxShapeComponentFixture::SetUpEditorFixtureImpl()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        // need to reflect EditorSphereShapeComponent in order for EditorBaseShapeComponent to be reflected
        m_editorSphereShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorSphereShapeComponent::CreateDescriptor());

        m_editorAxisAlignedBoxShapeComponentDescriptor =
            AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorAxisAlignedBoxShapeComponent::CreateDescriptor());

        ShapeComponentConfig::Reflect(serializeContext);
        AxisAlignedBoxShape::Reflect(serializeContext);
        m_editorSphereShapeComponentDescriptor->Reflect(serializeContext);
        m_editorAxisAlignedBoxShapeComponentDescriptor->Reflect(serializeContext);

        UnitTest::CreateDefaultEditorEntity("AxisAlignedBoxShapeComponentEntity", &m_entity);
        m_entityId = m_entity->GetId();
        m_entity->Deactivate();
        m_entityComponentIdPair =
            AZ::EntityComponentIdPair(m_entityId, m_entity->CreateComponent(EditorAxisAlignedBoxShapeComponentTypeId)->GetId());
        m_entity->Activate();
    }

    void EditorAxisAlignedBoxShapeComponentFixture::TearDownEditorFixtureImpl()
    {
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_entityId);
        m_entity = nullptr;
        m_entityId.SetInvalid();

        m_editorAxisAlignedBoxShapeComponentDescriptor.reset();
        m_editorSphereShapeComponentDescriptor.reset();
    }

    using EditorAxisAlignedBoxShapeComponentManipulatorFixture =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorAxisAlignedBoxShapeComponentFixture>;

    void SetUpAxisAlignedBoxShapeComponent(
        AZ::EntityId entityId, const AZ::Transform& transform, const AZ::Vector3& translationOffset, const AZ::Vector3& boxDimensions)
    {
        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        ShapeComponentRequestsBus::Event(entityId, &ShapeComponentRequests::SetTranslationOffset, translationOffset);
        BoxShapeComponentRequestsBus::Event(entityId, &BoxShapeComponentRequests::SetBoxDimensions, boxDimensions);
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, AxisAlignedBoxShapeSymmetricalDimensionManipulatorsScaleCorrectly)
    {
        const AZ::Transform transform(AZ::Vector3(7.0f, 5.0f, -2.0f), AZ::Quaternion::CreateIdentity(), 0.5f);
        const AZ::Vector3 translationOffset(-4.0f, -4.0f, 3.0f);
        const AZ::Vector3 boxDimensions(4.0f, 2.0f, 3.0f);
        SetUpAxisAlignedBoxShapeComponent(m_entityId, transform, translationOffset, boxDimensions);
        EnterComponentMode(m_entityId, EditorAxisAlignedBoxShapeComponentTypeId);

        // position the camera so it is looking down at the box
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationX(-AZ::Constants::HalfPi), AZ::Vector3(5.0f, 4.0f, 10.0f)));

        // position in world space which should allow grabbing the box's y scale manipulator
        const AZ::Vector3 worldStart(5.0f, 3.5f, -0.5f);
        const AZ::Vector3 worldEnd(5.0f, 4.0f, -0.5f);

        DragMouse(m_cameraState, m_actionDispatcher.get(), worldStart, worldEnd, AzToolsFramework::DefaultSymmetricalEditingModifier);

        ExpectBoxDimensions(m_entityId, AZ::Vector3(4.0f, 4.0f, 3.0f));
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, AxisAlignedBoxShapeAsymmetricalDimensionManipulatorsScaleCorrectly)
    {
        const AZ::Transform transform(AZ::Vector3(2.0f, 4.0f, -7.0f), AZ::Quaternion::CreateIdentity(), 1.5f);
        const AZ::Vector3 translationOffset(-5.0f, 3.0f, 1.0f);
        const AZ::Vector3 boxDimensions(2.0f, 6.0f, 4.0f);
        SetUpAxisAlignedBoxShapeComponent(m_entityId, transform, translationOffset, boxDimensions);
        EnterComponentMode(m_entityId, EditorAxisAlignedBoxShapeComponentTypeId);

        // position the camera so it is looking down at the box
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationX(-AZ::Constants::HalfPi), AZ::Vector3(-5.5f, 8.5f, 5.0f)));

        // position in world space which should allow grabbing the box's -x scale manipulator
        const AZ::Vector3 worldStart(-7.0f, 8.5f, -5.5f);
        const AZ::Vector3 worldEnd(-8.5f, 8.5f, -5.5f);

        DragMouse(m_cameraState, m_actionDispatcher.get(), worldStart, worldEnd);

        ExpectBoxDimensions(m_entityId, AZ::Vector3(3.0f, 6.0f, 4.0f));
        // the offset should have changed because the editing was asymmetrical
        ExpectTranslationOffset(m_entityId, translationOffset - AZ::Vector3::CreateAxisX(0.5f));
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, AxisAlignedBoxShapeRotatedEntityManipulatorSpaceCorrect)
    {
        const AZ::Transform transform(AZ::Vector3(7.0f, -6.0f, -2.0f), AZ::Quaternion(0.7f, 0.1f, -0.1f, 0.7f), 2.0f);
        const AZ::Vector3 translationOffset(-4.0f, 4.0f, 2.0f);
        const AZ::Vector3 boxDimensions(2.0f, 3.0f, 4.0f);
        SetUpAxisAlignedBoxShapeComponent(m_entityId, transform, translationOffset, boxDimensions);
        EnterComponentMode(m_entityId, EditorAxisAlignedBoxShapeComponentTypeId);

        // position the camera so it is looking down at the box
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationX(-AZ::Constants::HalfPi), AZ::Vector3(-1.0f, 2.0f, 15.0f)));

        // position in world space which should allow grabbing the box's x scale manipulator
        // the entity is rotated, but the box (and the manipulator space) should act as if it is not rotated
        const AZ::Vector3 worldStart(1.0f, 2.0f, 2.0f);
        const AZ::Vector3 worldEnd(3.0f, 2.0f, 2.0f);

        DragMouse(m_cameraState, m_actionDispatcher.get(), worldStart, worldEnd);

        ExpectBoxDimensions(m_entityId, AZ::Vector3(3.0f, 3.0f, 4.0f));
        // the offset should have changed because the editing was asymmetrical
        ExpectTranslationOffset(m_entityId, translationOffset + AZ::Vector3::CreateAxisX(0.5f));
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, AxisAlignedBoxShapeTranslationOffsetManipulatorsScaleCorrectly)
    {
        const AZ::Transform boxTransform(AZ::Vector3(-5.0f, 2.0f, 2.0f), AZ::Quaternion(0.3f, 0.3f, 0.1f, 0.9f), 1.5f);
        const AZ::Vector3 translationOffset(3.0f, 1.0f, -4.0f);
        const AZ::Vector3 boxDimensions(1.0f, 4.0f, 2.0f);
        SetUpAxisAlignedBoxShapeComponent(m_entityId, boxTransform, translationOffset, boxDimensions);
        EnterComponentMode(m_entityId, EditorAxisAlignedBoxShapeComponentTypeId);
        SetComponentSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::TranslationOffset);

        // position the camera so it is looking horizontally at the box
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, -10.0f, -3.0f)));

        // position in world space which should allow grabbing the box's z translation offset manipulator
        const AZ::Vector3 worldStart(-0.5f, 3.5f, -3.0f);

        // position in world space to move to 
        const AZ::Vector3 worldEnd(-0.5f, 3.5f, -1.5f);

        DragMouse(m_cameraState, m_actionDispatcher.get(), worldStart, worldEnd);

        ExpectTranslationOffset(m_entityId, translationOffset + AZ::Vector3::CreateAxisZ());
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, PressingKey1ShouldSetDimensionMode)
    {
        EnterComponentMode(m_entityId, EditorAxisAlignedBoxShapeComponentTypeId);
        SetComponentSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::TranslationOffset);
        ExpectSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::TranslationOffset);

        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_1);

        ExpectSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::Dimensions);
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, PressingKey2ShouldSetTranslationOffsetMode)
    {
        EnterComponentMode(m_entityId, EditorAxisAlignedBoxShapeComponentTypeId);
        ExpectSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::Dimensions);

        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_2);

        ExpectSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::TranslationOffset);
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, PressingKeyRInDimensionModeShouldResetBoxDimensions)
    {
        const AZ::Vector3 boxDimensions(2.0f, 2.0f, 2.0f);
        BoxShapeComponentRequestsBus::Event(m_entityId, &BoxShapeComponentRequests::SetBoxDimensions, boxDimensions);
        EnterComponentMode(m_entityId, EditorAxisAlignedBoxShapeComponentTypeId);

        ExpectBoxDimensions(m_entityId, boxDimensions);

        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_R);

        ExpectBoxDimensions(m_entityId, AZ::Vector3::CreateOne());
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, PressingKeyRInTranslationOffsetModeShouldResetTranslationOffset)
    {
        const AZ::Vector3 translationOffset(3.0f, 4.0f, 5.0f);
        ShapeComponentRequestsBus::Event(m_entityId, &ShapeComponentRequests::SetTranslationOffset, translationOffset);
        EnterComponentMode(m_entityId, EditorAxisAlignedBoxShapeComponentTypeId);
        SetComponentSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::TranslationOffset);

        ExpectTranslationOffset(m_entityId, translationOffset);

        QTest::keyPress(&m_editorActions.m_componentModeWidget, Qt::Key_R);

        ExpectTranslationOffset(m_entityId, AZ::Vector3::CreateZero());
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, CtrlMouseWheelUpShouldSetNextMode)
    {
        EnterComponentMode(m_entityId, EditorAxisAlignedBoxShapeComponentTypeId);
        ExpectSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::Dimensions);

        const auto handled = CtrlScroll(1.0f);

        EXPECT_EQ(handled, AzToolsFramework::ViewportInteraction::MouseInteractionResult::Viewport);
        ExpectSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::TranslationOffset);
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, CtrlMouseWheelDownShouldSetNextMode)
    {
        EnterComponentMode(m_entityId, EditorAxisAlignedBoxShapeComponentTypeId);
        SetComponentSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::TranslationOffset);
        ExpectSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::TranslationOffset);

        const auto handled = CtrlScroll(-1.0f);

        EXPECT_EQ(handled, AzToolsFramework::ViewportInteraction::MouseInteractionResult::Viewport);
        ExpectSubMode(m_entityComponentIdPair, AzToolsFramework::ShapeComponentModeRequests::SubMode::Dimensions);
    }
}

