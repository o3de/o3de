----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local FadeSlider = 
{
    Properties = 
    {
        FaderEntity = {default = EntityId()},
    },
}

function FadeSlider:OnActivate()
    -- Connect to the slider notification bus
    self.sliderHandler = UiSliderNotificationBus.Connect(self, self.entityId)
end

function FadeSlider:OnDeactivate()
    -- Deactivate from the slider notification bus only if we managed to connect
    if (self.sliderHandler ~= nil) then
        self.sliderHandler:Disconnect()
    end
end

function FadeSlider:OnSliderValueChanging(percent)
    -- Set the fade value to the slider value
    UiFaderBus.Event.SetFadeValue(self.Properties.FaderEntity, percent / 100)
end

function FadeSlider:OnSliderValueChanged(percent)
    -- Set the fade value to the slider value
    UiFaderBus.Event.SetFadeValue(self.Properties.FaderEntity, percent / 100)
end

return FadeSlider
