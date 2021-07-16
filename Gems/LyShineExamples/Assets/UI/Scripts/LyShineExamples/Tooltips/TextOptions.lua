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

local TextOptions = 
{
    Properties = 
    {
        WrapCheckbox = {default = EntityId()},
        AlignCheckbox = {default = EntityId()},
        RedSlider = {default = EntityId()},
        GreenSlider = {default = EntityId()},
        BlueSlider = {default = EntityId()},
        ColorImage = {default = EntityId()},
        FontSizeSlider = {default = EntityId()},
        FontSizeText = {default = EntityId()},
        TooltipText = {default = EntityId()},
        TooltipEntity = {default = EntityId()},
        TriggerModeDropdown = {default = EntityId()},
        DropdownOnClickOption = {default = EntityId()},
        DropdownOnPressOption = {default = EntityId()},
        DropdownOnHoverOption = {default = EntityId()}
    },
}

function TextOptions:OnActivate()
    self.wrapCBHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.WrapCheckbox)
    self.alignCBHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.AlignCheckbox)
    
    self.redSliderHandler = UiSliderNotificationBus.Connect(self, self.Properties.RedSlider)
    self.greenSliderHandler = UiSliderNotificationBus.Connect(self, self.Properties.GreenSlider)
    self.blueSliderHandler = UiSliderNotificationBus.Connect(self, self.Properties.BlueSlider)
    self.fontSizeSliderHandler = UiSliderNotificationBus.Connect(self, self.Properties.FontSizeSlider)
    
    self.dropdownHandler = UiDropdownNotificationBus.Connect(self, self.Properties.TriggerModeDropdown)
    
    self.tickBusHandler = TickBus.Connect(self);
end

function TextOptions:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()

    -- Init to the current color
    local color = UiTextBus.Event.GetColor(self.Properties.TooltipText)
    UiImageBus.Event.SetColor(self.Properties.ColorImage, color)
    UiSliderBus.Event.SetValue(self.Properties.RedSlider, color.r)
    UiSliderBus.Event.SetValue(self.Properties.GreenSlider, color.g)
    UiSliderBus.Event.SetValue(self.Properties.BlueSlider, color.b)
    
    -- Init to the current font size
    local fontSize = UiTextBus.Event.GetFontSize(self.Properties.TooltipText)
    UiSliderBus.Event.SetValue(self.Properties.FontSizeSlider, fontSize)
    UiTextBus.Event.SetText(self.Properties.FontSizeText, fontSize)
    
    -- Select OnHover as default trigger mode
    UiDropdownBus.Event.SetValue(self.Properties.TriggerModeDropdown, self.Properties.DropdownOnHoverOption)
end

function TextOptions:OnDeactivate()
    self.wrapCBHandler:Disconnect()
    self.alignCBHandler:Disconnect()
    self.redSliderHandler:Disconnect()
    self.greenSliderHandler:Disconnect()
    self.blueSliderHandler:Disconnect()
    self.fontSizeSliderHandler:Disconnect()
    self.dropdownHandler:Disconnect()
end

function TextOptions:OnSliderValueChanging(value)
    self:HandleSliderValueChange(UiSliderNotificationBus.GetCurrentBusId(), value)
end

function TextOptions:OnSliderValueChanged(value)
    self:HandleSliderValueChange(UiSliderNotificationBus.GetCurrentBusId(), value)
end

function TextOptions:HandleSliderValueChange(slider, value)
    if (slider == self.Properties.RedSlider) then
        local color = UiTextBus.Event.GetColor(self.Properties.TooltipText)
        color.r =  value
        -- Set text color
        UiTextBus.Event.SetColor(self.Properties.TooltipText, color)
        -- Update color image
        UiImageBus.Event.SetColor(self.Properties.ColorImage, color)
    elseif (slider == self.Properties.GreenSlider) then
        local color = UiTextBus.Event.GetColor(self.Properties.TooltipText)
        color.g =  value
        -- Set text color
        UiTextBus.Event.SetColor(self.Properties.TooltipText, color)
        -- Update color image
        UiImageBus.Event.SetColor(self.Properties.ColorImage, color)
    elseif (slider == self.Properties.BlueSlider) then
        local color = UiTextBus.Event.GetColor(self.Properties.TooltipText)
        color.b =  value
        -- Set text color
        UiTextBus.Event.SetColor(self.Properties.TooltipText, color)    
        -- Update color image
        UiImageBus.Event.SetColor(self.Properties.ColorImage, color)
    elseif (slider == self.Properties.FontSizeSlider) then
        -- Set font size
        UiTextBus.Event.SetFontSize(self.Properties.TooltipText, value)
        -- Update font size text
        UiTextBus.Event.SetText(self.Properties.FontSizeText, value)
    end
end

function TextOptions:OnCheckboxStateChange(isChecked)
    if (UiCheckboxNotificationBus.GetCurrentBusId() == self.Properties.WrapCheckbox) then
        -- Set text wrapping
        if (isChecked) then
            UiTextBus.Event.SetWrapText(self.Properties.TooltipText, eUiTextWrapTextSetting_Wrap)
        else
            UiTextBus.Event.SetWrapText(self.Properties.TooltipText, eUiTextWrapTextSetting_NoWrap)
        end
    elseif (UiCheckboxNotificationBus.GetCurrentBusId() == self.Properties.AlignCheckbox) then
        -- Set text alignment
        if (isChecked) then
            UiTextBus.Event.SetHorizontalTextAlignment(self.Properties.TooltipText, eUiHAlign_Center)
        else
            UiTextBus.Event.SetHorizontalTextAlignment(self.Properties.TooltipText, eUiHAlign_Left)
        end
    end
end

function TextOptions:OnDropdownValueChanged(value)
    if (UiDropdownNotificationBus.GetCurrentBusId() == self.Properties.TriggerModeDropdown) then
        if (value == self.Properties.DropdownOnHoverOption) then
            UiTooltipDisplayBus.Event.SetTriggerMode(self.Properties.TooltipEntity, 0);
        elseif (value == self.Properties.DropdownOnPressOption) then
            UiTooltipDisplayBus.Event.SetTriggerMode(self.Properties.TooltipEntity, 1)
        elseif (value == self.Properties.DropdownOnClickOption) then
            UiTooltipDisplayBus.Event.SetTriggerMode(self.Properties.TooltipEntity, 2)
        end
    end
end

return TextOptions
