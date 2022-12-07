/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LmbrCentralReflectionTest.h"
#include "Shape/EditorBoxShapeComponent.h"
#include "Shape/EditorSphereShapeComponent.h"
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ViewportInteraction.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace LmbrCentral
{
    // Serialized legacy EditorBoxShapeComponent v1.
    const char kEditorBoxComponentVersion1[] =
        R"DELIMITER(<ObjectStream version="1">
        <Class name="EditorBoxShapeComponent" field="element" version="1" type="{2ADD9043-48E8-4263-859A-72E0024372BF}">
            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                    <Class name="AZ::u64" field="Id" value="7702953324769442676" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
            </Class>
            <Class name="BoxShapeConfig" field="Configuration" version="1" type="{F034FBA2-AC2F-4E66-8152-14DFB90D6283}">
                <Class name="Vector3" field="Dimensions" value="0.3700000 0.5700000 0.6600000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
            </Class>
        </Class>
    </ObjectStream>)DELIMITER";

    class LoadEditorBoxShapeComponentFromVersion1
        : public LoadEditorComponentTest<EditorBoxShapeComponent>
    {
    protected:
        const char* GetSourceDataBuffer() const override { return kEditorBoxComponentVersion1; }
    };

    TEST_F(LoadEditorBoxShapeComponentFromVersion1, Application_IsRunning)
    {
        ASSERT_NE(GetApplication(), nullptr);
    }

    TEST_F(LoadEditorBoxShapeComponentFromVersion1, Components_Load)
    {
        EXPECT_NE(m_object.get(), nullptr);
    }

    TEST_F(LoadEditorBoxShapeComponentFromVersion1, EditorComponent_Found)
    {
        EXPECT_EQ(m_entity->GetComponents().size(), 2);
        EXPECT_NE(m_entity->FindComponent(m_object->GetId()), nullptr);
    }

    TEST_F(LoadEditorBoxShapeComponentFromVersion1, Dimensions_MatchesSourceData)
    {
        AZ::Vector3 dimensions = AZ::Vector3::CreateZero();
        BoxShapeComponentRequestsBus::EventResult(
            dimensions, m_entity->GetId(), &BoxShapeComponentRequests::GetBoxDimensions);

       EXPECT_EQ(dimensions, AZ::Vector3(0.37f, 0.57f, 0.66f));
    }

    class EditorBoxShapeComponentFixture : public UnitTest::ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorBoxShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorSphereShapeComponentDescriptor;

        AZ::Entity* m_entity = nullptr;
    };

    void EditorBoxShapeComponentFixture::SetUpEditorFixtureImpl()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        // need to reflect EditorSphereShapeComponent in order for EditorBaseShapeComponent to be reflected
        m_editorSphereShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorSphereShapeComponent::CreateDescriptor());

        m_editorBoxShapeComponentDescriptor =
            AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorBoxShapeComponent::CreateDescriptor());

        ShapeComponentConfig::Reflect(serializeContext);
        BoxShape::Reflect(serializeContext);
        m_editorSphereShapeComponentDescriptor->Reflect(serializeContext);
        m_editorBoxShapeComponentDescriptor->Reflect(serializeContext);

        UnitTest::CreateDefaultEditorEntity("BoxShapeComponentEntity", &m_entity);
        m_entity->Deactivate();
        m_entity->CreateComponent(AzToolsFramework::Components::EditorNonUniformScaleComponent::RTTI_Type());
        m_entity->CreateComponent(EditorBoxShapeComponentTypeId);
        m_entity->Activate();
    }

    void EditorBoxShapeComponentFixture::TearDownEditorFixtureImpl()
    {
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_entity->GetId());
        m_entity = nullptr;

        m_editorBoxShapeComponentDescriptor.reset();
        m_editorSphereShapeComponentDescriptor.reset();
    }

    using EditorBoxShapeComponentManipulatorFixture =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorBoxShapeComponentFixture>;

    TEST_F(EditorBoxShapeComponentManipulatorFixture, BoxShapeNonUniformScaleManipulatorsScaleCorrectly)
    {
        // a rotation which rotates the x-axis to (0.8, 0.6, 0)
        const AZ::Quaternion boxRotation(0.0f, 0.0f, 0.316228f, 0.948683f);
        AZ::Transform boxTransform = AZ::Transform::CreateFromQuaternionAndTranslation(boxRotation, AZ::Vector3(2.0f, 3.0f, 4.0f));
        boxTransform.SetUniformScale(1.5f);
        AZ::TransformBus::Event(m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, boxTransform);

        const AZ::Vector3 nonUniformScale(4.0f, 1.5f, 2.0f);
        AZ::NonUniformScaleRequestBus::Event(m_entity->GetId(), &AZ::NonUniformScaleRequests::SetScale, nonUniformScale);

        const AZ::Vector3 boxDimensions(1.0f, 2.0f, 2.5f);
        BoxShapeComponentRequestsBus::Event(m_entity->GetId(), &BoxShapeComponentRequests::SetBoxDimensions, boxDimensions);

        // enter the box shape component's component mode
        AzToolsFramework::SelectEntity(m_entity->GetId());
        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Events::AddSelectedComponentModesOfType,
            EditorBoxShapeComponentTypeId);

        // position the camera so it is looking down at the box
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationX(-AZ::Constants::HalfPi), AZ::Vector3(2.0f, 3.0f, 20.0f)));

        // position in world space which should allow grabbing the box's x scale manipulator
        // the unscaled position of the x scale manipulator in the box's local frame should be (0.5f, 0.0f, 0.0f)
        // after non-uniform scale, the manipulator position should be (2.0f, 0.0f, 0.0f)
        // after the scale of the entity transform, the manipulator position should be (3.0f, 0.0f, 0.0f)
        // after the rotation of the entity transform, the manipulator position should be (2.4f, 1.8f, 0.0f)
        // after the translation of the entity transform, the manipulator position should be (4.4f, 4.8f, 4.0f)
        const AZ::Vector3 worldStart(4.4f, 4.8f, 4.0f);

        // position in world space to move to
        const AZ::Vector3 worldEnd(6.8f, 6.6f, 4.0f);

        const auto screenStart = AzFramework::WorldToScreen(worldStart, m_cameraState);
        const auto screenEnd = AzFramework::WorldToScreen(worldEnd, m_cameraState);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to the position of the x scale manipulator
            ->MousePosition(screenStart)
            // drag to move the manipulator
            ->MouseLButtonDown()
            ->MousePosition(screenEnd)
            ->MouseLButtonUp();

        AZ::Vector3 newBoxDimensions = AZ::Vector3::CreateZero();
        BoxShapeComponentRequestsBus::EventResult(newBoxDimensions, m_entity->GetId(), &BoxShapeComponentRequests::GetBoxDimensions);

        const AZ::Vector3 expectedBoxDimensions(2.0f, 2.0f, 2.5f);
        // allow a reasonably high tolerance because we can't get better accuracy than the resolution of the viewport
        EXPECT_THAT(newBoxDimensions, UnitTest::IsCloseTolerance(expectedBoxDimensions, 1e-2f));
    }
}

