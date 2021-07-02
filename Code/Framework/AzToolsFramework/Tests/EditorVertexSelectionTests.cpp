/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Manipulators/EditorVertexSelection.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFramework.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkTestHelpers.h>
#include <AzManipulatorTestFramework/IndirectManipulatorViewportInteraction.h>
#include <AzManipulatorTestFramework/ImmediateModeActionDispatcher.h>

using namespace AzToolsFramework;

namespace UnitTest
{
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
        bool GetVertex(size_t index, AZ::Vector3& vertex) const override { return m_vertexContainer.GetVertex(index, vertex); }
        bool UpdateVertex(size_t index, const AZ::Vector3& vertex) override { return m_vertexContainer.UpdateVertex(index, vertex); };
        void AddVertex(const AZ::Vector3& vertex) override { m_vertexContainer.AddVertex(vertex); }
        bool InsertVertex(size_t index, const AZ::Vector3& vertex) override { return m_vertexContainer.InsertVertex(index, vertex); }
        bool RemoveVertex(size_t index) override { return m_vertexContainer.RemoveVertex(index); }
        void SetVertices(const AZStd::vector<AZ::Vector3>& vertices) override { m_vertexContainer.SetVertices(vertices); };
        void ClearVertices() override { m_vertexContainer.Clear(); }
        size_t Size() const override { return m_vertexContainer.Size(); }
        bool Empty() const override { return m_vertexContainer.Empty(); }

    private:
        AZ::VertexContainer<AZ::Vector3> m_vertexContainer;
    };

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

    class TestEditorVertexSelectionVariable
        : public EditorVertexSelectionVariable<AZ::Vector3>
    {
    public:
        AZ_CLASS_ALLOCATOR(TestEditorVertexSelectionVariable, AZ::SystemAllocator, 0)

        void ShowVertexDeletionWarning() override { /*noop*/ }
    };

    class EditorVertexSelectionFixture
        : public ToolsApplicationFixture
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
        m_vertexSelection.Create(
            AZ::EntityComponentIdPair(m_entityId, AZ::InvalidComponentId),
            g_mainManipulatorManagerId, AZStd::make_unique<NullHoverSelection>(),
            TranslationManipulators::Dimensions::Three, ConfigureTranslationManipulatorAppearance3d);
    }

    void EditorVertexSelectionFixture::PopulateVertices()
    {
        for (size_t vertIndex = 0; vertIndex < EditorVertexSelectionFixture::VertexCount; ++vertIndex)
        {
            InsertVertexAfter(
                AZ::EntityComponentIdPair(m_entityId, AZ::InvalidComponentId), 0, AZ::Vector3::CreateZero());
        }
    }
    void EditorVertexSelectionFixture::ClearVertices()
    {
        for (size_t vertIndex = 0; vertIndex < EditorVertexSelectionFixture::VertexCount; ++vertIndex)
        {
            SafeRemoveVertex<AZ::Vector3>(
                AZ::EntityComponentIdPair(m_entityId, AZ::InvalidComponentId), 0);
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
        m_vertexSelection.SnapVerticesToTerrain(ViewportInteraction::MouseInteractionEvent{});
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Then
        EXPECT_TRUE(editorEntityComponentChangeDetector.ChangeDetected());
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

    using EditorVertexSelectionManipulatorFixture =
        IndirectCallManipulatorViewportInteractionFixtureMixin<EditorVertexSelectionFixture>;

    TEST_F(EditorVertexSelectionManipulatorFixture, CannotDeleteAllVertices)
    {
        using ::testing::Eq;

        const auto entityComponentIdPair = AZ::EntityComponentIdPair(m_entityId, AZ::InvalidComponentId);

        const float horizontalPositions[] = {-1.5f, -0.5f, 0.5f, 1.5f};
        for (size_t vertIndex = 0; vertIndex < std::size(horizontalPositions); ++vertIndex)
        {
            InsertVertexAfter(
                entityComponentIdPair, vertIndex, AZ::Vector3(horizontalPositions[vertIndex], 5.0f, 0.0f));
        }

        // rebuild the vertex selection after adding the new verts
        RecreateVertexSelection();

        // build a vector of the vertex positions in screen space
        AZStd::vector<AzFramework::ScreenPoint> vertexScreenPositions;
        for (size_t vertIndex = 0; vertIndex < std::size(horizontalPositions); ++vertIndex)
        {
            AZ::Vector3 localVertex;
            bool found = false;
            AZ::FixedVerticesRequestBus<AZ::Vector3>::EventResult(
                found, m_entityId, &AZ::FixedVerticesRequestBus<AZ::Vector3>::Handler::GetVertex,
                vertIndex, localVertex);

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
} // namespace UnitTest
