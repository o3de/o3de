----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------

local ButtonAnimation = 
{
    Properties = 
    {
        IdleAnimName = {default = ""},
        HoverAnimName =  {default = ""},
        ExitAnimName =  {default = ""},
        Button = {default = EntityId()},
    },
    
    Exiting = false,
}

function ButtonAnimation:OnActivate()
    self.interactableHandler = UiInteractableNotificationBus.Connect(self, self.Properties.Button)
    self.buttonHandler = UiButtonNotificationBus.Connect(self, self.Properties.Button)

    self.tickBusHandler = TickBus.Connect(self)
    self.hovering = false
end

function ButtonAnimation:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()

    self.canvas = UiElementBus.Event.GetCanvas(self.entityId)

    self.animHandler = UiAnimationNotificationBus.Connect(self, self.canvas)
    
    -- Start the button sequence
    if (self.hovering) then
        UiAnimationBus.Event.StartSequence(self.canvas, self.Properties.HoverAnimName)
    else
        UiAnimationBus.Event.StartSequence(self.canvas, self.Properties.IdleAnimName)
    end
end


function ButtonAnimation:OnDeactivate()
    self.interactableHandler:Disconnect()
    self.buttonHandler:Disconnect()
    self.animHandler:Disconnect()
end

function ButtonAnimation:OnHoverStart()
    self.hovering = true
    if (self.Exiting == false and self.canvas ~= nil) then
        UiAnimationBus.Event.StopSequence(self.canvas, self.Properties.IdleAnimName)
        UiAnimationBus.Event.StartSequence(self.canvas, self.Properties.HoverAnimName)
    end
end

function ButtonAnimation:OnHoverEnd()
    self.hovering = false
    if (self.Exiting == false and self.canvas ~= nil) then
        UiAnimationBus.Event.StopSequence(self.canvas, self.Properties.HoverAnimName)
        UiAnimationBus.Event.StartSequence(self.canvas, self.Properties.IdleAnimName)
    end
end

function ButtonAnimation:OnButtonClick()
    if (UiButtonNotificationBus.GetCurrentBusId() == self.Properties.Button) then    
        UiAnimationBus.Event.StopSequence(self.canvas, self.Properties.HoverAnimName)
        UiAnimationBus.Event.StartSequence(self.canvas, self.Properties.ExitAnimName)
        
        self.Exiting = true
    end    
end

function ButtonAnimation:OnUiAnimationEvent(eventType, sequenceName)
    if (eventType == eUiAnimationEvent_Stopped) then
        if (sequenceName == self.Properties.ExitAnimName) then
            UiAnimationBus.Event.ResetSequence(self.canvas, self.Properties.ExitAnimName)

            if (self.hovering) then
                UiAnimationBus.Event.StartSequence(self.canvas, self.Properties.HoverAnimName)
            else
                UiAnimationBus.Event.StartSequence(self.canvas, self.Properties.IdleAnimName)
            end                        
            
            self.Exiting = false
        end
    end
end

return ButtonAnimation
