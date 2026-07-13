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
        FlipbookContentSpriteSheet = {default = EntityId()},
        FlipbookContentImageSequence = {default = EntityId()},
        StateText = {default = EntityId()},
        PlayStateText = {default = EntityId()},
        PlayStateImage = {default = EntityId()},
        StartButton = {default = EntityId()},
        StopButton = {default = EntityId()},
        LoopTypeDropdown = {default = EntityId()},
        ImageTypeDropdown = {default = EntityId()},
        ImageTypeDropdownSpritesheetOption = {default = EntityId()},
        ImageTypeDropdownImageSequenceOption = {default = EntityId()},
        ImageDescription = {default = EntityId()},
    },
}

function SequenceStates:OnActivate()
    self.requiresInit = true
    self.tickBusHandler = TickBus.Connect(self);

    -- By default, we'll use sprite-sheets for flipbooks. The user can switch
    -- to the image sequence version of the flipbook via dropdown.
    self.currentImage = self.Properties.FlipbookContentSpriteSheet
end

function SequenceStates:PrintImageDescription()
    local currentFrame = UiFlipbookAnimationBus.Event.GetCurrentFrame(self.currentImage, 0)
    local zeroPaddedFrameString = string.format("%02d", currentFrame)

    if self.currentImage == self.Properties.FlipbookContentSpriteSheet then
        UiTextBus.Event.SetText(self.Properties.ImageDescription, "flipbook_walking.tif (index " .. zeroPaddedFrameString .. ")")
    else
        UiTextBus.Event.SetText(self.Properties.ImageDescription, "flipbook_walking_" .. zeroPaddedFrameString .. ".png")
    end
end

function SequenceStates:OnTick(deltaTime, timePoint)
    if self.requiresInit == true then
        self.requiresInit = false
        self.numLoops = 0
        self.sequenceStartedString = "Sequence Started"
    
        UiElementBus.Event.SetIsEnabled(self.currentImage, true)
        self.canvas = UiElementBus.Event.GetCanvas(self.entityId)
        
        -- Handler for OnAction callbacks
        self.canvasNotificationBusHandler = UiCanvasNotificationBus.Connect(self, self.canvas)
        
        -- Handler for flipbook animation start/stop callbacks
        self.flipbookNotificationBusHandler = UiFlipbookAnimationNotificationsBus.Connect(self, self.currentImage)
        
        
        -- Initialize button states
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, true)
        UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, false)

        self:PrintImageDescription()

        -- Disconnect here because we want to change the value of the image-type dropdown value
        -- but we don't want to be notified of the change. The first time this logic gets
        -- executed the handler will be nil, however, so we have to guard against that
        if self.imageTypeDropdownNotificationBusHandler ~= nil then
            self.imageTypeDropdownNotificationBusHandler:Disconnect()
        end

        if self.currentImage == self.Properties.FlipbookContentSpriteSheet then
            UiDropdownBus.Event.SetValue(self.Properties.ImageTypeDropdown, self.Properties.ImageTypeDropdownSpritesheetOption)
        else
            UiDropdownBus.Event.SetValue(self.Properties.ImageTypeDropdown, self.Properties.ImageTypeDropdownImageSequenceOption)
        end
        
        -- Handler for dropd-down callbacks
        self.loopTypeDropdownNotificationBusHandler = UiDropdownNotificationBus.Connect(self, self.Properties.LoopTypeDropdown)
        self.imageTypeDropdownNotificationBusHandler = UiDropdownNotificationBus.Connect(self, self.Properties.ImageTypeDropdown)
    else
        local isPlaying = UiFlipbookAnimationBus.Event.IsPlaying(self.currentImage)
        if isPlaying == true then
            UiImageBus.Event.SetColor(self.Properties.PlayStateImage, Color(0, 255, 0))
            UiTextBus.Event.SetColor(self.Properties.PlayStateText, Color(0, 255, 0))

            self:PrintImageDescription()
        else
            UiImageBus.Event.SetColor(self.Properties.PlayStateImage, Color(255, 0, 0))
            UiTextBus.Event.SetColor(self.Properties.PlayStateText, Color(255, 0, 0))
        end
    end                                        
end


function SequenceStates:OnDeactivate()
    self.tickBusHandler:Disconnect()
    self.canvasNotificationBusHandler:Disconnect()
    self.flipbookNotificationBusHandler:Disconnect()
    self.loopTypeDropdownNotificationBusHandler:Disconnect()
    self.imageTypeDropdownNotificationBusHandler:Disconnect()
end

function SequenceStates:OnAction(entityId, actionName)
    if actionName == "StartPressed" then
        UiFlipbookAnimationBus.Event.Start(self.currentImage)
    elseif actionName == "StopPressed" then    
        UiFlipbookAnimationBus.Event.Stop(self.currentImage)
    end
end

function SequenceStates:OnAnimationStarted()
    -- Setup button states
    UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, false)
    UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, true)
    
    -- Set sequence state text
    UiTextBus.Event.SetText(self.Properties.StateText, self.sequenceStartedString)
    
    self.numLoops = 0
end

function SequenceStates:OnAnimationStopped()
    -- Update button states    
    UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StartButton, true)
    UiInteractableBus.Event.SetIsHandlingEvents(self.Properties.StopButton, false)

    -- Set sequence state text
    UiTextBus.Event.SetText(self.Properties.StateText, "Sequence Stopped")
end

function SequenceStates:OnLoopSequenceCompleted()
    self.numLoops = self.numLoops + 1

    local pluralString = ""
    if self.numLoops > 1 then
        pluralString = "s"
    end

    UiTextBus.Event.SetText(self.Properties.StateText, self.sequenceStartedString .. " (" .. tostring(self.numLoops) .. " loop" .. pluralString .. ")")
end

function SequenceStates:OnDropdownValueChanged(optionEntityId)

    -- Note: both the loop and image type dropdowns will execute this function

    local dropdownText = UiDropdownOptionBus.Event.GetTextElement(optionEntityId)
    local textValue = UiTextBus.Event.GetText(dropdownText)

    -- If the image type changes, we need to re-initialize based on the new
    -- image content entity (either sprite-sheet or image sequence)
    local currentLoopType = nil
    if UiDropdownNotificationBus.GetCurrentBusId() == self.Properties.ImageTypeDropdown then
        if (UiFlipbookAnimationBus.Event.IsPlaying(self.currentImage)) then
            UiFlipbookAnimationBus.Event.Stop(self.currentImage)
        end
        self.requiresInit = true
        self.flipbookNotificationBusHandler:Disconnect(self, self.currentImage)
        currentLoopType = UiFlipbookAnimationBus.Event.GetLoopType(self.currentImage)
    end

    if textValue == "None" then
        UiFlipbookAnimationBus.Event.SetLoopType(self.currentImage, eUiFlipbookAnimationLoopType_None)
    elseif textValue == "Linear" then
        UiFlipbookAnimationBus.Event.SetLoopType(self.currentImage, eUiFlipbookAnimationLoopType_Linear)
    elseif textValue =="PingPong" then
        UiFlipbookAnimationBus.Event.SetLoopType(self.currentImage, eUiFlipbookAnimationLoopType_PingPong)
    elseif textValue =="Sprite sheet" then
        UiElementBus.Event.SetIsEnabled(self.currentImage, false)
        self.currentImage = self.Properties.FlipbookContentSpriteSheet
    elseif textValue =="Image sequence" then
        UiElementBus.Event.SetIsEnabled(self.currentImage, false)
        self.currentImage = self.Properties.FlipbookContentImageSequence
    end

    -- If changing image type, restore the current loop type to the new image
    -- content entity and reset the frame.
    if UiDropdownNotificationBus.GetCurrentBusId() == self.Properties.ImageTypeDropdown then
        UiFlipbookAnimationBus.Event.SetLoopType(self.currentImage, currentLoopType)
        UiFlipbookAnimationBus.Event.SetCurrentFrame(self.currentImage, 0)

        self:PrintImageDescription()
    end
end

return SequenceStates
