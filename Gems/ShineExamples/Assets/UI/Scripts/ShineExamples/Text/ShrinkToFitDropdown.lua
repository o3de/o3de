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

local ShrinkToFitDropdown = 
{
    Properties = 
    {
        TextContent = {default = EntityId()},
        NoneOption = {default = EntityId()},
        UniformOption = {default = EntityId()},
        WidthOnlyOption = {default = EntityId()},
    },
}

function ShrinkToFitDropdown:OnActivate()
    self.radioButtonGroupBusHandler = UiRadioButtonGroupNotificationBus.Connect(self, self.entityId);
end

function ShrinkToFitDropdown:OnRadioButtonGroupStateChange(checkedRadioButton)
    if (checkedRadioButton == self.Properties.NoneOption) then
        UiTextBus.Event.SetShrinkToFit(self.Properties.TextContent, eUiTextShrinkToFit_None)
    elseif (checkedRadioButton == self.Properties.UniformOption) then
        UiTextBus.Event.SetShrinkToFit(self.Properties.TextContent, eUiTextShrinkToFit_Uniform)
    elseif (checkedRadioButton == self.Properties.WidthOnlyOption) then
        UiTextBus.Event.SetShrinkToFit(self.Properties.TextContent, eUiTextShrinkToFit_WidthOnly)
    end
end


function ShrinkToFitDropdown:OnDeactivate()
    self.radioButtonGroupBusHandler:Disconnect()
end

return ShrinkToFitDropdown
