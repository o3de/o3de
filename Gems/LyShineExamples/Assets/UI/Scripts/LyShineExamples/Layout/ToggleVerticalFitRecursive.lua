----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local ToggleVerticalFitRecursive =
{
    Properties =
    {
        ContainerElement = {default = EntityId()},
    },
}

function ToggleVerticalFitRecursive:OnActivate()
    self.checkboxHandler = UiCheckboxNotificationBus.Connect(self, self.entityId)
end

function ToggleVerticalFitRecursive:OnDeactivate()
    self.checkboxHandler:Disconnect()
end

function SetVerticalFitRecursive(element, verticalFit)
    UiLayoutFitterBus.Event.SetVerticalFit(element, verticalFit)
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        SetVerticalFitRecursive(children[i], verticalFit)
    end
end

function ToggleVerticalFitRecursive:OnCheckboxStateChange(isChecked)
    SetVerticalFitRecursive(self.Properties.ContainerElement, isChecked)
end

return ToggleVerticalFitRecursive
