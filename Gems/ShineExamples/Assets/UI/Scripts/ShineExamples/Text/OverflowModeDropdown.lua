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
