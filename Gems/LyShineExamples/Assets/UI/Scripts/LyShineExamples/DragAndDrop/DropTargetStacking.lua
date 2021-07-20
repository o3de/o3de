----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local DropTargetStacking = 
{
    Properties = 
    {
    },
}

function DropTargetStacking:IsValidDrop(draggable, dropTarget)
    local sourceDraggableImage = UiElementBus.Event.GetChild(draggable, 0)
    local sourceType = UiImageBus.Event.GetSpritePathname(sourceDraggableImage)

    local destType = ""
    if (UiElementBus.Event.GetNumChildElements(dropTarget) > 0) then
        local destDraggable = UiElementBus.Event.GetChild(dropTarget, 0)
        local destDraggableImage = UiElementBus.Event.GetChild(destDraggable, 0)
        destType = UiImageBus.Event.GetSpritePathname(destDraggableImage)
    end

    if (destType == "" or sourceType == destType) then
        return true
    else
        return false
    end
end

function DropTargetStacking:OnActivate()
    self.dropTargetHandler = UiDropTargetNotificationBus.Connect(self, self.entityId)
end

function DropTargetStacking:OnDeactivate()
    self.dropTargetHandler:Disconnect()
end

function DropTargetStacking:OnDropHoverStart(draggable)
    if (self:IsValidDrop(draggable, self.entityId)) then
        UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Valid)
        UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Valid)
    else
        UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Invalid)
        UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Invalid)
    end
end

function DropTargetStacking:OnDropHoverEnd(draggable)
    UiDraggableBus.Event.SetDragState(draggable, eUiDragState_Normal)
    UiDropTargetBus.Event.SetDropState(self.entityId, eUiDropState_Normal)
end

function DropTargetStacking:OnDrop(draggable)
    if (not self:IsValidDrop(draggable, self.entityId)) then
        return
    end
    
    -- get the source inventory count
    local sourceInventoryCount = 0
    local draggableImage = UiElementBus.Event.GetChild(draggable, 0)
    local draggableCounterBox = UiElementBus.Event.GetChild(draggableImage, 0)
    local draggableCounterText = UiElementBus.Event.GetChild(draggableCounterBox, 0)
    local textString = UiTextBus.Event.GetText(draggableCounterText)
    sourceInventoryCount = tonumber(textString)

    -- get the dest inventory count
    local destInventoryCount = 0
    local destChildCount = UiElementBus.Event.GetNumChildElements(self.entityId)
    local destDraggable = EntityId()
    local destDraggableCounterText = EntityId()
    local destTextString = ""
    if (destChildCount > 0) then
        destDraggable = UiElementBus.Event.GetChild(self.entityId, 0)
        
        if (destDraggable == draggable) then
            -- draggable was dropped on its original (current) slot - do nothing
            return
        end
        local destDraggableImage = UiElementBus.Event.GetChild(destDraggable, 0)
        local destDraggableCounterBox = UiElementBus.Event.GetChild(destDraggableImage, 0)
        destDraggableCounterText = UiElementBus.Event.GetChild(destDraggableCounterBox, 0)
        destTextString = UiTextBus.Event.GetText(destDraggableCounterText)
        destInventoryCount = tonumber(destTextString)
    end
    
    if (destInventoryCount == 0 and sourceInventoryCount == 1) then
        UiElementBus.Event.Reparent(draggable, self.entityId, EntityId())
    else
        if (destInventoryCount == 0) then
            local canvasEntity = UiElementBus.Event.GetCanvas(self.entityId)
            local clonedElement = UiCanvasBus.Event.CloneElement(canvasEntity, draggable, self.entityId, EntityId())
            -- set the count on the cloned element to one
            local clonedImage = UiElementBus.Event.GetChild(clonedElement, 0)
            local clonedCounterBox = UiElementBus.Event.GetChild(clonedImage, 0)
            local clonedCounterText = UiElementBus.Event.GetChild(clonedCounterBox, 0)
            UiTextBus.Event.SetText(clonedCounterText, "1")
            -- if using keyboard/gamepad make the we want to make sure the hover moves
            UiCanvasBus.Event.ForceHoverInteractable(canvasEntity, clonedElement)
        else
            -- increment dest
            destInventoryCount = destInventoryCount + 1
            destTextString = destInventoryCount
            UiTextBus.Event.SetText(destDraggableCounterText, destTextString)
            -- if using keyboard/gamepad make the we want to make sure the hover moves
            local canvasEntity = UiElementBus.Event.GetCanvas(self.entityId)
            UiCanvasBus.Event.ForceHoverInteractable(canvasEntity, destDraggable)
        end
        if (sourceInventoryCount == 1) then
            UiElementBus.Event.DestroyElement(draggable)
        else
            -- decrement source
            sourceInventoryCount = sourceInventoryCount - 1
            textString = sourceInventoryCount
            UiTextBus.Event.SetText(draggableCounterText, textString)
        end
    end
end

return DropTargetStacking
