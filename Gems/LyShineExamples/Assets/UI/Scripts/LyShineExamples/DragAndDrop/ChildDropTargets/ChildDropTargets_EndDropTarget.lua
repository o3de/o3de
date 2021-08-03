----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local ChildDropTargets_EndDropTarget = 
{
    Properties = 
    {
    },
}

function ChildDropTargets_EndDropTarget:OnActivate()
    self.dropTargetHandler = UiDropTargetNotificationBus.Connect(self, self.entityId)
end

function ChildDropTargets_EndDropTarget:OnDeactivate()
    self.dropTargetHandler:Disconnect()
end

function ChildDropTargets_EndDropTarget:OnDropHoverStart(draggable)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Valid)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Valid)
end

function ChildDropTargets_EndDropTarget:OnDropHoverEnd(draggable)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Normal)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Normal)
end

function ChildDropTargets_EndDropTarget:OnDrop(draggable)
    local parentLayout = UiElementBus.Event.GetParent(self.entityId)
    -- add before this end marker
    UiElementBus.Event.Reparent(draggable, parentLayout, self.entityId)
end

return ChildDropTargets_EndDropTarget
