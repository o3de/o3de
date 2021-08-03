----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local ToggleInputEnabledOnElementChildren = 
{
    Properties = 
    {
        ContainerElement = {default = EntityId()},
    },
}

function ToggleInputEnabledOnElementChildren:OnActivate()
    self.checkboxHandler = UiCheckboxNotificationBus.Connect(self, self.entityId)
end

function ToggleInputEnabledOnElementChildren:OnDeactivate()
    self.checkboxHandler:Disconnect()
end

function SetIsHandlingEventsRecursive(element, isHandlingEvents)
    UiInteractableBus.Event.SetIsHandlingEvents(element, isHandlingEvents)
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        SetIsHandlingEventsRecursive(children[i], isHandlingEvents)
    end
end

function ToggleInputEnabledOnElementChildren:OnCheckboxStateChange(isChecked)
    SetIsHandlingEventsRecursive(self.Properties.ContainerElement, isChecked)
end

return ToggleInputEnabledOnElementChildren
