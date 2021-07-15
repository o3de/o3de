----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------
-- DraggableCrossCanvasElement - script for a draggable element that can be dropped on drop targets
-- on other canvases.
--
-- This makes use of some of the more advanced feature of the UiDraggableBus, such as the proxy
-- features.
--
-- Note that the element with this script on will get cloned to make a proxy draggable element.
-- So this same script runs on both the original and the proxy.
----------------------------------------------------------------------------------------------------
local DraggableCrossCanvasElement = 
{
    Properties = 
    {
    },
}

function DraggableCrossCanvasElement:OnActivate()
    self.draggableHandler = UiDraggableNotificationBus.Connect(self, self.entityId)
    self.dragCanvas = EntityId()
    self.clonedElement = EntityId()
    self.originalParent = EntityId()
end

function DraggableCrossCanvasElement:OnDeactivate()
    self.draggableHandler:Disconnect()
end

function DraggableCrossCanvasElement:OnDragStart(position)

    self.isProxy = UiDraggableBus.Event.IsProxy(self.entityId)
    
    -- Since we want to support dropping on a drop target on any canvas we set this flag
    -- we want both the original and the proxy to set this flag, we turn it off on the drag end
    UiDraggableBus.Event.SetCanDropOnAnyCanvas(self.entityId, true)
    
    if (not self.isProxy) then
        -- this is the original, we need to make a new canvas in front of all the other canvases
        -- to move the proxy on
        -- If no changes have been made to canvas draw orders then the new canvas will draw in
        -- front of everything else. Otherwise, we would have to set the draw order to a high
        -- number here.
        self.dragCanvas = UiCanvasManagerBus.Broadcast.CreateCanvas()
    
        if (self.dragCanvas:IsValid()) then
            -- clone the original draggable making it a child of the root element on the new canvas
            self.clonedElement = UiCanvasBus.Event.CloneElement(self.dragCanvas, self.entityId, EntityId(), EntityId())
    
            if (self.clonedElement:IsValid()) then
                -- set the new cloned draggable element to act as a proxy for the original element
                UiDraggableBus.Event.SetAsProxy(self.clonedElement, self.entityId, position)
        
                -- hide the original element by reparenting it and disabling it so that we can drop
                -- the proxy at the original location if we want to
                self.originalParent = UiElementBus.Event.GetParent(self.entityId)
                UiElementBus.Event.Reparent(self.entityId, EntityId(), EntityId())
                UiElementBus.Event.SetIsEnabled(self.entityId, false)
            end
        end
    else
        -- This is the proxy, it gets an OnDragStart after SetAsProxy is called, at that point we
        -- to move it to where the cursor is
        UiTransformBus.Event.SetViewportPosition(self.entityId, position)
    end
end

function DraggableCrossCanvasElement:OnDrag(position)
    if (self.isProxy) then
        -- we do not move the original during the drag - just the proxy
        UiTransformBus.Event.SetViewportPosition(self.entityId, position)
    end
end

function DraggableCrossCanvasElement:OnDragEnd(position)

    if (self.isProxy) then
        -- this is the proxy, it gets OnDragEnd before the original, calling
        -- ProxyDragEnd will result in OnDragEnd being called on the original
        UiDraggableBus.Event.ProxyDragEnd(self.entityId, position)
    else
        -- this is the original

        if (self.dragCanvas:IsValid()) then
            if (self.clonedElement:IsValid()) then
                -- unhide the original and put it back under its original parent
                UiElementBus.Event.SetIsEnabled(self.entityId, true)
                UiElementBus.Event.Reparent(self.entityId, self.originalParent, EntityId())
        
                -- clean up by destroying the proxy element and the temporary canvas
                UiElementBus.Event.DestroyElement(self.clonedElement)
            end

            UiCanvasManagerBus.Broadcast.UnloadCanvas(self.dragCanvas)
        end
    end

    -- turn off the "Drop on any canvas flag" just for good measure
    UiDraggableBus.Event.SetCanDropOnAnyCanvas(self.entityId, false)

end

return DraggableCrossCanvasElement
