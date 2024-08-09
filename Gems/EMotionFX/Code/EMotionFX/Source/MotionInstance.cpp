/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/Allocators.h>

#include <MCore/Source/IDGenerator.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MotionInstance, MotionAllocator)

    MotionInstance::MotionInstance(Motion* motion, ActorInstance* actorInstance)
        : MCore::RefCounted()
    {
        AZ_Assert(actorInstance, "Expecting a valid actor instance");
        AZ_Assert(motion, "Motion pointer cannot be a nullptr");

        m_motion = motion;
        m_actorInstance = actorInstance;
        m_id = aznumeric_caster(MCore::GetIDGenerator().GenerateID());

        SetDeleteOnZeroWeight(true);
        SetCanOverwrite(true);
        SetMotionEventsEnabled(true);
        SetFreezeAtLastFrame(true);
        EnableFlag(BOOL_ISACTIVE);
        EnableFlag(BOOL_ISFIRSTREPOSUPDATE);
        EnableFlag(BOOL_BLENDBEFOREENDED);
        EnableFlag(BOOL_USEMOTIONEXTRACTION);

#if defined(EMFX_DEVELOPMENT_BUILD)
        if (actorInstance->GetIsOwnedByRuntime())
        {
            EnableFlag(BOOL_ISOWNEDBYRUNTIME);
        }
#endif // EMFX_DEVELOPMENT_BUILD

        m_eventHandlersByEventType.resize(EVENT_TYPE_MOTION_INSTANCE_LAST_EVENT - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT + 1);
        GetEventManager().OnCreateMotionInstance(this);
    }

    MotionInstance::~MotionInstance()
    {
        GetEventManager().OnDeleteMotionInstance(this);
        RemoveAllEventHandlers();
        m_subPool = nullptr;
    }

    MotionInstance* MotionInstance::Create(Motion* motion, ActorInstance* actorInstance)
    {
        return aznew MotionInstance(motion, actorInstance);
    }

    MotionInstance* MotionInstance::Create(void* memLocation, Motion* motion, ActorInstance* actorInstance)
    {
        return new(memLocation) MotionInstance(motion, actorInstance);
    }

    void MotionInstance::InitFromPlayBackInfo(const PlayBackInfo& info, bool resetCurrentPlaytime)
    {
        SetFadeTime             (info.m_blendOutTime);
        SetMixMode              (info.m_mix);
        SetMaxLoops             (info.m_numLoops);
        SetBlendMode            (info.m_blendMode);
        SetPlaySpeed            (info.m_playSpeed);
        SetWeight               (info.m_targetWeight, info.m_blendInTime);
        SetPriorityLevel        (info.m_priorityLevel);
        SetPlayMode             (info.m_playMode);
        SetRetargetingEnabled   (info.m_retarget);
        SetMotionExtractionEnabled(info.m_motionExtractionEnabled);
        SetFreezeAtLastFrame    (info.m_freezeAtLastFrame);
        SetMotionEventsEnabled  (info.m_enableMotionEvents);
        SetMaxPlayTime          (info.m_maxPlayTime);
        SetEventWeightThreshold (info.m_eventWeightThreshold);
        SetBlendOutBeforeEnded  (info.m_blendOutBeforeEnded);
        SetCanOverwrite         (info.m_canOverwrite);
        SetDeleteOnZeroWeight   (info.m_deleteOnZeroWeight);
        SetMirrorMotion         (info.m_mirrorMotion);
        SetFreezeAtTime         (info.m_freezeAtTime);
        SetIsInPlace            (info.m_inPlace);

        if (resetCurrentPlaytime)
        {
            m_currentTime = (info.m_playMode == PLAYMODE_BACKWARD) ? GetDuration() : 0.0f;
            m_lastCurTime = m_currentTime;
            m_timeDiffToEnd = GetDuration();
        }
    }

    void MotionInstance::SetCurrentTime(float time, bool resetLastTime)
    {
        m_currentTime = time;
        if (resetLastTime)
        {
            m_lastCurTime = time;
        }
    }

    void MotionInstance::ResetTimes()
    {
        m_currentTime    = (m_playMode == PLAYMODE_FORWARD) ? 0.0f : GetDuration();
        m_lastCurTime    = m_currentTime;
        m_totalPlayTime  = 0.0f;
        m_curLoops       = 0;
        m_lastLoops      = 0;
        m_timeDiffToEnd  = GetDuration();

        SetIsFrozen(false);
        EnableFlag(MotionInstance::BOOL_ISFIRSTREPOSUPDATE);
    }

    MotionInstance::PlayStateOut MotionInstance::CalcPlayState(const PlayStateIn& inState, float timePassed)
    {
        const float inCurrentTime = AZ::GetClamp(inState.m_currentTime, 0.0f, inState.m_duration);

        PlayStateOut outState;
        if (timePassed < AZ::Constants::FloatEpsilon || inState.m_isPaused)
        {
            outState.m_currentTime = inCurrentTime;
            outState.m_numLoops = inState.m_numLoops;
            outState.m_hasLooped = false;
            outState.m_isFrozen = inState.m_isFrozen;
            outState.m_timeDiffToEnd = (inState.m_playMode == PLAYMODE_FORWARD) ? inState.m_duration - inCurrentTime : inCurrentTime;
            return outState;
        }

        outState.m_numLoops = inState.m_numLoops;
        outState.m_isFrozen = inState.m_isFrozen;

        if (inState.m_playMode == PLAYMODE_FORWARD)
        {
            float newCurrentTime = inCurrentTime + (timePassed * inState.m_playSpeed);
            if (inState.m_maxLoops == EMFX_LOOPFOREVER)
            {
                outState.m_isFrozen = false;
                if (newCurrentTime >= inState.m_duration)
                {
                    outState.m_numLoops++;
                    outState.m_hasLooped = true;
                }
                newCurrentTime = (inState.m_duration > 0.0f) ? AZ::GetMod(newCurrentTime, inState.m_duration) : 0.0f;
            }
            else // Don't loop forever.
            {
                if (newCurrentTime >= inState.m_duration)
                {
                    outState.m_numLoops++;
                    outState.m_hasLooped = true;
                    if (outState.m_numLoops >= inState.m_maxLoops)
                    {
                        if (!inState.m_freezeAtLastFrame)
                        {
                            newCurrentTime = (inState.m_duration > 0.0f) ? AZ::GetMod(newCurrentTime, inState.m_duration) : 0.0f;
                            outState.m_numLoops = inState.m_maxLoops;
                        }
                        else
                        {
                            outState.m_numLoops = inState.m_maxLoops;
                            newCurrentTime = inState.m_duration;
                            if (inState.m_isFrozen) // Don't emit the looped state when we're frozen.
                            {
                                outState.m_hasLooped = false;
                            }
                            outState.m_isFrozen = true;
                        }
                    }
                    else
                    {
                        newCurrentTime = (inState.m_duration > 0.0f) ? AZ::GetMod(newCurrentTime, inState.m_duration) : 0.0f;
                        outState.m_isFrozen = false;
                    }
                }
            }

            // If we use the freeze at time setting.
            if (inState.m_freezeAtTime >= 0.0f)
            {
                if (newCurrentTime > inState.m_freezeAtTime)
                {
                    newCurrentTime = inState.m_freezeAtTime;
                }
            }

            if (newCurrentTime < 0.0f)
            {
                newCurrentTime = 0.0f;
            }

            outState.m_currentTime = newCurrentTime;
            outState.m_timeDiffToEnd = inState.m_duration - newCurrentTime;
        }
        else // Backward playback mode.
        {
            float newCurrentTime = inCurrentTime - (timePassed * inState.m_playSpeed);
            if (inState.m_maxLoops == EMFX_LOOPFOREVER)
            {
                outState.m_isFrozen = false;
                if (newCurrentTime <= 0.0f)
                {
                    outState.m_hasLooped = true;
                    outState.m_numLoops++;
                    newCurrentTime = (inState.m_duration > 0.0f) ? inState.m_duration + AZ::GetMod(newCurrentTime, inState.m_duration) : 0.0f;
                }
            }
            else // Don't loop forever.
            {
                if (newCurrentTime <= 0.0f)
                {
                    outState.m_numLoops++;
                    outState.m_hasLooped = true;
                    if (outState.m_numLoops >= inState.m_maxLoops)
                    {
                        if (!inState.m_freezeAtLastFrame)
                        {
                            newCurrentTime = (inState.m_duration > 0.0f) ? inState.m_duration + AZ::GetMod(newCurrentTime, inState.m_duration) : 0.0f;
                            outState.m_numLoops = inState.m_maxLoops;
                        }
                        else
                        {
                            outState.m_numLoops = inState.m_maxLoops;
                            newCurrentTime = 0.0f;
                            if (inState.m_isFrozen) // Don't emit the looped state when we're frozen.
                            {
                                outState.m_hasLooped = false;
                            }
                            outState.m_isFrozen = true;
                        }
                    }
                    else
                    {
                        newCurrentTime = (inState.m_duration > 0.0f) ? inState.m_duration + AZ::GetMod(newCurrentTime, inState.m_duration) : 0.0f;
                        outState.m_isFrozen = false;
                    }
                }
            }

            // If we use the freeze at time setting.
            if (inState.m_freezeAtTime >= 0.0f)
            {
                if (newCurrentTime < inState.m_freezeAtTime)
                {
                    newCurrentTime = inState.m_freezeAtTime;
                }
            }

            if (newCurrentTime < 0.0f)
            {
                newCurrentTime = 0.0f;
            }

            outState.m_currentTime = newCurrentTime;
            outState.m_timeDiffToEnd = newCurrentTime;
        }

        return outState;
    }

    void MotionInstance::ExtractMotionEvents(const PlayStateIn& inState, PlayStateOut& outState, AnimGraphEventBuffer& eventBuffer) const
    {
        if (inState.m_isFrozen)
        {
            return;
        }

        // This already handles looping inside the ExtractEvents.
        m_motion->GetEventTable()->ExtractEvents(inState.m_currentTime, outState.m_currentTime, this, &eventBuffer);
    }

    void MotionInstance::ProcessMotionEvents(const PlayStateIn& inState, PlayStateOut& outState) const
    {
        if (inState.m_isFrozen)
        {
            return;
        }

        // This already handles looping inside the ExtractEvents.
        m_motion->GetEventTable()->ProcessEvents(inState.m_currentTime, outState.m_currentTime, this);
    }

    MotionInstance::PlayStateIn MotionInstance::ConstructInputPlayState() const
    {
        PlayStateIn playState;
        playState.m_currentTime = m_currentTime;
        playState.m_duration = GetDuration();
        playState.m_playSpeed = m_playSpeed;
        playState.m_freezeAtTime = m_freezeAtTime;
        playState.m_numLoops = m_curLoops;
        playState.m_maxLoops = m_maxLoops;
        playState.m_playMode = m_playMode;
        playState.m_isFrozen = GetIsFrozen();
        playState.m_isPaused = GetIsPaused();
        playState.m_freezeAtLastFrame = GetFreezeAtLastFrame();
        return playState;
    }

    void MotionInstance::SetPlayState(const PlayStateIn& inState, const PlayStateOut& outState, bool triggerEvents)
    {
        m_currentTime = outState.m_currentTime;
        m_timeDiffToEnd = outState.m_timeDiffToEnd;
        m_curLoops = outState.m_numLoops;
        SetIsFrozen(outState.m_isFrozen);

        // If we became frozen.
        if (inState.m_freezeAtLastFrame && inState.m_isFrozen != outState.m_isFrozen && triggerEvents)
        {
            GetEventManager().OnIsFrozenAtLastFrame(this);
        }

        // If we looped.
        if (outState.m_hasLooped && triggerEvents)
        {
            GetEventManager().OnHasLooped(this);
        }
    }

    // Update the time values based on the motion playback settings.
    void MotionInstance::CalcNewTimeAfterUpdate(float timePassed, float* outNewTime) const
    {
        *outNewTime = CalcPlayState(ConstructInputPlayState(), timePassed).m_currentTime;
    }

    MotionInstance::PlayStateOut MotionInstance::CalcPlayStateAfterUpdate(float timePassed) const
    {
        return CalcPlayState(ConstructInputPlayState(), timePassed);
    }

    void MotionInstance::UpdateTime(float timePassed)
    {
        m_lastCurTime = m_currentTime;
        m_lastLoops = m_curLoops;
        const PlayStateIn inState = ConstructInputPlayState();
        PlayStateOut outState = CalcPlayState(inState, timePassed);
        SetPlayState(inState, outState, true);
        if (timePassed > 0.0f)
        {
            m_totalPlayTime += timePassed;
        }
    }

    void MotionInstance::Update(float timePassed)
    {
        if (!GetIsActive())
        {
            return;
        }

        const float currentTimePreUpdate = m_currentTime;
        UpdateTime(timePassed);

        // If UpdateTime() did not advance m_currentTime we can skip over ProcessEvents().
        if (!AZ::IsClose(m_lastCurTime, m_currentTime, AZ::Constants::FloatEpsilon))
        {
            // if we are blending towards the destination motion or layer.
            // Do this after UpdateTime(timePassed) and use (m_currentTime - m_lastCurTime)
            // as the elapsed time. This will function for Updates that use SetCurrentTime(time, false)
            // like Simple Motion component does with Track View. This will also work for motions that
            // have m_playSpeed that is not 1.0f.
            if (GetIsBlending())
            {
                const float duration = GetDuration();
                float deltaTime = 0.0f;
                if (m_playMode == PLAYMODE_FORWARD)
                {
                    // Playing forward, if the motion looped, need to consider the wrapped delta time
                    if (m_lastCurTime > m_currentTime)
                    {
                        // Need to add the last time up to the end of the motion, and the cur time from the start of the motion.
                        // That will give us the full wrap around time.
                        deltaTime = (duration - m_lastCurTime) + m_currentTime;
                    }
                    else
                    {
                        // No looping, simple time passed calc.
                        deltaTime = (m_currentTime - m_lastCurTime);
                    }
                }
                else
                {
                    // Playing in reverse, if the motion looped, need to consider the wrapped delta time
                    if (m_lastCurTime < m_currentTime)
                    {
                        // Need to add the last time up to the start of the motion, and the cur time from the end of the motion.
                        // That will give us the full wrap around time.
                        deltaTime = -m_lastCurTime + (m_currentTime - duration);
                    }
                    else
                    {
                        // No looping, simple time passed calc.
                        deltaTime = (m_lastCurTime - m_currentTime);
                    }
                }

                m_weight += m_weightDelta * deltaTime;

                // If we're increasing the weight.
                if (m_weightDelta >= 0)
                {
                    // If we reached our target weight, don't go past that.
                    if (m_weight >= m_targetWeight)
                    {
                        m_weight = m_targetWeight;
                        DisableFlag(BOOL_ISBLENDING);
                        GetEventManager().OnStopBlending(this);
                    }
                }
                else // If we're decreasing the weight.
                {
                    // If we reached our target weight, don't let it go lower than that.
                    if (m_weight <= m_targetWeight)
                    {
                        m_weight = m_targetWeight;
                        DisableFlag(BOOL_ISBLENDING);
                        GetEventManager().OnStopBlending(this);
                    }
                }
            }

            ProcessEvents(m_lastCurTime, m_currentTime);
        }

        m_lastCurTime = currentTimePreUpdate;
    }

    void MotionInstance::ProcessEvents(float oldTime, float newTime)
    {
        const float realTimePassed = newTime - oldTime;
        if (GetMotionEventsEnabled() &&
            !GetIsPaused() &&
            !AZ::IsClose(realTimePassed, 0.0f, AZ::Constants::FloatEpsilon) &&
            m_weight >= m_eventWeightThreshold &&
            !GetHasEnded())
        {
            m_motion->GetEventTable()->ProcessEvents(m_lastCurTime, m_currentTime, this);
        }
    }

    void MotionInstance::ExtractEvents(float oldTime, float newTime, AnimGraphEventBuffer* outBuffer)
    {
        const float realTimePassed = newTime - oldTime;
        if (GetMotionEventsEnabled() &&
            !GetIsPaused() &&
            !AZ::IsClose(realTimePassed, 0.0f, AZ::Constants::FloatEpsilon) &&
            m_weight >= m_eventWeightThreshold &&
            !GetHasEnded())
        {
            m_motion->GetEventTable()->ExtractEvents(m_lastCurTime, m_currentTime, this, outBuffer);
        }
    }

    void MotionInstance::ExtractEventsNonLoop(float oldTime, float newTime, AnimGraphEventBuffer* outBuffer)
    {
        const float realTimePassed = newTime - oldTime;
        if (AZ::GetAbs(realTimePassed) < AZ::Constants::FloatEpsilon)
        {
            return;
        }

        m_motion->GetEventTable()->ExtractEvents(oldTime, newTime, this, outBuffer);
    }

    void MotionInstance::UpdateByTimeValues(float oldTime, float newTime, AnimGraphEventBuffer* outEventBuffer)
    {
        // Get the values in valid range.
        const float duration = GetDuration();
        oldTime = AZ::GetClamp(oldTime, 0.0f, duration);
        newTime = AZ::GetClamp(newTime, 0.0f, duration);

        // Figure out our delta update time.
        float deltaTime = newTime - oldTime;
        if (m_playMode == PLAYMODE_FORWARD)
        {
            if (newTime < oldTime) // We have looped.
            {
                deltaTime = (duration - oldTime) + newTime;
            }
        }
        else // Backward.
        {
            AZ_Assert(m_playMode == PLAYMODE_BACKWARD, "Expected backward play mode.");
            if (newTime > oldTime)
            {
                deltaTime = (duration - newTime) + oldTime;
            }
            else
            {
                deltaTime = AZ::GetAbs(deltaTime);
            }
        }

        m_lastCurTime = oldTime;
        m_lastLoops = m_curLoops;

        // Build the input state and tweak it so it does what we want for this specific method.
        PlayStateIn inState = ConstructInputPlayState();
        inState.m_currentTime = oldTime;
        inState.m_playSpeed = 1.0f;

        // Calculate and set the new output state.
        PlayStateOut outState = CalcPlayState(inState, deltaTime);
        SetPlayState(inState, outState, true);

        // Extract the motion events.
        if (outEventBuffer)
        {
            ExtractMotionEvents(inState, outState, *outEventBuffer);
        }
    }

    // start a blend to the new weight
    void MotionInstance::SetWeight(float targetWeight, float blendTimeInSeconds)
    {
        // make sure the inputs are valid
        AZ_Assert(blendTimeInSeconds >= 0.0f, "Blend time has to be greater than zero.");
        AZ_Assert(targetWeight >= 0.0f && targetWeight <= 1.0f, "Target weight has to be between 0 and 1.");
        m_targetWeight = AZ::GetClamp(targetWeight, 0.0f, 1.0f);

        if (blendTimeInSeconds > 0.0f)
        {
            // calculate the rate of change of the weight value, so it goes towards the target weight
            m_weightDelta = (m_targetWeight - m_weight) / blendTimeInSeconds;

            // update the blendin time
            if (m_targetWeight > m_weight)
            {
                m_blendInTime = blendTimeInSeconds;
            }

            if (!GetIsBlending())
            {
                EnableFlag(BOOL_ISBLENDING);
                GetEventManager().OnStartBlending(this);
            }
        }
        else // blendtime is zero
        {
            if (m_targetWeight > m_weight)
            {
                m_blendInTime = 0.0f;
            }

            m_weight         = m_targetWeight;
            m_weightDelta    = 0.0f;

            if (GetIsBlending())
            {
                DisableFlag(BOOL_ISBLENDING);
                GetEventManager().OnStopBlending(this);
            }
        }
    }

    // stop the motion with a new fadeout time
    void MotionInstance::Stop(float fadeOutTime)
    {
        // trigger a motion event
        GetEventManager().OnStop(this);

        // update the fade out time
        SetFadeTime(fadeOutTime);

        // fade out
        SetWeight(0.0f, m_fadeTime);

        // we are stopping the motion
        EnableFlag(BOOL_ISSTOPPING);
    }

    // stop the motion using the currently setup fadeout time
    void MotionInstance::Stop()
    {
        // trigger a motion event
        GetEventManager().OnStop(this);

        // fade out
        SetWeight(0.0f, m_fadeTime);

        // we are stopping the motion
        EnableFlag(BOOL_ISSTOPPING);
    }

    void MotionInstance::SetIsActive(bool enabled)
    {
        if (GetIsActive() != enabled)
        {
            SetFlag(BOOL_ISACTIVE, enabled);
            GetEventManager().OnChangedActiveState(this);
        }
    }

    void MotionInstance::Pause()
    {
        if (!GetIsPaused())
        {
            EnableFlag(BOOL_ISPAUSED);
            GetEventManager().OnChangedPauseState(this);
        }
    }

    // unpause
    void MotionInstance::UnPause()
    {
        if (GetIsPaused())
        {
            DisableFlag(BOOL_ISPAUSED);
            GetEventManager().OnChangedPauseState(this);
        }
    }

    // enable or disable pause state
    void MotionInstance::SetPause(bool pauseEnabled)
    {
        if (GetIsPaused() != pauseEnabled)
        {
            SetFlag(BOOL_ISPAUSED, pauseEnabled);
            GetEventManager().OnChangedPauseState(this);
        }
    }

    // add a given event handler
    void MotionInstance::AddEventHandler(MotionInstanceEventHandler* eventHandler)
    {
        AZ_Assert(eventHandler, "Expected non-null event handler");
        eventHandler->SetMotionInstance(this);

        for (const EventTypes eventType : eventHandler->GetHandledEventTypes())
        {
            AZ_Assert(AZStd::find(m_eventHandlersByEventType[eventType - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT].begin(), m_eventHandlersByEventType[eventType - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT].end(), eventHandler) == m_eventHandlersByEventType[eventType - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT].end(),
                "Event handler already added to manager");
            m_eventHandlersByEventType[eventType - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT].emplace_back(eventHandler);
        }
    }

    // remove a given handler from memory
    void MotionInstance::RemoveEventHandler(MotionInstanceEventHandler* eventHandler)
    {
        for (const EventTypes eventType : eventHandler->GetHandledEventTypes())
        {
            EventHandlerVector& eventHandlers = m_eventHandlersByEventType[eventType - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
            eventHandlers.erase(AZStd::remove(eventHandlers.begin(), eventHandlers.end(), eventHandler), eventHandlers.end());
        }
    }

    //  remove all event handlers
    void MotionInstance::RemoveAllEventHandlers()
    {
#ifdef AZ_DEBUG_BUILD
        for (const EventHandlerVector& eventHandlers : m_eventHandlersByEventType)
        {
            AZ_Assert(eventHandlers.empty(), "Expected all event handlers to be removed");
        }
#endif
        m_eventHandlersByEventType.clear();
    }

    //--------------------

    // on a motion event
    void MotionInstance::OnEvent(const EventInfo& eventInfo) const
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_EVENT - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnEvent(eventInfo);
        }
    }

    // when this motion instance starts
    void MotionInstance::OnStartMotionInstance(PlayBackInfo* info)
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_START_MOTION_INSTANCE - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStartMotionInstance(info);
        }
    }

    // when this motion instance gets deleted
    void MotionInstance::OnDeleteMotionInstance()
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_DELETE_MOTION_INSTANCE - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnDeleteMotionInstance();
        }
    }

    // when this motion instance is stopped
    void MotionInstance::OnStop()
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STOP - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStop();
        }
    }

    // when it has looped
    void MotionInstance::OnHasLooped()
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_HAS_LOOPED - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnHasLooped();
        }
    }

    // when it reached the maximimum number of loops
    void MotionInstance::OnHasReachedMaxNumLoops()
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_HAS_REACHED_MAX_NUM_LOOPS - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnHasReachedMaxNumLoops();
        }
    }

    // when it has reached the maximimum playback time
    void MotionInstance::OnHasReachedMaxPlayTime()
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_HAS_REACHED_MAX_PLAY_TIME - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnHasReachedMaxPlayTime();
        }
    }

    // when it is frozen in the last frame
    void MotionInstance::OnIsFrozenAtLastFrame()
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_IS_FROZEN_AT_LAST_FRAME - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnIsFrozenAtLastFrame();
        }
    }

    // when the pause state changes
    void MotionInstance::OnChangedPauseState()
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CHANGED_PAUSE_STATE - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnChangedPauseState();
        }
    }

    // when the active state changes
    void MotionInstance::OnChangedActiveState()
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_CHANGED_ACTIVE_STATE - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnChangedActiveState();
        }
    }

    // when it starts blending in or out
    void MotionInstance::OnStartBlending()
    {
        const EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_START_BLENDING - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStartBlending();
        }
    }

    // when it stops blending
    void MotionInstance::OnStopBlending()
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_STOP_BLENDING - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnStopBlending();
        }
    }

    // when the motion instance is queued
    void MotionInstance::OnQueueMotionInstance(PlayBackInfo* info)
    {
        EventHandlerVector& eventHandlers = m_eventHandlersByEventType[EVENT_TYPE_ON_QUEUE_MOTION_INSTANCE - EVENT_TYPE_MOTION_INSTANCE_FIRST_EVENT];
        for (MotionInstanceEventHandler* eventHandler : eventHandlers)
        {
            eventHandler->OnQueueMotionInstance(info);
        }
    }

    void MotionInstance::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        SetFlag(BOOL_ISOWNEDBYRUNTIME, isOwnedByRuntime);
#else
        AZ_UNUSED(isOwnedByRuntime);
#endif
    }

    bool MotionInstance::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return (m_boolFlags & BOOL_ISOWNEDBYRUNTIME) != 0;
#else
        return true;
#endif
    }

    // calculate a world space transformation for a given node by sampling the motion at a given time
    void MotionInstance::CalcGlobalTransform(const AZStd::vector<size_t>& hierarchyPath, float timeValue, Transform* outTransform) const
    {
        Actor*      actor = m_actorInstance->GetActor();
        Skeleton*   skeleton = actor->GetSkeleton();

        // start with an identity transform
        Transform subMotionTransform;
        outTransform->Identity();

        // iterate from root towards the node (so backwards in the array)
        for (auto iter = rbegin(hierarchyPath); iter != rend(hierarchyPath); ++iter)
        {
            // get the current node index
            const size_t nodeIndex = *iter;
            m_motion->CalcNodeTransform(this, &subMotionTransform, actor, skeleton->GetNode(nodeIndex), timeValue, GetRetargetingEnabled());

            // multiply parent transform with the current node's transform
            outTransform->Multiply(subMotionTransform);
        }
    }

    // get relative movements between two non-looping time values
    void MotionInstance::CalcRelativeTransform(Node* rootNode, float curTime, float oldTime, Transform* outTransform) const
    {
        // get the facial setup of the actor
        Actor* actor = m_actorInstance->GetActor();

        // calculate the two node transformations
        Transform curNodeTransform;
        m_motion->CalcNodeTransform(this, &curNodeTransform, actor, rootNode, curTime, GetRetargetingEnabled());

        Transform oldNodeTransform;
        m_motion->CalcNodeTransform(this, &oldNodeTransform, actor, rootNode, oldTime, GetRetargetingEnabled());

        // calculate the relative transforms
        outTransform->m_position = curNodeTransform.m_position - oldNodeTransform.m_position;
        outTransform->m_rotation = curNodeTransform.m_rotation * oldNodeTransform.m_rotation.GetConjugate();
        outTransform->m_rotation.Normalize();
    }

    // extract the motion delta transform
    bool MotionInstance::ExtractMotion(Transform& outTrajectoryDelta)
    {
        // start clean
        outTrajectoryDelta.IdentityWithZeroScale();
        if (!GetMotionExtractionEnabled())
        {
            return true;
        }

        Actor*  actor               = m_actorInstance->GetActor();
        Node*   motionExtractNode   = actor->GetMotionExtractionNode();

        // check if we have enough data to actually calculate the results
        if (!motionExtractNode)
        {
            return false;
        }

        // get the motion extraction node index
        const size_t motionExtractionNodeIndex = motionExtractNode->GetNodeIndex();

        // get the current and previous time value from the motion instance
        float curTimeValue = GetCurrentTime();
        float oldTimeValue = GetLastCurrentTime();

        const MotionLinkData* motionLinkData = FindMotionLinkData(actor);
        if (!motionLinkData->IsJointActive(motionExtractionNodeIndex) || AZ::GetAbs(curTimeValue - oldTimeValue) < AZ::Constants::FloatEpsilon)
        {
            return false;
        }

        Transform trajectoryDelta = Transform::CreateIdentityWithZeroScale();

        // if the motion isn't paused
        if (!GetIsPaused())
        {
            // the relative transformations that we sample
            Transform relativeTrajectoryTransform;
            relativeTrajectoryTransform.Identity();

            // prevent looping from moving the character back to the origin if this is desired
            if (GetHasLooped() && !GetIsFrozen())
            {
                // if we're playing forward
                if (m_playMode == PLAYMODE_FORWARD)
                {
                    CalcRelativeTransform(motionExtractNode, GetDuration(), oldTimeValue, &relativeTrajectoryTransform);
                    oldTimeValue = 0.0f;
                }
                else // we're playing backward
                {
                    CalcRelativeTransform(motionExtractNode, 0.0f, oldTimeValue, &relativeTrajectoryTransform);
                    oldTimeValue = GetDuration();
                }

                // add the relative transform to the final values
                trajectoryDelta.m_position += relativeTrajectoryTransform.m_position;
                trajectoryDelta.m_rotation  = relativeTrajectoryTransform.m_rotation * trajectoryDelta.m_rotation;
            }

            // calculate the relative movement
            relativeTrajectoryTransform.Identity();
            CalcRelativeTransform(motionExtractNode, curTimeValue, oldTimeValue, &relativeTrajectoryTransform);

            // add the relative transform to the final values
            trajectoryDelta.m_position += relativeTrajectoryTransform.m_position;
            trajectoryDelta.m_rotation  = relativeTrajectoryTransform.m_rotation * trajectoryDelta.m_rotation;
        } // if not paused


        // Calculate the first frame's transform.
        Transform firstFrameTransform;
        m_motion->CalcNodeTransform(this, &firstFrameTransform, actor, motionExtractNode, 0.0f, GetRetargetingEnabled());

        // Calculate the current frame's transform.
        Transform currentFrameTransform;
        m_motion->CalcNodeTransform(this, &currentFrameTransform, actor, motionExtractNode, GetCurrentTime(), GetRetargetingEnabled());

        // Calculate the difference between the first frame of the motion and the bind pose transform.
        TransformData* transformData = m_actorInstance->GetTransformData();
        const Pose* bindPose = transformData->GetBindPose();
        AZ::Quaternion permBindPoseRotDiff = firstFrameTransform.m_rotation * bindPose->GetLocalSpaceTransform(motionExtractionNodeIndex).m_rotation.GetConjugate();
        AZ::Vector3 permBindPosePosDiff = bindPose->GetLocalSpaceTransform(motionExtractionNodeIndex).m_position - firstFrameTransform.m_position;
        permBindPoseRotDiff.SetX(0.0f);
        permBindPoseRotDiff.SetY(0.0f);
        permBindPoseRotDiff.Normalize();

        if (!(m_motion->GetMotionExtractionFlags() & MOTIONEXTRACT_CAPTURE_Z))
        {
            permBindPosePosDiff.SetZ(0.0f);
        }

        // If this is the first frame's motion extraction switch turn that flag off.
        AZ::Quaternion bindPoseRotDiff(0.0f, 0.0f, 0.0f, 1.0f);
        AZ::Vector3 bindPosePosDiff(0.0f, 0.0f, 0.0f);
        if (m_boolFlags & MotionInstance::BOOL_ISFIRSTREPOSUPDATE)
        {
            bindPoseRotDiff = permBindPoseRotDiff;
            bindPosePosDiff = permBindPosePosDiff;
        }

        // Capture rotation around the up axis only.
        trajectoryDelta.ApplyMotionExtractionFlags(m_motion->GetMotionExtractionFlags());

        AZ::Quaternion removeRot = currentFrameTransform.m_rotation * firstFrameTransform.m_rotation.GetConjugate();
        removeRot.SetX(0.0f);
        removeRot.SetY(0.0f);
        removeRot.Normalize();

        AZ::Quaternion rotation = removeRot.GetConjugate() * trajectoryDelta.m_rotation * permBindPoseRotDiff.GetConjugate();
        rotation.SetX(0.0f);
        rotation.SetY(0.0f);
        rotation.Normalize();

        AZ::Vector3 rotatedPos = rotation.TransformVector(trajectoryDelta.m_position - bindPosePosDiff);

        // Calculate the real trajectory delta, taking into account the actor instance rotation.
        outTrajectoryDelta.m_position = m_actorInstance->GetLocalSpaceTransform().m_rotation.TransformVector(rotatedPos);
        outTrajectoryDelta.m_rotation = trajectoryDelta.m_rotation * bindPoseRotDiff;
        outTrajectoryDelta.m_rotation.Normalize();

        if (m_boolFlags & MotionInstance::BOOL_ISFIRSTREPOSUPDATE)
        {
            DisableFlag(MotionInstance::BOOL_ISFIRSTREPOSUPDATE);
        }

        return true;
    }

    // get the sub pool
    MotionInstancePool::SubPool* MotionInstance::GetSubPool() const
    {
        return m_subPool;
    }

    // set the subpool
    void MotionInstance::SetSubPool(MotionInstancePool::SubPool* subPool)
    {
        m_subPool = subPool;
    }

    void MotionInstance::SetCustomData(void* customDataPointer)                 { m_customData = customDataPointer; }
    void* MotionInstance::GetCustomData() const                                 { return m_customData; }
    float MotionInstance::GetBlendInTime() const                                { return m_blendInTime; }
    float MotionInstance::GetCurrentTime() const                                { return m_currentTime; }
    float MotionInstance::GetDuration() const                                   { return m_motion->GetDuration(); }
    float MotionInstance::GetMaxTime() const                                    { return GetDuration(); }
    float MotionInstance::GetPlaySpeed() const                                  { return m_playSpeed; }
    Motion* MotionInstance::GetMotion() const                                   { return m_motion; }
    void MotionInstance::SetCurrentTimeNormalized(float normalizedTimeValue)    { m_currentTime = normalizedTimeValue * GetDuration(); }
    float MotionInstance::GetCurrentTimeNormalized() const                      { const float duration = GetDuration(); return (duration > 0.0f) ? m_currentTime / duration : 0.0f; }
    float MotionInstance::GetLastCurrentTime() const                            { return m_lastCurTime; }
    void MotionInstance::SetMotion(Motion* motion)                              { m_motion = motion; }
    void MotionInstance::SetPlaySpeed(float speed)                              { AZ_Assert(speed >= 0.0f, "Play speed cannot be negative."); m_playSpeed = speed; }
    void MotionInstance::SetPlayMode(EPlayMode mode)                            { m_playMode = mode; }
    EPlayMode MotionInstance::GetPlayMode() const                               { return m_playMode; }
    void MotionInstance::SetFadeTime(float fadeTime)                            { m_fadeTime = fadeTime; }
    float MotionInstance::GetFadeTime() const                                   { return m_fadeTime; }
    EMotionBlendMode MotionInstance::GetBlendMode() const                       { return m_blendMode; }
    float MotionInstance::GetWeight() const                                     { return m_weight; }
    float MotionInstance::GetTargetWeight() const                               { return m_targetWeight; }
    void MotionInstance::SetBlendMode(EMotionBlendMode mode)                    { m_blendMode = mode; }
    void MotionInstance::SetMirrorMotion(bool enabled)                          { SetFlag(BOOL_MIRRORMOTION, enabled); }
    bool MotionInstance::GetMirrorMotion() const                                { return (m_boolFlags & BOOL_MIRRORMOTION) != 0; }
    void MotionInstance::Rewind()                                               { SetCurrentTime(0.0f); }
    bool MotionInstance::GetHasEnded() const                                    { return (((m_maxLoops != EMFX_LOOPFOREVER) && (m_curLoops >= m_maxLoops)) || ((m_maxPlayTime > 0.0f) && (m_currentTime >= m_maxPlayTime))); }
    void MotionInstance::SetMixMode(bool mixModeEnabled)                        { SetFlag(BOOL_ISMIXING, mixModeEnabled); }
    bool MotionInstance::GetIsStopping() const                                  { return (m_boolFlags & BOOL_ISSTOPPING) != 0; }
    bool MotionInstance::GetIsPlaying() const                                   { return (!GetHasEnded() && !GetIsPaused()); }
    bool MotionInstance::GetIsMixing() const                                    { return (m_boolFlags & BOOL_ISMIXING) != 0; }
    bool MotionInstance::GetIsBlending() const                                  { return (m_boolFlags & BOOL_ISBLENDING) != 0; }
    bool MotionInstance::GetIsPaused() const                                    { return (m_boolFlags & BOOL_ISPAUSED); }
    void MotionInstance::SetMaxLoops(AZ::u32 numLoops)                          { m_maxLoops = numLoops; }
    AZ::u32 MotionInstance::GetMaxLoops() const                                 { return m_maxLoops; }
    bool MotionInstance::GetHasLooped() const                                   { return (m_curLoops != m_lastLoops); }
    void MotionInstance::SetNumCurrentLoops(AZ::u32 numCurrentLoops)            { m_curLoops = numCurrentLoops; }
    void MotionInstance::SetNumLastLoops(AZ::u32 numCurrentLoops)               { m_lastLoops = numCurrentLoops; }
    AZ::u32 MotionInstance::GetNumLastLoops() const                             { return m_lastLoops; }
    AZ::u32 MotionInstance::GetNumCurrentLoops() const                          { return m_curLoops; }
    bool MotionInstance::GetIsPlayingForever() const                            { return (m_maxLoops == EMFX_LOOPFOREVER); }
    ActorInstance* MotionInstance::GetActorInstance() const                     { return m_actorInstance; }
    AZ::u32 MotionInstance::GetPriorityLevel() const                            { return m_priorityLevel; }
    void MotionInstance::SetPriorityLevel(AZ::u32 priorityLevel)                { m_priorityLevel = priorityLevel; }
    bool MotionInstance::GetMotionExtractionEnabled() const                     { return (m_boolFlags & BOOL_USEMOTIONEXTRACTION) != 0; }
    void MotionInstance::SetMotionExtractionEnabled(bool enable)                { SetFlag(BOOL_USEMOTIONEXTRACTION, enable); }
    bool MotionInstance::GetCanOverwrite() const                                { return (m_boolFlags & BOOL_CANOVERWRITE) != 0; }
    void MotionInstance::SetCanOverwrite(bool canOverwrite)                     { SetFlag(BOOL_CANOVERWRITE, canOverwrite); }
    bool MotionInstance::GetDeleteOnZeroWeight() const                          { return (m_boolFlags & BOOL_DELETEONZEROWEIGHT) != 0; }
    void MotionInstance::SetDeleteOnZeroWeight(bool deleteOnZeroWeight)         { SetFlag(BOOL_DELETEONZEROWEIGHT, deleteOnZeroWeight); }
    bool MotionInstance::GetRetargetingEnabled() const                          { return (m_boolFlags & BOOL_RETARGET) != 0; }
    void MotionInstance::SetRetargetingEnabled(bool enabled)                    { SetFlag(BOOL_RETARGET, enabled); }
    bool MotionInstance::GetIsActive() const                                    { return (m_boolFlags & BOOL_ISACTIVE) != 0; }
    bool MotionInstance::GetIsFrozen() const                                    { return (m_boolFlags & BOOL_ISFROZENATLASTFRAME) != 0; }
    void MotionInstance::SetIsFrozen(bool isFrozen)
    {
        if (isFrozen)
        {
            EnableFlag(BOOL_ISFROZENATLASTFRAME);
        }
        else
        {
            DisableFlag(BOOL_ISFROZENATLASTFRAME);
        }
    }
    const MotionLinkData* MotionInstance::FindMotionLinkData(const Actor* actor) const { return m_motion->GetMotionData()->FindMotionLinkData(actor); }
    bool MotionInstance::GetMotionEventsEnabled() const                         { return (m_boolFlags & BOOL_ENABLEMOTIONEVENTS) != 0; }
    void MotionInstance::SetMotionEventsEnabled(bool enabled)                   { SetFlag(BOOL_ENABLEMOTIONEVENTS, enabled); }
    void MotionInstance::SetEventWeightThreshold(float weightThreshold)         { m_eventWeightThreshold = weightThreshold; }
    float MotionInstance::GetEventWeightThreshold() const                       { return m_eventWeightThreshold; }
    bool MotionInstance::GetFreezeAtLastFrame() const                           { return (m_boolFlags & BOOL_FREEZEATLASTFRAME) != 0; }
    void MotionInstance::SetBlendOutBeforeEnded(bool enabled)                   { SetFlag(BOOL_BLENDBEFOREENDED, enabled); }
    bool MotionInstance::GetBlendOutBeforeEnded() const                         { return (m_boolFlags & BOOL_BLENDBEFOREENDED) != 0; }
    void MotionInstance::SetFreezeAtLastFrame(bool enabled)                     { SetFlag(BOOL_FREEZEATLASTFRAME, enabled); }
    float MotionInstance::GetTotalPlayTime() const                              { return m_totalPlayTime; }
    void MotionInstance::SetTotalPlayTime(float playTime)                       { m_totalPlayTime = playTime; }
    float MotionInstance::GetMaxPlayTime() const                                { return m_maxPlayTime; }
    void MotionInstance::SetMaxPlayTime(float playTime)                         { m_maxPlayTime = playTime; }
    AZ::u32 MotionInstance::GetID() const                                       { return m_id; }
    float MotionInstance::GetTimeDifToLoopPoint() const                         { return m_timeDiffToEnd; }
    float MotionInstance::GetFreezeAtTime() const                               { return m_freezeAtTime; }
    void MotionInstance::SetFreezeAtTime(float timeInSeconds)                   { m_freezeAtTime = timeInSeconds; }
    bool MotionInstance::GetIsInPlace() const                                   { return (m_boolFlags & BOOL_INPLACE) != 0; }
    void MotionInstance::SetIsInPlace(bool inPlace)                             { SetFlag(BOOL_INPLACE, inPlace); }
    void MotionInstance::SetLastCurrentTime(float timeInSeconds)                { m_lastCurTime = timeInSeconds; }
    void MotionInstance::EnableFlag(AZ::u32 flag)                               { m_boolFlags |= flag; }
    void MotionInstance::DisableFlag(AZ::u32 flag)                              { m_boolFlags &= ~flag; }
    void MotionInstance::SetFlag(AZ::u32 flag, bool enabled)
    {
        if (enabled)
        {
            m_boolFlags |= flag;
        }
        else
        {
            m_boolFlags &= ~flag;
        }
    }
} // namespace EMotionFX
