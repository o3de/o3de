-- 
-- 
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
-- 

local EntityLookAt = 
{
    Properties = 
    {
        EntityTargetToFace = {default=EntityId()},
        Advanced = {default="Edit this script to face a different axis towards the target"},
    },
    Handlers =
    {
        MyTransform = {},
        TargetTransform = {},
    }
}

function EntityLookAt:OnActivate()
    if self.Properties.EntityTargetToFace:IsValid() then
        -- cache properties
        local myTM = TransformBus.Event.GetWorldTM(self.entityId)
        self.myPos = myTM:GetPosition()
        self.targetPos = TransformBus.Event.GetWorldTranslation(self.Properties.EntityTargetToFace)
        self.myScale = myTM:RetrieveScale()
        self.readyForRotation = true
        --attach handlers
        self.myTransform = TransformNotificationBus.Connect(self.Handlers.MyTransform, self.entityId)
        self.targetTransform = TransformNotificationBus.Connect(self.Handlers.TargetTransform, self.Properties.EntityTargetToFace)
        self.Handlers.MyTransform.parent = self
        self.Handlers.TargetTransform.parent = self
        self:UpdateFacing()
    end
end

function EntityLookAt.Handlers.TargetTransform:OnTransformChanged(localTM, worldTM)
    local distance = self.parent.targetPos - worldTM:GetPosition()
    -- if our target has moved, Update our facing
    if not distance:IsZero(0.0001) then
        self.parent.targetPos = worldTM:GetPosition()
        self.parent:UpdateFacing()
    end
end

function EntityLookAt.Handlers.MyTransform:OnTransformChanged(localTM, worldTM)
    -- cache the scale
    self.parent.myScale = worldTM:RetrieveScale()

    local distance = self.parent.myPos - worldTM:GetPosition()
    -- if we've moved then update our facing
    if self.parent.readyForRotation then
        self.parent.myPos = worldTM:GetPosition()
        self.parent:UpdateFacing()
    end
end

function EntityLookAt:UpdateFacing()
    self.readyForRotation = false
    -- you can use Model Space relative AxisType's of 
    -- YPositive : face directly at targetPos, 
    -- YNegative : face directly away from targetPos, 
    -- XPositive : "right shoulder" faces targetPos, 
    -- XNegative : "left shoulder" faces targetPos, 
    -- ZPositive : top faces targetPos
    -- ZNegative : bottom faces targetPos
    self.axisToFaceAtTarget = AxisType.YPositive
    local tm = MathUtils.CreateLookAt(self.myPos, self.targetPos, self.axisToFaceAtTarget)
    tm:MultiplyByScale(self.myScale)
    TransformBus.Event.SetWorldTM(self.entityId, tm)    
    self.readyForRotation = true
end

function EntityLookAt:OnDeactivate()
    self.Handlers.MyTransform = {}
    self.Handlers.TargetTransform = {}
    self.myTransform = nil
    self.targetTransform = nil
end

return EntityLookAt
