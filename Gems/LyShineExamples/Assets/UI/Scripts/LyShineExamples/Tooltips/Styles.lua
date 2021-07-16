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

local Styles = 
{
    Properties = 
    {
        TooltipDisplays = { default = { EntityId(), EntityId(), EntityId(), EntityId() } },
        StyleButtons = { default = { EntityId(), EntityId(), EntityId(), EntityId() } },
    },
}

function Styles:OnActivate()
    self.buttonHandlers = {}
    for i = 0, #self.Properties.StyleButtons do
        self.buttonHandlers[i] = UiButtonNotificationBus.Connect(self, self.Properties.StyleButtons[i])
    end
        
    self.tickBusHandler = TickBus.Connect(self);
end

function Styles:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()
    
    self.canvas = UiElementBus.Event.GetCanvas(self.entityId)
    
    -- Initialize selection
    self:UpdateSelection(0)
end

function Styles:OnDeactivate()
    for i = 0, #self.Properties.StyleButtons do
        self.buttonHandlers[i]:Disconnect()
    end
end

function Styles:OnButtonClick()
    local styleIndex = self:GetButtonIndex(UiButtonNotificationBus.GetCurrentBusId())
    
    -- Update selection
    self:UpdateSelection(styleIndex)
    
    -- Change tooltip display element
    UiCanvasBus.Event.SetTooltipDisplayElement(self.canvas, self.Properties.TooltipDisplays[styleIndex])
    
    -- Typically on a button click, the tooltip hides and does not show until the
    -- button changes hover states. Force the tooltip of the newly pressed
    -- button to show so the new tooltip display can be seen right away
    local invalidEntityId = EntityId()
    UiCanvasBus.Event.ForceHoverInteractable(self.canvas, invalidEntityId)
    UiCanvasBus.Event.ForceHoverInteractable(self.canvas, UiButtonNotificationBus.GetCurrentBusId()) 
end

function Styles:GetButtonIndex(entityId)    
    for i = 0, #self.Properties.StyleButtons do
        if (self.Properties.StyleButtons[i] == entityId) then
            return i
        end
    end

    return 0
end

function Styles:UpdateSelection(selectedIndex)
    for i = 0, #self.Properties.StyleButtons do
        local selectedImage = UiElementBus.Event.FindChildByName(self.Properties.StyleButtons[i], "Selected")
        if (i == selectedIndex) then
            UiElementBus.Event.SetIsEnabled(selectedImage, true)
        else
            UiElementBus.Event.SetIsEnabled(selectedImage, false)
        end
    end    
end

return Styles
