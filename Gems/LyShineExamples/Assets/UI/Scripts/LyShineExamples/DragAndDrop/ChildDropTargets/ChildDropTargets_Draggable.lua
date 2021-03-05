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

local ChildDropTargets_Draggable = 
{
    Properties = 
    {
        DragParent = {default = EntityId()},
    },
}

function ChildDropTargets_Draggable:OnActivate()
    self.draggableHandler = UiDraggableNotificationBus.Connect(self, self.entityId)
end

function ChildDropTargets_Draggable:OnDeactivate()
    self.draggableHandler:Disconnect()
end

function ChildDropTargets_Draggable:OnDragStart(position)
    self.originalPosition = UiTransformBus.Event.GetCanvasPosition(self.entityId)
    self.originalParent = UiElementBus.Event.GetParent(self.entityId)
    local index = UiElementBus.Event.GetIndexOfChildByEntityId(self.originalParent, self.entityId)
    self.originalBefore = EntityId()
    if (UiElementBus.Event.GetNumChildElements(self.originalParent) > index+1) then
        self.originalBefore = UiElementBus.Event.GetChild(self.originalParent, index+1)
    end
    UiElementBus.Event.Reparent(self.entityId, self.Properties.DragParent, EntityId())

    -- after reparenting the other drop targets will have moved since we removed an element
    -- from the layout. So force an immediate recompute of their positions and redo the drag
    -- which will redo the search for drop targets at the given position. This is important
    -- when using keyboard/gamepad.
    local canvasEntityId = UiElementBus.Event.GetCanvas(self.entityId)
    UiCanvasBus.Event.RecomputeChangedLayouts(canvasEntityId)
       UiDraggableBus.Event.RedoDrag(self.entityId, position)

    UiTransformBus.Event.SetCanvasPosition(self.entityId, position)
end

function ChildDropTargets_Draggable:OnDrag(position)
    UiTransformBus.Event.SetCanvasPosition(self.entityId, position)
end

function ChildDropTargets_Draggable:OnDragEnd(position)
    UiElementBus.Event.Reparent(self.entityId, self.originalParent, self.originalBefore)
       UiTransformBus.Event.SetCanvasPosition(self.entityId, self.originalPosition)
end

return ChildDropTargets_Draggable