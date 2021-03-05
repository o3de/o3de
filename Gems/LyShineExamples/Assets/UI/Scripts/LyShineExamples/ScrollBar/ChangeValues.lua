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