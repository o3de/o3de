----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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