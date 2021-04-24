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
