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
        AddColorsButton = {default = EntityId()},
        ColorImage = {default = EntityId()},
        ColorIndexText = {default = EntityId()},
    },
}

function DynamicScrollBox:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.Properties.AddColorsButton)

    self.dynamicSBoxDataHandler = UiDynamicScrollBoxDataBus.Connect(self, self.Properties.DynamicScrollBox)
    self.dynamicSBoxElementHandler = UiDynamicScrollBoxElementNotificationBus.Connect(self, self.Properties.DynamicScrollBox)

    self.tickBusHandler = TickBus.Connect(self);
end

function DynamicScrollBox:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()

    local canvas = UiElementBus.Event.GetCanvas(self.entityId)
    self.canvasNotificationBusHandler = UiCanvasNotificationBus.Connect(self, canvas)        
                        
    self:InitContent("StaticData/LyShineExamples/uiTestPaidColors.json")
end


function DynamicScrollBox:OnDeactivate()
    self.buttonHandler:Disconnect()
    self.canvasNotificationBusHandler:Disconnect()
    self.dynamicSBoxDataHandler:Disconnect()
    self.dynamicSBoxElementHandler:Disconnect()
end

function DynamicScrollBox:GetNumElements()
    local numColors = UiDynamicContentDatabaseBus.Broadcast.GetNumColors(eUiDynamicContentDBColorType_Paid)
    return numColors
end

function DynamicScrollBox:OnElementBecomingVisible(entityId, index)        
    -- Get the image of the child and set the color
    local image = UiElementBus.Event.FindChildByName(entityId, "Icon")
    local color = UiDynamicContentDatabaseBus.Broadcast.GetColor(eUiDynamicContentDBColorType_Paid, index)
    UiImageBus.Event.SetColor(image, color)
        
    -- Get the name text of the child and set the name
    local nameText = UiElementBus.Event.FindChildByName(entityId, "Name")
    local name = UiDynamicContentDatabaseBus.Broadcast.GetColorName(eUiDynamicContentDBColorType_Paid, index)
    UiTextBus.Event.SetText(nameText, name)
        
    -- Get the price text of the child and set the price
    local priceText = UiElementBus.Event.FindChildByName(entityId, "Price")
    local price = UiDynamicContentDatabaseBus.Broadcast.GetColorPrice(eUiDynamicContentDBColorType_Paid, index)
    UiTextBus.Event.SetText(priceText, price)        
end

function DynamicScrollBox:OnAction(entityId, actionName)
    if actionName == "IconClicked" then
        -- Set selected color
        index = UiDynamicScrollBoxBus.Event.GetLocationIndexOfChild(self.Properties.DynamicScrollBox, entityId)
        local color = UiDynamicContentDatabaseBus.Broadcast.GetColor(eUiDynamicContentDBColorType_Paid, index)
        UiImageBus.Event.SetColor(self.Properties.ColorImage, color)
        
        -- Set selected index
        UiTextBus.Event.SetText(self.Properties.ColorIndexText, index)
    end
end

function DynamicScrollBox:OnButtonClick()
    if (UiButtonNotificationBus.GetCurrentBusId() == self.Properties.AddColorsButton) then
        self:InitContent("StaticData/LyShineExamples/uiTestMorePaidColors.json")

        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.AddColorsButton, false)
    end    
end

function DynamicScrollBox:InitContent(jsonFilepath)
    -- Refresh the dynamic content database with the specified json file
    UiDynamicContentDatabaseBus.Broadcast.Refresh(eUiDynamicContentDBColorType_Paid, jsonFilepath)
    
    -- Refresh the dynamic scrollbox. This will trigger events from the
    -- UiDynamicScrollBoxDataBus and the UiDynamicScrollBoxElementNotificationBus
    UiDynamicScrollBoxBus.Event.RefreshContent(self.Properties.DynamicScrollBox)
    
    -- Force the hover interactable to be the scroll box.
    -- The scroll box is set to auto-activate, but it could still have the hover
    -- since it starts out having no children. Now that it may contain children,
    -- force it to be the hover in order to auto-activate it and pass the hover to its child
    local canvas = UiElementBus.Event.GetCanvas(self.entityId)
    UiCanvasBus.Event.ForceHoverInteractable(canvas, self.Properties.DynamicScrollBox)
end

return DynamicScrollBox
