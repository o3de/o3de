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
local MoveEntity = 
{
    Properties = 
    {
        IncomingGameplayEventName = "",
        IsRelative = {default=true, description="When true, the velocity will be applied relative to the entity's transform"},
        VelocityScale = {default=1.0, description="This will scale the vector before sending it"},
    },

}


function MoveEntity:OnActivate()
    local gameplayBusId = GameplayNotificationId(self.entityId, self.Properties.IncomingGameplayEventName, "Vector3")
    self.gameplayBus = GameplayNotificationBus.Connect(self, gameplayBusId)
end

function MoveEntity:SetVelocity(vectorValue)
    if self.Properties.IsRelative then
        local localTM = TransformBus.Event.GetWorldTM(self.entityId)
        localTM:SetTranslation(Vector3(0,0,0))
        localTM:ExtractScale()
        PhysicsComponentRequestBus.Event.SetVelocity(self.entityId, localTM * (vectorValue*self.Properties.VelocityScale))
    else
        PhysicsComponentRequestBus.Event.SetVelocity(self.entityId, vectorValue*self.Properties.VelocityScale)
    end
end

function MoveEntity:OnEventBegin(vectorValue)
    self:SetVelocity(vectorValue)
end

function MoveEntity:OnEventUpdating(vectorValue)
    self:SetVelocity(vectorValue)
end

function MoveEntity:OnEventEnd(vectorValue)
    self:SetVelocity(vectorValue)
end

function MoveEntity:OnDeactivate()
    self.gameplayBus:Disconnect()
end

return MoveEntity