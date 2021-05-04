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

local MarkupCheckBox = 
{
    Properties = 
    {
        ContainerElement = {default = EntityId()},
    },
}

function MarkupCheckBox:OnActivate()
    self.checkboxHandler = UiCheckboxNotificationBus.Connect(self, self.entityId)
end

function MarkupCheckBox:OnDeactivate()
    self.checkboxHandler:Disconnect()
end

function SetIsMarkupEnabledRecursive(element, isMarkupEnabled)
    UiTextBus.Event.SetIsMarkupEnabled(element, isMarkupEnabled)
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        SetIsMarkupEnabledRecursive(children[i], isMarkupEnabled)
    end
end

function MarkupCheckBox:OnCheckboxStateChange(isChecked)
    SetIsMarkupEnabledRecursive(self.Properties.ContainerElement, isChecked)
end

return MarkupCheckBox
