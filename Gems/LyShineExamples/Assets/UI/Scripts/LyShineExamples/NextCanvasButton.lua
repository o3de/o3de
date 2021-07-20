----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local NextCanvasButton = 
{
    Properties = 
    {
        canvasName = ""
    },
}

function NextCanvasButton:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
end

function NextCanvasButton:OnDeactivate()
    self.buttonHandler:Disconnect()
end

function NextCanvasButton:OnButtonClick()
    -- Load the next canvas
    UiCanvasManagerBus.Broadcast.LoadCanvas(self.Properties.canvasName)
    -- Unload the current canvas
    canvasId = UiElementBus.Event.GetCanvas(self.entityId)
    if (canvasId:IsValid()) then
        UiCanvasManagerBus.Broadcast.UnloadCanvas(canvasId)
    end
end

return NextCanvasButton
