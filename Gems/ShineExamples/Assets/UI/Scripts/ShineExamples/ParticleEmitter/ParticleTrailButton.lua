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

local ParticleTrailButton = 
{
    Properties = 
    {
        SequenceName = {default = ""},
        ButtonParticlesRoot = {default = EntityId()},
    },
}

function ParticleTrailButton:OnActivate()
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.entityId)
    self.tickBusHandler = TickBus.Connect(self);    
    
    self.canvas = UiElementBus.Event.GetCanvas(self.entityId)
end

function ParticleTrailButton:OnUiAnimationEvent(eventType, sequenceName)
    if (eventType == eUiAnimationEvent_Stopped) then
        UiInteractableBus.Event.SetIsHandlingEvents(self.entityId, true)
    end
end

function ParticleTrailButton:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()
    
    self.canvas = UiElementBus.Event.GetCanvas(self.entityId)
    self.canvasNotificationBusHandler = UiCanvasNotificationBus.Connect(self, self.canvas)
    self.animHandler = UiAnimationNotificationBus.Connect(self, self.canvas)
end

function ParticleTrailButton:OnDeactivate()
    self.buttonHandler:Disconnect()
    self.canvasNotificationBusHandler:Disconnect()
    self.animHandler:Disconnect()
end

function ParticleTrailButton:OnButtonClick()
    -- Animate particle trails
    UiAnimationBus.Event.StartSequence(self.canvas, self.Properties.SequenceName)
    local children = UiElementBus.Event.GetChildren(self.Properties.ButtonParticlesRoot)
    for i = 1,#children do
        UiParticleEmitterBus.Event.SetIsEmitting(children[i], true)
    end
    UiInteractableBus.Event.SetIsHandlingEvents(self.entityId, false)
end


return ParticleTrailButton
