/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxComponentModeCommon.h"
#include "EditorWhiteBoxEdgeRestoreMode.h"
#include "Viewport/WhiteBoxModifierUtil.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/sort.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    static const char* const FlipEdgeUndoRedoDesc = "Flip an edge to divide quad across different diagonal";
    static const char* const RestoreEdgeUndoRedoDesc = "Restore an edge to split two connected polygons";
    static const char* const RestoreVertexUndoRedoDesc = "Restore a vertex to split two connected edges";

    AZ_CLASS_ALLOCATOR_IMPL(EdgeRestoreMode, AZ::SystemAllocator)

    void EdgeRestoreMode::Refresh()
    {
        // noop
    }

    void EdgeRestoreMode::RegisterActionUpdaters()
    {
    }

    void EdgeRestoreMode::RegisterActions()
    {
    }

    void EdgeRestoreMode::BindActionsToModes(const AZStd::string& modeIdentifier)
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "WhiteBoxDefaultMode - could not get ActionManagerInterface on BindActionsToModes.");

        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.componentMode.end");
    }

    void EdgeRestoreMode::BindActionsToMenus()
    {
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EdgeRestoreMode::PopulateActions(
        [[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return {};
    }

    bool EdgeRestoreMode::HandleMouseInteraction(const ModeMouseInteraction& mouse)
    {
        const auto& mouseInteraction = mouse.m_mouseInteraction;
        const AZ::EntityComponentIdPair& entityComponentIdPair = mouse.m_entityComponentIdPair;
        const AZStd::optional<EdgeIntersection>& edgeIntersection = mouse.m_edgeIntersection;
        const AZStd::optional<PolygonIntersection>& polygonIntersection = mouse.m_polygonIntersection;
        const AZStd::optional<VertexIntersection>& vertexIntersection = mouse.m_vertexIntersection;

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        const auto closestIntersection =
            FindClosestGeometryIntersection(edgeIntersection, polygonIntersection, vertexIntersection);

        // clear for each event/update
        m_edgeIntersection.reset();
        m_vertexIntersection.reset();

        // update stored edge and vertex intersection
        switch (closestIntersection)
        {
        case GeometryIntersection::Edge:
            m_edgeIntersection = edgeIntersection;
            break;
        case GeometryIntersection::Vertex:
            m_vertexIntersection = vertexIntersection;
            break;
        default:
            // do nothing
            break;
        }

        if (InputRestore(mouseInteraction))
        {
            switch (closestIntersection)
            {
            // ensure we were actually hovering over an edge when clicking
            case GeometryIntersection::Edge:
                {
                    // attempt to restore an edge
                    // (an optional<> is returned potentially containing two split polygons if successful)
                    if (Api::RestoreEdge(
                            *whiteBox, edgeIntersection->m_closestEdgeWithHandle.m_handle, m_edgeHandlesBeingRestored))
                    {
                        RecordWhiteBoxAction(*whiteBox, entityComponentIdPair, RestoreEdgeUndoRedoDesc);
                        return true;
                    }
                }
                break;
            // ensure we were actually hovering over a vertex when clicking
            case GeometryIntersection::Vertex:
                {
                    // note: operation may fail if the vertex is isolated
                    if (Api::TryRestoreVertex(*whiteBox, vertexIntersection->m_closestVertexWithHandle.m_handle))
                    {
                        RecordWhiteBoxAction(*whiteBox, entityComponentIdPair, RestoreVertexUndoRedoDesc);
                    }

                    return true;
                }
                break;
            default:
                return false;
            }
        }

        if (InputFlipEdge(mouseInteraction))
        {
            switch (closestIntersection)
            {
            // ensure we were actually hovering over an edge when clicking
            case GeometryIntersection::Edge:
                {
                    // attempt to flip an edge
                    if (Api::FlipEdge(*whiteBox, edgeIntersection->m_closestEdgeWithHandle.m_handle))
                    {
                        RecordWhiteBoxAction(*whiteBox, entityComponentIdPair, FlipEdgeUndoRedoDesc);
                        return true;
                    }
                }
                break;
            default:
                return false;
            }
        }

        return false;
    }

    void EdgeRestoreMode::Display(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Transform& worldFromLocal,
        const IntersectionAndRenderData& renderData, const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        // clang-format off
        const Api::EdgeHandles interactiveEdgeHandles = m_edgeIntersection.has_value()
            ? Api::EdgeHandles{ m_edgeIntersection->m_closestEdgeWithHandle.m_handle }
            : Api::EdgeHandles{};
        // clang-format on

        debugDisplay.PushMatrix(worldFromLocal);

        // Depth test off so edges are always visible on top of the mesh surface
        // (with depth-test on, co-planar edges z-fight into invisibility).
        debugDisplay.DepthTestOff();

        // draw user (polygon border) edges in the standard dark colour
        DrawEdges(
            debugDisplay, ed_whiteBoxEdgeDefault, renderData.m_whiteBoxEdgeRenderData.m_bounds.m_user,
            interactiveEdgeHandles);
        // draw hidden/mesh edges in a bright orange so they stand out as "restorable"
        DrawEdges(
            debugDisplay, ed_whiteBoxEdgeUnselected, renderData.m_whiteBoxEdgeRenderData.m_bounds.m_mesh,
            interactiveEdgeHandles);

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // special handling for edges in the process of being restored - an edge may be clicked
        // and remain 'orphaned' from a polygon until another connection (loop) can be made.
        for (const Api::EdgeHandle& edgeHandleRestore : m_edgeHandlesBeingRestored)
        {
            if (AZStd::any_of(
                    interactiveEdgeHandles.begin(), interactiveEdgeHandles.end(),
                    [edgeHandleRestore](const Api::EdgeHandle edgeHandle)
                    {
                        return edgeHandle == edgeHandleRestore;
                    }))
            {
                continue;
            }

            debugDisplay.SetColor(ed_whiteBoxOutlineHover);
            const auto edgeGeom = Api::EdgeVertexPositions(*whiteBox, edgeHandleRestore);
            debugDisplay.DrawLine(edgeGeom[0], edgeGeom[1]);
        }

        // draw the hovered highlighted edge
        if (!interactiveEdgeHandles.empty() && m_edgeIntersection.has_value())
        {
            debugDisplay.SetColor(ed_whiteBoxOutlineSelection);
            debugDisplay.DrawLine(
                m_edgeIntersection->m_closestEdgeWithHandle.m_bound.m_start,
                m_edgeIntersection->m_closestEdgeWithHandle.m_bound.m_end);
        }

        debugDisplay.PopMatrix();
        debugDisplay.DepthTestOn(); // restore for vertex spheres

        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);

        const Api::VertexHandle hoveredVertexHandle =
            m_vertexIntersection.value_or(VertexIntersection{}).m_closestVertexWithHandle.m_handle;

        for (const auto& vertex : renderData.m_whiteBoxIntersectionData.m_vertexBounds)
        {
            if (hoveredVertexHandle == vertex.m_handle)
            {
                debugDisplay.SetColor(ed_whiteBoxVertexSelection);
            }
            else if (Api::VertexIsHidden(*whiteBox, vertex.m_handle))
            {
                debugDisplay.SetColor(ed_whiteBoxVertexHiddenColor);
            }
            else
            {
                debugDisplay.SetColor(ed_whiteBoxVertexRestoredColor);
            }

            // calculate the radius of the manipulator based
            // on the distance from the camera
            const float radius = cl_whiteBoxVertexManipulatorSize *
                AzToolsFramework::CalculateScreenToWorldMultiplier(
                                     worldFromLocal.TransformPoint(vertex.m_bound.m_center), cameraState);

            // note: we must manually transform position to world space to avoid the size
            // of the sphere being drawn incorrectly when scale is applied
            debugDisplay.DrawBall(worldFromLocal.TransformPoint(vertex.m_bound.m_center), radius);
        }
    }

} // namespace WhiteBox
