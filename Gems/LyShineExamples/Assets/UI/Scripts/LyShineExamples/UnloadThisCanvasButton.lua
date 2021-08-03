----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local UnloadThisCanvasButton = 
{
}

function UnloadThisCanvasButton:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
end

function UnloadThisCanvasButton:OnDeactivate()
    self.buttonHandler:Disconnect()
end

function UnloadThisCanvasButton:OnButtonClick()
    -- get canvas name from element
    canvasId = UiElementBus.Event.GetCanvas(self.entityId)
    if (canvasId:IsValid()) then
        UiCanvasManagerBus.Broadcast.UnloadCanvas(canvasId)
    end
end

return UnloadThisCanvasButton
