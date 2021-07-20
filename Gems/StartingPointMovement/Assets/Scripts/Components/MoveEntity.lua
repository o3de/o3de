----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
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
