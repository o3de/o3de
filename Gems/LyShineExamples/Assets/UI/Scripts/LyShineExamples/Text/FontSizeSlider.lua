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
