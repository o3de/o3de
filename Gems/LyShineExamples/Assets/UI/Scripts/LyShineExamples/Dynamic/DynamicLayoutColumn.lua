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

local DynamicLayoutColumn = 
{
    Properties = 
    {
        ScrollBox = {default = EntityId()},
        DynamicLayout = {default = EntityId()},
        AddColorsButton = {default = EntityId()},
        ColorImage = {default = EntityId()},
    },
}

function DynamicLayoutColumn:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.Properties.AddColorsButton)

    self.tickBusHandler = TickBus.Connect(self);
end

function DynamicLayoutColumn:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()

    local canvas = UiElementBus.Event.GetCanvas(self.entityId)
    self.canvasNotificationBusHandler = UiCanvasNotificationBus.Connect(self, canvas)        
                        
    self:InitContent("StaticData/LyShineExamples/uiTestFreeColors.json")
end


function DynamicLayoutColumn:OnDeactivate()
    self.buttonHandler:Disconnect()
    self.canvasNotificationBusHandler:Disconnect()
end

function DynamicLayoutColumn:OnAction(entityId, actionName)
    if actionName == "ColorClicked" then
        -- Get the index of the child that was pressed
        index = UiElementBus.Event.GetIndexOfChildByEntityId(self.Properties.DynamicLayout, entityId)
        
        -- Get the color at the specified index and set it on the image
        local color = UiDynamicContentDatabaseBus.Broadcast.GetColor(eUiDynamicContentDBColorType_Free, index)
        UiImageBus.Event.SetColor(self.Properties.ColorImage, color)
    end
end

function DynamicLayoutColumn:OnButtonClick()
    if (UiButtonNotificationBus.GetCurrentBusId() == self.Properties.AddColorsButton) then
        self:InitContent("StaticData/LyShineExamples/uiTestMoreFreeColors.json")

        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.AddColorsButton, false)
    end    
end

function DynamicLayoutColumn:InitContent(jsonFilepath)
    -- Refresh the dynamic content database with the specified json file
    UiDynamicContentDatabaseBus.Broadcast.Refresh(eUiDynamicContentDBColorType_Free, jsonFilepath)
    
    -- Set the number of children for the layout element based on the number of colors in the dynamic content database
    local numColors = UiDynamicContentDatabaseBus.Broadcast.GetNumColors(eUiDynamicContentDBColorType_Free)
    UiDynamicLayoutBus.Event.SetNumChildElements(self.Properties.DynamicLayout, numColors)

    for i=0,numColors-1 do
        -- Get child of the layout
        local child = UiElementBus.Event.GetChild(self.Properties.DynamicLayout, i)
        
        -- Get the image of the child and set the color
        local image = UiElementBus.Event.FindChildByName(child, "Color")
        local color = UiDynamicContentDatabaseBus.Broadcast.GetColor(eUiDynamicContentDBColorType_Free, i)
        UiImageBus.Event.SetColor(image, color)
        
        -- Get the text of the child and set the name
        local text = UiElementBus.Event.FindChildByName(child, "Name")
        local name = UiDynamicContentDatabaseBus.Broadcast.GetColorName(eUiDynamicContentDBColorType_Free, i)
        UiTextBus.Event.SetText(text, name)
    end
    
    -- Force the hover interactable to be the scroll box.
    -- The scroll box is set to auto-activate, but it could still have the hover
    -- since it starts out having no children. Now that it may contain children,
    -- force it to be the hover in order to auto-activate it and pass the hover to its child.
    -- Ensure that the layouts of the newly added children are up to date by forcing an
    -- immediate recompute. This is necessary for the scroll box to correctly determine
    -- which of its children should get the hover
    local canvas = UiElementBus.Event.GetCanvas(self.entityId)
    UiCanvasBus.Event.RecomputeChangedLayouts(canvas)
    UiCanvasBus.Event.ForceHoverInteractable(canvas, self.Properties.ScrollBox)
end

return DynamicLayoutColumn
