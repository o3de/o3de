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

local DropTargetLayoutDraggableChild = 
{
    Properties = 
    {
    },
}

function DropTargetLayoutDraggableChild:OnActivate()
    self.dropTargetHandler = UiDropTargetNotificationBus.Connect(self, self.entityId)
end

function DropTargetLayoutDraggableChild:OnDeactivate()
    self.dropTargetHandler:Disconnect()
end

function DropTargetLayoutDraggableChild:OnDropHoverStart(draggable)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Valid)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Valid)
end

function DropTargetLayoutDraggableChild:OnDropHoverEnd(draggable)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Normal)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Normal)
end

function DropTargetLayoutDraggableChild:OnDrop(draggable)
    local parentDraggable = UiElementBus.Event.GetParent(self.entityId)
    local parentLayout = UiElementBus.Event.GetParent(parentDraggable)
    UiElementBus.Event.Reparent(draggable, parentLayout, parentDraggable)
end

return DropTargetLayoutDraggableChild
