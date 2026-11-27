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

local LoadCanvasButton = 
{
    Properties = 
    {
        canvasName = ""
    },
}

function LoadCanvasButton:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
end

function LoadCanvasButton:OnDeactivate()
    self.buttonHandler:Disconnect()
end

function LoadCanvasButton:OnButtonClick()
    UiCanvasManagerBus.Broadcast.LoadCanvas(self.Properties.canvasName)
end

return LoadCanvasButton
