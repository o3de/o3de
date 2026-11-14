/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxEditorUtil.h"
#include "EditorWhiteBoxComponentModeBus.h"
#include "EditorWhiteBoxEdgeModifierBus.h"
#include "SubComponentModes/EditorWhiteBoxDefaultModeBus.h"
#include "Util/WhiteBoxMathUtil.h"
#include "Util/WhiteBoxMeshUtil.h"
#include "Util/WhiteBoxSnapUtil.h"
#include "Viewport/WhiteBoxModifierUtil.h"
#include "Viewport/WhiteBoxViewportConstants.h"
#include "WhiteBoxEdgeTranslationModifier.h"
#include "WhiteBoxManipulatorViews.h"
#include <AzCore/std/optional.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/numeric.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(EdgeTranslationModifier, AZ::SystemAllocator)

    //! Edges extrude on the same terms as polygons: Ctrl held, or the Sketch cluster's latched Extrude.
    static bool ExtrudeRequested(const AzToolsFramework::PlanarManipulator::Action& action, const bool sticky)
    {
        return action.m_modifiers.Ctrl() || sticky;
    }

    static bool BeginningExtrude(
        const AzToolsFramework::PlanarManipulator::Action& action, const AppendStage appendStage, const bool sticky)
    {
        return ExtrudeRequested(action, sticky) && appendStage == AppendStage::None;
    }

    static bool EndingExtrude(
        const AzToolsFramework::PlanarManipulator::Action& action, const AppendStage appendStage, const bool sticky)
    {
        return !ExtrudeRequested(action, sticky) && appendStage != AppendStage::None;
    }

    static bool AppendInactive(const AppendStage appendStage)
    {
        return appendStage == AppendStage::None || appendStage == AppendStage::Complete;
    }

    AZStd::array<AZ::Vector3, 2> GetEdgeNormalAxes(const AZ::Vector3& start, const AZ::Vector3& end)
    {
        AZ::Vector3 axis1, axis2;
        CalculateOrthonormalBasis((start - end).GetNormalized(), axis1, axis2);
        return {axis1, axis2};
    }

    EdgeTranslationModifier::EdgeTranslationModifier(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const Api::EdgeHandle edgeHandle,
        [[maybe_unused]] const AZ::Vector3& intersectionPoint)
        : m_hoveredEdgeHandle{edgeHandle}
        , m_entityComponentIdPair(entityComponentIdPair)
    {
        CreateManipulator();
    }

    EdgeTranslationModifier::~EdgeTranslationModifier()
    {
        // Safety net: being destroyed mid-drag (leaving component mode, switching sub-mode,
        // selection change) would otherwise leave the mesh wherever the drag had got to.
        CancelDrag(false);
        DestroyManipulator();
    }

    // return all vertex handles from a collection of edge handles
    // (ensure to remove duplicates as vertices will be shared across edges)
    static Api::VertexHandles VertexHandlesForEdges(const WhiteBoxMesh& whiteBox, const Api::EdgeHandles& edgeHandles)
    {
        Api::VertexHandles vertexHandles = AZStd::accumulate(
            edgeHandles.cbegin(), edgeHandles.cend(), Api::VertexHandles{},
            [&whiteBox](Api::VertexHandles vertexHandles, const Api::EdgeHandle edgeHandle) 
            {
                const auto edgeVertexHandles = Api::EdgeVertexHandles(whiteBox, edgeHandle);
                vertexHandles.insert(vertexHandles.end(), edgeVertexHandles.begin(), edgeVertexHandles.end());
                return vertexHandles;
            });
            
        AZStd::sort(vertexHandles.begin(), vertexHandles.end());
        vertexHandles.erase(AZStd::unique(vertexHandles.begin(), vertexHandles.end()), vertexHandles.end());
        return vertexHandles;
    }

    // note: edgeHandles is an inout param and will be modified if its size is 1
    static Api::EdgeHandle AttemptEdgeAppend(
        WhiteBoxMesh& whiteBox, Api::EdgeHandle hoveredEdgeHandle, Api::EdgeHandles& edgeHandles,
        const AZ::Vector3& extrudeVector)
    {
        // only allow edge extrusion with a single edge
        if (edgeHandles.size() == 1)
        {
            edgeHandles = {Api::TranslateEdgeAppend(whiteBox, hoveredEdgeHandle, extrudeVector)};
            return edgeHandles.front();
        }

        // no append occurred, return original edge handle
        return hoveredEdgeHandle;
    }

    void EdgeTranslationModifier::CreateManipulator()
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // calculate edge handle group (will be > 1 if connecting vertices have been hidden)
        m_edgeHandles = Api::EdgeGrouping(*whiteBox, m_hoveredEdgeHandle);

        const auto vertexPositions = Api::EdgeVertexPositions(*whiteBox, m_hoveredEdgeHandle);
        const auto axes = GetEdgeNormalAxes(vertexPositions[0], vertexPositions[1]);

        m_translationManipulator = AzToolsFramework::PlanarManipulator::MakeShared(
            EditorSpaceFromLocal(m_entityComponentIdPair));

        m_translationManipulator->AddEntityComponentIdPair(m_entityComponentIdPair);
        m_translationManipulator->SetLocalPosition(Api::EdgeMidpoint(*whiteBox, m_hoveredEdgeHandle));
        m_translationManipulator->SetAxes(axes[0], axes[1]);

        CreateView();

        m_translationManipulator->Register(AzToolsFramework::GetMainManipulatorManagerId());

        struct SharedState
        {
            // the previous position when moving the manipulator, used to calculate manipulator delta position
            AZ::Vector3 m_prevPosition;
            // the midpoint of the edge manipulator
            AZ::Vector3 m_edgeMidpoint = AZ::Vector3::CreateZero();
            // the position of the manipulator the moment an append is initiated
            AZ::Vector3 m_initiateAppendPosition = AZ::Vector3::CreateZero();
            // the distance the manipulator has moved from where it started when an append begins
            AZ::Vector3 m_activeAppendOffset = AZ::Vector3::CreateZero();
            // what state of appending are we currently in
            AppendStage m_appendStage = AppendStage::None;
            // has the modifier moved during the action
            bool m_moved = false;
            // vertex snapping: which of the edge group's vertices leads the drag. Resolved once at
            // mouse down (nearest to the cursor) and held, so the anchor cannot flip mid-drag.
            AZStd::optional<size_t> m_snapAnchorIndex;
            // Sum of snap corrections, to keep the absolute gizmo with the incrementally moved geometry.
            AZ::Vector3 m_snapAccumulatedOffset = AZ::Vector3::CreateZero();
        };

        auto sharedState = AZStd::make_shared<SharedState>();

        m_translationManipulator->InstallLeftMouseDownCallback(
            [this, sharedState](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                // record initial state at mouse down
                sharedState->m_prevPosition = action.LocalPosition();
                sharedState->m_edgeMidpoint = Api::EdgeMidpoint(*whiteBox, m_hoveredEdgeHandle);
                sharedState->m_appendStage = AppendStage::None;
                sharedState->m_moved = false;
                m_dragSnapshot = Api::CloneMesh(*whiteBox);
                m_dragEdgeHandlesSnapshot = m_edgeHandles;
                m_dragHoveredEdgeHandleSnapshot = m_hoveredEdgeHandle;
                m_dragCancelled = false;

                // vertex snapping: pick the anchor now so it stays fixed for the whole drag
                sharedState->m_snapAnchorIndex = SnapUtil::SnappingActive()
                    ? SnapUtil::FindAnchorIndex(
                          m_entityComponentIdPair.GetEntityId(), SnapUtil::ActiveViewportId(), *whiteBox,
                          VertexHandlesForEdges(*whiteBox, m_edgeHandles))
                    : AZStd::nullopt;
                SnapUtil::ClearActiveSnapTarget();
            });

        m_translationManipulator->InstallMouseMoveCallback(
            [this, sharedState](const AzToolsFramework::PlanarManipulator::Action& action)
            {
                // the drag was abandoned with Escape - ignore movement until the button is released
                if (m_dragCancelled)
                {
                    return;
                }

                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                // has the modifier moved during this interaction
                sharedState->m_moved = sharedState->m_moved ||
                    action.LocalPositionOffset().GetLength() >= cl_whiteBoxMouseClickDeltaThreshold;

                bool stickyExtrude = false;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    stickyExtrude, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetStickyExtrude);

                // reset append
                if (EndingExtrude(action, sharedState->m_appendStage, stickyExtrude))
                {
                    sharedState->m_appendStage = AppendStage::None;
                }

                // start trying to extrude
                if (BeginningExtrude(action, sharedState->m_appendStage, stickyExtrude))
                {
                    sharedState->m_appendStage = AppendStage::Initiated;
                    sharedState->m_initiateAppendPosition = action.LocalPosition();
                }

                const AZ::Vector3 position = action.LocalPosition();
                if (sharedState->m_appendStage == AppendStage::Initiated)
                {
                    const AZ::Vector3 extrudeVector = action.LocalPosition() - sharedState->m_initiateAppendPosition;
                    float extrudeMagnitude = AZ::GetAbs(extrudeVector.Dot(action.m_fixed.m_axis1)) +
                        AZ::GetAbs(extrudeVector.Dot(action.m_fixed.m_axis2));

                    // only extrude after having moved a small amount (to prevent overlapping verts
                    // and normals being calculated incorrectly)
                    if (extrudeMagnitude > 0)
                    {
                        sharedState->m_activeAppendOffset = action.LocalPositionOffset();

                        const Api::EdgeHandle nextEdgeHandle =
                            AttemptEdgeAppend(*whiteBox, m_hoveredEdgeHandle, m_edgeHandles, extrudeVector);

                        sharedState->m_edgeMidpoint = Api::EdgeMidpoint(*whiteBox, nextEdgeHandle);
                        sharedState->m_appendStage = AppendStage::Complete;

                        EditorWhiteBoxEdgeModifierNotificationBus::Broadcast(
                            &EditorWhiteBoxEdgeModifierNotificationBus::Events::OnEdgeModifierUpdatedEdgeHandle,
                            m_hoveredEdgeHandle, nextEdgeHandle);

                        m_hoveredEdgeHandle = nextEdgeHandle;
                    }
                }
                else if (AppendInactive(sharedState->m_appendStage))
                {
                    // get the distance the manipulator has moved since the last mouse move
                    const AZ::Vector3 displacement = position - sharedState->m_prevPosition;

                    // have to make sure we don't move verts more than once
                    const Api::VertexHandles vertexHandles = VertexHandlesForEdges(*whiteBox, m_edgeHandles);
                    for (const auto& vertexHandle : vertexHandles)
                    {
                        SetVertexPosition(
                            *whiteBox, vertexHandle, VertexPosition(*whiteBox, vertexHandle) + displacement);
                    }

                    // Snap the edge group so its anchor lands on the target; not after an extrude.
                    AZStd::optional<AZ::Vector3> snapTarget;
                    if (sharedState->m_appendStage == AppendStage::None &&
                        sharedState->m_snapAnchorIndex.has_value() &&
                        sharedState->m_snapAnchorIndex.value() < vertexHandles.size())
                    {
                        snapTarget = SnapUtil::FindSnapTargetWorld(
                            m_entityComponentIdPair.GetEntityId(), SnapUtil::ActiveViewportId(),
                            SnapUtil::ExcludeIndicesFromHandles(vertexHandles));

                        if (snapTarget.has_value())
                        {
                            const AZ::Vector3 anchorLocal = Api::VertexPosition(
                                *whiteBox, vertexHandles[sharedState->m_snapAnchorIndex.value()]);

                            sharedState->m_snapAccumulatedOffset += SnapUtil::ApplyAnchoredSnap(
                                *whiteBox, m_entityComponentIdPair.GetEntityId(), vertexHandles, anchorLocal,
                                snapTarget.value());
                        }
                    }

                    SnapUtil::SetActiveSnapTarget(snapTarget);
                }

                sharedState->m_prevPosition = position;

                // regular movement/translation of vertices
                if (AppendInactive(sharedState->m_appendStage))
                {
                    m_translationManipulator->SetLocalPosition(
                        sharedState->m_edgeMidpoint + action.LocalPositionOffset() -
                        sharedState->m_activeAppendOffset + sharedState->m_snapAccumulatedOffset);

                    EditorWhiteBoxComponentModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshPolygonScaleModifier);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshEdgeScaleModifier);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshPolygonTranslationModifier);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshEdgeTranslationModifier);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::RefreshVertexSelectionModifier);
                }

                Api::CalculateNormals(*whiteBox);
                Api::CalculatePlanarUVs(*whiteBox);

                EditorWhiteBoxComponentNotificationBus::Event(
                    m_entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
            });

        m_translationManipulator->InstallLeftMouseUpCallback(
            [this, sharedState]([[maybe_unused]] const AzToolsFramework::PlanarManipulator::Action& action)
            {
                if (m_dragCancelled)
                {
                    // reverted by Escape - nothing to commit
                    m_dragCancelled = false;
                    m_dragSnapshot = nullptr;
                    sharedState->m_snapAnchorIndex.reset();
                    sharedState->m_snapAccumulatedOffset = AZ::Vector3::CreateZero();
                    SnapUtil::ClearActiveSnapTarget();
                    return;
                }

                // we haven't moved, count as a click
                if (!sharedState->m_moved)
                {
                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair, &EditorWhiteBoxDefaultModeRequestBus::Events::CreateEdgeScaleModifier,
                        m_hoveredEdgeHandle);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        m_entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::AssignSelectedEdgeTranslationModifier);
                }
                else
                {
                    EditorWhiteBoxComponentRequestBus::Event(
                        m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
                }

                m_dragSnapshot = nullptr;
                sharedState->m_snapAnchorIndex.reset();
                sharedState->m_snapAccumulatedOffset = AZ::Vector3::CreateZero();
                SnapUtil::ClearActiveSnapTarget();
            });
    }

    void EdgeTranslationModifier::DestroyManipulator()
    {
        m_translationManipulator->Unregister();
        m_translationManipulator.reset();
    }

    bool EdgeTranslationModifier::MouseOver() const
    {
        return m_translationManipulator->MouseOver();
    }

    void EdgeTranslationModifier::ForwardMouseOverEvent(
        const AzToolsFramework::ViewportInteraction::MouseInteraction& interaction)
    {
        m_translationManipulator->ForwardMouseOverEvent(interaction);
    }

    void EdgeTranslationModifier::Refresh()
    {
        DestroyManipulator();
        CreateManipulator();
    }

    void EdgeTranslationModifier::CreateView()
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        const AZ::Vector3 edgeMidpoint = Api::EdgeMidpoint(*whiteBox, m_hoveredEdgeHandle);

        // if the size of the edge handles and views has changed
        // we know we need to either add or remove views
        if (m_edgeViews.size() != m_edgeHandles.size())
        {
            m_edgeViews.resize(m_edgeHandles.size());

            AZStd::generate(
                AZStd::begin(m_edgeViews), AZStd::end(m_edgeViews),
                []
                {
                    return AZStd::make_shared<ManipulatorViewEdge>();
                });
        }

        for (size_t edgeIndex = 0; edgeIndex < m_edgeHandles.size(); ++edgeIndex)
        {
            auto view = m_edgeViews[edgeIndex];
            const auto edgeHandle = m_edgeHandles[edgeIndex];

            const auto vertexHandles = Api::EdgeVertexHandles(*whiteBox, edgeHandle);
            // vertex positions in the local space of the entity
            const auto vertexPositions = Api::EdgeVertexPositions(*whiteBox, edgeHandle);

            // transform edge start/end positions to be in manipulator space (see UpdateIntersectionPoint)
            // (relative to m_translationManipulator local position)
            view->m_start = vertexPositions[0] - edgeMidpoint;
            view->m_end = vertexPositions[1] - edgeMidpoint;
            // record if start/end handles are hidden to adjust dimensions of manipulator view
            view->m_vertexStartEndHidden[0] = Api::VertexIsHidden(*whiteBox, vertexHandles[0]);
            view->m_vertexStartEndHidden[1] = Api::VertexIsHidden(*whiteBox, vertexHandles[1]);

            // only do selection colors for 'selected/hovered' edge handle
            if (edgeHandle == m_hoveredEdgeHandle)
            {
                view->SetColor(m_color, m_hoverColor);
            }
            else
            {
                view->SetColor(ed_whiteBoxOutlineHover, ed_whiteBoxOutlineHover);
            }

            view->SetWidth(m_width, m_hoverWidth);
        }

        m_translationManipulator->SetViews(
            AzToolsFramework::ManipulatorViews{m_edgeViews.cbegin(), m_edgeViews.cend()});
    }

    void EdgeTranslationModifier::SetColors(const AZ::Color& color, const AZ::Color& hoverColor)
    {
        m_color = color;
        m_hoverColor = hoverColor;
    }

    void EdgeTranslationModifier::SetWidths(const float width, const float hoverWidth)
    {
        m_width = width;
        m_hoverWidth = hoverWidth;
    }

    bool EdgeTranslationModifier::PerformingAction() const
    {
        return m_translationManipulator->PerformingAction();
    }

    bool EdgeTranslationModifier::CancelDrag(const bool notify)
    {
        if (!PerformingAction() || m_dragCancelled || !m_dragSnapshot)
        {
            return false;
        }

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        if (whiteBox == nullptr)
        {
            return false;
        }

        RestoreMeshFromSnapshot(*whiteBox, *m_dragSnapshot);
        m_edgeHandles = m_dragEdgeHandlesSnapshot;
        m_hoveredEdgeHandle = m_dragHoveredEdgeHandleSnapshot;

        // The manipulator holds its own interaction state until the mouse is released, so rather
        // than trying to abort it we latch a flag and ignore movement for the rest of the drag.
        m_dragCancelled = true;
        SnapUtil::ClearActiveSnapTarget();

        m_translationManipulator->SetLocalPosition(Api::EdgeMidpoint(*whiteBox, m_hoveredEdgeHandle));

        if (notify)
        {
            EditorWhiteBoxComponentModeRequestBus::Event(
                m_entityComponentIdPair,
                &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);

            EditorWhiteBoxComponentNotificationBus::Event(
                m_entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
        }

        return true;
    }
} // namespace WhiteBox
