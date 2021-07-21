----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local MultiSelectionDropdown = 
{
    Properties = 
    {
        Content = {default = EntityId()},
        Options = {default = {""}},
    },
}

function MultiSelectionDropdown:OnActivate()
    self.tickHandler = TickBus.Connect(self)
end

function MultiSelectionDropdown:OnTick()
    self.tickHandler:Disconnect()
    
    local numChildren = table.getn(self.Properties.Options)
    UiDynamicLayoutBus.Event.SetNumChildElements(self.Properties.Content, numChildren)
    
    for i=0,numChildren-1 do
        -- Get child of the layout
        local child = UiElementBus.Event.GetChild(self.Properties.Content, i)
        
        -- Get the text element of the child and set its text
        local textElement = UiElementBus.Event.FindChildByName(child, "Text")
        UiTextBus.Event.SetText(textElement, self.Properties.Options[i+1])
    end
end

function MultiSelectionDropdown:OnDeactivate()
end

return MultiSelectionDropdown
