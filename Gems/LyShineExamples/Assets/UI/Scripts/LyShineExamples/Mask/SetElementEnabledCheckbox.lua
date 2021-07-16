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

local SetElementEnabledCheckbox = 
{
    Properties = 
    {
        Element = {default = EntityId()},
    },
}

function SetElementEnabledCheckbox:OnActivate()
    self.checkboxHandler = UiCheckboxNotificationBus.Connect(self, self.entityId)
end

function SetElementEnabledCheckbox:OnDeactivate()
    self.checkboxHandler:Disconnect()
end

function SetElementEnabledCheckbox:OnCheckboxStateChange(isChecked)
    UiElementBus.Event.SetIsEnabled(self.Properties.Element, isChecked)
end

return SetElementEnabledCheckbox
