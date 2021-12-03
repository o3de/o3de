/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorVertexSelection.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/VertexContainerInterface.h>
#include <AzCore/std/sort.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <QApplication>
#include <QDoubleValidator>
#include <QMessageBox>
#include <QPushButton>
#include <QSpacerItem>

using Vertex2LookupReverseIter =
    AZStd::reverse_iterator<typename AzToolsFramework::IndexedTranslationManipulator<AZ::Vector2>::VertexLookup*>;
using Vertex3LookupReverseIter =
    AZStd::reverse_iterator<typename AzToolsFramework::IndexedTranslationManipulator<AZ::Vector3>::VertexLookup*>;

namespace std
{
    template<>
    struct iterator_traits<Vertex2LookupReverseIter>
    {
        using difference_type = typename Vertex2LookupReverseIter::difference_type;
        using value_type = typename Vertex2LookupReverseIter::value_type;
        using iterator_category = typename Vertex2LookupReverseIter::iterator_category;
        using pointer = typename Vertex2LookupReverseIter::pointer;
        using reference = typename Vertex2LookupReverseIter::reference;
    };

    template<>
    struct iterator_traits<Vertex3LookupReverseIter>
    {
        using difference_type = typename Vertex3LookupReverseIter::difference_type;
        using value_type = typename Vertex3LookupReverseIter::value_type;
        using iterator_category = typename Vertex3LookupReverseIter::iterator_category;
        using pointer = typename Vertex3LookupReverseIter::pointer;
        using reference = typename Vertex3LookupReverseIter::reference;
    };
} // namespace std

namespace AzToolsFramework
{
    static const char* const s_duplicateVerticesTitle = "Duplicate Vertices";
    static const char* const s_duplicateVerticesDesc = "Duplicate current vertex selection";
    static const char* const s_deleteVerticesTitle = "Delete Vertices";
    static const char* const s_deleteVerticesDesc = "Delete current vertex selection";
    static const char* const s_deselectVerticesTitle = "Deselect Vertices";
    static const char* const s_deselectVerticesDesc = "Deselect current vertex selection";

    static void RefreshUiAfterAddRemove(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        // ensure editor entity model (entity inspector) is refreshed
        OnEntityComponentPropertyChanged(entityComponentIdPair);

        // ensure property grid values are refreshed
        ToolsApplicationNotificationBus::Broadcast(&ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_EntireTree);
    }

    template<typename Vertex>
    bool EditorVertexSelectionBase<Vertex>::HandleMouse(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        m_editorBoxSelect.HandleMouseInteraction(mouseInteraction);

        switch (m_state)
        {
        case State::Selecting:
            break;
        case State::Translating:
            {
                if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Middle() &&
                    mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Down &&
                    mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Shift() &&
                    mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl())
                {
                    SnapVerticesToSurface(mouseInteraction);
                    return true;
                }

                if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                    mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::DoubleClick)
                {
                    ClearSelected();
                    return true;
                }
            }
            break;
        default:
            return false;
        }

        return false;
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::SnapVerticesToSurface(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        ScopedUndoBatch surfaceSnapUndo("Snap to Surface");
        ScopedUndoBatch::MarkEntityDirty(GetEntityId());

        const int viewportId = mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId;
        // get unsnapped surface position (world space)
        const AZ::Vector3 worldSurfacePosition = FindClosestPickIntersection(
            viewportId, mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates, EditorPickRayLength,
            GetDefaultEntityPlacementDistance());

        AZ::Transform worldFromLocal;
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        const AZ::Transform localFromWorld = worldFromLocal.GetInverse();

        // convert to local space
        const AZ::Vector3 localFinalSurfacePosition = localFromWorld.TransformPoint(worldSurfacePosition);
        SetSelectedPosition(localFinalSurfacePosition);

        OnEntityComponentPropertyChanged(GetEntityComponentIdPair());

        // ensure property grid values are refreshed
        ToolsApplicationNotificationBus::Broadcast(&ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, Refresh_Values);
    }

    // iterate over all vertices currently associated with the translation manipulator and update their
    // positions by taking their starting positions and modifying them by an offset.
    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::UpdateManipulatorsAndVerticesFromOffset(
        IndexedTranslationManipulator<Vertex>& translationManipulator,
        const AZ::Vector3& localManipulatorStartPosition,
        const AZ::Vector3& localManipulatorOffset)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // bind FixedVerticesRequestBus for improved performance
        typename AZ::FixedVerticesRequestBus<Vertex>::BusPtr fixedVertices;
        AZ::FixedVerticesRequestBus<Vertex>::Bind(fixedVertices, GetEntityId());

        translationManipulator.Process(
            [this, localManipulatorOffset, fixedVertices](typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertex)
            {
                vertex.m_offset = AZ::AdaptVertexIn<Vertex>(localManipulatorOffset);

                bool updated = false;
                const Vertex vertexPosition = vertex.m_start + vertex.m_offset;
                AZ::FixedVerticesRequestBus<Vertex>::EventResult(
                    updated, fixedVertices, &AZ::FixedVerticesRequestBus<Vertex>::Handler::UpdateVertex, vertex.m_index, vertexPosition);

                m_selectionManipulators[vertex.m_index]->SetLocalPosition(AZ::AdaptVertexOut(vertexPosition));
            });

        m_translationManipulator->m_manipulator.SetLocalPosition(localManipulatorStartPosition + localManipulatorOffset);

        // after vertex positions have changed, anything else which relies on their positions may update
        if (m_onVertexPositionsUpdated)
        {
            m_onVertexPositionsUpdated();
        }
    }

    // in OnMouseDown for various manipulators (linear/planar/surface), ensure we record the vertex starting position
    // for each vertex associated with the translation manipulator to use with offset calculations when updating.
    template<typename Vertex>
    void InitializeVertexLookup(IndexedTranslationManipulator<Vertex>& translationManipulator, const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // bind FixedVerticesRequestBus for improved performance
        typename AZ::FixedVerticesRequestBus<Vertex>::BusPtr fixedVertices;
        AZ::FixedVerticesRequestBus<Vertex>::Bind(fixedVertices, entityId);

        translationManipulator.Process(
            [fixedVertices](typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertexLookup)
            {
                Vertex vertex;
                bool found = false;
                AZ::FixedVerticesRequestBus<Vertex>::EventResult(
                    found, fixedVertices, &AZ::FixedVerticesRequestBus<Vertex>::Handler::GetVertex, vertexLookup.m_index, vertex);

                if (found)
                {
                    vertexLookup.m_start = vertex;
                    vertexLookup.m_offset = Vertex::CreateZero();
                }
            });
    }

    // create a translation manipulator for a specific vertex and setup its corresponding callbacks etc.
    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::CreateTranslationManipulator(
        const AZ::EntityComponentIdPair& entityComponentIdPair,
        const ManipulatorManagerId managerId,
        const Vertex& vertex,
        size_t vertexIndex)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // if we have a vertex (translation) manipulator active, ensure
        // it gets removed when clicking on another selection manipulator
        ClearSelected();

        // hide this selection manipulator when clicked
        // note: this will only happen when not no translation manipulator already exists, or when
        // not shift+clicking - in those cases we want to be able to toggle selected vertex on and off
        m_selectionManipulators[vertexIndex]->Unregister();
        m_selectionManipulators[vertexIndex]->ToggleSelected();

        // create a new translation manipulator bound for the selected vertexIndex
        m_translationManipulator = AZStd::make_shared<IndexedTranslationManipulator<Vertex>>(
            Dimensions(), vertexIndex, vertex, WorldFromLocalWithUniformScale(entityComponentIdPair.GetEntityId()),
            GetNonUniformScale(entityComponentIdPair.GetEntityId()));
        m_translationManipulator->m_manipulator.SetLineBoundWidth(AzToolsFramework::ManipulatorLineBoundWidth());

        // setup how the manipulator should look
        m_manipulatorConfiguratorFn(&m_translationManipulator->m_manipulator);
        // setup where manipulator should be
        m_translationManipulator->m_manipulator.SetLocalPosition(AdaptVertexOut(vertex));

        // linear manipulator callbacks
        m_translationManipulator->m_manipulator.InstallLinearManipulatorMouseDownCallback(
            [this]([[maybe_unused]] const LinearManipulator::Action& action)
            {
                BeginBatchMovement();
                InitializeVertexLookup(*m_translationManipulator, GetEntityId());
            });

        m_translationManipulator->m_manipulator.InstallLinearManipulatorMouseMoveCallback(
            [this](const LinearManipulator::Action& action)
            {
                UpdateManipulatorsAndVerticesFromOffset(
                    *m_translationManipulator, action.m_start.m_localPosition, action.LocalPositionOffset());
            });

        m_translationManipulator->m_manipulator.InstallLinearManipulatorMouseUpCallback(
            [this]([[maybe_unused]] const LinearManipulator::Action& action)
            {
                EndBatchMovement();
            });

        // planar manipulator callbacks
        m_translationManipulator->m_manipulator.InstallPlanarManipulatorMouseDownCallback(
            [this]([[maybe_unused]] const PlanarManipulator::Action& action)
            {
                BeginBatchMovement();
                InitializeVertexLookup(*m_translationManipulator, GetEntityId());
            });

        m_translationManipulator->m_manipulator.InstallPlanarManipulatorMouseMoveCallback(
            [this](const PlanarManipulator::Action& action)
            {
                UpdateManipulatorsAndVerticesFromOffset(
                    *m_translationManipulator, action.m_start.m_localPosition, action.LocalPositionOffset());
            });

        m_translationManipulator->m_manipulator.InstallPlanarManipulatorMouseUpCallback(
            [this]([[maybe_unused]] const PlanarManipulator::Action& action)
            {
                EndBatchMovement();
            });

        // surface manipulator callbacks
        m_translationManipulator->m_manipulator.InstallSurfaceManipulatorMouseDownCallback(
            [this]([[maybe_unused]] const SurfaceManipulator::Action& action)
            {
                BeginBatchMovement();
                InitializeVertexLookup(*m_translationManipulator, GetEntityId());
            });

        m_translationManipulator->m_manipulator.InstallSurfaceManipulatorMouseMoveCallback(
            [this](const SurfaceManipulator::Action& action)
            {
                UpdateManipulatorsAndVerticesFromOffset(
                    *m_translationManipulator, action.m_start.m_localPosition, action.LocalPositionOffset());
            });

        m_translationManipulator->m_manipulator.InstallSurfaceManipulatorMouseUpCallback(
            [this]([[maybe_unused]] const SurfaceManipulator::Action& action)
            {
                EndBatchMovement();
            });

        // register the m_translation manipulator so it appears where the selection manipulator previously was
        m_translationManipulator->m_manipulator.Register(managerId);
        m_translationManipulator->m_manipulator.AddEntityComponentIdPair(entityComponentIdPair);

        // update state when in translation mode so we can 'click off' to go back to normal selection mode
        SetState(EditorVertexSelectionBase<Vertex>::State::Translating);
    }

    // pull the vertex index from VertexLookup and build a new array of just the indices
    // to be used to simplify comparisons with items in or outside the active selection
    template<typename Vertex>
    AZStd::vector<AZ::u64> MapFromLookupsToIndices(
        const AZStd::vector<typename IndexedTranslationManipulator<Vertex>::VertexLookup>& vertexLookups)
    {
        AZStd::vector<AZ::u64> vertexIndices;
        vertexIndices.reserve(vertexLookups.size());

        AZStd::transform(
            vertexLookups.begin(), vertexLookups.end(), AZStd::back_inserter(vertexIndices),
            [](const typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertexLookup)
            {
                return vertexLookup.m_index;
            });

        return vertexIndices;
    }

    // data to be used during a box select action in mouse down/move/up and display
    struct BoxSelectData
    {
        AZStd::vector<AZ::u64> m_startSelection; // what vertices were in the selection when the box select started
        AZStd::vector<AZ::u64> m_activeSelection; // the total selection (including start and delta)
        AZStd::vector<AZ::u64> m_deltaSelection; // the vertices that are either to be added or removed from the
                                                 // selection depending on if it's additive or subtractive
        AZStd::vector<AZ::u64> m_all; // a list of all vertices in the container to select from
        bool m_additive = true; // is the box select adding or removing things from the selection
    };

    template<typename Vertex>
    void DoBoxSelect(
        const AZ::EntityId entityId,
        BoxSelectData& boxSelectData,
        const ViewportInteraction::KeyboardModifiers keyboardModifiers,
        const int viewportId,
        const EditorBoxSelect& editorBoxSelect,
        const AZStd::vector<AZStd::shared_ptr<SelectionManipulator>>& selectionManipulators)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // refresh selection manipulators and box select data when modifiers change
        // (switching from additive to subtractive)
        if (keyboardModifiers != editorBoxSelect.PreviousModifiers())
        {
            for (const AZ::u64 vertexIndex : boxSelectData.m_deltaSelection)
            {
                selectionManipulators[vertexIndex]->Deselect();
            }

            for (const AZ::u64 vertexIndex : boxSelectData.m_startSelection)
            {
                selectionManipulators[vertexIndex]->Select();
            }

            boxSelectData.m_deltaSelection.clear();
            boxSelectData.m_activeSelection = boxSelectData.m_startSelection;
        }

        const AzFramework::CameraState cameraState = GetCameraState(viewportId);

        // box select active (clicking and dragging)
        if (editorBoxSelect.BoxRegion())
        {
            AZ::Transform worldFromLocal;
            AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

            // bind FixedVerticesRequestBus for improved performance
            typename AZ::FixedVerticesRequestBus<Vertex>::BusPtr fixedVertices;
            AZ::FixedVerticesRequestBus<Vertex>::Bind(fixedVertices, entityId);

            for (AZ::u64 vertexIndex = 0; vertexIndex < boxSelectData.m_all.size(); ++vertexIndex)
            {
                Vertex localVertex;
                bool found = false;
                AZ::FixedVerticesRequestBus<Vertex>::EventResult(
                    found, fixedVertices, &AZ::FixedVerticesRequestBus<Vertex>::Handler::GetVertex, vertexIndex, localVertex);

                const AZ::Vector3 worldVertex = worldFromLocal.TransformPoint(AZ::AdaptVertexOut<Vertex>(localVertex));
                const AzFramework::ScreenPoint screenPosition = AzFramework::WorldToScreen(worldVertex, cameraState);

                // check if a vertex is inside the box select region
                if (editorBoxSelect.BoxRegion()->contains(ViewportInteraction::QPointFromScreenPoint(screenPosition)))
                {
                    // see if vertexIndex is in active selection
                    auto vertexIt =
                        AZStd::find(boxSelectData.m_activeSelection.begin(), boxSelectData.m_activeSelection.end(), vertexIndex);

                    if (!keyboardModifiers.Ctrl())
                    {
                        // if additive, and not in active selection, add the vertex to the active selection
                        // and track it in delta selection as well, mark the selection manipulator as selected
                        if (vertexIt == boxSelectData.m_activeSelection.end())
                        {
                            boxSelectData.m_activeSelection.push_back(vertexIndex);
                            boxSelectData.m_deltaSelection.push_back(vertexIndex);

                            selectionManipulators[vertexIndex]->Select();
                        }
                    }
                    else
                    {
                        // if not additive and the vertex is found in the active selection, remove it from both
                        // active selection and delta selection, mark the selection manipulator as deselected
                        if (vertexIt != boxSelectData.m_activeSelection.end())
                        {
                            boxSelectData.m_activeSelection.erase(vertexIt);
                            boxSelectData.m_deltaSelection.push_back(vertexIndex);

                            selectionManipulators[vertexIndex]->Deselect();
                        }
                    }
                }
                else
                {
                    // not in box region - see if vertexIndex is in delta selection
                    auto vertexItDelta =
                        AZStd::find(boxSelectData.m_deltaSelection.begin(), boxSelectData.m_deltaSelection.end(), vertexIndex);

                    // if we find the vertex in the delta selection
                    if (vertexItDelta != boxSelectData.m_deltaSelection.end())
                    {
                        // if additive, mark the vertex as deselected (will no longer be selected on mouse up)
                        // and remove it from delta selection
                        if (!keyboardModifiers.Ctrl())
                        {
                            selectionManipulators[vertexIndex]->Deselect();
                            boxSelectData.m_deltaSelection.erase(vertexItDelta);

                            // remove the vertex from the active selection as well
                            auto vertexItStart =
                                AZStd::find(boxSelectData.m_activeSelection.begin(), boxSelectData.m_activeSelection.end(), vertexIndex);

                            if (vertexItStart != boxSelectData.m_activeSelection.end())
                            {
                                boxSelectData.m_activeSelection.erase(vertexItStart);
                            }
                        }
                        else
                        {
                            // if not additive, mark the vertex as selected again (it was selected before this
                            // box select began)
                            selectionManipulators[vertexIndex]->Select();
                            boxSelectData.m_deltaSelection.erase(vertexItDelta);

                            // also add it back to the active selection
                            auto vertexItStart =
                                AZStd::find(boxSelectData.m_activeSelection.begin(), boxSelectData.m_activeSelection.end(), vertexIndex);

                            if (vertexItStart == boxSelectData.m_activeSelection.end())
                            {
                                boxSelectData.m_activeSelection.push_back(vertexIndex);
                            }
                        }
                    }
                }
            }
        }

        // track if we're in additive or subtractive mode
        boxSelectData.m_additive = !keyboardModifiers.Ctrl();
    }

    template<typename Vertex>
    EditorVertexSelectionBase<Vertex>::EditorVertexSelectionBase()
    {
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::Create(
        const AZ::EntityComponentIdPair& entityComponentIdPair,
        const ManipulatorManagerId managerId,
        AZStd::unique_ptr<HoverSelection> hoverSelection,
        const TranslationManipulators::Dimensions dimensions,
        const TranslationManipulatorConfiguratorFn translationManipulatorConfigurator)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_dimensions = dimensions;
        m_manipulatorManagerId = managerId;
        m_entityComponentIdPair = entityComponentIdPair;
        m_manipulatorConfiguratorFn = translationManipulatorConfigurator;
        m_hoverSelection = AZStd::move(hoverSelection);

        // bind FixedVerticesRequestBus for improved performance
        typename AZ::FixedVerticesRequestBus<Vertex>::BusPtr fixedVertices;
        AZ::FixedVerticesRequestBus<Vertex>::Bind(fixedVertices, GetEntityId());

        size_t vertexCount = 0;
        AZ::FixedVerticesRequestBus<Vertex>::EventResult(vertexCount, fixedVertices, &AZ::FixedVerticesRequestBus<Vertex>::Handler::Size);

        m_selectionManipulators.reserve(vertexCount);
        // initialize manipulators for all spline vertices
        for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
        {
            Vertex vertex;
            bool found = false;
            AZ::FixedVerticesRequestBus<Vertex>::EventResult(
                found, fixedVertices, &AZ::FixedVerticesRequestBus<Vertex>::Handler::GetVertex, vertexIndex, vertex);

            m_selectionManipulators.push_back(
                SelectionManipulator::MakeShared(WorldFromLocalWithUniformScale(GetEntityId()), GetNonUniformScale(GetEntityId())));
            const auto& selectionManipulator = m_selectionManipulators.back();

            selectionManipulator->Register(managerId);
            selectionManipulator->AddEntityComponentIdPair(entityComponentIdPair);
            selectionManipulator->SetLocalPosition(AdaptVertexOut(vertex));

            SetupSelectionManipulator(selectionManipulator, entityComponentIdPair, managerId, vertexIndex);
        }

        // lambdas capture shared_ptr by value to increment ref count
        auto vertexBoxSelectData = AZStd::make_shared<BoxSelectData>();

        m_editorBoxSelect.InstallLeftMouseDown(
            [this, vertexBoxSelectData](const ViewportInteraction::MouseInteractionEvent& /*mouseInteraction*/)
            {
                // grab currently selected entities (the starting selection)
                vertexBoxSelectData->m_startSelection = m_translationManipulator
                    ? MapFromLookupsToIndices<Vertex>(m_translationManipulator->m_vertices)
                    : AZStd::vector<AZ::u64>();

                // active selection is the same as start selection on mouse down
                vertexBoxSelectData->m_activeSelection = vertexBoxSelectData->m_startSelection;

                size_t size = 0;
                AZ::FixedVerticesRequestBus<Vertex>::EventResult(size, GetEntityId(), &AZ::FixedVerticesRequestBus<Vertex>::Handler::Size);

                // populate vector of all indices in container to compare against
                vertexBoxSelectData->m_all.resize(size);
                std::iota(vertexBoxSelectData->m_all.begin(), vertexBoxSelectData->m_all.end(), static_cast<AZ::u64>(0));
            });

        m_editorBoxSelect.InstallMouseMove(
            [this, vertexBoxSelectData](const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
            {
                DoBoxSelect<Vertex>(
                    GetEntityId(), *vertexBoxSelectData, mouseInteraction.m_mouseInteraction.m_keyboardModifiers,
                    mouseInteraction.m_mouseInteraction.m_interactionId.m_viewportId, m_editorBoxSelect, m_selectionManipulators);
            });

        m_editorBoxSelect.InstallLeftMouseUp(
            [this, vertexBoxSelectData]()
            {
                if (vertexBoxSelectData->m_additive)
                {
                    // bind FixedVerticesRequestBus for improved performance
                    typename AZ::FixedVerticesRequestBus<Vertex>::BusPtr fixedVertices;
                    AZ::FixedVerticesRequestBus<Vertex>::Bind(fixedVertices, GetEntityId());

                    const AZ::EntityComponentIdPair entityComponentIdPair = m_entityComponentIdPair;
                    for (size_t vertexIndex : vertexBoxSelectData->m_deltaSelection)
                    {
                        Vertex vertex;
                        bool found = false;
                        AZ::FixedVerticesRequestBus<Vertex>::EventResult(
                            found, fixedVertices, &AZ::FixedVerticesRequestBus<Vertex>::Handler::GetVertex, vertexIndex, vertex);

                        // if we already have a translation manipulator, add additional vertices to it
                        if (m_translationManipulator)
                        {
                            // otherwise add the new selected vertex
                            m_translationManipulator->m_vertices.push_back(
                                typename IndexedTranslationManipulator<Vertex>::VertexLookup{ vertex, Vertex::CreateZero(), vertexIndex });
                        }
                        else
                        {
                            // create a new translation manipulator if one did not already exist with the first vertex
                            CreateTranslationManipulator(entityComponentIdPair, m_manipulatorManagerId, vertex, vertexIndex);
                            // default to ensuring selection manipulators are 'selected'
                            m_selectionManipulators[vertexIndex]->Select();
                        }
                    }
                }
                else
                {
                    // removing vertices with an active translation manipulator
                    if (m_translationManipulator)
                    {
                        // iterate through all delta vertices (ones that were either
                        // added or removed during selection) and remove them
                        for (size_t vertexIndex : vertexBoxSelectData->m_deltaSelection)
                        {
                            auto vertexIt = AZStd::find_if(
                                m_translationManipulator->m_vertices.begin(), m_translationManipulator->m_vertices.end(),
                                [vertexIndex](const auto& vertexLookup)
                                {
                                    return vertexLookup.m_index == vertexIndex;
                                });

                            // remove vertex from translation manipulator
                            if (vertexIt != m_translationManipulator->m_vertices.end())
                            {
                                m_translationManipulator->m_vertices.erase(vertexIt);

                                // ensure it is registered to receive input and draw
                                if (!m_selectionManipulators[vertexIndex]->Registered())
                                {
                                    m_selectionManipulators[vertexIndex]->Register(m_manipulatorManagerId);
                                }
                            }
                        }

                        // if we have no vertices left, clear selection (restore all selection
                        // manipulators and destroy translation manipulator)
                        if (m_translationManipulator->m_vertices.empty())
                        {
                            ClearSelected();
                        }
                    }
                }

                // with a selection of more than one or zero, we want to ensure all selection
                // manipulators are registered (can be clicked on)
                if (vertexBoxSelectData->m_activeSelection.size() > 1)
                {
                    for (size_t vertexIndex : vertexBoxSelectData->m_activeSelection)
                    {
                        if (!m_selectionManipulators[vertexIndex]->Registered())
                        {
                            m_selectionManipulators[vertexIndex]->Register(m_manipulatorManagerId);
                        }
                    }
                }
                // special case handling for only one vertex - don't want to display it when
                // translation manipulator will be in exactly the same location
                else if (vertexBoxSelectData->m_activeSelection.size() == 1)
                {
                    if (vertexBoxSelectData->m_additive)
                    {
                        m_selectionManipulators[vertexBoxSelectData->m_activeSelection[0]]->Unregister();
                    }
                    else
                    {
                        m_selectionManipulators[vertexBoxSelectData->m_activeSelection[0]]->Register(m_manipulatorManagerId);
                    }
                }

                // update manipulator positions (ensure translation manipulator is
                // centered on current selection)
                RefreshTranslationManipulator();

                // restore state once box select has completed
                vertexBoxSelectData->m_startSelection.clear();
                vertexBoxSelectData->m_deltaSelection.clear();
                vertexBoxSelectData->m_activeSelection.clear();
                vertexBoxSelectData->m_all.clear();
            });

        m_editorBoxSelect.InstallDisplayScene(
            [this, vertexBoxSelectData](const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& /*debugDisplay*/)
            {
                const auto keyboardModifiers = AzToolsFramework::ViewportInteraction::QueryKeyboardModifiers();

                // when modifiers change ensure we refresh box selection for immediate update
                if (keyboardModifiers != m_editorBoxSelect.PreviousModifiers())
                {
                    DoBoxSelect<Vertex>(
                        GetEntityId(), *vertexBoxSelectData, keyboardModifiers, viewportInfo.m_viewportId, m_editorBoxSelect,
                        m_selectionManipulators);
                }
            });

        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(GetEntityContextId());

        // note: must be called after m_entityComponentIdPair is set
        PrepareActions();
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::Destroy()
    {
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();

        for (auto& manipulator : m_selectionManipulators)
        {
            manipulator->Unregister();
            manipulator.reset();
        }

        m_selectionManipulators.clear();

        if (m_translationManipulator)
        {
            m_translationManipulator->m_manipulator.Unregister();
            m_translationManipulator.reset();
        }

        if (m_hoverSelection)
        {
            m_hoverSelection.reset();
        }
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::ClearSelected()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // if translation manipulator is active, remove it when receiving this event and enable
        // the hover manipulator bounds again so points can be inserted again
        if (m_translationManipulator)
        {
            // re-enable all selection manipulators associated with the translation
            // manipulator which is now being removed.
            m_translationManipulator->Process(
                [this](typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertex)
                {
                    m_selectionManipulators[vertex.m_index]->Register(m_manipulatorManagerId);
                    m_selectionManipulators[vertex.m_index]->Deselect();
                });

            m_translationManipulator->m_manipulator.Unregister();
            m_translationManipulator.reset();
        }

        if (m_hoverSelection)
        {
            m_hoverSelection->Register(m_manipulatorManagerId);
        }

        SetState(EditorVertexSelectionBase<Vertex>::State::Selecting);
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_editorBoxSelect.DisplayScene(viewportInfo, debugDisplay);

        UpdateManipulatorSpace<Vertex>(viewportInfo);
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::DisplayViewport2d(
        const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        m_editorBoxSelect.Display2d(viewportInfo, debugDisplay);
    }

    template<typename Vertex>
    template<typename V, typename AZStd::enable_if<AZStd::is_same<V, AZ::Vector3>::value>::type*>
    void EditorVertexSelectionBase<Vertex>::UpdateManipulatorSpace(const AzFramework::ViewportInfo& viewportInfo)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // check if 'shift' is being held to move to parent space
        bool worldSpace = false;
        ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
            worldSpace, viewportInfo.m_viewportId,
            &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::ShowingWorldSpace);

        // update the manipulator to be in the correct space if it changed
        if (m_translationManipulator && !m_translationManipulator->m_manipulator.PerformingAction() && worldSpace != m_worldSpace)
        {
            const AZ::Transform worldFromLocal = WorldFromLocalWithUniformScale(GetEntityId());

            m_translationManipulator->m_manipulator.SetLocalOrientation(
                worldSpace ? QuaternionFromTransformNoScaling(worldFromLocal).GetInverseFull() : AZ::Quaternion::CreateIdentity());

            m_worldSpace = worldSpace;
        }
    }
    template<typename Vertex>
    template<typename V, typename AZStd::enable_if<AZStd::is_same<V, AZ::Vector2>::value>::type*>
    void EditorVertexSelectionBase<Vertex>::UpdateManipulatorSpace(const AzFramework::ViewportInfo& /*viewportInfo*/) const
    {
    }

    template<typename Vertex>
    static bool CanDeleteSelection(const AZ::EntityId entityId, const int64_t selectedCount)
    {
        size_t vertexCount = 0;
        AZ::VariableVerticesRequestBus<Vertex>::EventResult(vertexCount, entityId, &AZ::VariableVerticesRequestBus<Vertex>::Handler::Size);

        // prevent deleting all vertices
        const int64_t remaining = aznumeric_cast<int64_t>(vertexCount) - selectedCount;
        return remaining >= 1;
    }

    template<typename Vertex>
    void EditorVertexSelectionVariable<Vertex>::ShowVertexDeletionWarning()
    {
        QMessageBox::information(
            AzToolsFramework::GetActiveWindow(), "Information", "It is not possible to delete all vertices.", QMessageBox::Ok,
            QMessageBox::NoButton);
    }

    template<typename Vertex>
    void EditorVertexSelectionVariable<Vertex>::DestroySelected()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const AZ::EntityId entityId = EditorVertexSelectionBase<Vertex>::GetEntityId();

        ScopedUndoBatch duplicateUndo("Delete Vertices");
        ScopedUndoBatch::MarkEntityDirty(entityId);

        // if the translation manipulator is active, remove it and destroy the vertices associated with it
        // enable the hover manipulator bounds so points can be inserted again
        if (auto* manipulator = EditorVertexSelectionBase<Vertex>::m_translationManipulator.get())
        {
            if (!CanDeleteSelection<Vertex>(entityId, manipulator->m_vertices.size()))
            {
                ShowVertexDeletionWarning();
                return;
            }

            // keep an additional reference to the translation manipulator while removing it so it is not
            // destroyed prematurely
            AZStd::shared_ptr<IndexedTranslationManipulator<Vertex>> translationManipulator =
                EditorVertexSelectionBase<Vertex>::m_translationManipulator;

            // ensure we remove vertices in reverse order
            std::sort(
                translationManipulator->m_vertices.rbegin(), translationManipulator->m_vertices.rend(),
                [](const typename IndexedTranslationManipulator<Vertex>::VertexLookup& lhs,
                   const typename IndexedTranslationManipulator<Vertex>::VertexLookup& rhs)
                {
                    return lhs.m_index < rhs.m_index;
                });

            translationManipulator->Process(
                [this](typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertex)
                {
                    SafeRemoveVertex<Vertex>(EditorVertexSelectionBase<Vertex>::GetEntityComponentIdPair(), vertex.m_index);
                });

            translationManipulator->m_manipulator.Unregister();
            translationManipulator.reset();
        }

        if (EditorVertexSelectionBase<Vertex>::m_hoverSelection)
        {
            EditorVertexSelectionBase<Vertex>::m_hoverSelection->Register(EditorVertexSelectionBase<Vertex>::GetManipulatorManagerId());
        }

        EditorVertexSelectionBase<Vertex>::SetState(EditorVertexSelectionBase<Vertex>::State::Selecting);
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::SetSelectedPosition(const AZ::Vector3& localPosition)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_translationManipulator)
        {
            BeginBatchMovement();

            InitializeVertexLookup(*m_translationManipulator, GetEntityId());
            // note: AdaptVertexIn/Out is to ensure we clamp the vertex local Z position to 0 if
            // dealing with Vector2s when setting the position of the manipulator.
            const AZ::Vector3 localOffset = localPosition - m_translationManipulator->m_manipulator.GetLocalTransform().GetTranslation();
            UpdateManipulatorsAndVerticesFromOffset(
                *m_translationManipulator, AZ::AdaptVertexOut<Vertex>(AZ::AdaptVertexIn<Vertex>(localPosition)),
                AZ::AdaptVertexOut<Vertex>(AZ::AdaptVertexIn<Vertex>(localOffset)));

            RefreshTranslationManipulator();

            EndBatchMovement();
        }
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::SetVertexPositionsUpdatedCallback(const AZStd::function<void()>& callback)
    {
        m_onVertexPositionsUpdated = callback;
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::RefreshTranslationManipulator()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // bind FixedVerticesRequestBus for improved performance
        typename AZ::FixedVerticesRequestBus<Vertex>::BusPtr fixedVertices;
        AZ::FixedVerticesRequestBus<Vertex>::Bind(fixedVertices, GetEntityId());

        if (m_translationManipulator && !m_translationManipulator->m_vertices.empty())
        {
            // calculate average position of selected vertices for translation manipulator
            MidpointCalculator midpointCalculator;
            m_translationManipulator->Process(
                [&midpointCalculator, fixedVertices](typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertex)
                {
                    Vertex v;
                    bool found = false;
                    AZ::FixedVerticesRequestBus<Vertex>::EventResult(
                        found, fixedVertices, &AZ::FixedVerticesRequestBus<Vertex>::Handler::GetVertex, vertex.m_index, v);

                    if (found)
                    {
                        midpointCalculator.AddPosition(AZ::AdaptVertexOut(v));
                    }
                });

            m_translationManipulator->m_manipulator.SetLocalPosition(AZ::AdaptVertexOut(midpointCalculator.CalculateMidpoint()));
        }
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::RefreshLocal()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // we do not want to refresh our local state while a batch movement is in progress,
        // even if we have been signalled to do so by a callback
        if (m_batchMovementInProgress)
        {
            return;
        }

        // bind FixedVerticesRequestBus for improved performance
        typename AZ::FixedVerticesRequestBus<Vertex>::BusPtr fixedVertices;
        AZ::FixedVerticesRequestBus<Vertex>::Bind(fixedVertices, GetEntityId());

        // the vertices that can be selected
        for (size_t manipulatorIndex = 0; manipulatorIndex < m_selectionManipulators.size(); ++manipulatorIndex)
        {
            Vertex vertex;
            bool found = false;
            AZ::FixedVerticesRequestBus<Vertex>::EventResult(
                found, fixedVertices, &AZ::FixedVerticesRequestBus<Vertex>::Handler::GetVertex, manipulatorIndex, vertex);

            if (found)
            {
                m_selectionManipulators[manipulatorIndex]->SetLocalPosition(AZ::AdaptVertexOut(vertex));
            }
        }

        RefreshTranslationManipulator();

        if (m_hoverSelection)
        {
            m_hoverSelection->Refresh();
        }

        SetBoundsDirty();
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::RefreshSpace(const AZ::Transform& worldFromLocal, const AZ::Vector3& nonUniformScale)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        for (auto& manipulator : m_selectionManipulators)
        {
            manipulator->SetSpace(TransformUniformScale(worldFromLocal));
            manipulator->SetNonUniformScale(nonUniformScale);
        }

        if (m_translationManipulator)
        {
            m_translationManipulator->m_manipulator.SetSpace(TransformUniformScale(worldFromLocal));
            m_translationManipulator->m_manipulator.SetNonUniformScale(nonUniformScale);
        }

        if (m_hoverSelection)
        {
            m_hoverSelection->SetSpace(TransformUniformScale(worldFromLocal));
            m_hoverSelection->SetNonUniformScale(nonUniformScale);
            m_hoverSelection->Refresh();
        }

        SetBoundsDirty();
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::SetBoundsDirty()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        for (auto& manipulator : m_selectionManipulators)
        {
            manipulator->SetBoundsDirty();
        }

        if (m_translationManipulator)
        {
            m_translationManipulator->m_manipulator.SetBoundsDirty();
        }

        if (m_hoverSelection)
        {
            m_hoverSelection->SetBoundsDirty();
        }
    }

    // handle correctly selecting/deselecting vertices in a vertex selection.
    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::SelectionManipulatorSelectCallback(
        const size_t vertexIndex,
        const ViewportInteraction::MouseInteraction& interaction,
        const AZ::EntityComponentIdPair& entityComponentIdPair,
        const ManipulatorManagerId managerId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        Vertex vertex;
        bool found = false;
        AZ::FixedVerticesRequestBus<Vertex>::EventResult(
            found, GetEntityId(), &AZ::FixedVerticesRequestBus<Vertex>::Handler::GetVertex, vertexIndex, vertex);

        if (m_translationManipulator != nullptr && interaction.m_keyboardModifiers.Ctrl())
        {
            // ensure all selection manipulators are enabled when selecting more than one (the first
            // will have been disabled when only selecting an individual vertex
            m_translationManipulator->Process(
                [this, managerId](typename IndexedTranslationManipulator<Vertex>::VertexLookup& vertexLookup)
                {
                    m_selectionManipulators[vertexLookup.m_index]->Register(managerId);
                });

            // if selection manipulator was selected, find it in the vector of vertices stored in
            // the translation manipulator and remove it
            if (m_selectionManipulators[vertexIndex]->Selected())
            {
                auto it = AZStd::find_if(
                    m_translationManipulator->m_vertices.begin(), m_translationManipulator->m_vertices.end(),
                    [vertexIndex](const typename IndexedTranslationManipulator<Vertex>::VertexLookup vertexLookup)
                    {
                        return vertexIndex == vertexLookup.m_index;
                    });

                if (it != m_translationManipulator->m_vertices.end())
                {
                    m_translationManipulator->m_vertices.erase(it);
                }
            }
            else
            {
                // otherwise add the new selected vertex
                m_translationManipulator->m_vertices.push_back(
                    typename IndexedTranslationManipulator<Vertex>::VertexLookup{ vertex, Vertex::CreateZero(), vertexIndex });
            }

            // if we have deselected a selection manipulator, and there is only one left, unregister/disable
            // it as we do not want to draw it at the same position as the translation manipulator
            if (m_translationManipulator->m_vertices.size() == 1)
            {
                m_selectionManipulators[m_translationManipulator->m_vertices.back().m_index]->Unregister();
            }

            // toggle state after a press
            m_selectionManipulators[vertexIndex]->ToggleSelected();

            RefreshLocal();
        }
        else
        {
            // if one does not already exist, or we're not holding shift, create a new translation
            // manipulator at this vertex
            CreateTranslationManipulator(entityComponentIdPair, managerId, vertex, vertexIndex);
        }
    }

    // configure the selection manipulator for fixed editor selection - this configures the view and action
    // of interacting with the selection manipulator. Vertices can just be selected (create a translation
    // manipulator) but not added or removed.
    template<typename Vertex>
    void EditorVertexSelectionFixed<Vertex>::SetupSelectionManipulator(
        const AZStd::shared_ptr<SelectionManipulator>& selectionManipulator,
        const AZ::EntityComponentIdPair& entityComponentIdPair,
        const ManipulatorManagerId managerId,
        const size_t vertexIndex)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // setup selection manipulator
        const AZStd::shared_ptr<ManipulatorViewSphere> selectionView = AzToolsFramework::CreateManipulatorViewSphere(
            AZ::Color(1.0f, 0.0f, 0.0f, 1.0f), g_defaultManipulatorSphereRadius,
            [&selectionManipulator](
                const ViewportInteraction::MouseInteraction& /*mouseInteraction*/, const bool mouseOver, const AZ::Color& defaultColor)
            {
                if (selectionManipulator->Selected())
                {
                    return AZ::Color(1.0f, 1.0f, 0.0f, 1.0f);
                }

                const float opacity[2] = { 0.5f, 1.0f };
                return AZ::Color(defaultColor.GetR(), defaultColor.GetG(), defaultColor.GetB(), opacity[mouseOver]);
            });

        selectionManipulator->SetViews(ManipulatorViews{ selectionView });

        selectionManipulator->InstallLeftMouseUpCallback(
            [this, entityComponentIdPair, vertexIndex, managerId](const ViewportInteraction::MouseInteraction& interaction)
            {
                EditorVertexSelectionBase<Vertex>::SelectionManipulatorSelectCallback(
                    vertexIndex, interaction, entityComponentIdPair, managerId);
            });
    }

    // configure the selection manipulator for variable editor selection - this configures the view and action
    // of interacting with the selection manipulator. In this case, hovering the mouse with a modifier key held
    // will indicate removal, and clicking with a modifier key will remove the vertex.
    template<typename Vertex>
    void EditorVertexSelectionVariable<Vertex>::SetupSelectionManipulator(
        const AZStd::shared_ptr<SelectionManipulator>& selectionManipulator,
        const AZ::EntityComponentIdPair& entityComponentIdPair,
        const ManipulatorManagerId managerId,
        const size_t vertexIndex)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // setup selection manipulator
        const AZStd::shared_ptr<ManipulatorViewSphere> manipulatorView = AzToolsFramework::CreateManipulatorViewSphere(
            AZ::Color(1.0f, 0.0f, 0.0f, 1.0f), g_defaultManipulatorSphereRadius,
            [&selectionManipulator](
                const ViewportInteraction::MouseInteraction& mouseInteraction, const bool mouseOver, const AZ::Color& defaultColor)
            {
                if (mouseInteraction.m_keyboardModifiers.Alt() && mouseOver)
                {
                    // indicate removal of manipulator
                    return AZ::Color(0.5f, 0.5f, 0.5f, 0.5f);
                }

                // highlight or not if mouse is over
                const float opacity[2] = { 0.5f, 1.0f };
                if (selectionManipulator->Selected())
                {
                    return AZ::Color(1.0f, 1.0f, 0.0f, opacity[mouseOver]);
                }

                return AZ::Color(defaultColor.GetR(), defaultColor.GetG(), defaultColor.GetB(), opacity[mouseOver]);
            });

        selectionManipulator->SetViews(ManipulatorViews{ manipulatorView });

        selectionManipulator->InstallLeftMouseUpCallback(
            [this, entityComponentIdPair, vertexIndex, managerId](const ViewportInteraction::MouseInteraction& interaction)
            {
                if (interaction.m_keyboardModifiers.Alt())
                {
                    if (!CanDeleteSelection<Vertex>(entityComponentIdPair.GetEntityId(), /*selectedCount=*/1))
                    {
                        ShowVertexDeletionWarning();
                        return;
                    }

                    SafeRemoveVertex<Vertex>(entityComponentIdPair, vertexIndex);
                }
                else
                {
                    EditorVertexSelectionBase<Vertex>::SelectionManipulatorSelectCallback(
                        vertexIndex, interaction, entityComponentIdPair, managerId);
                }
            });
    }

    template<typename Vertex>
    AZStd::vector<ActionOverride> EditorVertexSelectionBase<Vertex>::ActionOverrides() const
    {
        return m_actionOverrides[static_cast<size_t>(m_state)];
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::SetState(const State state)
    {
        using namespace AzToolsFramework::ComponentModeFramework;

        if (state != m_state)
        {
            m_state = state;
            ComponentModeSystemRequestBus::Broadcast(&ComponentModeSystemRequests::RefreshActions);
        }
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::BeginBatchMovement()
    {
        AZ_Assert(!m_batchMovementInProgress, "Cannot begin a batch movement while one is already in progress");
        m_batchMovementInProgress = true;
    }

    template<typename Vertex>
    void EditorVertexSelectionBase<Vertex>::EndBatchMovement()
    {
        AZ_Assert(m_batchMovementInProgress, "Cannot end a batch movement while one is not active");
        m_batchMovementInProgress = false;

        // our hover selection may depend directly on the updated vertex positions. we must ensure we
        // refresh the hover selection after updating the vertices themselves so it stays up to date.
        if (m_hoverSelection)
        {
            m_hoverSelection->Refresh();
        }

        SetBoundsDirty();
    }

    template<typename Vertex>
    void EditorVertexSelectionFixed<Vertex>::PrepareActions()
    {
        ActionOverride backAction = CreateBackAction(
            "Deselect Vertex", "Deselect current vertex selection",
            [this]()
            {
                EditorVertexSelectionBase<Vertex>::ClearSelected();
            });

        backAction.SetEntityComponentIdPair(EditorVertexSelectionBase<Vertex>::GetEntityComponentIdPair());

        EditorVertexSelectionBase<Vertex>::m_actionOverrides[static_cast<size_t>(EditorVertexSelectionBase<Vertex>::State::Translating)] =
            AZStd::vector<ActionOverride>{ backAction };
    }

    template<typename Vertex>
    Vertex EditorVertexSelectionVariable<Vertex>::InsertSelectedInPlace(
        AZStd::vector<typename IndexedTranslationManipulator<Vertex>::VertexLookup>& manipulators)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // utility to calculate the center point of the selected vertices after duplication
        MidpointCalculator midpointCalculator;

        // sort in descending order
        std::sort(
            manipulators.rbegin(), manipulators.rend(),
            [](const typename IndexedTranslationManipulator<Vertex>::VertexLookup& lhs,
               const typename IndexedTranslationManipulator<Vertex>::VertexLookup& rhs)
            {
                return lhs.m_index < rhs.m_index;
            });

        // iterate over current selection
        for (size_t manipulatorIndex = 0; manipulatorIndex < manipulators.size(); ++manipulatorIndex)
        {
            // add all positions to mid point calculator to later calculate center position
            midpointCalculator.AddPosition(AZ::AdaptVertexOut(manipulators[manipulatorIndex].CurrentPosition()));

            // insert vertex after currently selected vertex
            InsertVertexAfter<Vertex>(
                EditorVertexSelectionBase<Vertex>::GetEntityComponentIdPair(), manipulators[manipulatorIndex].m_index,
                manipulators[manipulatorIndex].CurrentPosition());

            // want to bump the index on to the newly inserted vertex
            // (so the duplicated vertex is now the selected one)
            ++manipulators[manipulatorIndex].m_index;
        }

        // update selected indices based on vertices inserted before each vertex
        // (start at last selected vertex and work back as sort order was descending)
        size_t indexShift = manipulators.size() - 1;
        for (size_t manipulatorIndex = 0; manipulatorIndex < manipulators.size(); ++manipulatorIndex)
        {
            manipulators[manipulatorIndex].m_index += indexShift--;
        }

        return AZ::AdaptVertexIn<Vertex>(midpointCalculator.CalculateMidpoint());
    }

    template<typename Vertex>
    void EditorVertexSelectionVariable<Vertex>::DuplicateSelected()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        ScopedUndoBatch duplicateUndo("Duplicate Vertices");
        ScopedUndoBatch::MarkEntityDirty(EditorVertexSelectionBase<Vertex>::GetEntityId());

        // make a copy of selected vertices
        AZStd::vector<typename IndexedTranslationManipulator<Vertex>::VertexLookup> vertices =
            EditorVertexSelectionBase<Vertex>::m_translationManipulator->m_vertices;

        // duplicate the selected vertices and insert them
        // after each currently selected vertex
        const Vertex localCenterPosition = InsertSelectedInPlace(vertices);

        // create translation manipulator for duplicated vertices at new position
        EditorVertexSelectionBase<Vertex>::CreateTranslationManipulator(
            EditorVertexSelectionBase<Vertex>::GetEntityComponentIdPair(), EditorVertexSelectionBase<Vertex>::GetManipulatorManagerId(),
            localCenterPosition, vertices[0].m_index);

        // clear all selection manipulators to default unselected state
        for (auto& selectionManipulator : EditorVertexSelectionBase<Vertex>::m_selectionManipulators)
        {
            selectionManipulator->Register(EditorVertexSelectionBase<Vertex>::GetManipulatorManagerId());
            selectionManipulator->Deselect();
        }

        // select the newly duplicated vertices
        for (size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex)
        {
            EditorVertexSelectionBase<Vertex>::m_selectionManipulators[vertices[vertexIndex].m_index]->Select();
        }

        // assign updated indexed vertices back to translation manipulator
        EditorVertexSelectionBase<Vertex>::m_translationManipulator->m_vertices = vertices;
    }

    template<typename Vertex>
    void EditorVertexSelectionVariable<Vertex>::PrepareActions()
    {
        ActionOverride deleteAction = CreateDeleteAction(
            s_deleteVerticesTitle, s_deleteVerticesDesc,
            [this]()
            {
                DestroySelected();
            });

        const AZ::EntityComponentIdPair entityComponentIdPair(
            EditorVertexSelectionBase<Vertex>::GetEntityId(), EditorVertexSelectionBase<Vertex>::GetComponentId());

        // note: important to register which entity/component id pair this action is associated with
        deleteAction.SetEntityComponentIdPair(entityComponentIdPair);

        ActionOverride deselectAction = CreateBackAction(
            s_deselectVerticesTitle, s_deselectVerticesDesc,
            [this]()
            {
                EditorVertexSelectionBase<Vertex>::ClearSelected();
            });

        // note: important to register which entity/component id pair this action is associated with
        deselectAction.SetEntityComponentIdPair(entityComponentIdPair);

        EditorVertexSelectionBase<Vertex>::m_actionOverrides[static_cast<size_t>(EditorVertexSelectionBase<Vertex>::State::Translating)] =
            AZStd::vector<ActionOverride>{ ActionOverride()
                                               .SetUri(AzToolsFramework::s_duplicateAction)
                                               .SetKeySequence(QKeySequence(Qt::CTRL + Qt::Key_D))
                                               .SetTitle(s_duplicateVerticesTitle)
                                               .SetTip(s_duplicateVerticesDesc)
                                               .SetCallback(
                                                   [this]()
                                                   {
                                                       DuplicateSelected();
                                                   })
                                               .SetEntityComponentIdPair(entityComponentIdPair),
                                           deleteAction, deselectAction };
    }

    template<typename Vertex>
    void InsertVertexAfter(const AZ::EntityComponentIdPair& entityComponentIdPair, const size_t vertexIndex, const Vertex& localPosition)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        size_t size = 0;
        AZ::VariableVerticesRequestBus<Vertex>::EventResult(
            size, entityComponentIdPair.GetEntityId(), &AZ::VariableVerticesRequestBus<Vertex>::Handler::Size);

        const size_t insertPosition = vertexIndex + 1;
        if (insertPosition >= size)
        {
            AZ::VariableVerticesRequestBus<Vertex>::Event(
                entityComponentIdPair.GetEntityId(), &AZ::VariableVerticesRequestBus<Vertex>::Handler::AddVertex, localPosition);
        }
        else
        {
            bool updated = false;
            AZ::VariableVerticesRequestBus<Vertex>::EventResult(
                updated, entityComponentIdPair.GetEntityId(), &AZ::VariableVerticesRequestBus<Vertex>::Handler::InsertVertex,
                insertPosition, localPosition);
        }

        RefreshUiAfterAddRemove(entityComponentIdPair);
    }

    template<typename Vertex>
    void SafeRemoveVertex(const AZ::EntityComponentIdPair& entityComponentIdPair, const size_t vertexIndex)
    {
        bool removed = false;
        AZ::VariableVerticesRequestBus<Vertex>::EventResult(
            removed, entityComponentIdPair.GetEntityId(), &AZ::VariableVerticesRequestBus<Vertex>::Handler::RemoveVertex, vertexIndex);

        RefreshUiAfterAddRemove(entityComponentIdPair);
    }

    // explicit instantiations
    template void InsertVertexAfter(const AZ::EntityComponentIdPair& entityComponentIdPair, size_t, const AZ::Vector2&);
    template void InsertVertexAfter(const AZ::EntityComponentIdPair& entityComponentIdPair, size_t, const AZ::Vector3&);
    template void SafeRemoveVertex<AZ::Vector2>(const AZ::EntityComponentIdPair& entityComponentIdPair, size_t vertexIndex);
    template void SafeRemoveVertex<AZ::Vector3>(const AZ::EntityComponentIdPair& entityComponentIdPair, size_t vertexIndex);

    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(EditorVertexSelectionFixed<AZ::Vector2>, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(EditorVertexSelectionFixed<AZ::Vector3>, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(EditorVertexSelectionVariable<AZ::Vector2>, AZ::SystemAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL_TEMPLATE(EditorVertexSelectionVariable<AZ::Vector3>, AZ::SystemAllocator, 0)

    template class EditorVertexSelectionBase<AZ::Vector2>;
    template class EditorVertexSelectionBase<AZ::Vector3>;
    template class EditorVertexSelectionFixed<AZ::Vector2>;
    template class EditorVertexSelectionFixed<AZ::Vector3>;
    template class EditorVertexSelectionVariable<AZ::Vector2>;
    template class EditorVertexSelectionVariable<AZ::Vector3>;

} // namespace AzToolsFramework
