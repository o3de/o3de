/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <ScriptedEntityTweener/ScriptedEntityTweenerEnums.h>

namespace AZ
{
    class EntityId;
    struct Uuid;
}

namespace AZStd
{
    class any;
}

namespace ScriptedEntityTweener
{
    class ScriptedEntityTweenerRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        
        virtual ~ScriptedEntityTweenerRequests() = default;

        //! Animate a property or properties on component(s) on an entity
        virtual void AnimateEntity(const AZ::EntityId& entityId, const AnimationParameters& params) = 0;

        //! Sets optional animation parameters to be used on next AnimateEntityScript call, needed as lua implementation doesn't support > 13 args in non-debug builds
        virtual void SetOptionalParams(float timeIntoAnimation,
            float duration,
            int easingMethod,
            int easingType,
            float delayTime,
            int timesToPlay,
            bool isFrom,
            bool isPlayingBackward,
            const AZ::Uuid& animationId,
            int timelineId,
            int onCompleteCallbackId,
            int onUpdateCallbackId,
            int onLoopCallbackId) = 0;
            
        //! Script exposed version of the AnimateEntity call
        virtual void AnimateEntityScript(const AZ::EntityId& entityId,
            const AZStd::string& componentName,
            const AZStd::string& virtualPropertyName,
            const AZStd::any& paramTarget) = 0;
            
        //! Stop all animations on an entityId, if timelineId is specified (non-zero), only stop animations associated with the timelineId
        virtual void Stop(int timelineId, const AZ::EntityId& entityId) = 0;

        //! Pause a specific animation, if timelineId is specified (non-zero), only pause for animations associated with the timelineId
        virtual void Pause(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName) = 0;

        //! Resume a specific animation, if timelineId is specified (non-zero), only resume for animations associated with the timelineId
        virtual void Resume(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName) = 0;

        //! Change direction an animation is playing, if timelineId (non-zero), only change for animations associated with the timelineId
        virtual void SetPlayDirectionReversed(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName, bool isPlayingBackward) = 0;

        //! Set playback speed for a specific animation, by percentage (1.0f is default, 2.0f is 2x as fast, 0.5 is half as fast).
        virtual void SetSpeed(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName, float speed) = 0;

        //! Set initial values for a specific animation with an animationId.
        virtual void SetInitialValue(const AZ::Uuid& animationId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName, const AZStd::any& initialValue) = 0;

        //! Get current value
        virtual AZStd::any GetVirtualPropertyValue(const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName) = 0;

        virtual void Reset() = 0;
    };
    using ScriptedEntityTweenerBus = AZ::EBus<ScriptedEntityTweenerRequests>;

    class ScriptedEntityTweenerNotifications 
        : public AZ::EBusTraits
    {
    public: // member functions

        virtual ~ScriptedEntityTweenerNotifications() = default;

        //! Called if lua provided a callback ID via AnimateEntityLua, and animation completed.
        virtual void OnComplete(int callbackId) = 0;

        //! Called if lua provided a callback ID via AnimateEntityLua, and animation updated.
        virtual void OnUpdate(int callbackId, const AZStd::any& currentVal, float progressPercent) = 0;

        //! Called if lua provided a callback ID via AnimateEntityLua, and animation looped.
        virtual void OnLoop(int callbackId) = 0;

        //! Typically called on animation finish to remove any update or loop callbacks
        virtual void RemoveCallback(int callbackId) = 0;

        //! Called when an animation associated with a timeline starts
        virtual void OnTimelineAnimationStart(int timelineId, const AZ::Uuid& uuid, const AZStd::string& componentName, const AZStd::string& propertyName) = 0;
    };
    using ScriptedEntityTweenerNotificationsBus = AZ::EBus<ScriptedEntityTweenerNotifications>;
} // namespace ScriptedEntityTweener
