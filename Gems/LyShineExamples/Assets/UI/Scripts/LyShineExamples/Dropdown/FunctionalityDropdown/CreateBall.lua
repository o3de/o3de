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

local CreateBall = 
{
    Properties = 
    {
        Ball = {default = EntityId()},
        OtherButtons = {default = {EntityId()}},
    },
}

function CreateBall:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
    self.tickHandler = TickBus.Connect(self)
end

function CreateBall:OnTick()
    self.tickHandler:Disconnect()
    
    UiElementBus.Event.SetIsEnabled(self.Properties.Ball, false)

    self.canvasNotificationBusHandler = UiCanvasNotificationBus.Connect(self, UiElementBus.Event.GetCanvas(self.entityId))
end

function CreateBall:OnAction(entityId, actionName)
    if (actionName == "BallDestroyed") then
        for i = 0, #self.Properties.OtherButtons do
            UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.OtherButtons[i], false)
        end
    end
end

function CreateBall:OnButtonClick()
    UiElementBus.Event.SetIsEnabled(self.Properties.Ball, true)
    
    for i = 0, #self.Properties.OtherButtons do
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.OtherButtons[i], true)
    end
end

function CreateBall:OnDeactivate()
    self.buttonHandler:Disconnect()
end

return CreateBall
