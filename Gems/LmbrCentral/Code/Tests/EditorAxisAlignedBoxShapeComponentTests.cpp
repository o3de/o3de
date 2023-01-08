/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LmbrCentralReflectionTest.h"
#include "Shape/EditorAxisAlignedBoxShapeComponent.h"
#include "Shape/EditorSphereShapeComponent.h"
#include <AZTestShared/Utils/Utils.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ViewportInteraction.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>

namespace LmbrCentral
{
    class EditorAxisAlignedBoxShapeComponentFixture
        : public UnitTest::ToolsApplicationFixture<>
        , public UnitTest::RegistryTestHelper
    {
    public:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorAxisAlignedBoxShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorSphereShapeComponentDescriptor;

        AZ::Entity* m_entity = nullptr;
    };

    void EditorAxisAlignedBoxShapeComponentFixture::SetUpEditorFixtureImpl()
    {
        RegistryTestHelper::SetUp(LmbrCentral::ShapeComponentTranslationOffsetEnabled, true);

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
        m_entity->Deactivate();
        m_entity->CreateComponent(EditorAxisAlignedBoxShapeComponentTypeId);
        m_entity->Activate();
    }

    void EditorAxisAlignedBoxShapeComponentFixture::TearDownEditorFixtureImpl()
    {
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_entity->GetId());
        m_entity = nullptr;

        m_editorAxisAlignedBoxShapeComponentDescriptor.reset();
        m_editorSphereShapeComponentDescriptor.reset();

        RegistryTestHelper::TearDown();
    }

    using EditorAxisAlignedBoxShapeComponentManipulatorFixture =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorAxisAlignedBoxShapeComponentFixture>;

    void SetUpAxisAlignedBoxShapeComponentAndEnterComponentMode(
        AZ::Entity* entity, const AZ::Transform& transform, const AZ::Vector3& translationOffset, const AZ::Vector3& boxDimensions)
    {
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        ShapeComponentRequestsBus::Event(entity->GetId(), &ShapeComponentRequests::SetTranslationOffset, translationOffset);
        BoxShapeComponentRequestsBus::Event(entity->GetId(), &BoxShapeComponentRequests::SetBoxDimensions, boxDimensions);

        AzToolsFramework::SelectEntity(entity->GetId());
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Events::AddSelectedComponentModesOfType,
            EditorAxisAlignedBoxShapeComponentTypeId);
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, AxisAlignedBoxShapeSymmetricalEditingManipulatorsScaleCorrectly)
    {
        const AZ::Transform transform(AZ::Vector3(7.0f, 5.0f, -2.0f), AZ::Quaternion::CreateIdentity(), 0.5f);
        const AZ::Vector3 translationOffset(-4.0f, -4.0f, 3.0f);
        const AZ::Vector3 boxDimensions(4.0f, 2.0f, 3.0f);
        SetUpAxisAlignedBoxShapeComponentAndEnterComponentMode(m_entity, transform, translationOffset, boxDimensions);

        // position the camera so it is looking down at the box
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationX(-AZ::Constants::HalfPi), AZ::Vector3(5.0f, 4.0f, 10.0f)));

        // position in world space which should allow grabbing the box's y scale manipulator
        const AZ::Vector3 worldStart(5.0f, 3.5f, -0.5f);
        const AZ::Vector3 worldEnd(5.0f, 4.0f, -0.5f);
        const auto screenStart = AzFramework::WorldToScreen(worldStart, m_cameraState);
        const auto screenEnd = AzFramework::WorldToScreen(worldEnd, m_cameraState);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to the position of the y scale manipulator
            ->MousePosition(screenStart)
            ->KeyboardModifierDown(AzToolsFramework::DefaultSymmetricalEditingModifier)
            // drag to move the manipulator
            ->MouseLButtonDown()
            ->MousePosition(screenEnd)
            ->MouseLButtonUp();

        AZ::Vector3 newBoxDimensions = AZ::Vector3::CreateZero();
        BoxShapeComponentRequestsBus::EventResult(newBoxDimensions, m_entity->GetId(), &BoxShapeComponentRequests::GetBoxDimensions);

        const AZ::Vector3 expectedBoxDimensions(4.0f, 4.0f, 3.0f);
        // allow a reasonably high tolerance because we can't get better accuracy than the resolution of the viewport
        EXPECT_THAT(newBoxDimensions, UnitTest::IsCloseTolerance(expectedBoxDimensions, 0.1f));
    }

    TEST_F(EditorAxisAlignedBoxShapeComponentManipulatorFixture, AxisAlignedBoxShapeAsymmetricalEditingManipulatorsScaleCorrectly)
    {
        const AZ::Transform transform(AZ::Vector3(2.0f, 4.0f, -7.0f), AZ::Quaternion::CreateIdentity(), 1.5f);
        const AZ::Vector3 translationOffset(-5.0f, 3.0f, 1.0f);
        const AZ::Vector3 boxDimensions(2.0f, 6.0f, 4.0f);
        SetUpAxisAlignedBoxShapeComponentAndEnterComponentMode(m_entity, transform, translationOffset, boxDimensions);

        // position the camera so it is looking down at the box
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationX(-AZ::Constants::HalfPi), AZ::Vector3(-5.5f, 8.5f, 5.0f)));

        // position in world space which should allow grabbing the box's -x scale manipulator
        const AZ::Vector3 worldStart(-7.0f, 8.5f, -5.5f);
        const AZ::Vector3 worldEnd(-8.5f, 8.5f, -5.5f);
        const auto screenStart = AzFramework::WorldToScreen(worldStart, m_cameraState);
        const auto screenEnd = AzFramework::WorldToScreen(worldEnd, m_cameraState);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to the position of the -x scale manipulator
            ->MousePosition(screenStart)
            // drag to move the manipulator
            ->MouseLButtonDown()
            ->MousePosition(screenEnd)
            ->MouseLButtonUp();

        AZ::Vector3 newBoxDimensions = AZ::Vector3::CreateZero();
        BoxShapeComponentRequestsBus::EventResult(newBoxDimensions, m_entity->GetId(), &BoxShapeComponentRequests::GetBoxDimensions);

        const AZ::Vector3 expectedBoxDimensions(3.0f, 6.0f, 4.0f);
        // allow a reasonably high tolerance because we can't get better accuracy than the resolution of the viewport
        const float tolerance = 0.1f;
        EXPECT_THAT(newBoxDimensions, UnitTest::IsCloseTolerance(expectedBoxDimensions, tolerance));

        // the offset should have changed because the editing was asymmetrical
        AZ::Vector3 newOffset = AZ::Vector3::CreateZero();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            newOffset, m_entity->GetId(), &LmbrCentral::ShapeComponentRequests::GetTranslationOffset);
        EXPECT_THAT(newOffset, UnitTest::IsCloseTolerance(translationOffset - AZ::Vector3::CreateAxisX(0.5f), tolerance));
    }
}

