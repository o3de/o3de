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
