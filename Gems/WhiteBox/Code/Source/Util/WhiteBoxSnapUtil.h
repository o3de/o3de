/*
 * White Box - vertex snapping helpers shared by the draggable viewport tools. Every entry point
 * reports "no snap" when snapping is off.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace WhiteBox
{
    //! Vertex snapping helpers shared by the White Box viewport tools.
    namespace SnapUtil
    {
        //! Is Snap to Vertex on. Cheap - check it before building exclusion lists.
        bool SnappingActive();

        //!@{
        //! The viewport the mouse is interacting with, recorded by the component mode's mouse handler
        //! because most manipulator actions carry no viewport id. Invalid until the first mouse event.
        void SetActiveViewportId(int viewportId);
        int ActiveViewportId();
        //!@}

        //! Convert between world and mesh-local space, including any Non-Uniform Scale (applied the
        //! same way the snap vertex source publishes positions).
        AZ::Vector3 MeshLocalFromWorld(AZ::EntityId entityId, const AZ::Vector3& worldPosition);
        AZ::Vector3 MeshWorldFromLocal(AZ::EntityId entityId, const AZ::Vector3& localPosition);

        //! Index of the vertex nearest the cursor on screen: the anchor that lands on the snap target.
        //! Resolve it once at mouse-down. Nothing when the cursor is off the viewport or the list is empty.
        AZStd::optional<size_t> FindAnchorIndex(
            AZ::EntityId entityId, int viewportId, const AZStd::vector<AZ::Vector3>& localPositions);

        //! Convenience overload taking vertex handles and reading their positions from @p whiteBox.
        AZStd::optional<size_t> FindAnchorIndex(
            AZ::EntityId entityId, int viewportId, const WhiteBoxMesh& whiteBox, const Api::VertexHandles& vertexHandles);

        //! The snappable vertex under the cursor in world space, ignoring @p excludeSourceIndices (the dragged element).
        AZStd::optional<AZ::Vector3> FindSnapTargetWorld(
            AZ::EntityId entityId, int viewportId, const AZStd::vector<AZ::s64>& excludeSourceIndices);

        //! Build an exclusion list from the handles of the element being dragged.
        AZStd::vector<AZ::s64> ExcludeIndicesFromHandles(const Api::VertexHandles& vertexHandles);

        //! Translate @p vertexHandles so @p anchorLocal lands on @p targetWorld. Returns the local offset applied.
        AZ::Vector3 ApplyAnchoredSnap(
            WhiteBoxMesh& whiteBox,
            AZ::EntityId entityId,
            const Api::VertexHandles& vertexHandles,
            const AZ::Vector3& anchorLocal,
            const AZ::Vector3& targetWorld);

        //! Draw the highlight on the vertex currently being snapped to (world space, depth test off).
        void DrawSnapTarget(AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& worldPosition);

        //!@{
        //! The snap target of the drag in progress, drawn once per frame by the component mode.
        //! Editor-only global state (one drag at a time); tools must Clear it on mouse up.
        void SetActiveSnapTarget(const AZStd::optional<AZ::Vector3>& worldPosition);
        void ClearActiveSnapTarget();
        void DrawActiveSnapTarget(AzFramework::DebugDisplayRequests& debugDisplay);
        //!@}
    } // namespace SnapUtil
} // namespace WhiteBox
