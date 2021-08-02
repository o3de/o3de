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
local AddPhysicsImpulse = 
{
    Properties = 
    {
        IncomingGameplayEventName = "",
        IsRelative = {default=true, description="When true, the impulse will be applied relative to the entity's transform"},
        ImpulseScale = {default=1.0, description="This will scale the vector before sending it"},
    },

}


function AddPhysicsImpulse:OnActivate()
    local gameplayBusId = GameplayNotificationId(self.entityId, self.Properties.IncomingGameplayEventName, "Vector3")
    self.gameplayBus = GameplayNotificationBus.Connect(self, gameplayBusId)
end

function AddPhysicsImpulse:AddImpulse(vectorValue)
    if self.Properties.IsRelative then
        local localTM = TransformBus.Event.GetWorldTM(self.entityId)
        localTM:SetTranslation(Vector3(0,0,0))
        localTM:ExtractScale()
        PhysicsComponentRequestBus.Event.AddImpulse(self.entityId, localTM * (vectorValue*self.Properties.ImpulseScale))
    else
        PhysicsComponentRequestBus.Event.AddImpulse(self.entityId, vectorValue*self.Properties.ImpulseScale)
    end
end

function AddPhysicsImpulse:OnEventBegin(vectorValue)
    self:AddImpulse(vectorValue)
end

function AddPhysicsImpulse:OnEventUpdating(vectorValue)
    self:AddImpulse(vectorValue)
end

function AddPhysicsImpulse:OnEventEnd(vectorValue)
    self:AddImpulse(vectorValue)
end

function AddPhysicsImpulse:OnDeactivate()
    self.gameplayBus:Disconnect()
end

return AddPhysicsImpulse
