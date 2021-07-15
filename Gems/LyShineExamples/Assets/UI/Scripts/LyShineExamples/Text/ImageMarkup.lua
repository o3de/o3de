----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local ImageMarkup = 
{
    Properties = 
    {
        AttribInteractables = { default = { EntityId(), EntityId(), EntityId(), EntityId(), EntityId(), EntityId(), EntityId() } },
        AttribEnabledCBs = { default = { EntityId(), EntityId(), EntityId(), EntityId(), EntityId(), EntityId(), EntityId() } },
        
        VAlignDDContent = {default = EntityId()},
        HeightDDContent = {default = EntityId()},
        
        DefaultsButton = {default = EntityId()},
        MarkupText = {default = EntityId()},
    },
    
    -- Attributes
    vAlignIndex = 1,
    heightIndex = 2,
    scaleIndex = 3,
    yOffsetIndex = 4,
    xPaddingIndex = 5,
    lPaddingIndex = 6,
    rPaddingIndex = 7,
    
    numAttributes = 7,
    
    -- Attribute names
    attribNames = { "vAlign", "height", "scale", "yOffset", "xPadding", "lPadding", "rPadding" },
    
    -- Attribute default values
    attribDefaults = { "baseline", "fontAscent", 1, 0, 0, 0, 0 },
    
    -- Attribute current values
    curAttribValues = {},    
}

function ImageMarkup:OnActivate()
    self.notificationHandlers = {}
    self.notificationHandlers[#self.notificationHandlers + 1] = UiButtonNotificationBus.Connect(self, self.Properties.DefaultsButton)
    self.notificationHandlers[#self.notificationHandlers + 1] = UiDropdownNotificationBus.Connect(self, self:GetAttribInteractable(self.vAlignIndex))
    self.notificationHandlers[#self.notificationHandlers + 1] = UiDropdownNotificationBus.Connect(self, self:GetAttribInteractable(self.heightIndex))
    self.notificationHandlers[#self.notificationHandlers + 1] = UiSliderNotificationBus.Connect(self, self:GetAttribInteractable(self.scaleIndex))
    self.notificationHandlers[#self.notificationHandlers + 1] = UiSliderNotificationBus.Connect(self, self:GetAttribInteractable(self.yOffsetIndex))
    self.notificationHandlers[#self.notificationHandlers + 1] = UiSliderNotificationBus.Connect(self, self:GetAttribInteractable(self.xPaddingIndex))
    self.notificationHandlers[#self.notificationHandlers + 1] = UiSliderNotificationBus.Connect(self, self:GetAttribInteractable(self.lPaddingIndex))
    self.notificationHandlers[#self.notificationHandlers + 1] = UiSliderNotificationBus.Connect(self, self:GetAttribInteractable(self.rPaddingIndex))
    
    for i = 0, self.numAttributes - 1 do
        self.notificationHandlers[#self.notificationHandlers + 1] = UiCheckboxNotificationBus.Connect(self, self.Properties.AttribEnabledCBs[i])
    end
    
    self.tickBusHandler = TickBus.Connect(self);
end

function ImageMarkup:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()

    self.vAlignOptions = {}
    local numOptions = UiElementBus.Event.GetNumChildElements(self.Properties.VAlignDDContent)
    for i = 0, numOptions - 1 do
        local child = UiElementBus.Event.GetChild(self.Properties.VAlignDDContent, i)
        local name = UiElementBus.Event.GetName(child)
        self.vAlignOptions[name] = child
    end
    
    self.heightOptions = {}
    numOptions = UiElementBus.Event.GetNumChildElements(self.Properties.HeightDDContent)
    for i = 0, numOptions - 1 do
        local child = UiElementBus.Event.GetChild(self.Properties.HeightDDContent, i)
        local name = UiElementBus.Event.GetName(child)
        self.heightOptions[name] = child
    end    
    
    self:SetToDefaults()
end

function ImageMarkup:OnDeactivate()
    for index, handler in ipairs(self.notificationHandlers) do
        handler:Disconnect()
    end
end

function ImageMarkup:GetAttribInteractable(index)
    return self.Properties.AttribInteractables[index - 1]
end

function ImageMarkup:GetAttribIndexFromCheckbox(checkbox)
    for i = 1, self.numAttributes do
        if self.Properties.AttribEnabledCBs[i - 1] == checkbox then
            return i
        end
    end
    
    return -1
end

function ImageMarkup:GetAttribIndexFromInteractable(interactable)
    for i = 1, self.numAttributes do
        if self:GetAttribInteractable(i) == interactable then
            return i
        end
    end
    
    return -1
end

function ImageMarkup:SetToDefaults()
    for i = 1, self.numAttributes do
        self.curAttribValues[i] = self.attribDefaults[i]
    end
    
    UiDropdownBus.Event.SetValue(self:GetAttribInteractable(self.vAlignIndex), self.vAlignOptions[self.curAttribValues[self.vAlignIndex]])
    UiRadioButtonGroupBus.Event.SetState(self.Properties.VAlignDDContent, self.vAlignOptions[self.attribDefaults[self.vAlignIndex]], true)
    UiDropdownBus.Event.SetValue(self:GetAttribInteractable(self.heightIndex), self.heightOptions[self.curAttribValues[self.heightIndex]])
    UiRadioButtonGroupBus.Event.SetState(self.Properties.HeightDDContent, self.heightOptions[self.attribDefaults[self.heightIndex]], true)
    UiSliderBus.Event.SetValue(self:GetAttribInteractable(self.scaleIndex), self.curAttribValues[self.scaleIndex])
    UiSliderBus.Event.SetValue(self:GetAttribInteractable(self.yOffsetIndex), self.curAttribValues[self.yOffsetIndex])
    UiSliderBus.Event.SetValue(self:GetAttribInteractable(self.xPaddingIndex), self.curAttribValues[self.xPaddingIndex])
    UiSliderBus.Event.SetValue(self:GetAttribInteractable(self.lPaddingIndex), self.curAttribValues[self.lPaddingIndex])
    UiSliderBus.Event.SetValue(self:GetAttribInteractable(self.rPaddingIndex), self.curAttribValues[self.rPaddingIndex])
    
    self:SetCheckboxState(self.Properties.AttribEnabledCBs[self.lPaddingIndex - 1], false)
    self:SetCheckboxState(self.Properties.AttribEnabledCBs[self.rPaddingIndex - 1], false)
    
    self:UpdateMarkupText()
end

function ImageMarkup:GetImageTagText(imagePath)
    local tagText = "<img src=" .. '\"' .. imagePath .. '\"'
    
    local attribsString = ""
    for i = 1, self.numAttributes do
        local enabled = UiInteractableBus.Event.IsHandlingEvents(self:GetAttribInteractable(i))
        if enabled then
            if self.curAttribValues[i] ~= self.attribDefaults[i] or i > self.xPaddingIndex then
                attribsString = attribsString .. ' ' .. self.attribNames[i] .. '=' .. '\"' .. self.curAttribValues[i] .. '\"'
            end
        end
    end
    
    tagText = tagText .. attribsString .. "/>"
    
    return tagText
end

function ImageMarkup:UpdateMarkupText()
    local image1TagText = self:GetImageTagText("UI/Textures/LyShineExamples/scroll_box_icon_4")
    local image2TagText = self:GetImageTagText("UI/Textures/LyShineExamples/scroll_box_icon_3")
    
    local markupText = "This text" .. image1TagText .. "contains images using the attribute" .. image2TagText .. "values above."

    UiTextBus.Event.SetText(self.Properties.MarkupText, markupText)
end

function ImageMarkup:HandleCheckboxStateChange(checkbox, checked)
    local attribIndex = self:GetAttribIndexFromCheckbox(checkbox)
    UiInteractableBus.Event.SetIsHandlingEvents(self:GetAttribInteractable(attribIndex), checked)
end

function ImageMarkup:SetCheckboxState(checkbox, checked)
    UiCheckboxBus.Event.SetState(checkbox, checked)
    self:HandleCheckboxStateChange(checkbox, checked)
end

-- Button handlers

function ImageMarkup:OnButtonClick()
    self:SetToDefaults()
end

-- Dropdown handlers

function ImageMarkup:OnDropdownValueChanged(entityId)
    local dropdown = UiDropdownNotificationBus.GetCurrentBusId()
    local attribIndex = self:GetAttribIndexFromInteractable(dropdown)
    self.curAttribValues[attribIndex] = UiElementBus.Event.GetName(entityId)
    
    self:UpdateMarkupText()
end

-- Slider handlers

function ImageMarkup:OnSliderValueChanging(value)
    self:OnSliderValueChanged(value)
end

function ImageMarkup:OnSliderValueChanged(value)
    local slider = UiSliderNotificationBus.GetCurrentBusId()
    local attribIndex = self:GetAttribIndexFromInteractable(slider)
    local roundedValue = tonumber(string.format("%.2f", value))
    self.curAttribValues[attribIndex] = roundedValue
    
    self:UpdateMarkupText()
end

-- Checkbox handlers

function ImageMarkup:OnCheckboxStateChange(checked)
    local checkbox = UiCheckboxNotificationBus.GetCurrentBusId()
    self:HandleCheckboxStateChange(checkbox, checked)
    
    self:UpdateMarkupText()
end

return ImageMarkup
