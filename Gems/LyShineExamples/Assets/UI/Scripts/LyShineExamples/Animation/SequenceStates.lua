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

local SequenceStates = 
{
    Properties = 
    {
        SequenceName = {default = ""},
        StateText = {default = EntityId()},
        StartButton = {default = EntityId()},
        PauseButton = {default = EntityId()},
        ResumeButton = {default = EntityId()},
        StopButton = {default = EntityId()},
        AbortButton = {default = EntityId()},
        ResetButton = {default = EntityId()},
    },
}

function SequenceStates:OnActivate()
    self.tickBusHandler = TickBus.Connect(self);    
end

function SequenceStates:OnTick(deltaTime, timePoint)
    self.tickBusHandler:Disconnect()

    self.canvas = UiElementBus.Event.GetCanvas(self.entityId)
    self.canvasNotificationBusHandler = UiCanvasNotificationBus.Connect(self, self.canvas)
    
    self.animHandler = UiAnimationNotificationBus.Connect(self, self.canvas)
    
    -- Initialize button states
    UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, true)
    UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.PauseButton, false)
    UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResumeButton, false)
    UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, false)
    UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.AbortButton, false)
    UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResetButton, false)                                            
end


function SequenceStates:OnDeactivate()
    self.canvasNotificationBusHandler:Disconnect()
    self.animHandler:Disconnect()
end

function SequenceStates:OnAction(entityId, actionName)
    if actionName == "StartPressed" then
        -- Update button states
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.PauseButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResumeButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.AbortButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResetButton, false)        
    
        -- Start sequence
        UiAnimationBus.Event.StartSequence(self.canvas, self.Properties.SequenceName)
    elseif actionName == "PausePressed" then
        -- Update button states
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.PauseButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResumeButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.AbortButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResetButton, false)    
    
        -- Pause sequence
        UiAnimationBus.Event.PauseSequence(self.canvas, self.Properties.SequenceName)
    elseif actionName == "ResumePressed" then
        -- Update button states
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.PauseButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResumeButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.AbortButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResetButton, false)    
    
        -- Resume sequence
        UiAnimationBus.Event.ResumeSequence(self.canvas, self.Properties.SequenceName)    
    elseif actionName == "StopPressed" then    
        -- Stop sequence. Button states are updated in the animation handler    
        UiAnimationBus.Event.StopSequence(self.canvas, self.Properties.SequenceName)
    elseif actionName == "AbortPressed" then
        -- Abort sequence. Buttons states are updated in the animation handler
        UiAnimationBus.Event.AbortSequence(self.canvas, self.Properties.SequenceName)
    elseif actionName == "ResetPressed" then
        -- Update button states
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.PauseButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResumeButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.AbortButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResetButton, false)    

        -- Reset sequence
        UiAnimationBus.Event.ResetSequence(self.canvas, self.Properties.SequenceName)
    end
end

function SequenceStates:OnUiAnimationEvent(eventType, sequenceName)
    if (eventType == eUiAnimationEvent_Started) then
        -- Update state text
        UiTextBus.Event.SetText(self.Properties.StateText, "Sequence Started")
    elseif (eventType == eUiAnimationEvent_Stopped) then
        -- Update button states
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.PauseButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResumeButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.AbortButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResetButton, true)    
    
        -- Update state text
        UiTextBus.Event.SetText(self.Properties.StateText, "Sequence Stopped")
    elseif (eventType == eUiAnimationEvent_Aborted) then
        -- Update button states
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.PauseButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResumeButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.AbortButton, false)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.ResetButton, true)        
        
        -- Update state text
        UiTextBus.Event.SetText(self.Properties.StateText, "Sequence Aborted")
    elseif (eventType == eUiAnimationEvent_Updated) then
        -- Update state text
        UiTextBus.Event.SetText(self.Properties.StateText, "Sequence Playing")
    end
end

return SequenceStates
