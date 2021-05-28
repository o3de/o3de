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
local held = 
{
    Properties = 
    {
        IncomingInputEventName = "",
        OutgoingGameplayEventName = "",
    },

}

function held:OnActivate()
    local inputBusId = InputEventNotificationId(self.Properties.IncomingInputEventName)
    self.inputBus = InputEventNotificationBus.Connect(self, inputBusId)
end

function held:OnPressed(floatValue)
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "float"), floatValue)
end

function held:OnHeld(floatValue)
    GameplayNotificationBus.Event.OnEventUpdating(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "float"), floatValue)
end

function held:OnReleased(floatValue)
    GameplayNotificationBus.Event.OnEventEnd(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "float"), floatValue)
end

function held:OnDeactivate()
    self.inputBus:Disconnect()
end

return held
