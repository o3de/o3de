/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/VertexContainer.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/HoverSelection.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/SelectionManipulator.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/ViewportSelection/EditorBoxSelect.h>

namespace AzToolsFramework
{
    //! Concrete implementation of AZ::VariableVertices backed by an AZ::VertexContainer.
    template<typename Vertex>
    class VariableVerticesVertexContainer : public AZ::VariableVertices<Vertex>
    {
    public:
        explicit VariableVerticesVertexContainer(AZ::VertexContainer<Vertex>& vertexContainer)
            : m_vertexContainer(vertexContainer)
        {
        }

        bool GetVertex(size_t index, Vertex& vertex) const override
        {
            return m_vertexContainer.GetVertex(index, vertex);
        }

        bool UpdateVertex(size_t index, const Vertex& vertex) override
        {
            return m_vertexContainer.UpdateVertex(index, vertex);
        };

        void AddVertex(const Vertex& vertex) override
        {
            m_vertexContainer.AddVertex(vertex);
        }

        bool InsertVertex(size_t index, const Vertex& vertex) override
        {
            return m_vertexContainer.InsertVertex(index, vertex);
        }

        bool RemoveVertex(size_t index) override
        {
            return m_vertexContainer.RemoveVertex(index);
        }

        void SetVertices(const AZStd::vector<Vertex>& vertices) override
        {
            m_vertexContainer.SetVertices(vertices);
        };

        void ClearVertices() override
        {
            m_vertexContainer.Clear();
        }

        size_t Size() const override
        {
            return m_vertexContainer.Size();
        }

        bool Empty() const override
        {
            return m_vertexContainer.Empty();
        }

    private:
        AZ::VertexContainer<Vertex>& m_vertexContainer;
    };

    //! Concrete implementation of AZ::FixedVertices backed by an AZStd::array.
    template<typename Vertex, size_t Count>
    class FixedVerticesArray : public AZ::FixedVertices<Vertex>
    {
    public:
        explicit FixedVerticesArray(AZStd::array<Vertex, Count>& array)
            : m_array(array)
        {
        }

        bool GetVertex(size_t index, Vertex& vertex) const override
        {
            if (index < m_array.size())
            {
                vertex = m_array[index];
                return true;
            }

            return false;
        }

        bool UpdateVertex(size_t index, const Vertex& vertex) override
        {
            if (index < m_array.size())
            {
                m_array[index] = vertex;
                return true;
                ;
            }

            return false;
        }

        size_t Size() const override
        {
            return m_array.size();
        }

    private:
        AZStd::array<Vertex, Count>& m_array;
    };

    //! EditorVertexSelection provides an interface for a collection of manipulators to expose
    //! editing of vertices in a container/collection. EditorVertexSelection is templated on the
    //! type of Vertex (Vector2/Vector3) stored in the container.
    //! EditorVertexSelectionBase provides common behavior shared across Fixed and Variable selections.
    template<typename Vertex>
    class EditorVertexSelectionBase
        : private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzFramework::ViewportDebugDisplayEventBus::Handler
    {
    public:
        EditorVertexSelectionBase();
        EditorVertexSelectionBase(EditorVertexSelectionBase&&) = default;
        EditorVertexSelectionBase& operator=(EditorVertexSelectionBase&&) = default;
        virtual ~EditorVertexSelectionBase() = default;

        //! Setup and configure the EditorVertexSelection for operation.
        void Create(
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            ManipulatorManagerId managerId,
            AZStd::unique_ptr<HoverSelection> hoverSelection,
            TranslationManipulators::Dimensions dimensions,
            TranslationManipulatorConfiguratorFn translationManipulatorConfigurator);

        //! Create a translation manipulator for a given vertex.
        void CreateTranslationManipulator(
            const AZ::EntityComponentIdPair& entityComponentIdPair, ManipulatorManagerId managerId, const Vertex& vertex, size_t index);

        //! Destroy all manipulators associated with the vertex selection.
        void Destroy();

        //! Set custom callback for when vertex positions are updated.
        void SetVertexPositionsUpdatedCallback(const AZStd::function<void()>& callback);

        //! Update manipulators based on local changes to vertex positions.
        void RefreshLocal();
        //! Update the translation manipulator to be correctly positioned based
        //! on the current selection (recenter it).
        void RefreshTranslationManipulator();
        //! Update manipulators based on changes to the entity's transform and non-uniform scale.
        void RefreshSpace(const AZ::Transform& worldFromLocal, const AZ::Vector3& nonUniformScale = AZ::Vector3::CreateOne());

        //! Set bounds dirty (need recalculating) for all owned manipulators (selection, translation, hover).
        void SetBoundsDirty();

        //! How should the EditorVertexSelection respond to mouse input.
        virtual bool HandleMouse(const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        //! Snap the selected vertices to the terrain.
        //! Note: With a multi-selection the manipulator will be translated to the picked
        //! terrain position with all vertices moved relative to it.
        void SnapVerticesToSurface(const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

        //! The Actions provided by the EditorVertexSelection while it is active.
        //! e.g. Vertex deletion, duplication etc.
        AZStd::vector<ActionOverride> ActionOverrides() const;

        //! Let the EditorVertexSelection know a batch movement is about to begin so it
        //! can avoid certain unnecessary updates.
        void BeginBatchMovement();

        //! Let the EditorVertexSelection know a batch movement has ended so it can return
        //! to its normal state.
        void EndBatchMovement();

        //! Set the position of the TranslationManipulators (if active).
        void SetSelectedPosition(const AZ::Vector3& localPosition);

        AZ::EntityId GetEntityId() const
        {
            return m_entityComponentIdPair.GetEntityId();
        }

    protected:
        //! Internal interface for EditorVertexSelection.
        virtual void SetupSelectionManipulator(
            const AZStd::shared_ptr<SelectionManipulator>& selectionManipulator,
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            ManipulatorManagerId managerId,
            size_t index) = 0;
        virtual void PrepareActions() = 0;

        //! Default behavior when clicking on a selection manipulator (representing a vertex).
        void SelectionManipulatorSelectCallback(
            size_t index,
            const ViewportInteraction::MouseInteraction& interaction,
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            ManipulatorManagerId managerId);

        //! Destroy the translation manipulator and deselect all vertices.
        void ClearSelected();

        AZ::ComponentId GetComponentId() const
        {
            return m_entityComponentIdPair.GetComponentId();
        }

        const AZ::EntityComponentIdPair& GetEntityComponentIdPair() const
        {
            return m_entityComponentIdPair;
        }

        ManipulatorManagerId GetManipulatorManagerId() const
        {
            return m_manipulatorManagerId;
        }

        //! Is the translation vertex manipulator in 2D or 3D.
        TranslationManipulators::Dimensions Dimensions() const
        {
            return m_dimensions;
        }

        //! How to configure the translation manipulator (view and axes).
        TranslationManipulatorConfiguratorFn ConfiguratorFn() const
        {
            return m_manipulatorConfiguratorFn;
        }

        //! The state we are in when editing vertices.
        enum class State
        {
            Selecting,
            Translating,
        };

        void SetState(State state);

        AZStd::unique_ptr<HoverSelection> m_hoverSelection =
            nullptr; //!< Interface to hover selection, representing bounds that can be selected.
        AZStd::shared_ptr<IndexedTranslationManipulator<Vertex>> m_translationManipulator =
            nullptr; //!< Manipulator when vertex is selected to translate it.
        AZStd::vector<AZStd::shared_ptr<SelectionManipulator>>
            m_selectionManipulators; //!< Manipulators for each vertex when entity is selected.
        AZStd::array<AZStd::vector<ActionOverride>, 2> m_actionOverrides; //!< Available actions corresponding to each mode.

    private:
        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        // AzFramework::ViewportDebugDisplayEventBus
        void DisplayViewport2d(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        //! Set selected manipulator and vertices position from offset from starting position when pressed.
        void UpdateManipulatorsAndVerticesFromOffset(
            IndexedTranslationManipulator<Vertex>& translationManipulator,
            const AZ::Vector3& localManipulatorStartPosition,
            const AZ::Vector3& localManipulatorOffset);

        template<typename V, typename AZStd::enable_if<AZStd::is_same<V, AZ::Vector3>::value>::type* = nullptr>
        void UpdateManipulatorSpace(const AzFramework::ViewportInfo& viewportInfo);
        template<typename V, typename AZStd::enable_if<AZStd::is_same<V, AZ::Vector2>::value>::type* = nullptr>
        void UpdateManipulatorSpace(const AzFramework::ViewportInfo& viewportInfo) const;

        EditorBoxSelect m_editorBoxSelect; //!< Provide box select support for vertex selection.
        AZ::EntityComponentIdPair m_entityComponentIdPair; //!< Id of the Entity and Component this editor vertex selection was created on.
        ManipulatorManagerId m_manipulatorManagerId; //!< Id of the manager manipulators created from this type will be associated with.
        TranslationManipulators::Dimensions m_dimensions =
            TranslationManipulators::Dimensions::Three; //!< The dimensions this vertex selection was created with.
        TranslationManipulatorConfiguratorFn m_manipulatorConfiguratorFn =
            nullptr; //!< Function pointer set on Create to decide look and functionality of translation manipulator.
        AZStd::function<void()> m_onVertexPositionsUpdated = nullptr; //!< Callback for when vertex positions are changed.
        State m_state = State::Selecting; //!< Different states VertexSelection can be in.
        bool m_worldSpace = false; //!< Are the manipulators being used in local or world space.
        bool m_batchMovementInProgress = false; //!< If a batch movement operation is in progress we do not want to
                                                //!< refresh the VertexSelection during it for performance reasons.
    };

    //! EditorVertexSelectionFixed provides selection and editing for a fixed length number of
    //! vertices. New vertices cannot be inserted/added or removed.
    template<typename Vertex>
    class EditorVertexSelectionFixed : public EditorVertexSelectionBase<Vertex>
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EditorVertexSelectionFixed() = default;
        EditorVertexSelectionFixed(EditorVertexSelectionFixed&&) = default;
        EditorVertexSelectionFixed& operator=(EditorVertexSelectionFixed&&) = default;

    private:
        // EditorVertexSelectionBase
        void SetupSelectionManipulator(
            const AZStd::shared_ptr<SelectionManipulator>& selectionManipulator,
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            ManipulatorManagerId managerId,
            size_t index) override;
        void PrepareActions() override;
    };

    //! EditorVertexSelectionVariable provides selection and editing for a variable length number of
    //! vertices. New vertices can be inserted/added or removed from the collection.
    template<typename Vertex>
    class EditorVertexSelectionVariable : public EditorVertexSelectionBase<Vertex>
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        EditorVertexSelectionVariable() = default;
        EditorVertexSelectionVariable(EditorVertexSelectionVariable&&) = default;
        EditorVertexSelectionVariable& operator=(EditorVertexSelectionVariable&&) = default;

        void DuplicateSelected();
        void DestroySelected();

    protected:
        // EditorVertexSelectionBase
        void SetupSelectionManipulator(
            const AZStd::shared_ptr<SelectionManipulator>& selectionManipulator,
            const AZ::EntityComponentIdPair& entityComponentIdPair,
            ManipulatorManagerId managerId,
            size_t vertIndex) override;

        //! Presents a warning to the user that vertices will not be deleted.
        //! @note Allow overriding by derived classes to make this a noop if required.
        virtual void ShowVertexDeletionWarning();

    private:
        void PrepareActions() override;

        //! @return The center point of the selected vertices.
        Vertex InsertSelectedInPlace(AZStd::vector<typename IndexedTranslationManipulator<Vertex>::VertexLookup>& manipulators);
    };

    //! Helper for inserting a vertex in a variable vertices container.
    template<typename Vertex>
    void InsertVertexAfter(const AZ::EntityComponentIdPair& entityComponentIdPair, size_t vertIndex, const Vertex& localPosition);

    //! Helper for removing a vertex in a variable vertices container.
    //! Remove a vertex from the container and ensure the associated manipulator is unset and
    //! property display values are refreshed.
    template<typename Vertex>
    void SafeRemoveVertex(const AZ::EntityComponentIdPair& entityComponentIdPair, size_t vertexIndex);

} // namespace AzToolsFramework
