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

local ChangeValues = 
{
    Properties = 
    {
        CurrentValue = {default = EntityId()},
        ChangedValue = {default = EntityId()},
    },
}

function ChangeValues:OnActivate()
    self.scrollBarHandler = UiScrollerNotificationBus.Connect(self, self.entityId)
end

function ChangeValues:OnDeactivate()
    self.scrollBarHandler:Disconnect()
end

function ChangeValues:OnScrollerValueChanged(value)
    local formattedValue = string.format("%.2f", value)
    UiTextBus.Event.SetText(self.Properties.ChangedValue, formattedValue)
end

function ChangeValues:OnScrollerValueChanging(value)
    local formattedValue = string.format("%.2f", value)
    UiTextBus.Event.SetText(self.Properties.CurrentValue, formattedValue)
end

return ChangeValues
