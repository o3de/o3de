----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local DraggableStackingElement = 
{
    Properties = 
    {
        DragParent = {default = EntityId()},
    },
}

function DraggableStackingElement:OnActivate()
    self.draggableHandler = UiDraggableNotificationBus.Connect(self, self.entityId)
end

function DraggableStackingElement:OnDeactivate()
    self.draggableHandler:Disconnect()
end

function DraggableStackingElement:OnDragStart(position)
    local draggableImage = UiElementBus.Event.GetChild(self.entityId, 0)
    self.draggableCounterBox = UiElementBus.Event.GetChild(draggableImage, 0)
    self.draggableCounterText = UiElementBus.Event.GetChild(self.draggableCounterBox, 0)
    local textString = UiTextBus.Event.GetText(self.draggableCounterText)
    local inventoryCount = tonumber(textString)

    self.originalPosition = UiTransformBus.Event.GetViewportPosition(self.entityId)
    self.originalParent = UiElementBus.Event.GetParent(self.entityId)

    if (inventoryCount > 1) then
        self.canvasEntity = UiElementBus.Event.GetCanvas(self.entityId)
        self.clonedElement = UiCanvasBus.Event.CloneElement(self.canvasEntity, self.entityId, self.originalParent, EntityId())
        inventoryCount = inventoryCount - 1

        local clonedImage = UiElementBus.Event.GetChild(self.clonedElement, 0)
        local clonedCounterBox = UiElementBus.Event.GetChild(clonedImage, 0)
        local clonedCounterText = UiElementBus.Event.GetChild(clonedCounterBox, 0)
        UiTextBus.Event.SetText(clonedCounterText, inventoryCount)
    else
        self.clonedElement = nil
    end
    
    -- hide the count 
    UiElementBus.Event.SetIsEnabled(self.draggableCounterBox, false)
    
    UiElementBus.Event.Reparent(self.entityId, self.Properties.DragParent, EntityId())
    UiTransformBus.Event.SetViewportPosition(self.entityId, position)
end

function DraggableStackingElement:OnDrag(position)
    UiTransformBus.Event.SetViewportPosition(self.entityId, position)
end

function DraggableStackingElement:OnDragEnd(position)
    if (self.clonedElement) then
        UiElementBus.Event.DestroyElement(self.clonedElement)
    end

    UiElementBus.Event.Reparent(self.entityId, self.originalParent, EntityId())
       UiTransformBus.Event.SetViewportPosition(self.entityId, self.originalPosition)

    -- show the count 
    UiElementBus.Event.SetIsEnabled(self.draggableCounterBox, true)

end

return DraggableStackingElement
