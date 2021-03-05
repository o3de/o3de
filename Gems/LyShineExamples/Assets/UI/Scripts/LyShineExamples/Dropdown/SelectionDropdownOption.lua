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

local SelectionDropdownOption = 
{
    Properties = 
    {
        DropdownContent = {default = EntityId()},
        OptionsDisplay = {default = EntityId()},
    },
}

function SelectionDropdownOption:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
end

function SelectionDropdownOption:OnButtonClick()
    -- Add a new child to the selected options list
    local numChildren = UiElementBus.Event.GetNumChildElements(self.Properties.OptionsDisplay)
    UiDynamicLayoutBus.Event.SetNumChildElements(self.Properties.OptionsDisplay, numChildren + 1)
    
    -- Get the text that corresponds to our option
    local textChild = UiElementBus.Event.FindChildByName(self.entityId, "Text")
    local text = UiTextBus.Event.GetText(textChild)
    
    -- Get the child that was last added (index = numChildren)
    local newChild = UiElementBus.Event.GetChild(self.Properties.OptionsDisplay, numChildren)
    UiTextBus.Event.SetText(newChild, text)
    
    -- Remove this option from the dropdown
    UiElementBus.Event.DestroyElement(self.entityId)
end

function SelectionDropdownOption:OnDeactivate()
    self.buttonHandler:Disconnect()
end

return SelectionDropdownOption