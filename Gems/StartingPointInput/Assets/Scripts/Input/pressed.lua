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
local pressed = 
{
    Properties = 
    {
        IncomingInputEventName = "",
        OutgoingGameplayEventName = "",
    },
}

function pressed:OnActivate()
    local inputBusId = InputEventNotificationId(self.Properties.IncomingInputEventName)
    self.inputBus = InputEventNotificationBus.Connect(self, inputBusId)
end

function pressed:OnPressed(floatValue)
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "float"), floatValue)
end

function pressed:OnHeld(floatValue)
end

function pressed:OnReleased(floatValue)
    GameplayNotificationBus.Event.OnEventEnd(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "float"), floatValue)
end

function pressed:OnDeactivate()
    self.inputBus:Disconnect()
end

return pressed
