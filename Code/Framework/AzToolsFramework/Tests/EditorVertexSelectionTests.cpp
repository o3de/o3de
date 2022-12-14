/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <Tests/Utils/Printers.h>

#include <Tests/BoundsTestComponent.h>

namespace UnitTest
{
    const auto TestComponentId = AZ::ComponentId(1234);

    // test implementation of variable/fixed vertex request buses
    // (to be used in place of spline/polygon prism etc)
    class TestVariableVerticesVertexContainer
        : public AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler
        , public AZ::VariableVerticesRequestBus<AZ::Vector3>::Handler
    {
    public:
        void Connect(AZ::EntityId entityId);
        void Disconnect();

        // FixedVerticesRequestBus/VariableVerticesRequestBus ...
        bool GetVertex(size_t index, AZ::Vector3& vertex) const override;
        bool UpdateVertex(size_t index, const AZ::Vector3& vertex) override;
        void AddVertex(const AZ::Vector3& vertex) override;
        bool InsertVertex(size_t index, const AZ::Vector3& vertex) override;
        bool RemoveVertex(size_t index) override;
        void SetVertices(const AZStd::vector<AZ::Vector3>& vertices) override;
        void ClearVertices() override;
        size_t Size() const override;
        bool Empty() const override;

    private:
        AZ::VertexContainer<AZ::Vector3> m_vertexContainer;
    };

    bool TestVariableVerticesVertexContainer::GetVertex(size_t index, AZ::Vector3& vertex) const
    {
        return m_vertexContainer.GetVertex(index, vertex);
    }

    bool TestVariableVerticesVertexContainer::UpdateVertex(size_t index, const AZ::Vector3& vertex)
    {
        return m_vertexContainer.UpdateVertex(index, vertex);
    }

    void TestVariableVerticesVertexContainer::AddVertex(const AZ::Vector3& vertex)
    {
        m_vertexContainer.AddVertex(vertex);
    }

    bool TestVariableVerticesVertexContainer::InsertVertex(size_t index, const AZ::Vector3& vertex)
    {
        return m_vertexContainer.InsertVertex(index, vertex);
    }

    bool TestVariableVerticesVertexContainer::RemoveVertex(size_t index)
    {
        return m_vertexContainer.RemoveVertex(index);
    }

    void TestVariableVerticesVertexContainer::SetVertices(const AZStd::vector<AZ::Vector3>& vertices)
    {
        m_vertexContainer.SetVertices(vertices);
    }

    void TestVariableVerticesVertexContainer::ClearVertices()
    {
        m_vertexContainer.Clear();
    }

    size_t TestVariableVerticesVertexContainer::Size() const
    {
        return m_vertexContainer.Size();
    }

    bool TestVariableVerticesVertexContainer::Empty() const
    {
        return m_vertexContainer.Empty();
    }

    void TestVariableVerticesVertexContainer::Connect(const AZ::EntityId entityId)
    {
        AZ::VariableVerticesRequestBus<AZ::Vector3>::Handler::BusConnect(entityId);
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusConnect(entityId);
    }

    void TestVariableVerticesVertexContainer::Disconnect()
    {
        AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::BusDisconnect();
        AZ::VariableVerticesRequestBus<AZ::Vector3>::Handler::BusDisconnect();
    }

    class TestEditorVertexSelectionVariable : public AzToolsFramework::EditorVertexSelectionVariable<AZ::Vector3>
    {
    public:
        AZ_CLASS_ALLOCATOR(TestEditorVertexSelectionVariable, AZ::SystemAllocator, 0)

        void ShowVertexDeletionWarning() override
        {
            // noop
        }
    };

    class EditorVertexSelectionFixture : public ToolsApplicationFixture<>
    {
    public:
        void SetUpEditorFixtureImpl() override
        {
            m_entityId = CreateDefaultEditorEntity("Default");
            m_vertexContainer.Connect(m_entityId);
            RecreateVertexSelection();
        }

        void TearDownEditorFixtureImpl() override
        {
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequestBus::Events::DestroyEditorEntity, m_entityId);

            m_vertexContainer.Disconnect();
            m_vertexSelection.Destroy();
        }

        void RecreateVertexSelection();
        void PopulateVertices();
        void ClearVertices();

        static const AZ::u32 VertexCount = 4;

        AZ::EntityId m_entityId;
        TestEditorVertexSelectionVariable m_vertexSelection;
        TestVariableVerticesVertexContainer m_vertexContainer;
    };

    const AZ::u32 EditorVertexSelectionFixture::VertexCount;

    void EditorVertexSelectionFixture::RecreateVertexSelection()
    {
        namespace aztf = AzToolsFramework;
        m_vertexSelection.Create(
            AZ::EntityComponentIdPair(m_entityId, TestComponentId), aztf::g_mainManipulatorManagerId,
            AZStd::make_unique<aztf::NullHoverSelection>(), aztf::TranslationManipulators::Dimensions::Three,
            aztf::ConfigureTranslationManipulatorAppearance3d);
    }

    void EditorVertexSelectionFixture::PopulateVertices()
    {
        for (size_t vertIndex = 0; vertIndex < EditorVertexSelectionFixture::VertexCount; ++vertIndex)
        {
            AzToolsFramework::InsertVertexAfter(AZ::EntityComponentIdPair(m_entityId, TestComponentId), 0, AZ::Vector3::CreateZero());
        }
    }
    void EditorVertexSelectionFixture::ClearVertices()
    {
        for (size_t vertIndex = 0; vertIndex < EditorVertexSelectionFixture::VertexCount; ++vertIndex)
        {
            AzToolsFramework::SafeRemoveVertex<AZ::Vector3>(AZ::EntityComponentIdPair(m_entityId, TestComponentId), 0);
        }
    }

    TEST_F(EditorVertexSelectionFixture, PropertyEditorEntityChangeAfterVertexAdded)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        // connect before insert vertex
        EditorEntityComponentChangeDetector editorEntityComponentChangeDetector(m_entityId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        PopulateVertices();
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(editorEntityComponentChangeDetector.ChangeDetected());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorVertexSelectionFixture, PropertyEditorEntityChangeAfterVertexRemoved)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        PopulateVertices();

        // connect after insert vertex
        EditorEntityComponentChangeDetector editorEntityComponentChangeDetector(m_entityId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        ClearVertices();
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(editorEntityComponentChangeDetector.ChangeDetected());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    TEST_F(EditorVertexSelectionFixture, PropertyEditorEntityChangeAfterTerrainSnap)
    {
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Given
        PopulateVertices();

        // connect after insert vertex
        EditorEntityComponentChangeDetector editorEntityComponentChangeDetector(m_entityId);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // When
        // just provide a placeholder mouse interaction event in this case
        m_vertexSelection.SnapVerticesToSurface(AzToolsFramework::ViewportInteraction::MouseInteractionEvent{});
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(editorEntityComponentChangeDetector.ChangeDetected());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    using EditorVertexSelectionManipulatorFixture = IndirectCallManipulatorViewportInteractionFixtureMixin<EditorVertexSelectionFixture>;

    TEST_F(EditorVertexSelectionManipulatorFixture, CannotDeleteAllVertices)
    {
        using ::testing::Eq;

        const auto entityComponentIdPair = AZ::EntityComponentIdPair(m_entityId, TestComponentId);

        const float horizontalPositions[] = { -1.5f, -0.5f, 0.5f, 1.5f };
        for (size_t vertIndex = 0; vertIndex < AZStd::size(horizontalPositions); ++vertIndex)
        {
            AzToolsFramework::InsertVertexAfter(entityComponentIdPair, vertIndex, AZ::Vector3(horizontalPositions[vertIndex], 5.0f, 0.0f));
        }

        // rebuild the vertex selection after adding the new vertices
        RecreateVertexSelection();

        // build a vector of the vertex positions in screen space
        AZStd::vector<AzFramework::ScreenPoint> vertexScreenPositions;
        for (size_t vertIndex = 0; vertIndex < AZStd::size(horizontalPositions); ++vertIndex)
        {
            AZ::Vector3 localVertex = AZ::Vector3::CreateZero();
            bool found = false;
            AZ::FixedVerticesRequestBus<AZ::Vector3>::EventResult(
                found, m_entityId, &AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::GetVertex, vertIndex, localVertex);

            if (found)
            {
                // note: entity position is at the origin so localVertex position is equivalent to world
                vertexScreenPositions.push_back(AzFramework::WorldToScreen(localVertex, m_cameraState));
            }
        }

        // select each vertex (by holding ctrl)
        m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(vertexScreenPositions[0])
            ->KeyboardModifierDown(AzToolsFramework::ViewportInteraction::KeyboardModifier::Control)
            ->MouseLButtonDown()
            ->MouseLButtonUp()
            ->MousePosition(vertexScreenPositions[1])
            ->MouseLButtonDown()
            ->MouseLButtonUp()
            ->MousePosition(vertexScreenPositions[2])
            ->MouseLButtonDown()
            ->MouseLButtonUp()
            ->MousePosition(vertexScreenPositions[3])
            ->MouseLButtonDown()
            ->MouseLButtonUp();

        // and then attempt to delete them
        m_vertexSelection.DestroySelected();

        size_t vertexCountAfter = 0;
        AZ::VariableVerticesRequestBus<AZ::Vector3>::EventResult(
            vertexCountAfter, m_entityId, &AZ::VariableVerticesRequestBus<AZ::Vector3>::Events::Size);

        // deleting all vertices is disallowed - size should remain the same
        EXPECT_THAT(vertexCountAfter, Eq(EditorVertexSelectionFixture::VertexCount));
    }

    TEST_F(EditorVertexSelectionManipulatorFixture, CannotDeleteLastVertexWithManipulator)
    {
        using ::testing::Eq;

        const auto entityComponentIdPair = AZ::EntityComponentIdPair(m_entityId, TestComponentId);

        // add a single vertex (in front of the camera)
        AzToolsFramework::InsertVertexAfter(entityComponentIdPair, 0, AZ::Vector3::CreateAxisY(5.0f));

        // rebuild the vertex selection after adding the new vertices
        RecreateVertexSelection();

        AzFramework::ScreenPoint vertexScreenPosition;
        {
            AZ::Vector3 localVertex;
            bool found = false;
            AZ::FixedVerticesRequestBus<AZ::Vector3>::EventResult(
                found, m_entityId, &AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::GetVertex, 0, localVertex);

            if (found)
            {
                // note: entity position is at the origin so localVertex position is equivalent to world
                vertexScreenPosition = AzFramework::WorldToScreen(localVertex, m_cameraState);
            }
        }

        // attempt to delete the vertex by clicking with Alt held
        m_actionDispatcher->CameraState(m_cameraState)
            ->MousePosition(vertexScreenPosition)
            ->KeyboardModifierDown(AzToolsFramework::ViewportInteraction::KeyboardModifier::Alt)
            ->MouseLButtonDown()
            ->MouseLButtonUp();

        size_t vertexCountAfter = 0;
        AZ::VariableVerticesRequestBus<AZ::Vector3>::EventResult(
            vertexCountAfter, m_entityId, &AZ::VariableVerticesRequestBus<AZ::Vector3>::Events::Size);

        // deleting the last vertex through a manipulator is disallowed - size should remain the same
        EXPECT_THAT(vertexCountAfter, Eq(1));
    }

    static AZ::EntityId CreateEntityForVertexIntersectionPlacement(EditorVertexSelectionManipulatorFixture& fixture)
    {
        auto* app = fixture.GetApplication();
        app->RegisterComponentDescriptor(BoundsTestComponent::CreateDescriptor());
        app->RegisterComponentDescriptor(RenderGeometryIntersectionTestComponent::CreateDescriptor());

        AZ::Entity* entityGround = nullptr;
        AZ::EntityId entityIdGround = CreateDefaultEditorEntity("EntityGround", &entityGround);

        entityGround->Deactivate();
        auto ground = entityGround->CreateComponent<RenderGeometryIntersectionTestComponent>();
        entityGround->Activate();

        ground->m_localBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-10.0f, -10.0f, -0.5f), AZ::Vector3(10.0f, 10.0f, 0.5f));

        return entityIdGround;
    }

    static AZStd::vector<AzFramework::ScreenPoint> SetupVertices(
        const AZ::EntityId entityId, EditorVertexSelectionManipulatorFixture& fixture)
    {
        const auto entityComponentIdPair = AZ::EntityComponentIdPair(entityId, TestComponentId);
        const float horizontalPositions[] = { -3.0f, -1.0f, 1.0f, 3.0f };
        for (size_t vertIndex = 0; vertIndex < AZStd::size(horizontalPositions); ++vertIndex)
        {
            AzToolsFramework::InsertVertexAfter(entityComponentIdPair, vertIndex, AZ::Vector3(horizontalPositions[vertIndex], 0.0f, 0.0f));
        }

        // rebuild the vertex selection after adding the new vertices
        fixture.RecreateVertexSelection();

        // build a vector of the vertex positions in screen space
        AZStd::vector<AzFramework::ScreenPoint> vertexScreenPositions;
        for (size_t vertIndex = 0; vertIndex < AZStd::size(horizontalPositions); ++vertIndex)
        {
            AZ::Vector3 localVertex;
            bool found = false;
            AZ::FixedVerticesRequestBus<AZ::Vector3>::EventResult(
                found, entityId, &AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::GetVertex, vertIndex, localVertex);

            if (found)
            {
                const AZ::Vector3 worldVertex = AzToolsFramework::GetWorldTransform(entityId).TransformPoint(localVertex);
                vertexScreenPositions.push_back(AzFramework::WorldToScreen(worldVertex, fixture.m_cameraState));
            }
        }

        return vertexScreenPositions;
    }

    AzToolsFramework::ViewportInteraction::MouseInteractionEvent BuildMiddleMouseDownEvent(
        const AzFramework::ScreenPoint& screenPosition, const AzFramework::ViewportId viewportId)
    {
        AzToolsFramework::ViewportInteraction::MousePick mousePick;
        mousePick.m_screenCoordinates = screenPosition;

        AzToolsFramework::ViewportInteraction::MouseInteraction mouseInteraction;
        mouseInteraction.m_interactionId.m_cameraId = AZ::EntityId();
        mouseInteraction.m_interactionId.m_viewportId = viewportId;
        mouseInteraction.m_mouseButtons =
            AzToolsFramework::ViewportInteraction::MouseButtonsFromButton(AzToolsFramework::ViewportInteraction::MouseButton::Middle);
        mouseInteraction.m_mousePick = mousePick;
        mouseInteraction.m_keyboardModifiers = AzToolsFramework::ViewportInteraction::KeyboardModifiers(
            static_cast<AZ::u32>(AzToolsFramework::ViewportInteraction::KeyboardModifier::Shift) |
            static_cast<AZ::u32>(AzToolsFramework::ViewportInteraction::KeyboardModifier::Ctrl));

        return AzToolsFramework::ViewportInteraction::MouseInteractionEvent(
            mouseInteraction, AzToolsFramework::ViewportInteraction::MouseEvent::Down, /*captured=*/false);
    }

    TEST_F(EditorVertexSelectionManipulatorFixture, VertexPlacedWhereIntersectionPointIsFoundWithCustomReferenceSpace)
    {
        const AZ::EntityId entityIdGround = CreateEntityForVertexIntersectionPlacement(*this);

        // position ground
        AzToolsFramework::SetWorldTransform(
            entityIdGround,
            AZ::Transform::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(-20.0f)) * AZ::Matrix3x3::CreateRotationY(AZ::DegToRad(-40.0f)) *
                    AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(60.0f)),
                AZ::Vector3(14.0f, -6.0f, 5.0f)));

        // camera (go to position format) - 12.00, 18.00, 16.00, -38.00, -175.00
        m_cameraState.m_viewportSize = AzFramework::ScreenSize(1280, 720);
        AzFramework::SetCameraTransform(
            m_cameraState,
            AZ::Transform::CreateFromMatrix3x3AndTranslation(
                AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(-175.0f)) * AZ::Matrix3x3::CreateRotationX(AZ::DegToRad(-38.0f)),
                AZ::Vector3(12.0f, 18.0f, 16.0f)));

        // create orientated and scaled transform for vertex selection entity transform
        auto vertexSelectionTransform = AZ::Transform::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateRotationZ(AZ::DegToRad(45.0f)), AZ::Vector3(14.0f, 7.0f, 5.0f));
        vertexSelectionTransform.MultiplyByUniformScale(3.0f);

        // set the initial starting position of the vertex selection
        AzToolsFramework::SetWorldTransform(m_entityId, vertexSelectionTransform);

        auto vertexScreenPositions = SetupVertices(m_entityId, *this);

        // press and drag the mouse (starting where the surface manipulator is)
        // select each vertex (by holding ctrl)
        m_actionDispatcher->CameraState(m_cameraState)->MousePosition(vertexScreenPositions[0])->MouseLButtonDown()->MouseLButtonUp();

        const auto finalPositionWorld = AZ::Vector3(14.3573294f, -8.94695091f, 7.08627319f);
        // calculate the position in screen space of the final position of the entity
        const auto finalPositionScreen = AzFramework::WorldToScreen(finalPositionWorld, m_cameraState);

        auto middleMouseDownEvent =
            BuildMiddleMouseDownEvent(finalPositionScreen, m_viewportManipulatorInteraction->GetViewportInteraction().GetViewportId());

        // explicitly handle mouse event in vertex selection instance
        m_vertexSelection.HandleMouse(middleMouseDownEvent);

        // read back the position of the vertex now
        AZ::Vector3 localVertex = AZ::Vector3::CreateZero();
        bool found = false;
        AZ::FixedVerticesRequestBus<AZ::Vector3>::EventResult(
            found, m_entityId, &AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::GetVertex, 0, localVertex);

        // transform to world space
        const AZ::Vector3 worldVertex = vertexSelectionTransform.TransformPoint(localVertex);

        EXPECT_THAT(found, ::testing::IsTrue());
        // ensure final world positions match
        EXPECT_THAT(worldVertex, IsCloseTolerance(finalPositionWorld, 0.01f));
    }
} // namespace UnitTest
