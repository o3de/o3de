/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Viewport/ViewportSettings.h>
#include <EditorShapeTestUtils.h>
#include <LmbrCentralReflectionTest.h>
#include <Shape/EditorCapsuleShapeComponent.h>
#include <Shape/EditorSphereShapeComponent.h>

namespace LmbrCentral
{
    // Serialized legacy EditorCapsuleShapeComponent v1.
    const char kEditorCapsuleComponentVersion1[] =
        R"DELIMITER(<ObjectStream version="1">
        <Class name="EditorCapsuleShapeComponent" field="element" version="1" type="{06B6C9BE-3648-4DA2-9892-755636EF6E19}">
            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                    <Class name="AZ::u64" field="Id" value="10467239283436660413" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
            </Class>
            <Class name="CapsuleShapeConfig" field="Configuration" version="1" type="{00931AEB-2AD8-42CE-B1DC-FA4332F51501}">
                <Class name="float" field="Height" value="0.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                <Class name="float" field="Radius" value="1.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
        </Class>
    </ObjectStream>)DELIMITER";

    class LoadEditorCapsuleShapeComponentFromVersion1
        : public LoadEditorComponentTest<EditorCapsuleShapeComponent>
    {
    protected:
        const char* GetSourceDataBuffer() const override { return kEditorCapsuleComponentVersion1; }
    };

    TEST_F(LoadEditorCapsuleShapeComponentFromVersion1, Application_IsRunning)
    {
        ASSERT_NE(GetApplication(), nullptr);
    }

    TEST_F(LoadEditorCapsuleShapeComponentFromVersion1, Components_Load)
    {
        EXPECT_NE(m_object.get(), nullptr);
    }

    TEST_F(LoadEditorCapsuleShapeComponentFromVersion1, EditorComponent_Found)
    {
        EXPECT_EQ(m_entity->GetComponents().size(), 2);
        EXPECT_NE(m_entity->FindComponent(m_object->GetId()), nullptr);
    }

    TEST_F(LoadEditorCapsuleShapeComponentFromVersion1, Height_MatchesSourceData)
    {
        float height = 0.0f;
        CapsuleShapeComponentRequestsBus::EventResult(
            height, m_entity->GetId(), &CapsuleShapeComponentRequests::GetHeight);

        EXPECT_FLOAT_EQ(height, 0.57f);
    }

    TEST_F(LoadEditorCapsuleShapeComponentFromVersion1, Radius_MatchesSourceData)
    {
        float radius = 0.0f;
        CapsuleShapeComponentRequestsBus::EventResult(
            radius, m_entity->GetId(), &CapsuleShapeComponentRequests::GetRadius);

        EXPECT_FLOAT_EQ(radius, 1.57f);
    }

    class EditorCapsuleShapeComponentFixture
        : public UnitTest::ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorCapsuleShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorSphereShapeComponentDescriptor;

        AZ::Entity* m_entity = nullptr;
        AZ::EntityId m_entityId;
        AZ::EntityComponentIdPair m_entityComponentIdPair;
    };

    void EditorCapsuleShapeComponentFixture::SetUpEditorFixtureImpl()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        // need to reflect EditorSphereShapeComponent in order for EditorBaseShapeComponent to be reflected
        m_editorSphereShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorSphereShapeComponent::CreateDescriptor());

        m_editorCapsuleShapeComponentDescriptor =
            AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorCapsuleShapeComponent::CreateDescriptor());

        ShapeComponentConfig::Reflect(serializeContext);
        CapsuleShape::Reflect(serializeContext);
        m_editorSphereShapeComponentDescriptor->Reflect(serializeContext);
        m_editorCapsuleShapeComponentDescriptor->Reflect(serializeContext);

        UnitTest::CreateDefaultEditorEntity("CapsuleShapeComponentEntity", &m_entity);
        m_entityId = m_entity->GetId();
        m_entity->Deactivate();
        m_entityComponentIdPair =
            AZ::EntityComponentIdPair(m_entityId, m_entity->CreateComponent(EditorCapsuleShapeComponentTypeId)->GetId());
        m_entity->Activate();
    }

    void EditorCapsuleShapeComponentFixture::TearDownEditorFixtureImpl()
    {
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_entityId);
        m_entity = nullptr;
        m_entityId.SetInvalid();

        m_editorCapsuleShapeComponentDescriptor.reset();
        m_editorSphereShapeComponentDescriptor.reset();
    }

    using EditorCapsuleShapeComponentManipulatorFixture =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorCapsuleShapeComponentFixture>;

    void SetUpCapsuleShapeComponent(
        AZ::EntityId entityId,
        const AZ::Transform& transform,
        const AZ::Vector3& translationOffset,
        float radius,
        float height)
    {
        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        ShapeComponentRequestsBus::Event(entityId, &ShapeComponentRequests::SetTranslationOffset, translationOffset);
        CapsuleShapeComponentRequestsBus::Event(entityId, &CapsuleShapeComponentRequests::SetRadius, radius);
        CapsuleShapeComponentRequestsBus::Event(entityId, &CapsuleShapeComponentRequests::SetHeight, height);
    }

    TEST_F(EditorCapsuleShapeComponentManipulatorFixture, CapsuleShapeSymmetricalHeightManipulatorsScaleCorrectly)
    {
        AZ::Transform capsuleTransform(AZ::Vector3(6.0f, -3.0f, 4.0f), AZ::Quaternion(0.3f, 0.1f, -0.3f, 0.9f), 2.0f);
        const float radius = 0.5f;
        const float height = 2.0f;
        const AZ::Vector3 translationOffset(-5.0f, 3.0f, -2.0f);
        SetUpCapsuleShapeComponent(m_entity->GetId(), capsuleTransform, translationOffset, radius, height);
        EnterComponentMode(m_entityId, EditorCapsuleShapeComponentTypeId);

        // position the camera so it is looking at the capsule
        AzFramework::SetCameraTransform(m_cameraState, AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, -5.0f, 10.0f)));

        const AZ::Vector3 worldStart(1.6f, 6.84f, 8.88f);
        const AZ::Vector3 worldEnd(1.6f, 6.6f, 9.2f);

        DragMouse(m_cameraState, m_actionDispatcher.get(), worldStart, worldEnd, AzToolsFramework::DefaultSymmetricalEditingModifier);

        ExpectCapsuleHeight(m_entity->GetId(), 2.4f);
    }

    TEST_F(EditorCapsuleShapeComponentManipulatorFixture, CapsuleShapeAsymmetricalHeightManipulatorsScaleCorrectly)
    {
        AZ::Transform capsuleTransform(AZ::Vector3(2.0f, -6.0f, 5.0f), AZ::Quaternion(0.7f, -0.1f, -0.1f, 0.7f), 0.5f);
        const float radius = 2.0f;
        const float height = 7.0f;
        const AZ::Vector3 translationOffset(2.0f, 5.0f, -3.0f);
        SetUpCapsuleShapeComponent(m_entity->GetId(), capsuleTransform, translationOffset, radius, height);
        EnterComponentMode(m_entityId, EditorCapsuleShapeComponentTypeId);

        // position the camera so it is looking at the capsule
        AzFramework::SetCameraTransform(m_cameraState, AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, -10.0f, 7.5f)));

        const AZ::Vector3 worldStart(3.87f, -3.16f, 7.5f);
        const AZ::Vector3 worldEnd(3.73f, -3.64f, 7.5f);

        DragMouse(m_cameraState, m_actionDispatcher.get(), worldStart, worldEnd);

        ExpectCapsuleHeight(m_entity->GetId(), 6.0f);
    }

    TEST_F(EditorCapsuleShapeComponentManipulatorFixture, CapsuleShapeRadiusManipulatorScalesCorrectly)
    {
        AZ::Transform capsuleTransform(AZ::Vector3(-4.0f, -5.0f, 1.0f), AZ::Quaternion::CreateIdentity(), 2.5f);
        const float radius = 1.0f;
        const float height = 5.0f;
        const AZ::Vector3 translationOffset(6.0f, 3.0f, -2.0f);
        SetUpCapsuleShapeComponent(m_entity->GetId(), capsuleTransform, translationOffset, radius, height);
        EnterComponentMode(m_entityId, EditorCapsuleShapeComponentTypeId);

        // position the camera so it is looking at the capsule
        AzFramework::SetCameraTransform(m_cameraState, AZ::Transform::CreateTranslation(AZ::Vector3(15.0f, -5.0f, -5.0f)));

        const AZ::Vector3 worldStart(13.5f, 2.5f, -4.0f);
        const AZ::Vector3 worldEnd(14.75f, 2.5f, -4.0f);

        DragMouse(m_cameraState, m_actionDispatcher.get(), worldStart, worldEnd);

        ExpectCapsuleRadius(m_entity->GetId(), 1.5f);
    }
}

