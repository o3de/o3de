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

local ToggleMaskingOnElementChildren = 
{
    Properties = 
    {
        ContainerElement = {default = EntityId()},
    },
}

function ToggleMaskingOnElementChildren:OnActivate()
    self.checkboxHandler = UiCheckboxNotificationBus.Connect(self, self.entityId)
end

function ToggleMaskingOnElementChildren:OnDeactivate()
    self.checkboxHandler:Disconnect()
end

function SetIsMaskEnabledRecursive(element, isMasking)
    UiMaskBus.Event.SetIsMaskingEnabled(element, isMasking)
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        SetIsMaskEnabledRecursive(children[i], isMasking)
    end
end

function ToggleMaskingOnElementChildren:OnCheckboxStateChange(isChecked)
    SetIsMaskEnabledRecursive(self.Properties.ContainerElement, isChecked)
end

return ToggleMaskingOnElementChildren
