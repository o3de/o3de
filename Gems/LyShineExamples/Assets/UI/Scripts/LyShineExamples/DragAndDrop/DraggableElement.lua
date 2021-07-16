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

local DraggableElement = 
{
    Properties = 
    {
        DragParent = {default = EntityId()},
    },
}

function DraggableElement:OnActivate()
    self.draggableHandler = UiDraggableNotificationBus.Connect(self, self.entityId)
end

function DraggableElement:OnDeactivate()
    self.draggableHandler:Disconnect()
end

function DraggableElement:OnDragStart(position)
    self.originalPosition = UiTransformBus.Event.GetViewportPosition(self.entityId)
    self.originalParent = UiElementBus.Event.GetParent(self.entityId)
    UiElementBus.Event.Reparent(self.entityId, self.Properties.DragParent, EntityId())
end

function DraggableElement:OnDrag(position)
    UiTransformBus.Event.SetViewportPosition(self.entityId, position)
end

function DraggableElement:OnDragEnd(position)
    UiElementBus.Event.Reparent(self.entityId, self.originalParent, EntityId())
       UiTransformBus.Event.SetViewportPosition(self.entityId, self.originalPosition)
end

return DraggableElement
