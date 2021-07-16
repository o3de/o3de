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

local FadeButton = 
{
    Properties = 
    {
        FaderEntity = {default = EntityId()},
        FadeValueSlider = {default = EntityId()},
        FadeSpeedMinusButton = {default = EntityId()},
        FadeSpeedPlusButton = {default = EntityId()},
        FadeSpeedText = {default = EntityId()},
    },
}

function FadeButton:OnActivate()
    -- Connect to the button notification bus on our entity to know when to start the animation
    self.animButtonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
    -- Connect to the slider notification bus to know what fade value to give to the animation
    self.sliderHandler = UiSliderNotificationBus.Connect(self, self.Properties.FadeValueSlider)
    -- Connect to the button notification buses of the minus and plus buttons to know when to change the fade speed
    self.minusButtonHandler = UiButtonNotificationBus.Connect(self, self.Properties.FadeSpeedMinusButton)
    self.plusButtonHandler = UiButtonNotificationBus.Connect(self, self.Properties.FadeSpeedPlusButton)
    
    -- Initialize the fade value (needs to be the same as the start fade value slider value)
    self.fadeValue = 0
    -- Initialize the fade speed (needs to be the same as the start fade speed text)
    self.fadeSpeed = 5
end

function FadeButton:OnDeactivate()
    -- Disconnect from all our buses
    self.animButtonHandler:Disconnect()
    self.sliderHandler:Disconnect()
    self.minusButtonHandler:Disconnect()
    self.plusButtonHandler:Disconnect()
end

function FadeButton:OnSliderValueChanging(percent)
    -- Set the fade value to the slider value
    self.fadeValue = percent / 100
end

function FadeButton:OnSliderValueChanged(percent)
    -- Set the fade value to the slider value
    self.fadeValue = percent / 100
end

function FadeButton:OnButtonClick()
    -- If the animation button was clicked
    if (UiButtonNotificationBus.GetCurrentBusId() == self.entityId) then
        -- Start the fade animation
        UiFaderBus.Event.Fade(self.Properties.FaderEntity, self.fadeValue, self.fadeSpeed)
        
    -- Else if the minus button was clicked
    elseif (UiButtonNotificationBus.GetCurrentBusId() == self.Properties.FadeSpeedMinusButton) then
        -- Decrement the animation speed (min = 1)
        self.fadeSpeed = math.max(1, self.fadeSpeed - 1)
        -- And update the text
        UiTextBus.Event.SetText(self.Properties.FadeSpeedText, self.fadeSpeed)
        
    -- Else the plus button was clicked
    else
        -- Decrement the animation speed (max = 10)
        self.fadeSpeed = math.min(self.fadeSpeed + 1, 10)
        -- And update the text
        UiTextBus.Event.SetText(self.Properties.FadeSpeedText, self.fadeSpeed)
    end
end

return FadeButton
