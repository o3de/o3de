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

local FontSizeSlider = 
{
    Properties = 
    {
        RerenderedFontEntity = {default = EntityId()},        
        FontEntity = {default = EntityId()},
    },
}

function FontSizeSlider:OnActivate()
    -- Connect to the slider notification bus
    self.sliderHandler = UiSliderNotificationBus.Connect(self, self.entityId)
end

function FontSizeSlider:OnDeactivate()
    -- Deactivate from the slider notification bus
    self.sliderHandler:Disconnect()
end

function UpdateFontSize(entity, percent)
    -- Resize the font
    UiTextBus.Event.SetFontSize(entity, 12 + percent)
end

function FontSizeSlider:OnSliderValueChanging(percent)
    -- Set the size value to the slider value
    UpdateFontSize(self.Properties.RerenderedFontEntity, percent)
    UpdateFontSize(self.Properties.FontEntity, percent)
end

function FontSizeSlider:OnSliderValueChanged(percent)
    -- Set the size value to the slider value
    UpdateFontSize(self.Properties.RerenderedFontEntity, percent)
    UpdateFontSize(self.Properties.FontEntity, percent)
end

return FontSizeSlider