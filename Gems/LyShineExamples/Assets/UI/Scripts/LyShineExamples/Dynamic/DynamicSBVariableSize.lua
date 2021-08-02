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

local DynamicScrollBox = 
{
    Properties = 
    {
        DynamicScrollBox = {default = EntityId()},
        ScrollBar = {default = EntityId()},
        AddCheckBox = {default = EntityId()},
        ScrollToEndButton = {default = EntityId()},
    },
    
    addDelay = 1,
    numElementsToAdd = 1,
    keepAtEndIfAtEnd = true,
    
    tickTimer = 0,
    firstTick = true,
    addEnabled = true
}

function DynamicScrollBox:OnActivate()
    self.dynamicSBoxDataHandler = UiDynamicScrollBoxDataBus.Connect(self, self.Properties.DynamicScrollBox)
    self.dynamicSBoxElementHandler = UiDynamicScrollBoxElementNotificationBus.Connect(self, self.Properties.DynamicScrollBox)
    self.scrollBarHandler = UiScrollerNotificationBus.Connect(self, self.Properties.ScrollBar)
    self.addCheckBoxHandler = UiCheckboxNotificationBus.Connect(self, self.Properties.AddCheckBox)
    self.scrollButtonHandler = UiButtonNotificationBus.Connect(self, self.Properties.ScrollToEndButton)
    self.tickBusHandler = TickBus.Connect(self);
end

function DynamicScrollBox:OnTick(deltaTime, timePoint)
    if self.firstTick == true then
        self:InitContent("StaticData/LyShineExamples/uiTestMorePaidColors.json")        
        self.firstTick = false
    else
        if self.addEnabled == true then
            self.tickTimer = self.tickTimer + deltaTime
            if self.tickTimer >= self.addDelay then
                UiDynamicScrollBoxBus.Event.AddElementsToEnd(self.Properties.DynamicScrollBox, self.numElementsToAdd, self.keepAtEndIfAtEnd)
                self.tickTimer = 0
            end
        end
    end
end

function DynamicScrollBox:OnDeactivate()
    self.dynamicSBoxDataHandler:Disconnect()
    self.dynamicSBoxElementHandler:Disconnect()
    self.scrollBarHandler:Disconnect()
    self.addCheckBoxHandler:Disconnect()
    self.scrollButtonHandler:Disconnect()
    self.tickBusHandler:Disconnect()
end

function DynamicScrollBox:InitContent(jsonFilepath)
    -- Refresh the dynamic content database with the specified json file
    UiDynamicContentDatabaseBus.Broadcast.Refresh(eUiDynamicContentDBColorType_Paid, jsonFilepath)
    
    -- Refresh the dynamic scrollbox. This will trigger events from the
    -- UiDynamicScrollBoxDataBus and the UiDynamicScrollBoxElementNotificationBus
    UiDynamicScrollBoxBus.Event.RefreshContent(self.Properties.DynamicScrollBox)    
end

function DynamicScrollBox:GetMessageFromIndex(index)
    local formattedValue
    local i = index % 5
    if i == 0 then
        formattedValue = "This list contains elements of varying height"
    elseif i == 1 then
        formattedValue = "The auto calculate flag is on, so element heights are automatically calculated by the Dynamic Scroll Box component using layout methods"
    elseif i == 2 then
        formattedValue = "An estimated element height has not been provided, so all element heights are calculated up front. When an estimated element height is provided, sizes are only calculated when elements come into view. This is useful for lists that contain many elements"
    elseif i == 3 then
        formattedValue = "An element is added to this list every second. If the list is scrolled all the way to the bottom, it will remain scrolled to the bottom when elements are added"
    elseif i == 4 then
        formattedValue = "All features can also be applied to horizontal lists"
    end
    
    return formattedValue
end

-- Dynamic ScrollBox handlers

function DynamicScrollBox:GetNumElements()
    return 1
end

function DynamicScrollBox:OnPrepareElementForSizeCalculation(entityId, index)
    -- set message body
    local messageField = UiElementBus.Event.FindDescendantByName(entityId, "ChatText")
 
    local formattedValue = self:GetMessageFromIndex(index)
    UiTextBus.Event.SetText(messageField, formattedValue)    
end

function DynamicScrollBox:OnElementBecomingVisible(entityId, index)        
    local formattedValue

    -- set player name
    local playerName = UiElementBus.Event.FindDescendantByName(entityId, "PlayerName")
    formattedValue = string.format("Player%d", index)
    UiTextBus.Event.SetText(playerName, formattedValue)

    -- set player image color
    local numColors = UiDynamicContentDatabaseBus.Broadcast.GetNumColors(eUiDynamicContentDBColorType_Paid)
    local color = UiDynamicContentDatabaseBus.Broadcast.GetColor(eUiDynamicContentDBColorType_Paid, index % numColors)
    local playerImage = UiElementBus.Event.FindDescendantByName(entityId, "PlayerBackground")
    UiImageBus.Event.SetColor(playerImage, color)

    -- set message body
    local messageField = UiElementBus.Event.FindDescendantByName(entityId, "ChatText")
     formattedValue = self:GetMessageFromIndex(index)
    UiTextBus.Event.SetText(messageField, formattedValue)

    -- set background color
    local background = UiElementBus.Event.FindDescendantByName(entityId, "Background")
    local color
    if index % 2 == 0 then
        color = Color(167/255, 217/255, 232/255)
    else
        color = Color(245/255, 255/255, 255/255)
    end
    UiImageBus.Event.SetColor(background, color)
end

-- CheckBox handlers

function DynamicScrollBox:OnCheckboxStateChange(checked)
    self.addEnabled = checked
    if self.addEnabled then
        self.tickTimer = self.addDelay
    end
end

-- Button handlers

function DynamicScrollBox:OnButtonClick()
    UiDynamicScrollBoxBus.Event.ScrollToEnd(self.Properties.DynamicScrollBox)
    self.tickTimer = 0
end

-- Scroller handlers

function DynamicScrollBox:OnScrollerValueChanging(value)
    self:OnScrollerValueChanged(value)
end

function DynamicScrollBox:OnScrollerValueChanged(value)
    local enabled
    if Math.IsClose(value, 1.0, 0.01) then
        enabled = false
    else
        enabled = true
    end
    UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ScrollToEndButton, enabled)
end

return DynamicScrollBox
