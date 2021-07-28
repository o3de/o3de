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

local ToggleInteractionMaskingOnElementChildren = 
{
    Properties = 
    {
        ContainerElement = {default = EntityId()},
    },
}

function ToggleInteractionMaskingOnElementChildren:OnActivate()
    self.checkboxHandler = UiCheckboxNotificationBus.Connect(self, self.entityId)
end

function ToggleInteractionMaskingOnElementChildren:OnDeactivate()
    self.checkboxHandler:Disconnect()
end

function SetIsInteractionMaskEnabledRecursive(element, isMasking)
    UiMaskBus.Event.SetIsInteractionMaskingEnabled(element, isMasking)
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        SetIsInteractionMaskEnabledRecursive(children[i], isMasking)
    end
end

function ToggleInteractionMaskingOnElementChildren:OnCheckboxStateChange(isChecked)
    SetIsInteractionMaskEnabledRecursive(self.Properties.ContainerElement, isChecked)
end

return ToggleInteractionMaskingOnElementChildren
