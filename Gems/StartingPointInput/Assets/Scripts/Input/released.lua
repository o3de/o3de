----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
local released = 
{
    Properties = 
    {
        IncomingInputEventName = "",
        OutgoingGameplayEventName = "",
    },
}

function released:OnActivate()
    local inputBusId = InputEventNotificationId(self.Properties.IncomingInputEventName)
    self.inputBus = InputEventNotificationBus.Connect(self, inputBusId)
end

function released:OnPressed(floatValue)
    GameplayNotificationBus.Event.OnEventEnd(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "float"), floatValue)
end

function released:OnHeld(floatValue)
end

function released:OnReleased(floatValue)
    GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "float"), floatValue)
end

function released:OnDeactivate()
    self.inputBus:Disconnect()
end

return released
