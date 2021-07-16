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
