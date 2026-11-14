/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Util/WhiteBoxEditorUtil.h"
#include "EditorWhiteBoxComponentModeBus.h"
#include "EditorWhiteBoxPolygonModifierBus.h"
#include "SubComponentModes/EditorWhiteBoxDefaultModeBus.h"
#include "Util/WhiteBoxMeshUtil.h"
#include "Util/WhiteBoxSnapUtil.h"
#include "Viewport/WhiteBoxManipulatorViews.h"
#include "Viewport/WhiteBoxModifierUtil.h"
#include "Viewport/WhiteBoxViewportConstants.h"
#include "WhiteBoxPolygonTranslationModifier.h"

#include <AzCore/std/optional.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    
    AZ_CLASS_ALLOCATOR_IMPL(PolygonTranslationModifier, AZ::SystemAllocator)

    PolygonTranslationModifier::PolygonTranslationModifier(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const Api::PolygonHandle& polygonHandle,
        [[maybe_unused]] const AZ::Vector3& intersectionPoint)
        : m_entityComponentIdPair(entityComponentIdPair)
        , m_polygonHandle(polygonHandle)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        m_vertexHandles = Api::PolygonVertexHandles(*whiteBox, m_polygonHandle);

        CreateManipulator();
    }

    PolygonTranslationModifier::~PolygonTranslationModifier()
    {
        // Safety net - see the note in EdgeTranslationModifier's destructor.
        CancelDrag(false);
        DestroyManipulator();
    }

    void PolygonTranslationModifier::CreateManipulator()
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        m_translationManipulator = AzToolsFramework::LinearManipulator::MakeShared(
            EditorSpaceFromLocal(m_entityComponentIdPair));

        m_translationManipulator->AddEntityComponentIdPair(m_entityComponentIdPair);
        m_translationManipulator->SetLocalPosition(Api::PolygonMidpoint(*whiteBox, m_polygonHandle));
        m_translationManipulator->SetAxis(Api::PolygonNormal(*whiteBox, m_polygonHandle));

        CreateView();

        m_translationManipulator->Register(AzToolsFramework::GetMainManipulatorManagerId());

        struct SharedState
        {
            AZStd::vector<AZ::Vector3> m_vertexPositions;
            // what state of appending are we currently in
            AppendStage m_appendStage = AppendStage::None;
            // the position of the manipulator the moment an append is initiated
            AZ::Vector3 m_initiateAppendPosition = AZ::Vector3::CreateZero();
            // the distance the manipulator has moved from where it started when an append begins
            AZ::Vector3 m_activeAppendOffset = AZ::Vector3::CreateZero();
            // the midpoint of the edge manipulator
            AZ::Vector3 m_polygonMidpoint = AZ::Vector3::CreateZero();
            // has the modifier moved during the action
            bool m_moved = false;
            // vertex snapping: which of the polygon's vertices leads the drag. Resolved once at
            // mouse down (nearest to the cursor) and held, so the anchor cannot flip mid-drag.
            AZStd::optional<size_t> m_snapAnchorIndex;
        };

        auto sharedState = AZStd::make_shared<SharedState>();

        m_translationManipulator->InstallLeftMouseDownCallback(
            [this, sharedState](const AzToolsFramework::LinearManipulator::Action& /*action*/)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

                sharedState->m_appendStage = AppendStage::None;
                sharedState->m_activeAppendOffset = AZ::Vector3::CreateZero();
                sharedState->m_vertexPositions = Api::VertexPositions(*whiteBox, m_vertexHandles);
                sharedState->m_polygonMidpoint = Api::PolygonMidpoint(*whiteBox, m_polygonHandle);
                sharedState->m_moved = false;
                m_dragSnapshot = Api::CloneMesh(*whiteBox);
                m_dragPolygonHandleSnapshot = m_polygonHandle;
                m_dragVertexHandlesSnapshot = m_vertexHandles;
                m_dragCancelled = false;

                // vertex snapping: pick the anchor now so it stays fixed for the whole drag
                sharedState->m_snapAnchorIndex = SnapUtil::SnappingActive()
                    ? SnapUtil::FindAnchorIndex(
                          m_entityComponentIdPair.GetEntityId(), SnapUtil::ActiveViewportId(), *whiteBox, m_vertexHandles)
                    : AZStd::nullopt;
                SnapUtil::ClearActiveSnapTarget();
            });

        m_translationManipulator->InstallMouseMoveCallback(
            [this, sharedState](const AzToolsFramework::LinearManipulator::Action& action) mutable
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

                // Extrude with Ctrl held or the Sketch cluster's latched Extrude.
                bool stickyExtrude = false;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    stickyExtrude, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetStickyExtrude);
                const bool extruding = action.m_modifiers.Ctrl() || stickyExtrude;

                // reset append
                if (!extruding && sharedState->m_appendStage != AppendStage::None)
                {
                    sharedState->m_appendStage = AppendStage::None;
                }

                // start trying to extrude
                if (extruding && sharedState->m_appendStage == AppendStage::None)
                {
                    sharedState->m_appendStage = AppendStage::Initiated;
                    sharedState->m_initiateAppendPosition = action.LocalPosition();
                }

                if (sharedState->m_appendStage == AppendStage::Initiated)
                {
                    const AZ::Vector3 extrudeVector = action.LocalPosition() - sharedState->m_initiateAppendPosition;
                    const float extrudeMagnitude = extrudeVector.Dot(action.m_fixed.m_axis);
                    // only extrude after having moved a small amount (to prevent overlapping verts
                    // and normals being calculated incorrectly)
                    if (fabsf(extrudeMagnitude) > 0.0f)
                    {
                        // extrude the new side
                        const auto appendedPolygonHandles =
                            Api::TranslatePolygonAppendAdvanced(*whiteBox, m_polygonHandle, extrudeMagnitude);

                        // update our shared state to hold the new values after extrusion
                        m_vertexHandles =
                            Api::PolygonVertexHandles(*whiteBox, appendedPolygonHandles.m_appendedPolygonHandle);
                        sharedState->m_appendStage = AppendStage::Complete;

                        // remember the current offset when we start extruding (to stop any snapping)
                        sharedState->m_activeAppendOffset = action.LocalPositionOffset();
                        sharedState->m_polygonMidpoint =
                            Api::PolygonMidpoint(*whiteBox, appendedPolygonHandles.m_appendedPolygonHandle);

                        // make sure all vertex positions are refreshed and match the correct handle
                        for (size_t vertexIndex = 0; vertexIndex < m_vertexHandles.size(); ++vertexIndex)
                        {
                            const Api::VertexHandle vertexHandle = m_vertexHandles[vertexIndex];
                            sharedState->m_vertexPositions[vertexIndex] = Api::VertexPosition(*whiteBox, vertexHandle);
                        }

                        // notify primary polygon modifier has changed
                        EditorWhiteBoxPolygonModifierNotificationBus::Event(
                            m_entityComponentIdPair,
                            &EditorWhiteBoxPolygonModifierNotificationBus::Events::
                                OnPolygonModifierUpdatedPolygonHandle,
                            m_polygonHandle, appendedPolygonHandles.m_appendedPolygonHandle);

                        // notify all other restored polygon handle pairs (that may have been removed and added)
                        for (const auto& restoredPolygonHandlePair : appendedPolygonHandles.m_restoredPolygonHandles)
                        {
                            EditorWhiteBoxPolygonModifierNotificationBus::Event(
                                m_entityComponentIdPair,
                                &EditorWhiteBoxPolygonModifierNotificationBus::Events::
                                    OnPolygonModifierUpdatedPolygonHandle,
                                restoredPolygonHandlePair.m_before, restoredPolygonHandlePair.m_after);
                        }

                        m_polygonHandle = appendedPolygonHandles.m_appendedPolygonHandle;
                    }
                }

                // regular movement/translation of vertices
                if (sharedState->m_appendStage == AppendStage::None ||
                    sharedState->m_appendStage == AppendStage::Complete)
                {
                    const AZ::Vector3 dragOffset = action.LocalPositionOffset() - sharedState->m_activeAppendOffset;

                    // Snap the polygon so its anchor lands on the target; recomputed each frame, not cumulative.
                    AZStd::optional<AZ::Vector3> snapTarget;
                    AZ::Vector3 snapOffset = AZ::Vector3::CreateZero();

                    // Suppressed once an extrude has happened: the extrude constrains movement to
                    // the polygon normal and snapping would yank it off that axis.
                    if (sharedState->m_appendStage == AppendStage::None &&
                        sharedState->m_snapAnchorIndex.has_value() &&
                        sharedState->m_snapAnchorIndex.value() < sharedState->m_vertexPositions.size())
                    {
                        snapTarget = SnapUtil::FindSnapTargetWorld(
                            m_entityComponentIdPair.GetEntityId(), SnapUtil::ActiveViewportId(),
                            SnapUtil::ExcludeIndicesFromHandles(m_vertexHandles));

                        if (snapTarget.has_value())
                        {
                            const AZ::Vector3 anchorUnsnapped =
                                sharedState->m_vertexPositions[sharedState->m_snapAnchorIndex.value()] + dragOffset;

                            snapOffset =
                                SnapUtil::MeshLocalFromWorld(
                                    m_entityComponentIdPair.GetEntityId(), snapTarget.value()) -
                                anchorUnsnapped;
                        }
                    }

                    SnapUtil::SetActiveSnapTarget(snapTarget);

                    size_t vertexIndex = 0;
                    for (const Api::VertexHandle& vertexHandle : m_vertexHandles)
                    {
                        const AZ::Vector3 vertexPosition =
                            sharedState->m_vertexPositions[vertexIndex++] + dragOffset + snapOffset;

                        Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
                    }

                    m_translationManipulator->SetLocalPosition(
                        sharedState->m_polygonMidpoint + dragOffset + snapOffset);

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

                // inefficient but easy/effective - just rebuild the whole mesh after every change
                EditorWhiteBoxComponentNotificationBus::Event(
                    m_entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
            });

        m_translationManipulator->InstallLeftMouseUpCallback(
            [entityComponentIdPair = m_entityComponentIdPair, sharedState,
             this]([[maybe_unused]] const AzToolsFramework::LinearManipulator::Action& action)
            {
                if (m_dragCancelled)
                {
                    // reverted by Escape - nothing to commit
                    m_dragCancelled = false;
                    m_dragSnapshot = nullptr;
                    sharedState->m_snapAnchorIndex.reset();
                    SnapUtil::ClearActiveSnapTarget();
                    return;
                }

                // we haven't moved, count as a click
                if (!sharedState->m_moved)
                {
                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        entityComponentIdPair, &EditorWhiteBoxDefaultModeRequestBus::Events::CreatePolygonScaleModifier,
                        m_polygonHandle);

                    EditorWhiteBoxDefaultModeRequestBus::Event(
                        entityComponentIdPair,
                        &EditorWhiteBoxDefaultModeRequestBus::Events::AssignSelectedPolygonTranslationModifier);
                }
                else
                {
                    EditorWhiteBoxComponentRequestBus::Event(
                        entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
                }

                m_dragSnapshot = nullptr;
                sharedState->m_snapAnchorIndex.reset();
                SnapUtil::ClearActiveSnapTarget();
            });
    }

    void PolygonTranslationModifier::DestroyManipulator()
    {
        m_translationManipulator->Unregister();
        m_translationManipulator.reset();
    }

    void PolygonTranslationModifier::ForwardMouseOverEvent(
        const AzToolsFramework::ViewportInteraction::MouseInteraction& interaction)
    {
        m_translationManipulator->ForwardMouseOverEvent(interaction);
    }

    bool PolygonTranslationModifier::MouseOver() const
    {
        return m_translationManipulator->MouseOver();
    }

    void PolygonTranslationModifier::Refresh()
    {
        DestroyManipulator();
        CreateManipulator();
    }

    void PolygonTranslationModifier::SetPolygonHandle(const Api::PolygonHandle& polygonHandle)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        m_polygonHandle = polygonHandle;

        // ensure we update our cached values
        m_vertexHandles = Api::PolygonVertexHandles(*whiteBox, polygonHandle);
    }

    void PolygonTranslationModifier::CreateView()
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        Api::VertexPositionsCollection outlines = Api::PolygonBorderVertexPositions(*whiteBox, m_polygonHandle);
        AZStd::vector<AZ::Vector3> triangles = Api::PolygonFacesPositions(*whiteBox, m_polygonHandle);        
        const AZ::Vector3 polygonMidpoint = Api::PolygonMidpoint(*whiteBox, m_polygonHandle);

        // translate points into local space of the manipulator (see UpdateIntersectionPoint)
        // (relative to m_translationManipulator local position)
        for (auto& outline : outlines)
        {
            TranslatePoints(outline, -polygonMidpoint);
        }
        
        const AZ::Vector3 normal = Api::PolygonNormal(*whiteBox, m_polygonHandle);
        TranslatePoints(triangles, -polygonMidpoint);

        if (!m_polygonView)
        {
            m_polygonView = CreateManipulatorViewPolygon(triangles, outlines);
        }
        else
        {
            m_polygonView->m_outlines = outlines;
            m_polygonView->m_triangles = triangles;
        }

        m_polygonView->m_polygonViewOverlapOffset = AZ::Transform::CreateTranslation(normal * ed_whiteBoxPolygonViewOverlapOffset);
        m_polygonView->m_fillColor = m_fillColor;
        m_polygonView->m_outlineColor = m_outlineColor;

        m_translationManipulator->SetViews(AzToolsFramework::ManipulatorViews{m_polygonView});
    }

    bool PolygonTranslationModifier::PerformingAction() const
    {
        return m_translationManipulator->PerformingAction();
    }

    bool PolygonTranslationModifier::CancelDrag(const bool notify)
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
        m_polygonHandle = m_dragPolygonHandleSnapshot;
        m_vertexHandles = m_dragVertexHandlesSnapshot;

        // The manipulator holds its own interaction state until the mouse is released, so rather
        // than trying to abort it we latch a flag and ignore movement for the rest of the drag.
        m_dragCancelled = true;
        SnapUtil::ClearActiveSnapTarget();

        m_translationManipulator->SetLocalPosition(Api::PolygonMidpoint(*whiteBox, m_polygonHandle));

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
