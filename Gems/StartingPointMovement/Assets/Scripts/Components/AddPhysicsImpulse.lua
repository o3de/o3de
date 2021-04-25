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
