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

----------------------------------------------------------------------------------------------------
-- DropTargetCrossCanvas - script for a drop target element that can be dropped on from other
-- canvases. This script is designed to be used with DraggableCrossCanvasElement
----------------------------------------------------------------------------------------------------
local DropTargetCrossCanvas = 
{
    Properties = 
    {
    },
}

function DropTargetCrossCanvas:OnActivate()
    self.dropTargetHandler = UiDropTargetNotificationBus.Connect(self, self.entityId)
end

function DropTargetCrossCanvas:OnDeactivate()
    self.dropTargetHandler:Disconnect()
end

function DropTargetCrossCanvas:OnDropHoverStart(draggable)
    if (UiDraggableBus.Event.IsProxy(draggable)) then
        if (UiElementBus.Event.GetNumChildElements(self.entityId) <= 0) then
            UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Valid)
            UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Valid)
        else
            UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Invalid)
            UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Invalid)
        end
    end
end

function DropTargetCrossCanvas:OnDropHoverEnd(draggable)
    if (UiDraggableBus.Event.IsProxy(draggable)) then
        UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Normal)
        UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Normal)
    else
        UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Normal)
        UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Normal)
    end
end

function DropTargetCrossCanvas:OnDrop(draggable)
    if (not UiDraggableBus.Event.IsProxy(draggable)) then
        if (UiElementBus.Event.GetNumChildElements(self.entityId) <= 0) then
            local myCanvasEntity = UiElementBus.Event.GetCanvas(self.entityId)
            local draggableCanvasEntity = UiElementBus.Event.GetCanvas(draggable)
            if (myCanvasEntity == draggableCanvasEntity) then
                UiElementBus.Event.Reparent(draggable, self.entityId, EntityId())
            else
                -- clone the draggable and remove it from its original canvas
                UiCanvasBus.Event.CloneElement(myCanvasEntity, draggable, self.entityId, EntityId())
                UiElementBus.Event.DestroyElement(draggable)
            end
        end
    end
end

return DropTargetCrossCanvas
