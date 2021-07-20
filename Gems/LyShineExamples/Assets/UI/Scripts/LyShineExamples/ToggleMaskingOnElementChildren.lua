----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
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
