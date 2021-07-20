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

if g_timelineCounter == nil then
    g_timelineCounter = 0
    g_animationCallbackCounter = 0
    g_animationCallbacks = {}
end

local ScriptedEntityTweener = {}

function ScriptedEntityTweener:OnActivate()
    if self.tweenerNotificationHandler == nil then
        self.animationParameterShortcuts = 
        {
            --UI Related
            ["opacity"] = {"UiFaderComponent", "Fade" },
            ["imgColor"] = {"UiImageComponent", "Color" },
            ["imgAlpha"] = {"UiImageComponent", "Alpha" },
            ["imgFill"] = {"UiImageComponent", "FillAmount" },
            ["imgFillAngle"] = {"UiImageComponent", "RadialFillStartAngle" },
            ["layoutMinWidth"] = {"UiLayoutCellComponent", "MinWidth" },
            ["layoutMinHeight"] = {"UiLayoutCellComponent", "MinHeight" },
            ["layoutTargetWidth"] = {"UiLayoutCellComponent", "TargetWidth" },
            ["layoutTargetHeight"] = {"UiLayoutCellComponent", "TargetHeight" },
            ["layoutExtraWidthRatio"] = {"UiLayoutCellComponent", "ExtraWidthRatio" },
            ["layoutExtraHeightRatio"] = {"UiLayoutCellComponent", "ExtraHeightRatio" },
            ["layoutColumnPadding"] = {"UiLayoutColumnComponent", "Padding" },
            ["layoutColumnSpacing"] = {"UiLayoutColumnComponent", "Spacing" },
            ["layoutRowPadding"] = {"UiLayoutRowComponent", "Padding" },
            ["layoutRowSpacing"] = {"UiLayoutRowComponent", "Spacing" },
            ["scrollHandleSize"] = {"UiScrollBarComponent", "HandleSize" },
            ["scrollHandleMinPixelSize"] = {"UiScrollBarComponent", "MinHandlePixelSize" },
            ["scrollValue"] = {"UiScrollBarComponent", "Value" },
            ["sliderValue"] = {"UiSliderComponent", "Value" },
            ["sliderMinValue"] = {"UiSliderComponent", "MinValue" },
            ["sliderMaxValue"] = {"UiSliderComponent", "MaxValue" },
            ["sliderStepValue"] = {"UiSliderComponent", "StepValue" },
            ["textSize"] = {"UiTextComponent", "FontSize" },
            ["textColor"] = {"UiTextComponent", "Color" },
            ["textCharacterSpace"] = {"UiTextComponent", "CharacterSpacing" },
            ["textSpacing"] = {"UiTextComponent", "LineSpacing" },
            ["textInputSelectionColor"] = {"UiTextInputComponent", "TextSelectionColor" },
            ["textInputCursorColor"] = {"UiTextInputComponent", "TextCursorColor" },
            ["textInputCursorBlinkInterval"] = {"UiTextInputComponent", "CursorBlinkInterval" },
            ["textInputMaxStringLength"] = {"UiTextInputComponent", "MaxStringLength" },
            ["tooltipDelayTime"] = {"UiTooltipDisplayComponent", "DelayTime" },
            ["tooltipDisplayTime"] = {"UiTooltipDisplayComponent", "DisplayTime" },
            ["scaleX"] = {"UiTransform2dComponent", "ScaleX" },
            ["scaleY"] = {"UiTransform2dComponent", "ScaleY" },
            ["pivotX"] = {"UiTransform2dComponent", "PivotX" },
            ["pivotY"] = {"UiTransform2dComponent", "PivotY" },
            ["x"] = {"UiTransform2dComponent", "LocalPositionX" },
            ["y"] = {"UiTransform2dComponent", "LocalPositionY" },
            ["rotation"] = {"UiTransform2dComponent", "Rotation" },
            ["w"] = {"UiTransform2dComponent", "LocalWidth" },
            ["h"] = {"UiTransform2dComponent", "LocalHeight" },
            
            --3d transform
            ["3dposition"] = {"TransformComponent", "Position" },
            ["3drotation"] = {"TransformComponent", "Rotation" },
            ["3dscale"] = {"TransformComponent", "Scale" },
            --Camera
            ["camFov"] = {"CameraComponent", "FieldOfView" },
            ["camNear"] = {"CameraComponent", "NearClipDistance" },
            ["camFar"] = {"CameraComponent", "FarClipDistance" },
            --[[
            --Some available virtual properties without shortcuts
            --Lights
            [""] = {"LightComponent", "Visible" },
            [""] = {"LightComponent", "Color" },
            [""] = {"LightComponent", "DiffuseMultiplier" },
            [""] = {"LightComponent", "SpecularMultiplier" },
            [""] = {"LightComponent", "Ambient" },
            [""] = {"LightComponent", "PointMaxDistance" },
            [""] = {"LightComponent", "PointAttenuationBulbSize" },
            [""] = {"LightComponent", "AreaMaxDistance" },
            [""] = {"LightComponent", "AreaWidth" },
            [""] = {"LightComponent", "AreaHeight" },
            [""] = {"LightComponent", "AreaFOV" },
            [""] = {"LightComponent", "ProjectorMaxDistance" },
            [""] = {"LightComponent", "ProjectorAttenuationBulbSize" },
            [""] = {"LightComponent", "ProjectorFOV" },
            [""] = {"LightComponent", "ProjectorNearPlane" },
            [""] = {"LightComponent", "ProbeAreaDimensions" },
            [""] = {"LightComponent", "ProbeSortPriority" },
            [""] = {"LightComponent", "ProbeBoxProjected" },
            [""] = {"LightComponent", "ProbeBoxHeight" },
            [""] = {"LightComponent", "ProbeBoxLength" },
            [""] = {"LightComponent", "ProbeBoxWidth" },
            [""] = {"LightComponent", "ProbeAttenuationFalloff" },
            --Particles
            [""] = {"ParticleComponent", "Visible" },
            [""] = {"ParticleComponent", "Enable" },
            [""] = {"ParticleComponent", "ColorTint" },
            [""] = {"ParticleComponent", "CountScale" },
            [""] = {"ParticleComponent", "TimeScale" },
            [""] = {"ParticleComponent", "SpeedScale" },
            [""] = {"ParticleComponent", "GlobalSizeScale" },
            [""] = {"ParticleComponent", "ParticleSizeScaleX" },
            [""] = {"ParticleComponent", "ParticleSizeScaleY" },
            --Static mesh
            ["meshVisibility"] = {"StaticMeshComponent", "Visibility" },
            --]]
            --Add any more custom shortcuts here,  [string] = { ComponentName, VirtualProperty }
        }
        self.animationEaseMethodShortcuts = 
        {
            ["Linear"] = ScriptedEntityTweenerEasingMethod_Linear,
            ["Quad"] = ScriptedEntityTweenerEasingMethod_Quad,
            ["Cubic"] = ScriptedEntityTweenerEasingMethod_Cubic,
            ["Quart"] = ScriptedEntityTweenerEasingMethod_Quart,
            ["Quint"] = ScriptedEntityTweenerEasingMethod_Quint,
            ["Sine"] = ScriptedEntityTweenerEasingMethod_Sine,
            ["Expo"] = ScriptedEntityTweenerEasingMethod_Expo,
            ["Circ"] = ScriptedEntityTweenerEasingMethod_Circ,
            ["Elastic"] = ScriptedEntityTweenerEasingMethod_Elastic,
            ["Back"] = ScriptedEntityTweenerEasingMethod_Back,
            ["Bounce"] = ScriptedEntityTweenerEasingMethod_Bounce,
        }
        self.animationEaseTypeShortcuts = 
        {
            [1] = {"InOut", ScriptedEntityTweenerEasingType_InOut},
            [2] = {"In", ScriptedEntityTweenerEasingType_In},
            [3] = {"Out", ScriptedEntityTweenerEasingType_Out},
        }
        self.tweenerNotificationHandler = ScriptedEntityTweenerNotificationBus.Connect(self)
        self.tickBusHandler = TickBus.Connect(self)
        self.activationCount = 0
        self.timelineRefs = {}
    end
    self.activationCount = self.activationCount + 1
end

function ScriptedEntityTweener:OnDeactivate()
    if self.tweenerNotificationHandler then
        self.activationCount = self.activationCount - 1
        if self.activationCount == 0 then
            self.tweenerNotificationHandler:Disconnect()
            self.tweenerNotificationHandler = nil
            self.tickBusHandler:Disconnect()
            self.tickBusHandler = nil
        end
    end
end

function ScriptedEntityTweener:Stop(id)
    ScriptedEntityTweenerBus.Broadcast.Stop(0, id)
end

function ScriptedEntityTweener:Set(id, args)
    self:StartAnimation(id, 0, args)
end

function ScriptedEntityTweener:Play(id, duration, args, toArgs)
    self:StartAnimation(id, duration, args, toArgs)
end

function ScriptedEntityTweener:StartAnimation(id, duration, args, toArgs)
    if self.animationParameterShortcuts == nil then
        Debug.Log("ScriptedEntityTweener: Make sure to call OnActivate() and OnDeactivate() for this table when requiring this file")
        return
    end
    if type(id) == "table" and duration == nil and args == nil and toArgs == nil then
        --Legacy support for old API, where it was just one table passed in containing args and duration
        args = id
    else
        args.id = id
        args.duration = duration
    end
    if self:ValidateAnimationParameters(args) == false then
        return
    end
    if toArgs then
        --Optional toArgs, if specified, will make args behave as a Set call, before starting the animation specified in toArgs
        self:Set(id, args)
        self:Play(id, duration, toArgs)
        return
    end
    
    local easeMethod = args.easeMethod
    local easeType = args.easeType
    if args.ease then
        --Parse shortcut ease parameter
        if args.easeMethod then
            Debug.Log("ScriptedEntityTweener: Warning, easeMethod will be overriden by ease parameter")
        end
        if args.easeType then
            Debug.Log("ScriptedEntityTweener: Warning, easeType will be overriden by ease parameter")
        end
        
        for easeName, easeValue in pairs(self.animationEaseMethodShortcuts) do
            if string.match(args.ease, easeName) then
                easeMethod = easeValue
                break
            end
        end
        
        for i,easeTypeData in ipairs(self.animationEaseTypeShortcuts) do
            local easeStr = easeTypeData[1]
            --if ease contains this easeStr, use this type
            if string.match(args.ease, easeStr) then
                easeType = easeTypeData[2]
                break
            end
        end
    end

    local optionalParams = 
    {
        timeIntoTween = args.timeIntoTween or 0, --Example: If timeIntoTween = 0.5 and duration = 1, the animation will instantly set the animation to 50% complete.
        duration = args.duration or 0,
        easeMethod = easeMethod or ScriptedEntityTweenerEasingMethod_Linear,
        easeType =  easeType or ScriptedEntityTweenerEasingType_Out,
        delay = args.delay or 0.0,
        timesToPlay = args.timesToPlay or 1,
        isFrom = args.isFrom,
        isPlayingBackward = args.isPlayingBackward,
        uuid = args.uuid or Uuid.CreateNull()
    }
    self:ConvertShortcutsToVirtualProperties(args)
    local virtualPropertiesToUse = args.virtualProperties
    if args.timelineParams ~= nil then
        optionalParams.delay = optionalParams.delay + args.timelineParams.initialStartTime
        optionalParams.timeIntoTween = optionalParams.timeIntoTween + args.timelineParams.timeIntoTween
        optionalParams.timelineId = args.timelineParams.timelineId
        optionalParams.uuid = args.timelineParams.uuidOverride
        
        if args.timelineParams.durationOverride ~= nil then
            optionalParams.duration = args.timelineParams.durationOverride
        end
        
        if args.timelineParams.seekDelayOverride ~= nil then
            optionalParams.delay = args.timelineParams.seekDelayOverride
        end
        
        if args.timelineParams.reversePlaybackOverride then
            if optionalParams.isPlayingBackward == nil then
                optionalParams.isPlayingBackward = false
            end
            optionalParams.isPlayingBackward = not optionalParams.isPlayingBackward
        end
    end
    
    --Create animation callbacks with an id, id sent to C so C can notify this system to trigger the callback.
    if args.onComplete ~= nil then
        optionalParams.onComplete = self:CreateCallback(args.onComplete)
    end
    if args.onUpdate ~= nil then
        optionalParams.onUpdate = self:CreateCallback(args.onUpdate)
    end
    if args.onLoop ~= nil then
        optionalParams.onLoop = self:CreateCallback(args.onLoop)
    end
    
    for componentData, paramTarget in pairs(virtualPropertiesToUse) do
        ScriptedEntityTweenerBus.Broadcast.SetOptionalParams(
            optionalParams.timeIntoTween,
            optionalParams.duration,
            optionalParams.easeMethod, 
            optionalParams.easeType, 
            optionalParams.delay, 
            optionalParams.timesToPlay, 
            optionalParams.isFrom == true,
            optionalParams.isPlayingBackward == true,
            optionalParams.uuid,
            optionalParams.timelineId or 0, 
            optionalParams.onComplete or 0,
            optionalParams.onUpdate or 0,
            optionalParams.onLoop or 0)
            
        ScriptedEntityTweenerBus.Broadcast.AnimateEntity(
            args.id,
            componentData[1],
            componentData[2],
            paramTarget)
    end
end

function ScriptedEntityTweener:ValidateAnimationParameters(args)
    --check for required params
    if args == nil then
        Debug.Log("ScriptedEntityTweener: animation with invalid args, args == nil")
        return false
    end
    if args.id == nil then
        Debug.Log("ScriptedEntityTweener: animation with no id specified " .. args)
        return false
    end
    if not args.id:IsValid() then
        --Fail silently
        return
    end
    return true
end

function ScriptedEntityTweener:ConvertShortcutsToVirtualProperties(args)
    for shortcutName, componentData in pairs(self.animationParameterShortcuts) do
        local paramTarget = args[shortcutName]
        if paramTarget ~= nil then
            if args.virtualProperties == nil then
                args.virtualProperties = {}
            end
            local virtualPropertyKey = {componentData[1], componentData[2]}
            args.virtualProperties[virtualPropertyKey] = paramTarget
            args[shortcutName] = nil
        end
    end
end

--Callback related
function ScriptedEntityTweener:CreateCallback(fnCallback)
    --todo: better uuid generation
    g_animationCallbackCounter = g_animationCallbackCounter + 1
    g_animationCallbacks[g_animationCallbackCounter] = fnCallback
    return g_animationCallbackCounter
end

function ScriptedEntityTweener:RemoveCallback(callbackId)
    g_animationCallbacks[callbackId] = nil
end

function ScriptedEntityTweener:CallCallback(callbackId)
    local callbackFn = g_animationCallbacks[callbackId]
    if callbackFn ~= nil then
        callbackFn()
    end
end

function ScriptedEntityTweener:OnComplete(callbackId)
    self:CallCallback(callbackId)
    self:RemoveCallback(callbackId)
end

function ScriptedEntityTweener:OnUpdate(callbackId, currentValue, progressPercent)
    local callbackFn = g_animationCallbacks[callbackId]
    if callbackFn ~= nil then
        callbackFn(currentValue, progressPercent)
    end
end

function ScriptedEntityTweener:OnLoop(callbackId)
    self:CallCallback(callbackId)
end

--Timeline related

function ScriptedEntityTweener:OnTick(deltaTime, timePoint)
    for timelineId, timeline in pairs(self.timelineRefs) do
        if (timeline.isPaused == false and timeline.playingSpeed ~= 0) then
            timeline.currentSeekTime = timeline.currentSeekTime + deltaTime * timeline.playingSpeed
            if (timeline.playingSpeed > 0 and timeline.currentSeekTime >= timeline.duration) then
                timeline.currentSeekTime = timeline.duration
                timeline.playingSpeed = 0
            elseif (timeline.playingSpeed < 0 and timeline.currentSeekTime <= 0) then
                timeline.currentSeekTime = 0
                timeline.playingSpeed = 0
            end
        end
    end
end

function ScriptedEntityTweener:GetUniqueTimelineId()
    g_timelineCounter = g_timelineCounter + 1
    return g_timelineCounter
end

function ScriptedEntityTweener:TimelineCreate()
    local timeline = {}
    timeline.animations = {}
    timeline.labels = {}
    timeline.duration = 0
    timeline.isPaused = false
    timeline.timelineId = self:GetUniqueTimelineId()
    timeline.currentSeekTime = 0
    timeline.playingSpeed = 0 -- 0 when not playing, >0 when playing forward, <0 when playing backwards
    
    --Gets the duration of a specific animation
    timeline.GetDurationOfAnim = function(timelineSelf, animParams)
        return ((animParams.duration or 0) - (animParams.timeIntoTween or 0)) + (animParams.delay or 0)
    end
    
    --Timeline configuration functions
    timeline.Add = function(timelineSelf, id, duration, animParams, timelineParams)
        --Timeline is a collection of animations
        --Operations like play, pause, reverse, etc
        --    Any animation is automatically added to the end of the animation.
        --        Optional parameters:
        --        timelineParams.offset will add it to the end +time specified, negative offsets allowed. 
        --        timelineParams.initialStartTime will set the timeline to start playing at a certain time within the timeline
        --        timelineParams.label will specify the timelime to start playing at the label.
        if timelineSelf == nil then
            Debug.Log("ScriptedEntityTweener:TimelineAdd no timeline")
            return
        end
        
        if type(id) == "table" and (duration == nil or type(duration) == "table") and animParams == nil and timelineParams == nil then
            --Legacy support for old API, where the animParams table already contained id and duration
            animParams = id
            timelineParams = duration
        else
            animParams.id = id
            animParams.duration = duration
        end
        if self:ValidateAnimationParameters(animParams) == false then
            Debug.Log("ScriptedEntityTweener:TimelineAdd invalid animation parameters for timline uuid " .. animParams)
            return
        end
    
        local optionalParams = 
        {
            offset = 0,
        }
        if timelineParams ~= nil then
            if timelineParams.label then
                optionalParams.initialStartTime = timelineSelf.labels[timelineParams.label]
            else
                optionalParams.initialStartTime = timelineParams.initialStartTime
            end
            
            optionalParams.offset = timelineParams.offset or 0
        end
        animParams.timelineParams = {}
        --initialStartTime is either specified from user as a number or a label, otherwise append animation to end of timeline
        animParams.timelineParams.initialStartTime = optionalParams.initialStartTime or timelineSelf.duration
        --user can also offset the time the animation is appended to.
        animParams.timelineParams.initialStartTime = animParams.timelineParams.initialStartTime + optionalParams.offset
        --additional time into tween specified from timeline, to allow timeline to seek into the middle of an animation
        
        animParams.timelineParams.timeIntoTween = 0
        --additional delay and duration overrides to allow seeking
        animParams.timelineParams.seekDelayOverride = nil
        animParams.timelineParams.durationOverride = nil
        
        animParams.timelineParams.timelineId = timelineSelf.timelineId
        animParams.timelineParams.uuidOverride = Uuid.Create()
        
        self:ConvertShortcutsToVirtualProperties(animParams)
        animParams.timelineParams.initialValues = {}
        for componentData, paramTarget in pairs(animParams.virtualProperties) do
            --Cache initial values for backwards playback purposes.
            animParams.timelineParams.initialValues[componentData] = ScriptedEntityTweenerBus.Broadcast.GetVirtualPropertyValue(animParams.id, componentData[1], componentData[2])
        end
        
        timelineSelf.animations[#timelineSelf.animations + 1] = animParams
        
        local animEndTime = animParams.timelineParams.initialStartTime + timelineSelf:GetDurationOfAnim(animParams)
        if animEndTime > timelineSelf.duration then
            timelineSelf.duration = animEndTime
        end
        
        table.sort(timelineSelf.animations, 
            function(first, second)
                return first.timelineParams.initialStartTime < second.timelineParams.initialStartTime
            end
        )
    end
    
    timeline.AddLabel = function(timelineSelf, labelId, labelTime)
        if labelId == nil then
            Debug.Log("Warning: TimelineLabel: labelId is nil")
            return
        end
        
        if labelTime == nil then
            Debug.Log("TimelineLabel: label " .. labelId .. " doesn't have a labelTime")
            return
        end
        
        if timelineSelf.labels[labelId] ~= nil then
            Debug.Log("Warning: TimelineLabel: label " .. labelId .. " already exists")
        end
        
        timelineSelf.labels[labelId] = labelTime
    end
    
    --Timeline control functions
    timeline.Play = function(timelineSelf, labelOrTime)
        timelineSelf:Resume()
        local startTime = 0
        if labelOrTime ~= nil then
            local typeInfo = type(labelOrTime)
            if typeInfo == "string" then
                startTime = timelineSelf.labels[labelOrTime] or 0
            elseif typeInfo == "number" then
                startTime = labelOrTime
            end
        end
        
        timelineSelf:Seek(startTime)
    end
    
    timeline.Stop = function(timelineSelf)
        timelineSelf.playingSpeed = 0
        timelineSelf.isPaused = false
        for i=1, #timelineSelf.animations do
            local animParams = timelineSelf.animations[i]
            ScriptedEntityTweenerBus.Broadcast.Stop(timelineSelf.timelineId, animParams.id)
        end
    end
    
    timeline.Pause = function(timelineSelf)
        if timelineSelf.isPaused == false then
            timelineSelf.isPaused = true
            for i=1, #timelineSelf.animations do
                local animParams = timelineSelf.animations[i]
                for componentData, paramTarget in pairs(animParams.virtualProperties) do
                    ScriptedEntityTweenerBus.Broadcast.Pause(timelineSelf.timelineId, animParams.id, componentData[1], componentData[2])
                end
            end
        end
    end
    
    timeline.Resume = function(timelineSelf)
        if timelineSelf.isPaused == true then
            timelineSelf.isPaused = false
            for i=1, #timelineSelf.animations do
                local animParams = timelineSelf.animations[i]
                for componentData, paramTarget in pairs(animParams.virtualProperties) do
                    ScriptedEntityTweenerBus.Broadcast.Resume(timelineSelf.timelineId, animParams.id, componentData[1], componentData[2])
                end
            end
        end
    end
    
    timeline.ResetRuntimeVars = function(timelineSelf, animParams)
        --Reset all timeline params that might have been set by a previous Seek or Reverse call
        animParams.timelineParams.timeIntoTween = 0
        animParams.timelineParams.reversePlaybackOverride = nil
        animParams.timelineParams.seekDelayOverride = nil
        animParams.timelineParams.durationOverride = nil
    end
    
    timeline.Seek = function(timelineSelf, seekTime)
        local typeInfo = type(seekTime)
        if typeInfo == "string" then
            seekTime = timelineSelf.labels[seekTime] or 0
        end
        
        if seekTime < 0 then
            seekTime = 0
        elseif seekTime > timelineSelf.duration then
            seekTime = timelineSelf.duration
        end
        
        timelineSelf:Stop()
        timelineSelf.isPaused = false
        timelineSelf.playingSpeed = 1
        
        --Go through each animation in the timeline and configure them to play as appropriate
        timelineSelf.currentSeekTime = seekTime
        
        local runningDuration = 0
        for i=1, #timelineSelf.animations do
            local animParams = timelineSelf.animations[i]
            
            local prevCompletionState = runningDuration < seekTime
            local animEndTime = animParams.timelineParams.initialStartTime + timelineSelf:GetDurationOfAnim(animParams)
            if animEndTime > runningDuration then
                runningDuration = animEndTime
            end
            local currentCompletionState = runningDuration < seekTime
            
            timelineSelf:ResetRuntimeVars(animParams)
            
            if runningDuration <= seekTime then
                --Current animation already complete
                animParams.timelineParams.seekDelayOverride = 0
                animParams.timelineParams.durationOverride = 0
            elseif prevCompletionState ~= currentCompletionState then
                --Current animation is partially complete
                local diff = runningDuration - seekTime
                animParams.timelineParams.timeIntoTween = timelineSelf:GetDurationOfAnim(animParams) - diff
                animParams.timelineParams.seekDelayOverride = 0
            elseif runningDuration > seekTime then
                --Current animation still needs to be played
                animParams.timelineParams.seekDelayOverride = (animParams.delay or 0) + (animParams.timelineParams.initialStartTime - seekTime)
            end
        end
        
        for i=1, #timelineSelf.animations do
            local animParams = timelineSelf.animations[i]
            self:StartAnimation(animParams)
        end
    end
    
    timeline.PlayBackwards = function(timelineSelf, specificSeekTime)
        local seekTime = specificSeekTime
        if seekTime == nil then
            seekTime = timelineSelf.currentSeekTime
        end
        
        if seekTime < 0 then
            seekTime = 0
        elseif seekTime > timelineSelf.duration then
            seekTime = timelineSelf.duration
        end
        
        -- Check if timeline needs to seek first
        local seekNeeded = false
        if seekTime > timelineSelf.currentSeekTime then
            -- Only animations that have already played or are currently playing can be played backwards
            seekNeeded = true
        elseif (seekTime > 0 and seekTime < timelineSelf.duration) then
            if (timelineSelf.playingSpeed == 0 or seekTime ~= timelineSelf.currentSeekTime) then
                -- In order for playback to work correctly, an animation that is partially completed needs to
                -- already be added to the tweener, and that animation needs to start playing from the same time
                -- into the animation
                seekNeeded = true
            end
        end

        if seekNeeded == true then
            timelineSelf:Seek(seekTime)
        else
            timelineSelf.currentSeekTime = seekTime
        end
                
        if timelineSelf.isPaused then
            timelineSelf:Resume()
        end
        timelineSelf.playingSpeed = -1
        
        local animsToPlay = {}
        
        local runningDuration = 0
        for i=1, #timelineSelf.animations do
            local animParams = timelineSelf.animations[i]
            
            local animEndTime = animParams.timelineParams.initialStartTime + timelineSelf:GetDurationOfAnim(animParams)
            if animEndTime > runningDuration then
                runningDuration = animEndTime
            end
            
            timelineSelf:ResetRuntimeVars(animParams)
            
            if runningDuration <= seekTime then
                --Current animation has already played, queue it to play again backwards
                animParams.timelineParams.timeIntoTween = timelineSelf:GetDurationOfAnim(animParams)
                animParams.timelineParams.seekDelayOverride = seekTime - runningDuration
                animParams.timelineParams.reversePlaybackOverride = true
                animsToPlay[#animsToPlay + 1] = animParams
            else
                --Current animation is either partially complete or hasn't been played
                for componentData, paramTarget in pairs(animParams.virtualProperties) do
                    ScriptedEntityTweenerBus.Broadcast.SetPlayDirectionReversed(timelineSelf.timelineId, animParams.id, componentData[1], componentData[2], true)
                end
            end
        end
        
        table.sort(animsToPlay, 
            function(first, second)
                return first.timelineParams.initialStartTime < second.timelineParams.initialStartTime
            end
        )
        
        for i=1,#animsToPlay do
            local animParams = animsToPlay[i]
            --Copy last initial values as start animation will overwrite with current initial values
            local initialValues = {}
            for componentData, paramInitial in pairs(animParams.timelineParams.initialValues) do
                initialValues[componentData] = paramInitial
            end
            self:StartAnimation(animParams)
            --Set the newly started animation's initial start position from cached start position.
            for componentData, paramInitial in pairs(initialValues) do
                ScriptedEntityTweenerBus.Broadcast.SetInitialValue(animParams.timelineParams.uuidOverride, animParams.id, componentData[1], componentData[2], paramInitial)
            end
        end
    end
    
    timeline.SetSpeed = function(timelineSelf, multiplier)
        if multiplier > 0 then
            if timelineSelf.playingSpeed > 0 then
                timelineSelf.playingSpeed = multiplier
            elseif timelineSelf.playingSpeed < 0 then
                timelineSelf.playingSpeed = -multiplier
            end
            
            for i=1, #timelineSelf.animations do
                local animParams = timelineSelf.animations[i]
                for componentData, paramTarget in pairs(animParams.virtualProperties) do
                    ScriptedEntityTweenerBus.Broadcast.SetSpeed(timelineSelf.timelineId, animParams.id, componentData[1], componentData[2], multiplier)
                end
            end
        else
            Debug.Log("ScriptedEntityTweener: Warning, SetSpeed multiplier must be greater than 0")
        end
    end
    
    --SetCompletion - Seek to animation percentage rather than specific time or label
    timeline.SetCompletion = function(timelineSelf, percentage)
        timelineSelf:Seek(timelineSelf.duration * percentage)
    end
    
    timeline.GetCurrentSeekTime = function(timelineSelf)
        return timelineSelf.currentSeekTime
    end
    
    timeline.GetDuration = function(timelineSelf)
        return timelineSelf.duration
    end    
    
    self.timelineRefs[timeline.timelineId] = timeline
    
    return timeline
end

function ScriptedEntityTweener:TimelineDestroy(timeline)
    --If a timeline is created, it should eventually be destroyed, otherwise they'll never be removed from self.timelineRef's table and garbage collected.
    self.timelineRefs[timeline.timelineId] = nil
end

function ScriptedEntityTweener:OnTimelineAnimationStart(timelineId, animUuid, addressComponentName, addressPropertyName)
    --Update cached initial value whenever a timeline animation starts, this data is needed if the user ever wants to play the animation backwards
    local timeline = self.timelineRefs[timelineId]
    if timeline == nil then
        return
    end
    
    for i=1,#timeline.animations do
        local animParams = timeline.animations[i]
        if animParams.timelineParams.uuidOverride == animUuid then
            for componentData, paramTarget in pairs(animParams.virtualProperties) do
                if componentData[1] == addressComponentName and componentData[2] == addressPropertyName then
                    if animParams.duration == 0 or animParams.isFrom == true then
                        animParams.timelineParams.initialValues[componentData] = paramTarget
                    else
                        animParams.timelineParams.initialValues[componentData] = ScriptedEntityTweenerBus.Broadcast.GetVirtualPropertyValue(animParams.id, componentData[1], componentData[2])
                    end
                    break
                end
            end
        end
    end
end 

return ScriptedEntityTweener
