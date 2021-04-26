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

local SelectionDropdownSelectedOption = 
{
    Properties = 
    {
        DropdownContent = {default = EntityId()},
        OptionsDisplay = {default = EntityId()},
    },
}

function SelectionDropdownSelectedOption:OnActivate()
    local deleteButton = UiElementBus.Event.FindChildByName(self.entityId, "Delete")
    self.buttonHandler = UiButtonNotificationBus.Connect(self, deleteButton)
end

function SelectionDropdownSelectedOption:OnButtonClick()
    -- Add a new child to the dropdown content list
    local numChildren = UiElementBus.Event.GetNumChildElements(self.Properties.DropdownContent)
    UiDynamicLayoutBus.Event.SetNumChildElements(self.Properties.DropdownContent, numChildren + 1)
    
    -- Get the text that corresponds to our option
    local text = UiTextBus.Event.GetText(self.entityId)
    
    -- Get the child that was last added (index = numChildren)
    local newChild = UiElementBus.Event.GetChild(self.Properties.DropdownContent, numChildren)
    local textChild = UiElementBus.Event.FindChildByName(newChild, "Text")
    UiTextBus.Event.SetText(textChild, text)
    
    -- Remove this option from the options display
    UiElementBus.Event.DestroyElement(self.entityId)
end

function SelectionDropdownSelectedOption:OnDeactivate()
    self.buttonHandler:Disconnect()
end

return SelectionDropdownSelectedOption
