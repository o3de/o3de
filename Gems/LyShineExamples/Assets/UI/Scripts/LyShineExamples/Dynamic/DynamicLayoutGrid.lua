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

local DynamicLayoutGrid = 
{
    Properties = 
    {
        FirstScrollBox = {default = EntityId()},
        AddColorsButton = {default = EntityId()},
        DynamicLayouts = { default = { EntityId(), EntityId() } },
    },
}

function DynamicLayoutGrid:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.Properties.AddColorsButton)

    self.tickBusHandler = TickBus.Connect(self);
end

function DynamicLayoutGrid:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()    
                        
    self:InitContent("StaticData/LyShineExamples/uiTestFreeColors.json")
end


function DynamicLayoutGrid:OnDeactivate()
    self.buttonHandler:Disconnect()
end

function DynamicLayoutGrid:OnButtonClick()
    if (UiButtonNotificationBus.GetCurrentBusId() == self.Properties.AddColorsButton) then
        self:InitContent("StaticData/LyShineExamples/uiTestMoreFreeColors.json")

        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.AddColorsButton, false)
        
        local canvas = UiElementBus.Event.GetCanvas(self.entityId)
        UiCanvasBus.Event.ForceHoverInteractable(canvas, self.Properties.FirstScrollBox)        
    end    
end

function DynamicLayoutGrid:InitContent(jsonFilepath)
    -- Refresh the dynamic content database with the specified json file
    UiDynamicContentDatabaseBus.Broadcast.Refresh(eUiDynamicContentDBColorType_Free, jsonFilepath)
    
    -- Get the number of colors in the dynamic content database
    local numColors = UiDynamicContentDatabaseBus.Broadcast.GetNumColors(eUiDynamicContentDBColorType_Free)
    
    for i=0,#self.Properties.DynamicLayouts do
        -- Set the number of children for the layout element
        UiDynamicLayoutBus.Event.SetNumChildElements(self.Properties.DynamicLayouts[i], numColors)
    
        for j=0,numColors-1 do
            -- Get child of the layout
            local child = UiElementBus.Event.GetChild(self.Properties.DynamicLayouts[i], j)
            
            -- Set the color
            local color = UiDynamicContentDatabaseBus.Broadcast.GetColor(eUiDynamicContentDBColorType_Free, j)
            UiImageBus.Event.SetColor(child, color)        
        end
    end
end

return DynamicLayoutGrid
