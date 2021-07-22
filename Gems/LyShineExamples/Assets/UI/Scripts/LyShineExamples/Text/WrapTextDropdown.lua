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

local WrapTextDropdown = 
{
    Properties = 
    {
        TextContent = {default = EntityId()},
        NoWrapOption = {default = EntityId()},
        WrapTextOption = {default = EntityId()},
    },
}

function WrapTextDropdown:OnActivate()
    self.radioButtonGroupBusHandler = UiRadioButtonGroupNotificationBus.Connect(self, self.entityId);
end

function WrapTextDropdown:OnRadioButtonGroupStateChange(checkedRadioButton)
    if (checkedRadioButton == self.Properties.NoWrapOption) then
        UiTextBus.Event.SetWrapText(self.Properties.TextContent, eUiTextWrapTextSetting_NoWrap)
    elseif (checkedRadioButton == self.Properties.WrapTextOption) then
        UiTextBus.Event.SetWrapText(self.Properties.TextContent, eUiTextWrapTextSetting_Wrap)
    end
end


function WrapTextDropdown:OnDeactivate()
    self.radioButtonGroupBusHandler:Disconnect()
end

return WrapTextDropdown
