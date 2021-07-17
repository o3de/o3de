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

local DropTarget = 
{
    Properties = 
    {
    },
}

function DropTarget:OnActivate()
    self.dropTargetHandler = UiDropTargetNotificationBus.Connect(self, self.entityId)
end

function DropTarget:OnDeactivate()
    self.dropTargetHandler:Disconnect()
end

function DropTarget:OnDropHoverStart(draggable)
    if (UiElementBus.Event.GetNumChildElements(self.entityId) <= 0) then
        UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Valid)
        UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Valid)
    else
        UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Invalid)
        UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Invalid)
    end
end

function DropTarget:OnDropHoverEnd(draggable)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Normal)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Normal)
end

function DropTarget:OnDrop(draggable)
    if (UiElementBus.Event.GetNumChildElements(self.entityId) <= 0) then
        UiElementBus.Event.Reparent(draggable, self.entityId, EntityId())
    end
end

return DropTarget
