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
local gameplayMultiHandler = require('Scripts.Utils.Components.GameplayUtils')

local ordered_event_combination = 
{
    Properties = 
    {
        IncomingGameplayEventNames = {"",},
        OutgoingGameplayEventName = "",
        TimeToCompleteAllEvents = 1,
    },
    BusIds = {},
    DeprecatedBusIds = {},
    Handlers = {},
    DeprecatedHandlers = {},
}

function ordered_event_combination:OnActivate()
    for nameCount = 1, #self.Properties.IncomingGameplayEventNames do
        self.BusIds[nameCount] = GameplayNotificationId(self.entityId, self.Properties.IncomingGameplayEventNames[nameCount], "float")
        self.Handlers[nameCount] = gameplayMultiHandler.ConnectMultiHandlers{
            [self.BusIds[nameCount]] = {
            OnEventBegin = function(floatValue) self:OnEventBegin(floatValue) end,
            },
        }

        -- support for deprecated APIs.  This will be removed in 1.12
        self.DeprecatedBusIds[nameCount] = GameplayNotificationId(self.entityId, self.Properties.IncomingGameplayEventNames[nameCount])
        self.DeprecatedHandlers[nameCount] = gameplayMultiHandler.ConnectMultiHandlers{
            [self.DeprecatedBusIds[nameCount]] = {
            OnEventBegin = function(floatValue) self:OnDeprecatedHandlerEventBegin(floatValue) end,
            },
        }
    end
    
    self.nextExpectedEventIndex = 1
    self.nextExpectedDeprecatedEventIndex = 1
    self.accumulatedTime = 0
end

function ordered_event_combination:OnDeactivate()
    for nameCount = 1, #self.Properties.IncomingGameplayEventNames do
        self.Handlers[nameCount]:Disconnect()
        self.DeprecatedHandlers[nameCount]:Disconnect()
    end
    self.TickHandler:Disconnect()
end

function ordered_event_combination:OnDeprecatedHandlerEventBegin(floatValue)
    self.TickHandler = TickBus.Connect(self, 0)
    if GameplayNotificationBus.GetCurrentBusId() == self.DeprecatedBusIds[self.nextExpectedDeprecatedEventIndex] then
        self.nextExpectedDeprecatedEventIndex = self.nextExpectedDeprecatedEventIndex + 1
        if (self.nextExpectedDeprecatedEventIndex > #self.Properties.IncomingGameplayEventNames) then
            self:Reset(true, floatValue)
        end
    elseif self.nextExpectedDeprecatedEventIndex > 1 then
        self:Reset(false, 0)
    end
end

function ordered_event_combination:OnEventBegin(floatValue)
    self.TickHandler = TickBus.Connect(self, 0)
    if GameplayNotificationBus.GetCurrentBusId() == self.BusIds[self.nextExpectedEventIndex] then
        self.nextExpectedEventIndex = self.nextExpectedEventIndex + 1
        if (self.nextExpectedEventIndex > #self.Properties.IncomingGameplayEventNames) then
            self:Reset(true, floatValue)
        end
    elseif self.nextExpectedEventIndex > 1 then
        self:Reset(false, 0)
    end
end

function ordered_event_combination:OnTick(dt, scriptTime)
    if self.nextExpectedEventIndex == 1 then 
        self.TickHandler:Disconnect()
        return
    end
    self.accumulatedTime = self.accumulatedTime + dt
    if self.accumulatedTime > self.Properties.TimeToCompleteAllEvents then
        self:Reset(false, 0)
    end
end

function ordered_event_combination:Reset(success, floatValue)
    self.TickHandler:Disconnect()
    self.nextExpectedEventIndex = 1
    self.nextExpectedDeprecatedEventIndex = 1
    self.accumulatedTime = 0
    if success then
        GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName), floatValue)
        GameplayNotificationBus.Event.OnEventBegin(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "float"), floatValue)
    else
        GameplayNotificationBus.Event.OnEventEnd(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName), floatValue)
        GameplayNotificationBus.Event.OnEventEnd(GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "float"), floatValue)
    end
end

return ordered_event_combination
