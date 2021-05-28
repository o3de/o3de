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
