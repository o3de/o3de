/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>

namespace AzFramework
{
    class DebugDisplayRequests;
}

namespace AzToolsFramework
{
    namespace ViewportSnapping
    {
        //! Applies geometry snapping to entity transform-gizmo drags.
        //!
        //! Owned by EditorDefaultSelection, which calls into it from its mouse handlers and its
        //! viewport draw. Kept out of that class rather than inlined into it because the two have
        //! nothing to do with each other: EditorDefaultSelection is about selection and component
        //! mode dispatch, and would gain a third unrelated responsibility along with the drag state
        //! to go with it.
        //!
        //! Two behaviours, both of which have to live at this level:
        //!
        //! 1. Right click abandons a drag in progress, broadcast on ViewportDragCancelRequestBus.
        //!    This has to happen above the manipulator manager, which swallows every mouse press
        //!    while a manipulator is interacting - see ViewportDragCancelBus.h.
        //! 2. While the transform gizmo is translating a selection with a snap mode active, the
        //!    selected entities are nudged so that an anchor point lands on the snap target under
        //!    the cursor.
        class AZTF_API EntitySnapDragHandler
        {
        public:
            //! Handle a mouse event seen before the manipulator manager gets it.
            //! @return True if the event was consumed and should not be forwarded.
            bool HandleMouseViewportInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

            //! Handle a manipulator mouse event, before it reaches the manipulator manager.
            //! @return True if the event was consumed and should not be forwarded.
            bool HandleMouseManipulatorInteractionBefore(
                const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

            //! Apply the snap correction.
            //!
            //! Must run *after* the manipulator manager has handled the event: the gizmo writes
            //! this frame's unsnapped positions during that call, and the correction is applied on
            //! top of them.
            void HandleMouseManipulatorInteractionAfter(const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

            //! Draw the marker on the position currently being snapped to.
            void DisplayViewportSelection(AzFramework::DebugDisplayRequests& debugDisplay) const;

        private:
            //! Right button just went down - ask any tool with a drag in flight to abandon it.
            //! @return True if a drag was cancelled, in which case the click is consumed.
            bool TryCancelDrag(const ViewportInteraction::MouseInteractionEvent& mouseInteraction);

            //! Forget the in-progress drag and drop the snap marker.
            void ResetEntityDrag();

            //! State for the entity drag in progress.
            //!
            //! Resolved on the first mouse move of a gizmo translate and held until the manipulator
            //! stops interacting, because re-resolving the anchor every frame would let it flip to
            //! a different vertex mid-drag as the cursor moves.
            struct EntityDragState
            {
                bool m_active = false;

                //! The entities to move. Descendants of other selected entities are filtered out:
                //! they follow their parent, and moving both would apply the offset twice.
                AZStd::vector<AZ::EntityId> m_entities;

                //! Every selected entity, so that none of them can be snapped to.
                AZStd::vector<AZ::EntityId> m_excludeEntities;

                //! Offset from the gizmo pivot to the point leading the drag, fixed for the drag.
                AZ::Vector3 m_anchorOffsetFromPivot = AZ::Vector3::CreateZero();
            };

            EntityDragState m_entityDrag;

            //! World position currently snapped to, for the viewport marker.
            AZStd::optional<AZ::Vector3> m_snapTargetWorld;
        };
    } // namespace ViewportSnapping
} // namespace AzToolsFramework
