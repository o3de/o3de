----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
local gameplayMultiHandler = require('Scripts.Utils.Components.GameplayUtils')
local vectorized_combination = 
{
    Properties = 
    {
        XCoordEventName = "",
        YCoordEventName = "",
        ZCoordEventName = "",
        OutgoingGameplayEventName = "",
        DeadZoneLength = 0.2,
    },
}

function vectorized_combination:OnActivate()
    self.vectorizedValue = Vector3(0,0,0)
    self.active = false
    self.outgoingId = GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName, "Vector3")
    self.vectorizedHandlers = gameplayMultiHandler.ConnectMultiHandlers{
        [GameplayNotificationId(self.entityId, self.Properties.XCoordEventName, "float")] = {
            OnEventBegin = function(floatValue) self:UpdateX(floatValue) end,
            OnEventUpdating = function(floatValue) self:UpdateX(floatValue) end,
            OnEventEnd = function(floatValue) self:UpdateX(0) end,
        },
        [GameplayNotificationId(self.entityId, self.Properties.YCoordEventName, "float")] = {
            OnEventBegin = function(floatValue) self:UpdateY(floatValue) end,
            OnEventUpdating = function(floatValue) self:UpdateY(floatValue) end,
            OnEventEnd = function(floatValue) self:UpdateY(0) end,
        },
        [GameplayNotificationId(self.entityId, self.Properties.ZCoordEventName, "float")] = {
            OnEventBegin = function(floatValue) self:UpdateZ(floatValue) end,
            OnEventUpdating = function(floatValue) self:UpdateZ(floatValue) end,
            OnEventEnd = function(floatValue) self:UpdateZ(0) end,
        },
    }
    
    -- deprecated bus id's will be removed in 1.12
    -------------------------------------------------------------------------
    self.deprecatedOutgoingId = GameplayNotificationId(self.entityId, self.Properties.OutgoingGameplayEventName)
    self.deprecatedVectorizedHandlers = gameplayMultiHandler.ConnectMultiHandlers{
        [GameplayNotificationId(self.entityId, self.Properties.XCoordEventName)] = {
            OnEventBegin = function(floatValue) self:UpdateX(floatValue) end,
            OnEventUpdating = function(floatValue) self:UpdateX(floatValue) end,
            OnEventEnd = function(floatValue) self:UpdateX(0) end,
        },
        [GameplayNotificationId(self.entityId, self.Properties.YCoordEventName)] = {
            OnEventBegin = function(floatValue) self:UpdateY(floatValue) end,
            OnEventUpdating = function(floatValue) self:UpdateY(floatValue) end,
            OnEventEnd = function(floatValue) self:UpdateY(0) end,
        },
        [GameplayNotificationId(self.entityId, self.Properties.ZCoordEventName)] = {
            OnEventBegin = function(floatValue) self:UpdateZ(floatValue) end,
            OnEventUpdating = function(floatValue) self:UpdateZ(floatValue) end,
            OnEventEnd = function(floatValue) self:UpdateZ(0) end,
        },
    }
    -------------------------------------------------------------------------
end

function vectorized_combination:OnDeactivate()
    self.vectorizedHandlers:Disconnect()
    self.deprecatedVectorizedHandlers:Disconnect()
end

function vectorized_combination:SendGameplayEvent()
    local shouldBeActive = self.vectorizedValue:GetLength() > self.Properties.DeadZoneLength
    if shouldBeActive and not self.active then
        GameplayNotificationBus.Event.OnEventBegin(self.outgoingId, self.vectorizedValue, "float")
    elseif shouldBeActive and self.active then
        GameplayNotificationBus.Event.OnEventUpdating(self.outgoingId, self.vectorizedValue, "float")
    elseif not shouldBeActive and self.active then
        GameplayNotificationBus.Event.OnEventEnd(self.outgoingId, self.vectorizedValue, "float")
    end
    self.active = shouldBeActive
    -- otherwise we were not active and shouldn't be active so do nothing
end

-- X coordinate handlers
function vectorized_combination:UpdateX(floatValue)
    self.vectorizedValue.x = floatValue
    self:SendGameplayEvent()
end

-- Y coordinate handlers
function vectorized_combination:UpdateY(floatValue)
    self.vectorizedValue.y = floatValue
    self:SendGameplayEvent()
end

-- Z coordinate handlers
function vectorized_combination:UpdateZ(floatValue)
    self.vectorizedValue.z = floatValue
    self:SendGameplayEvent()
end



--deprecated methods will be removed in 1.12
-------------------------------------------------------------------------
function vectorized_combination:DeprecatedSendGameplayEvent()
    local shouldBeActive = self.vectorizedValue:GetLength() > self.Properties.DeadZoneLength
    if shouldBeActive and not self.active then
        GameplayNotificationBus.Event.OnEventBegin(self.deprecatedOutgoingId, self.vectorizedValue)
    elseif shouldBeActive and self.active then
        GameplayNotificationBus.Event.OnEventUpdating(self.deprecatedOutgoingId, self.vectorizedValue)
    elseif not shouldBeActive and self.active then
        GameplayNotificationBus.Event.OnEventEnd(self.deprecatedOutgoingId, self.vectorizedValue)
    end
    self.active = shouldBeActive
    -- otherwise we were not active and shouldn't be active so do nothing
end

-- X coordinate handlers
function vectorized_combination:DeprecatedUpdateX(floatValue)
    self.vectorizedValue.x = floatValue
    self:DeprecatedSendGameplayEvent()
end

-- Y coordinate handlers
function vectorized_combination:DeprecatedUpdateY(floatValue)
    self.vectorizedValue.y = floatValue
    self:DeprecatedSendGameplayEvent()
end

-- Z coordinate handlers
function vectorized_combination:DeprecatedUpdateZ(floatValue)
    self.vectorizedValue.z = floatValue
    self:DeprecatedSendGameplayEvent()
end
-------------------------------------------------------------------------

return vectorized_combination
