----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
--
----------------------------------------------------------------------------------------------------

local ResetBall = 
{
    Properties = 
    {
        Ball = {default = EntityId()},
    },
}

function ResetBall:OnActivate()
    self.tickHandler = TickBus.Connect(self)
end

function ResetBall:OnTick()
    self.tickHandler:Disconnect()

    self.canvasNotificationBusHandler = UiCanvasNotificationBus.Connect(self, UiElementBus.Event.GetCanvas(self.entityId))
    
    -- Save the default properties of the ball
    self.defaultOffsets = UiTransform2dBus.Event.GetOffsets(self.Properties.Ball)
    self.defaultColor = UiImageBus.Event.GetColor(self.Properties.Ball)
end

function ResetBall:OnAction(entityId, actionName)
    -- This action is sent when the create button is clicked and when the reset button is clicked
    if (actionName == "ResetBall") then
        UiTransform2dBus.Event.SetOffsets(self.Properties.Ball, self.defaultOffsets)
        UiImageBus.Event.SetColor(self.Properties.Ball, self.defaultColor)
    end
end

function ResetBall:OnDeactivate()
    self.canvasNotificationBusHandler:Disconnect()
end

return ResetBall
