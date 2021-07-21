----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    },    
}

function DynamicScrollBox:OnActivate()
    self.dynamicSBoxDataHandler = UiDynamicScrollBoxDataBus.Connect(self, self.Properties.DynamicScrollBox)
    self.dynamicSBoxElementHandler = UiDynamicScrollBoxElementNotificationBus.Connect(self, self.Properties.DynamicScrollBox)
    self.tickBusHandler = TickBus.Connect(self);
end

function DynamicScrollBox:OnDeactivate()
    self.dynamicSBoxDataHandler:Disconnect()
    self.dynamicSBoxElementHandler:Disconnect()
end

function DynamicScrollBox:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()
    self:InitContent("StaticData/LyShineExamples/uiTestMorePaidColors.json")        
end

function DynamicScrollBox:InitContent(jsonFilepath)
    -- Refresh the dynamic content database with the specified json file
    UiDynamicContentDatabaseBus.Broadcast.Refresh(eUiDynamicContentDBColorType_Paid, jsonFilepath)
    
    -- Refresh the dynamic scrollbox. This will trigger events from the
    -- UiDynamicScrollBoxDataBus and the UiDynamicScrollBoxElementNotificationBus
    UiDynamicScrollBoxBus.Event.RefreshContent(self.Properties.DynamicScrollBox)    
end

function DynamicScrollBox:GetElementIndex(sectionIndex, index)
    local absoluteIndex
    if sectionIndex == 0 then
        absoluteIndex = index
    else
        absoluteIndex = ((sectionIndex * (sectionIndex + 1)) / 2) + index
    end
    
    return absoluteIndex
end

function DynamicScrollBox:GetMessageFromIndex(sectionIndex, index)
    local formattedValue
    local i = self:GetElementIndex(sectionIndex, index) % 5
    if i == 0 then
        formattedValue = "This list contains elements and headers that vary in height"
    elseif i == 1 then
        formattedValue = "The element heights are automatically calculated by the Dynamic Scroll Box component using layout methods"
    elseif i == 2 then
        formattedValue = "The header heights are not set to automatically calculate, so their sizes are provided via the UiDynamicScrollBoxDataBus. In this case, each header is a few pixels taller than the one before it"
    elseif i == 3 then
        formattedValue = "An estimated element height has been provided, so sizes are only calculated when an element comes into view. This is useful when the list contains many elements"
    elseif i == 4 then
        formattedValue = "All features can also be applied to horizontal lists"
    end

    return formattedValue
end

function DynamicScrollBox:GetHeaderMessageFromIndex(sectionIndex)
    local formattedValue = string.format("Header %d", sectionIndex)
    return formattedValue
end

-- Dynamic ScrollBox handlers

function DynamicScrollBox:GetNumSections()
    return 10
end

function DynamicScrollBox:GetNumElementsInSection(sectionIndex)
    return (sectionIndex + 1) * 1
end

function DynamicScrollBox:OnPrepareElementInSectionForSizeCalculation(entityId, sectionIndex, index)
    -- set message body
    local messageField = UiElementBus.Event.FindDescendantByName(entityId, "ChatText")
 
    local formattedValue = self:GetMessageFromIndex(sectionIndex, index)
    UiTextBus.Event.SetText(messageField, formattedValue)
end

function DynamicScrollBox:OnElementInSectionBecomingVisible(entityId, sectionIndex, index)
    local formattedValue

    -- set player name
    local playerName = UiElementBus.Event.FindDescendantByName(entityId, "PlayerName")
    formattedValue = string.format("Player%d.%d", sectionIndex, index)
    UiTextBus.Event.SetText(playerName, formattedValue)

    -- set player image color
    local numColors = UiDynamicContentDatabaseBus.Broadcast.GetNumColors(eUiDynamicContentDBColorType_Paid)
    local colorIndex = self:GetElementIndex(sectionIndex, index) % numColors
    local color = UiDynamicContentDatabaseBus.Broadcast.GetColor(eUiDynamicContentDBColorType_Paid, colorIndex)
    local playerImage = UiElementBus.Event.FindDescendantByName(entityId, "PlayerBackground")
    UiImageBus.Event.SetColor(playerImage, color)

    -- set message body
    local messageField = UiElementBus.Event.FindDescendantByName(entityId, "ChatText")
     formattedValue = self:GetMessageFromIndex(sectionIndex, index)
    UiTextBus.Event.SetText(messageField, formattedValue)
end

function DynamicScrollBox:GetSectionHeaderHeight(sectionIndex)
    local height = 40 + (sectionIndex * 5)
    return height
end

function DynamicScrollBox:OnSectionHeaderBecomingVisible(entityId, sectionIndex)
    -- set header title
    local headerTitle = UiElementBus.Event.FindDescendantByName(entityId, "HeaderTitle")
     local formattedValue = self:GetHeaderMessageFromIndex(sectionIndex)
    UiTextBus.Event.SetText(headerTitle, formattedValue)    
end

return DynamicScrollBox
