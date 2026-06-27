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

local SwitchGroup = 
{
    Properties = 
    {
        LeftGroup = {default = EntityId()},
        RightGroup = {default = EntityId()},
        SwitchButton = {default = EntityId()},
        SwitchButtonText = {default = EntityId()},
    },
}

function SwitchGroup:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
    self.isInLeft = true
end

function SwitchGroup:OnButtonClick()
    if (self.isInLeft) then
        UiRadioButtonGroupBus.Event.RemoveRadioButton(self.Properties.LeftGroup, self.Properties.SwitchButton)
        UiRadioButtonGroupBus.Event.AddRadioButton(self.Properties.RightGroup, self.Properties.SwitchButton)
        UiTextBus.Event.SetColor(self.Properties.SwitchButtonText, Color(0, 0, 1))
        self.isInLeft = false
    else
        UiRadioButtonGroupBus.Event.RemoveRadioButton(self.Properties.RightGroup, self.Properties.SwitchButton)
        UiRadioButtonGroupBus.Event.AddRadioButton(self.Properties.LeftGroup, self.Properties.SwitchButton)
        UiTextBus.Event.SetColor(self.Properties.SwitchButtonText, Color(1, 0, 0))
        self.isInLeft = true
    end
end

function SwitchGroup:OnDeactivate()
    self.buttonHandler:Disconnect()
end

return SwitchGroup
