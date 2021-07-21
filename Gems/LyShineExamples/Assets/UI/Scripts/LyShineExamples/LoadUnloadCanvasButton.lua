----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local LoadUnloadCanvasButton = 
{
    Properties = 
    {
        canvasName = ""
    },
}

function LoadUnloadCanvasButton:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
    self.canvasId = EntityId()
end

function LoadUnloadCanvasButton:OnDeactivate()
    self.buttonHandler:Disconnect()
    if (self.canvasId:IsValid()) then
        UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasId)
        self.canvasId = EntityId()
    end
end

function LoadUnloadCanvasButton:OnButtonClick()
    if (self.canvasId:IsValid()) then
        UiCanvasManagerBus.Broadcast.UnloadCanvas(self.canvasId)
        self.canvasId = EntityId()
    else
        self.canvasId = UiCanvasManagerBus.Broadcast.LoadCanvas(self.Properties.canvasName)
    end
end

return LoadUnloadCanvasButton
