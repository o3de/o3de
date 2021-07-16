----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local ChildDropTargets_LayoutDropTarget = 
{
    Properties = 
    {
        EndDropTarget = {default = EntityId()},
    },
}

function ChildDropTargets_LayoutDropTarget:OnActivate()
    self.dropTargetHandler = UiDropTargetNotificationBus.Connect(self, self.entityId)
end

function ChildDropTargets_LayoutDropTarget:OnDeactivate()
    self.dropTargetHandler:Disconnect()
end

function ChildDropTargets_LayoutDropTarget:OnDropHoverStart(draggable)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Valid)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Valid)
end

function ChildDropTargets_LayoutDropTarget:OnDropHoverEnd(draggable)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Normal)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Normal)
end

function ChildDropTargets_LayoutDropTarget:OnDrop(draggable)
    -- insert before the invisible end drop target
    UiElementBus.Event.Reparent(draggable, self.entityId, self.Properties.EndDropTarget)
end

return ChildDropTargets_LayoutDropTarget
