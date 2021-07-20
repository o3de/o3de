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
local RotateEntity = 
{
    Properties = 
    {
        IncomingGameplayEventName = "",
        AxisOfRotation = {default=Vector3(0,0,1), description="The axis to rotate around"},
        IsRelative = {default=true, description="When true, the entity's transform will transform the AxisOfRotation"},
        RotationSpeed = {default=60, description="Degrees per second."},
    },

}


function RotateEntity:OnActivate()
    local gameplayBusId = GameplayNotificationId(self.entityId, self.Properties.IncomingGameplayEventName, "float")
    self.gameplayBus = GameplayNotificationBus.Connect(self, gameplayBusId)
end

function RotateEntity:ApplyRotation(floatValue)
    local axisOfRotation = self.Properties.AxisOfRotation:GetNormalized()
    local localTM = TransformBus.Event.GetWorldTM(self.entityId)
    local translation = localTM:GetTranslation()
    localTM:SetTranslation(Vector3(0,0,0))
    local scale = localTM:ExtractScale()
    
    if not self.Properties.IsRelative then
        axisOfRotation = axisOfRotation * localTM
    end
    
    local dt = TickRequestBus.Broadcast.GetTickDeltaTime()
    local rotationAmount = floatValue * dt * Math.DegToRad(self.Properties.RotationSpeed)
    
    local finalTM = localTM * Transform.CreateFromQuaternion(Quaternion.CreateFromAxisAngle(axisOfRotation, rotationAmount):GetNormalized())
    finalTM:SetPosition(translation)
    finalTM:MultiplyByScale(scale)
    TransformBus.Event.SetWorldTM(self.entityId, finalTM)
end

function RotateEntity:OnEventBegin(floatValue)
    self:ApplyRotation(floatValue)
end

function RotateEntity:OnEventUpdating(floatValue)
    self:ApplyRotation(floatValue)
end

function RotateEntity:OnEventEnd(floatValue)
    self:ApplyRotation(floatValue)
end

function RotateEntity:OnDeactivate()
    self.gameplayBus:Disconnect()
end

return RotateEntity
