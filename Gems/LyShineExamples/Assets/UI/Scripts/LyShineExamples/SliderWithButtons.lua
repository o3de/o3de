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

local SliderWithButtons = 
{
    Properties = 
    {
        MinusButton = {default = EntityId()},
        PlusButton = {default = EntityId()},
        Slider = {default = EntityId()},
    },
}

function SliderWithButtons:OnActivate()
    self.minusButtonHandler = UiButtonNotificationBus.Connect(self, self.Properties.MinusButton)
    self.plusButtonHandler = UiButtonNotificationBus.Connect(self, self.Properties.PlusButton)
end

function SliderWithButtons:OnDeactivate()
    self.minusButtonHandler:Disconnect()
    self.plusButtonHandler:Disconnect()
end

function SliderWithButtons:OnButtonClick()
    local curValue = UiSliderBus.Event.GetValue(self.Properties.Slider)
    local stepValue = UiSliderBus.Event.GetStepValue(self.Properties.Slider)
    
    if (stepValue <= 0) then
        stepValue = 10
    end

    if (UiButtonNotificationBus.GetCurrentBusId() == self.Properties.MinusButton) then
        UiSliderBus.Event.SetValue(self.Properties.Slider, curValue - stepValue)
    else
        UiSliderBus.Event.SetValue(self.Properties.Slider, curValue + stepValue)
    end
end

return SliderWithButtons
