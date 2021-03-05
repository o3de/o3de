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

local OverflowModeDropdrown = 
{
    Properties = 
    {
        TextContent = {default = EntityId()},
        Overflow = {default = EntityId()},
        ClipText = {default = EntityId()},
        Ellipsis = {default = EntityId()},
    },
}

function OverflowModeDropdrown:OnActivate()
    self.radioButtonGroupBusHandler = UiRadioButtonGroupNotificationBus.Connect(self, self.entityId);
end

function OverflowModeDropdrown:OnRadioButtonGroupStateChange(checkedRadioButton)
    if (checkedRadioButton == self.Properties.Overflow) then
        UiTextBus.Event.SetOverflowMode(self.Properties.TextContent, eUiTextOverflowMode_OverflowText)
    elseif (checkedRadioButton == self.Properties.ClipText) then
        UiTextBus.Event.SetOverflowMode(self.Properties.TextContent, eUiTextOverflowMode_ClipText)
    else
        UiTextBus.Event.SetOverflowMode(self.Properties.TextContent, eUiTextOverflowMode_Ellipsis)
    end
end


function OverflowModeDropdrown:OnDeactivate()
    self.radioButtonGroupBusHandler:Disconnect()
end

return OverflowModeDropdrown
