/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorShapeTestUtils.h>
#include <LmbrCentralReflectionTest.h>
#include <Shape/EditorSphereShapeComponent.h>

namespace LmbrCentral
{
    // Serialized legacy EditorSphereShapeComponent v1.
    const char kEditorSphereComponentVersion1[] =
        R"DELIMITER(<ObjectStream version="1">
        <Class name="EditorSphereShapeComponent" field="element" version="1" type="{2EA56CBF-63C8-41D9-84D5-0EC2BECE748E}">
            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                    <Class name="AZ::u64" field="Id" value="11428802534905560348" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
            </Class>
            <Class name="SphereShapeConfig" field="Configuration" version="1" type="{4AADFD75-48A7-4F31-8F30-FE4505F09E35}">
                <Class name="float" field="Radius" value="0.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
        </Class>
    </ObjectStream>)DELIMITER";

    class LoadEditorSphereShapeComponentFromVersion1
        : public LoadEditorComponentTest<EditorSphereShapeComponent>
    {
    protected:
        const char* GetSourceDataBuffer() const override { return kEditorSphereComponentVersion1; }
    };

    TEST_F(LoadEditorSphereShapeComponentFromVersion1, Application_IsRunning)
    {
        ASSERT_NE(GetApplication(), nullptr);
    }

    TEST_F(LoadEditorSphereShapeComponentFromVersion1, Components_Load)
    {
        EXPECT_NE(m_object.get(), nullptr);
    }

    TEST_F(LoadEditorSphereShapeComponentFromVersion1, EditorComponent_Found)
    {
        EXPECT_EQ(m_entity->GetComponents().size(), 2);
        EXPECT_NE(m_entity->FindComponent(m_object->GetId()), nullptr);
    }

    TEST_F(LoadEditorSphereShapeComponentFromVersion1, Radius_MatchesSourceData)
    {
        float radius = 0.0f;
        SphereShapeComponentRequestsBus::EventResult(
            radius, m_entity->GetId(), &SphereShapeComponentRequests::GetRadius);

        EXPECT_FLOAT_EQ(radius, 0.57f);
    }

    class EditorSphereShapeComponentFixture
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

    void EditorSphereShapeComponentFixture::SetUpEditorFixtureImpl()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        m_editorSphereShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorSphereShapeComponent::CreateDescriptor());

        ShapeComponentConfig::Reflect(serializeContext);
        SphereShape::Reflect(serializeContext);
        m_editorSphereShapeComponentDescriptor->Reflect(serializeContext);

        UnitTest::CreateDefaultEditorEntity("CapsuleShapeComponentEntity", &m_entity);
        m_entityId = m_entity->GetId();
        m_entity->Deactivate();
        m_entityComponentIdPair =
            AZ::EntityComponentIdPair(m_entityId, m_entity->CreateComponent(EditorSphereShapeComponentTypeId)->GetId());
        m_entity->Activate();
    }

    void EditorSphereShapeComponentFixture::TearDownEditorFixtureImpl()
    {
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_entityId);
        m_entity = nullptr;
        m_entityId.SetInvalid();

        m_editorSphereShapeComponentDescriptor.reset();
    }

    using EditorSphereShapeComponentManipulatorFixture =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorSphereShapeComponentFixture>;

    void SetUpSphereShapeComponent(
        AZ::EntityId entityId,
        const AZ::Transform& transform,
        const AZ::Vector3& translationOffset,
        float radius)
    {
        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM, transform);
        ShapeComponentRequestsBus::Event(entityId, &ShapeComponentRequests::SetTranslationOffset, translationOffset);
        SphereShapeComponentRequestsBus::Event(entityId, &SphereShapeComponentRequests::SetRadius, radius);
    }

    TEST_F(EditorSphereShapeComponentManipulatorFixture, SphereShapeRadiusManipulatorScalesCorrectly)
    {
        AZ::Transform sphereTransform(AZ::Vector3(6.0f, -3.0f, 2.0f), AZ::Quaternion::CreateIdentity(), 0.5f);
        const float radius = 3.0f;
        const AZ::Vector3 translationOffset(-3.0f, -5.0f, 2.0f);
        SetUpSphereShapeComponent(m_entity->GetId(), sphereTransform, translationOffset, radius);
        EnterComponentMode(m_entityId, EditorSphereShapeComponentTypeId);

        // position the camera so it is looking at the sphere
        AzFramework::SetCameraTransform(m_cameraState, AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, -15.0f, 2.5f)));

        const AZ::Vector3 worldStart(6.0f, -5.5f, 3.0f);
        const AZ::Vector3 worldEnd(6.5f, -5.5f, 3.0f);

        DragMouse(m_cameraState, m_actionDispatcher.get(), worldStart, worldEnd);

        ExpectSphereRadius(m_entity->GetId(), 4.0f);
    }
} // namespace LmbrCentral
