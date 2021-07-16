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

local DrawCalls = 
{
    Properties = 
    {
        CanvasCheckbox = {default = EntityId()},
        MaskCheckbox = {default = EntityId()},
        GradientMaskCheckbox = {default = EntityId()},
        RtFaderCheckbox = {default = EntityId()},
        RtOuterFaderCheckbox = {default = EntityId()},
        BlendModeCheckbox = {default = EntityId()},
        SrgbCheckbox = {default = EntityId()},
        ExceedMaxTexturesCheckbox = {default = EntityId()},
        ExceedMaxVertsCheckbox = {default = EntityId()},
        AnimatePosCheckbox = {default = EntityId()},
        AnimateScaleCheckbox = {default = EntityId()},
        AnimateRotationCheckbox = {default = EntityId()},
        ParticlesCheckbox = {default = EntityId()},
        InnerFaderSlider = {default = EntityId()},
        OuterFaderSlider = {default = EntityId()},
        DebugDisplayDropdown = {default = EntityId()},
        ReportDrawCallsButton = {default = EntityId()},
    },
}

function DrawCalls:OnActivate()
    self.initializationHandler = UiInitializationBus.Connect(self, self.entityId);
end

function DrawCalls:InGamePostActivate()
    self.initializationHandler:Disconnect()
    self.initializationHandler = nil

    self:SetIsCanvasLoaded(true)
        
    -- Handlers for checkboxes
    self.CanvasCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.CanvasCheckbox)
    self.MaskCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.MaskCheckbox)
    self.GradientMaskCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.GradientMaskCheckbox)
    self.RtFaderCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.RtFaderCheckbox)
    self.RtOuterFaderCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.RtOuterFaderCheckbox)
    self.BlendModeCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.BlendModeCheckbox)
    self.SrgbCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.SrgbCheckbox)
    self.ExceedMaxTexturesCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.ExceedMaxTexturesCheckbox)
    self.ExceedMaxVertsCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.ExceedMaxVertsCheckbox)
    self.AnimatePosCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.AnimatePosCheckbox)
    self.AnimateScaleCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.AnimateScaleCheckbox)
    self.AnimateRotationCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.AnimateRotationCheckbox)
    self.ParticlesCheckboxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.ParticlesCheckbox)
    
    self.InnerFaderSliderHandler = UiSliderNotificationBus.Connect(self, self.Properties.InnerFaderSlider)
    self.OuterFaderSliderHandler = UiSliderNotificationBus.Connect(self, self.Properties.OuterFaderSlider)

    self.DebugDisplayDropdownHandler = UiDropdownNotificationBus.Connect(self, self.Properties.DebugDisplayDropdown)
    self.ReportDrawCallsButtonHandler = UiButtonNotificationBus.Connect(self, self.Properties.ReportDrawCallsButton)
end


function DrawCalls:OnDeactivate()
    self.CanvasCheckboxHandler:Disconnect()
    self.MaskCheckboxHandler:Disconnect()
    self.GradientMaskCheckboxHandler:Disconnect()
    self.RtFaderCheckboxHandler:Disconnect()
    self.RtOuterFaderCheckboxHandler:Disconnect()
    self.BlendModeCheckboxHandler:Disconnect()
    self.SrgbCheckboxHandler:Disconnect()
    self.ExceedMaxTexturesCheckboxHandler:Disconnect()
    self.ExceedMaxVertsCheckboxHandler:Disconnect()
    self.AnimatePosCheckboxHandler:Disconnect()
    self.AnimateScaleCheckboxHandler:Disconnect()
    self.AnimateRotationCheckboxHandler:Disconnect()
    self.ParticlesCheckboxHandler:Disconnect()
    
    self.InnerFaderSliderHandler:Disconnect()
    self.OuterFaderSliderHandler:Disconnect()
    
    self.DebugDisplayDropdownHandler:Disconnect()
    self.ReportDrawCallsButtonHandler:Disconnect()
end

function DrawCalls:OnCheckboxStateChange(isChecked)
    local element = UiCheckboxNotificationBus.GetCurrentBusId()
    if (element == self.Properties.CanvasCheckbox) then
        self:SetIsCanvasLoaded(isChecked)
    elseif (not (self.exampleCanvas == nil)) then
        if (element == self.Properties.MaskCheckbox) then
            SetIsMaskEnabledRecursive(self.performanceElements, isChecked)
        elseif (element == self.Properties.GradientMaskCheckbox) then
            SetIsGradientMaskEnabledRecursive(self.performanceElements, isChecked)
        elseif (element == self.Properties.RtFaderCheckbox) then
            SetIsRtFaderEnabledRecursive(self.performanceElements, isChecked)
        elseif (element == self.Properties.RtOuterFaderCheckbox) then
            SetIsRtOuterFaderEnabled(self.exampleCanvasBackground, isChecked)
        elseif (element == self.Properties.BlendModeCheckbox) then
            EnableBlendModeAddRecursive(self.performanceElements, isChecked)
        elseif (element == self.Properties.SrgbCheckbox) then
            EnableSrgbRecursive(self.performanceElements, isChecked)
        elseif (element == self.Properties.ExceedMaxTexturesCheckbox) then
            EnableExtraTexturesRecursive(self.performanceElements, isChecked)
        elseif (element == self.Properties.ExceedMaxVertsCheckbox) then
            EnableExtraSlicesRecursive(self.performanceElements, isChecked)
        elseif (element == self.Properties.AnimatePosCheckbox) then
            EnableAnimatePos(self.exampleCanvas, isChecked)
        elseif (element == self.Properties.AnimateScaleCheckbox) then
            EnableAnimateScale(self.exampleCanvas, isChecked)
        elseif (element == self.Properties.AnimateRotationCheckbox) then
            EnableAnimateRotation(self.exampleCanvas, isChecked)
        elseif (element == self.Properties.ParticlesCheckbox) then
            EnableParticlesRecursive(self.performanceElements, isChecked)
        end
    end
end

function DrawCalls:OnSliderValueChanging(value)
    if (not (self.exampleCanvas == nil)) then
        local element = UiSliderNotificationBus.GetCurrentBusId()
        if (element == self.Properties.InnerFaderSlider) then
            SetInnerFaderValueRecursive(self.performanceElements, value)
        elseif (element == self.Properties.OuterFaderSlider) then
            SetOuterFaderValue(self.exampleCanvasBackground, value)
        end
    end
end

function DrawCalls:OnDropdownValueChanged(value)
    local dropdownElement = UiDropdownNotificationBus.GetCurrentBusId()
    
    if (dropdownElement == self.Properties.DebugDisplayDropdown) then
        local contentElement = UiElementBus.Event.FindDescendantByName(dropdownElement, "Content")
        local index = UiElementBus.Event.GetIndexOfChildByEntityId(contentElement, value)
        
        ConsoleRequestBus.Broadcast.ExecuteConsoleCommand("ui_DisplayDrawCallData 0")
        ConsoleRequestBus.Broadcast.ExecuteConsoleCommand("ui_DisplayTextureData 0")
        ConsoleRequestBus.Broadcast.ExecuteConsoleCommand("ui_DisplayCanvasData 0")
        
        if (index == 0) then
            ConsoleRequestBus.Broadcast.ExecuteConsoleCommand("ui_DisplayDrawCallData 1")
        elseif (index == 1) then
            ConsoleRequestBus.Broadcast.ExecuteConsoleCommand("ui_DisplayTextureData 1")
        elseif (index == 2) then
            ConsoleRequestBus.Broadcast.ExecuteConsoleCommand("ui_DisplayCanvasData 1")
        end

    end

end

function DrawCalls:OnButtonClick()
    local element = UiButtonNotificationBus.GetCurrentBusId()
    if (element == self.Properties.ReportDrawCallsButton) then
        ConsoleRequestBus.Broadcast.ExecuteConsoleCommand("ui_ReportDrawCalls")
    end
end

function DrawCalls:SetIsCanvasLoaded(enabled)
    if (enabled) then
        if (self.exampleCanvas == nil) then
            self.exampleCanvas = UiCanvasManagerBus.Broadcast.LoadCanvas("UI/Canvases/LyShineExamples/Performance/DrawCallsExample.uicanvas")
            self.exampleCanvasBackground = UiCanvasBus.Event.FindElementByName(self.exampleCanvas, "Background");
            self.performanceElements = UiCanvasBus.Event.FindElementByName(self.exampleCanvas, "PerformanceElements");
            
            SetIsMaskEnabledRecursive(self.performanceElements, UiCheckboxBus.Event.GetState(self.Properties.MaskCheckbox))
            SetIsGradientMaskEnabledRecursive(self.performanceElements, UiCheckboxBus.Event.GetState(self.Properties.GradientMaskCheckbox))
            SetIsRtFaderEnabledRecursive(self.performanceElements, UiCheckboxBus.Event.GetState(self.Properties.RtFaderCheckbox))
            SetIsRtOuterFaderEnabled(self.exampleCanvasBackground, UiCheckboxBus.Event.GetState(self.Properties.RtOuterFaderCheckbox))
            EnableBlendModeAddRecursive(self.performanceElements, UiCheckboxBus.Event.GetState(self.Properties.BlendModeCheckbox))
            EnableSrgbRecursive(self.performanceElements, UiCheckboxBus.Event.GetState(self.Properties.SrgbCheckbox))
            EnableExtraTexturesRecursive(self.performanceElements, UiCheckboxBus.Event.GetState(self.Properties.ExceedMaxTexturesCheckbox))
            EnableExtraSlicesRecursive(self.performanceElements, UiCheckboxBus.Event.GetState(self.Properties.ExceedMaxVertsCheckbox))
            EnableAnimatePos(self.exampleCanvas, UiCheckboxBus.Event.GetState(self.Properties.AnimatePosCheckbox))
            EnableAnimateScale(self.exampleCanvas, UiCheckboxBus.Event.GetState(self.Properties.AnimateScaleCheckbox))
            EnableParticlesRecursive(self.performanceElements, UiCheckboxBus.Event.GetState(self.Properties.ParticlesCheckbox))

            SetInnerFaderValueRecursive(self.performanceElements, UiSliderBus.Event.GetValue(self.Properties.InnerFaderSlider))
            
            SetOuterFaderValue(self.exampleCanvasBackground, UiSliderBus.Event.GetValue(self.Properties.OuterFaderSlider))
        end
    else
        if (not (self.exampleCanvas == nil)) then
            UiCanvasManagerBus.Broadcast.UnloadCanvas(self.exampleCanvas)
            self.exampleCanvas = nil
            self.exampleCanvasBackground = nil
            self.performanceElements = nil
        end
    end

end

function SetIsMaskEnabledRecursive(element, enabled)
    UiMaskBus.Event.SetIsMaskingEnabled(element, enabled)
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        SetIsMaskEnabledRecursive(children[i], enabled)
    end
end

function SetIsGradientMaskEnabledRecursive(element, enabled)
    UiMaskBus.Event.SetUseRenderToTexture(element, enabled)
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        SetIsGradientMaskEnabledRecursive(children[i], enabled)
    end
end

function SetIsRtFaderEnabledRecursive(element, enabled)
    UiFaderBus.Event.SetUseRenderToTexture(element, enabled)
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        SetIsRtFaderEnabledRecursive(children[i], enabled)
    end
end

function SetIsRtOuterFaderEnabled(element, enabled)
    UiFaderBus.Event.SetUseRenderToTexture(element, enabled)
end

function EnableBlendModeAddRecursive(element, enabled)
    local name = UiElementBus.Event.GetName(element)
    if (name == "BlendModeNormal") then
        UiElementBus.Event.SetIsEnabled(element, not enabled)
    elseif (name == "BlendModeAdd") then
        UiElementBus.Event.SetIsEnabled(element, enabled)
    end
    
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        EnableBlendModeAddRecursive(children[i], enabled)
    end
end

function EnableSrgbRecursive(element, enabled)
    local name = UiElementBus.Event.GetName(element)
    if (name == "RttImage") then
        UiImageBus.Event.SetIsRenderTargetSRGB(element, enabled)
    end
    
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        EnableSrgbRecursive(children[i], enabled)
    end
end

function EnableExtraTexturesRecursive(element, enabled)
    local name = UiElementBus.Event.GetName(element)
    if (name == "ImageTexture17" or name == "ImageTexture18") then
        UiElementBus.Event.SetIsEnabled(element, enabled)
    end
    
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        EnableExtraTexturesRecursive(children[i], enabled)
    end
end

function EnableExtraSlicesRecursive(element, enabled)
    local name = UiElementBus.Event.GetName(element)
    if (name == "AdditionalPerformanceRows") then
        UiElementBus.Event.SetIsEnabled(element, enabled)
    end
    
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        EnableExtraSlicesRecursive(children[i], enabled)
    end
end

function EnableAnimatePos(canvas, enabled)
    if (enabled) then
        UiAnimationBus.Event.StartSequence(canvas, "AnimatePos")
    else
        UiAnimationBus.Event.StopSequence(canvas, "AnimatePos")
    end
end

function EnableAnimateScale(canvas, enabled)
    if (enabled) then
        UiAnimationBus.Event.StartSequence(canvas, "AnimateScale")
    else
        UiAnimationBus.Event.StopSequence(canvas, "AnimateScale")
    end
end

function EnableAnimateRotation(canvas, enabled)
    if (enabled) then
        UiAnimationBus.Event.StartSequence(canvas, "AnimateRot")
    else
        UiAnimationBus.Event.StopSequence(canvas, "AnimateRot")
    end
end

function EnableParticlesRecursive(element, enabled)
    local name = UiElementBus.Event.GetName(element)
    if (name == "ParticleEmitter") then
        UiElementBus.Event.SetIsEnabled(element, enabled)
    end
    
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        EnableParticlesRecursive(children[i], enabled)
    end
end

function SetInnerFaderValueRecursive(element, value)
    UiFaderBus.Event.SetFadeValue(element, value)
    -- iterate over children of the specified element
    local children = UiElementBus.Event.GetChildren(element)
    for i = 1,#children do
        SetInnerFaderValueRecursive(children[i], value)
    end
end

function SetOuterFaderValue(element, value)
    UiFaderBus.Event.SetFadeValue(element, value)
end

return DrawCalls
