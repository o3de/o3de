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

local ImageFillTypes = 
{
    Properties = 
    {
        FilledImages = { default = { EntityId(), EntityId(), EntityId(), EntityId() } },
        Dropdowns = { default = { EntityId(), EntityId(), EntityId(), EntityId() } },
        RadialStartAngleSlider = { default = EntityId() },
        SpriteRadioButtonGroup = { default = EntityId() },
    },
}


function ImageFillTypes:OnActivate()
    self.tickBusHandler = TickBus.Connect(self)
    self.totalTime = 0
    self.timeOverride = 0
    
    self.dropdownHandlers = {}
    for i = 0, #self.Properties.Dropdowns do
        self.dropdownHandlers[i] = UiDropdownNotificationBus.Connect(self, self.Properties.Dropdowns[i])
    end
    
    self.radialStartAngleSliderHandler = UiSliderNotificationBus.Connect(self, self.Properties.RadialStartAngleSlider)
    
    self.spriteRadioButtonGroupHandler = UiRadioButtonGroupNotificationBus.Connect(self, self.Properties.SpriteRadioButtonGroup)

    self:InitAutomatedTestEvents()
end

function ImageFillTypes:OnTick(deltaTime, timePoint)
    self.totalTime = self.totalTime + deltaTime

    -- [Automated Testing] Overrides the fill value
    if self.timeOverride > 0 then
        self.totalTime = self.timeOverride
    end

    for i = 0, #self.Properties.FilledImages do
        fillAmount = 1.0-((math.cos(self.totalTime)*0.5)+0.5) -- Scale cos output to the range [0,1]
        UiImageBus.Event.SetFillAmount(self.Properties.FilledImages[i], fillAmount)
    end
end

function ImageFillTypes:OnDeactivate()
    self.tickBusHandler:Disconnect()
    
    for i = 0, #self.Properties.Dropdowns do
        self.dropdownHandlers[i]:Disconnect()
    end
    self.radialStartAngleSliderHandler:Disconnect()
    self.spriteRadioButtonGroupHandler:Disconnect()

    self:DeInitAutomatedTestEvents()
end

function ImageFillTypes:OnDropdownValueChanged(entityId)
    local dropdown = UiDropdownNotificationBus.GetCurrentBusId()
    local parent = UiElementBus.Event.GetParent(entityId)
    local selectedIndex = UiElementBus.Event.GetIndexOfChildByEntityId(parent, entityId)
    if dropdown == self.Properties.Dropdowns[0] then
        -- Linear
        UiImageBus.Event.SetEdgeFillOrigin(self.Properties.FilledImages[0], selectedIndex)
    elseif dropdown == self.Properties.Dropdowns[2] then
        -- Radial corner
        UiImageBus.Event.SetCornerFillOrigin(self.Properties.FilledImages[2], selectedIndex)
    elseif dropdown == self.Properties.Dropdowns[3] then
        -- Radial edge
        UiImageBus.Event.SetEdgeFillOrigin(self.Properties.FilledImages[3], selectedIndex)
    end
    
    self.totalTime = 0
    self:OnTick(0, 0)
end

function ImageFillTypes:OnSliderValueChanged(value)
    -- Radial
    local slider = UiSliderNotificationBus.GetCurrentBusId()
    
    if slider == self.Properties.RadialStartAngleSlider then    
        UiImageBus.Event.SetRadialFillStartAngle(self.Properties.FilledImages[1], value)
    end
    
    self.totalTime = 0
    self:OnTick(0, 0)    
end

function ImageFillTypes:OnRadioButtonGroupStateChange(entityId)
    local selectedIndex = UiElementBus.Event.GetIndexOfChildByEntityId(self.Properties.SpriteRadioButtonGroup, entityId)
    
    if selectedIndex == 0 then
        -- No sprite
        for i = 0, #self.Properties.FilledImages do        
            UiImageBus.Event.SetSpritePathname(self.Properties.FilledImages[i], "")
        end
    elseif selectedIndex == 1 then
        -- Sprite
        for i = 0, #self.Properties.FilledImages do        
            UiImageBus.Event.SetImageType(self.Properties.FilledImages[i], eUiImageType_StretchedToFit)
            UiImageBus.Event.SetSpritePathname(self.Properties.FilledImages[i], "ui/textures/lyshineexamples/scroll_box_icon_5.tif")
        end        
    elseif selectedIndex == 2 then
        -- Sliced sprite
        for i = 0, #self.Properties.FilledImages do        
            UiImageBus.Event.SetImageType(self.Properties.FilledImages[i], eUiImageType_Sliced)
            UiImageBus.Event.SetSpritePathname(self.Properties.FilledImages[i], "ui/textures/lyshineexamples/button.tif")
            UiImageBus.Event.SetFillCenter(self.Properties.FilledImages[i], true)
        end
    elseif selectedIndex == 3 then
        -- Sliced sprite, no center
        for i = 0, #self.Properties.FilledImages do        
            UiImageBus.Event.SetImageType(self.Properties.FilledImages[i], eUiImageType_Sliced)
            UiImageBus.Event.SetSpritePathname(self.Properties.FilledImages[i], "ui/textures/lyshineexamples/button.tif")
            UiImageBus.Event.SetFillCenter(self.Properties.FilledImages[i], false)
        end                
    end    
end

-- [Automated Testing] setup
function ImageFillTypes:InitAutomatedTestEvents()
    self.automatedTestSetFillValueId = GameplayNotificationId(EntityId(), "AutomatedTestSetFillValue", "float");
    self.automatedTestSetFillValueHandler = GameplayNotificationBus.Connect(self, self.automatedTestSetFillValueId);
end

-- [Automated Testing] event handling
function ImageFillTypes:OnEventBegin(value)
    if (GameplayNotificationBus.GetCurrentBusId() == self.automatedTestSetFillValueId) then
        self.timeOverride = value
    end
end

-- [Automated Testing] teardown
function ImageFillTypes:DeInitAutomatedTestEvents()
    if (self.automatedTestSetFillValueHandler ~= nil) then
        self.automatedTestSetFillValueHandler:Disconnect();
        self.automatedTestSetFillValueHandler = nil;
    end
end

return ImageFillTypes
