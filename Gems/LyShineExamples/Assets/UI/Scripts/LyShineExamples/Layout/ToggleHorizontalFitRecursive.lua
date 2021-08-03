----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local ToggleHorizontalFitRecursive =
{
    Properties =
    {
        ContainerElement = {default = EntityId()},
    },
}

function ToggleHorizontalFitRecursive:OnActivate()
    self.checkboxHandler = UiCheckboxNotificationBus.Connect(self, self.entityId)
end

function ToggleHorizontalFitRecursive:OnDeactivate()
    self.checkboxHandler:Disconnect()
end

function SetHorizontalFitRecursive(element, horizontalFit)
    UiLayoutFitterBus.Event.SetHorizontalFit(element, horizontalFit)
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        SetHorizontalFitRecursive(children[i], horizontalFit)
    end
end

function ToggleHorizontalFitRecursive:OnCheckboxStateChange(isChecked)
    SetHorizontalFitRecursive(self.Properties.ContainerElement, isChecked)
end

return ToggleHorizontalFitRecursive
