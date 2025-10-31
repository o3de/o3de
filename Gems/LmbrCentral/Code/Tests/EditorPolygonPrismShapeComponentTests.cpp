/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LmbrCentralReflectionTest.h"
#include "Shape/EditorPolygonPrismShapeComponent.h"
#include "Shape/EditorSphereShapeComponent.h"
#include <AZTestShared/Math/MathTestHelpers.h>
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
    // Serialized legacy EditorPolygonPrismShapeComponent v1.
    const char kEditorPolygonPrismComponentVersion1[] =
        R"DELIMITER(<ObjectStream version="1">
        <Class name="EditorPolygonPrismShapeComponent" field="element" version="1" type="{5368F204-FE6D-45C0-9A4F-0F933D90A785}">
            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
                    <Class name="AZ::u64" field="Id" value="2508877310741125152" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                </Class>
            </Class>
            <Class name="PolygonPrismCommon" field="Configuration" version="1" type="{BDB453DE-8A51-42D0-9237-13A9193BE724}">
                <Class name="AZStd::shared_ptr" field="PolygonPrism" type="{2E879A16-9143-5862-A5B3-EDED931C60BC}">
                    <Class name="PolygonPrism" field="element" version="1" type="{F01C8BDD-6F24-4344-8945-521A8750B30B}">
                        <Class name="float" field="Height" value="1.5700000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                        <Class name="VertexContainer&lt;Vector2 &gt;" field="VertexContainer" type="{EBE98B36-0783-5226-9739-064BD41EBB52}">
                            <Class name="AZStd::vector" field="Vertices" type="{82AC1A71-2EA7-5FBC-9B3B-72B1CCFDD292}">
                                <Class name="Vector2" field="element" value="-0.5700000 -0.5700000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                <Class name="Vector2" field="element" value="0.5700000 -0.5700000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                <Class name="Vector2" field="element" value="0.5700000 0.5700000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                <Class name="Vector2" field="element" value="-0.5700000 0.5700000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                            </Class>
                        </Class>
                    </Class>
                </Class>
            </Class>
        </Class>
    </ObjectStream>)DELIMITER";

    class LoadEditorPolygonPrismShapeComponentFromVersion1 : public LoadEditorComponentTest<EditorPolygonPrismShapeComponent>
    {
    protected:
        const char* GetSourceDataBuffer() const override
        {
            return kEditorPolygonPrismComponentVersion1;
        }
    };

    TEST_F(LoadEditorPolygonPrismShapeComponentFromVersion1, Application_IsRunning)
    {
        ASSERT_NE(GetApplication(), nullptr);
    }

    TEST_F(LoadEditorPolygonPrismShapeComponentFromVersion1, Components_Load)
    {
        EXPECT_NE(m_object.get(), nullptr);
    }

    TEST_F(LoadEditorPolygonPrismShapeComponentFromVersion1, EditorComponent_Found)
    {
        EXPECT_EQ(m_entity->GetComponents().size(), 2);
        EXPECT_NE(m_entity->FindComponent(m_object->GetId()), nullptr);
    }

    TEST_F(LoadEditorPolygonPrismShapeComponentFromVersion1, Height_MatchesSourceData)
    {
        AZ::ConstPolygonPrismPtr polygonPrism;
        PolygonPrismShapeComponentRequestBus::EventResult(
            polygonPrism, m_entity->GetId(), &PolygonPrismShapeComponentRequestBus::Events::GetPolygonPrism);

        EXPECT_FLOAT_EQ(polygonPrism->GetHeight(), 1.57f);
    }

    TEST_F(LoadEditorPolygonPrismShapeComponentFromVersion1, Vertices_MatchesSourceData)
    {
        AZ::ConstPolygonPrismPtr polygonPrism;
        PolygonPrismShapeComponentRequestBus::EventResult(
            polygonPrism, m_entity->GetId(), &PolygonPrismShapeComponentRequestBus::Events::GetPolygonPrism);

        AZStd::vector<AZ::Vector2> sourceVertices = { AZ::Vector2(-0.57f, -0.57f), AZ::Vector2(0.57f, -0.57f), AZ::Vector2(0.57f, 0.57f),
                                                      AZ::Vector2(-0.57f, 0.57f) };
        EXPECT_EQ(polygonPrism->m_vertexContainer.GetVertices(), sourceVertices);
    }

    class EditorPolygonPrismShapeComponentFixture : public UnitTest::ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorPolygonPrismShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_editorSphereShapeComponentDescriptor;

        AZ::Entity* m_entity = nullptr;
    };

    void EditorPolygonPrismShapeComponentFixture::SetUpEditorFixtureImpl()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        // need to reflect EditorSphereShapeComponent in order for EditorBaseShapeComponent to be reflected
        m_editorSphereShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorSphereShapeComponent::CreateDescriptor());

        m_editorPolygonPrismShapeComponentDescriptor =
            AZStd::unique_ptr<AZ::ComponentDescriptor>(EditorPolygonPrismShapeComponent::CreateDescriptor());

        ShapeComponentConfig::Reflect(serializeContext);
        PolygonPrismShape::Reflect(serializeContext);
        m_editorSphereShapeComponentDescriptor->Reflect(serializeContext);
        m_editorPolygonPrismShapeComponentDescriptor->Reflect(serializeContext);

        UnitTest::CreateDefaultEditorEntity("PolygonPrismShapeComponentEntity", &m_entity);
        m_entity->Deactivate();
        m_entity->CreateComponent(AzToolsFramework::Components::EditorNonUniformScaleComponent::RTTI_Type());
        m_entity->CreateComponent(EditorPolygonPrismShapeComponentTypeId);
        m_entity->Activate();
    }

    void EditorPolygonPrismShapeComponentFixture::TearDownEditorFixtureImpl()
    {
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_entity->GetId());
        m_entity = nullptr;

        m_editorPolygonPrismShapeComponentDescriptor.reset();
        m_editorSphereShapeComponentDescriptor.reset();
    }

    using EditorPolygonPrismShapeComponentManipulatorFixture =
        UnitTest::IndirectCallManipulatorViewportInteractionFixtureMixin<EditorPolygonPrismShapeComponentFixture>;

    TEST_F(EditorPolygonPrismShapeComponentManipulatorFixture, PolygonPrismNonUniformScaleManipulatorsScaleCorrectly)
    {
        // set the non-uniform scale and enter the polygon prism shape component's component mode
        const AZ::Vector3 nonUniformScale(2.0f, 3.0f, 4.0f);
        AZ::NonUniformScaleRequestBus::Event(m_entity->GetId(), &AZ::NonUniformScaleRequests::SetScale, nonUniformScale);

        AzToolsFramework::SelectEntity(m_entity->GetId());

        AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
            &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Events::AddSelectedComponentModesOfType,
            EditorPolygonPrismShapeComponentTypeId);

        // position the camera so it is looking down at the polygon prism
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationX(-AZ::Constants::HalfPi), AZ::Vector3(0.0f, 0.0f, 20.0f)));

        // the first vertex of the polygon prism should be at (-2, -2, 0) in its local space
        // because of the non-uniform scale, that should be (-4, -6, 0) in world space
        const AZ::Vector3 worldStart(-4.0f, -6.0f, 0.0f);

        // position in world space to drag the vertex to
        const AZ::Vector3 worldEnd(-8.0f, -9.0f, 0.0f);

        const auto screenStart = AzFramework::WorldToScreen(worldStart, m_cameraState);
        const auto screenEnd = AzFramework::WorldToScreen(worldEnd, m_cameraState);

        // diagonal offset to ensure we interact with the planar manipulator and not one of the linear manipulators
        const AzFramework::ScreenVector offset(50, -50);

        m_actionDispatcher
            ->CameraState(m_cameraState)
            // move the mouse to the first vertex of the polygon prism
            ->MousePosition(screenStart)
            // click to activate the manipulator
            ->MouseLButtonDown()
            ->MouseLButtonUp()
            // offset the mouse position slightly to ensure we get the planar manipulator and not one of the linear manipulators
            ->MousePosition(screenStart + offset)
            // drag to move the manipulator
            ->MouseLButtonDown()
            ->MousePosition(screenEnd + offset)
            ->MouseLButtonUp();

        AZ::PolygonPrismPtr polygonPrism = nullptr;
        PolygonPrismShapeComponentRequestBus::EventResult(
            polygonPrism, m_entity->GetId(), &PolygonPrismShapeComponentRequests::GetPolygonPrism);

        AZ::Vector2 vertex;
        polygonPrism->m_vertexContainer.GetVertex(0, vertex);

        // dragging the vertex to (-8, -9, 0) in world space should move its local translation to (-4, -3, 0)
        AZ::Vector2 expectedVertex(-4.0f, -3.0f);
        EXPECT_THAT(vertex, UnitTest::IsCloseTolerance(expectedVertex, 1e-2f));

        // now check the manipulator is still in the correct position relative to the vertex
        // by starting a drag from the new vertex world position
        m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(screenEnd + offset)
            ->MouseLButtonDown()
            ->MousePosition(screenStart + offset)
            ->MouseLButtonUp();

        polygonPrism->m_vertexContainer.GetVertex(0, vertex);
        expectedVertex = AZ::Vector2(-2.0f, -2.0f);
        EXPECT_THAT(vertex, UnitTest::IsCloseTolerance(expectedVertex, 1e-2f));
    }
} // namespace LmbrCentral
