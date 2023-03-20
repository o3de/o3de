/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <ScriptedEntityTweener/ScriptedEntityTweenerBus.h>
#include <ScriptedEntityTweener/ScriptedEntityTweenerEnums.h>

#include "ScriptedEntityTweenerSystemComponent.h"

namespace ScriptedEntityTweener
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //! ScriptedEntityTweenerNotificationBusHandler Behavior context handler class
    class ScriptedEntityTweenerNotificationBusHandler :
        public ScriptedEntityTweenerNotificationsBus::Handler,
        public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(ScriptedEntityTweenerNotificationBusHandler, "{118D3961-17C7-46E9-B9AC-61682D57E8D3}", AZ::SystemAllocator,
            OnComplete, OnUpdate, OnLoop, RemoveCallback, OnTimelineAnimationStart);

        void OnComplete(int callbackId) override
        {
            Call(FN_OnComplete, callbackId);
        }

        void OnUpdate(int callbackId, const AZStd::any& currentVal, float progressPercent) override
        {
            Call(FN_OnUpdate, callbackId, currentVal, progressPercent);
        }

        void OnLoop(int callbackId) override
        {
            Call(FN_OnLoop, callbackId);
        }

        void RemoveCallback(int callbackId) override
        {
            Call(FN_RemoveCallback, callbackId);
        }

        void OnTimelineAnimationStart(int timelineId, const AZ::Uuid& uuid, const AZStd::string& componentName, const AZStd::string& propertyName) override
        {
            Call(FN_OnTimelineAnimationStart, timelineId, uuid, componentName, propertyName);
        }
    };

    void ScriptedEntityTweenerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ScriptedEntityTweenerSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<ScriptedEntityTweenerBus>("ScriptedEntityTweenerBus")
                ->Event("AnimateEntity", &ScriptedEntityTweenerBus::Events::AnimateEntityScript)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::List)
                ->Event("SetOptionalParams", &ScriptedEntityTweenerBus::Events::SetOptionalParams)
                ->Event("Stop", &ScriptedEntityTweenerBus::Events::Stop)
                ->Event("Pause", &ScriptedEntityTweenerBus::Events::Pause)
                ->Event("Resume", &ScriptedEntityTweenerBus::Events::Resume)
                ->Event("SetPlayDirectionReversed", &ScriptedEntityTweenerBus::Events::SetPlayDirectionReversed)
                ->Event("SetSpeed", &ScriptedEntityTweenerBus::Events::SetSpeed)
                ->Event("SetInitialValue", &ScriptedEntityTweenerBus::Events::SetInitialValue)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::List)
                ->Event("GetVirtualPropertyValue", &ScriptedEntityTweenerBus::Events::GetVirtualPropertyValue);

            behaviorContext->EBus<ScriptedEntityTweenerNotificationsBus>("ScriptedEntityTweenerNotificationBus")
                ->Handler<ScriptedEntityTweenerNotificationBusHandler>();

            behaviorContext->Enum<(int)ScriptedEntityTweener::EasingMethod::Linear>("ScriptedEntityTweenerEasingMethod_Linear")
                ->Enum<(int)ScriptedEntityTweener::EasingMethod::Quad>("ScriptedEntityTweenerEasingMethod_Quad")
                ->Enum<(int)ScriptedEntityTweener::EasingMethod::Cubic>("ScriptedEntityTweenerEasingMethod_Cubic")
                ->Enum<(int)ScriptedEntityTweener::EasingMethod::Quart>("ScriptedEntityTweenerEasingMethod_Quart")
                ->Enum<(int)ScriptedEntityTweener::EasingMethod::Quint>("ScriptedEntityTweenerEasingMethod_Quint")
                ->Enum<(int)ScriptedEntityTweener::EasingMethod::Sine>("ScriptedEntityTweenerEasingMethod_Sine")
                ->Enum<(int)ScriptedEntityTweener::EasingMethod::Expo>("ScriptedEntityTweenerEasingMethod_Expo")
                ->Enum<(int)ScriptedEntityTweener::EasingMethod::Circ>("ScriptedEntityTweenerEasingMethod_Circ")
                ->Enum<(int)ScriptedEntityTweener::EasingMethod::Elastic>("ScriptedEntityTweenerEasingMethod_Elastic")
                ->Enum<(int)ScriptedEntityTweener::EasingMethod::Back>("ScriptedEntityTweenerEasingMethod_Back")
                ->Enum<(int)ScriptedEntityTweener::EasingMethod::Bounce>("ScriptedEntityTweenerEasingMethod_Bounce")
                ->Enum<(int)ScriptedEntityTweener::EasingType::In>("ScriptedEntityTweenerEasingType_In")
                ->Enum<(int)ScriptedEntityTweener::EasingType::Out>("ScriptedEntityTweenerEasingType_Out")
                ->Enum<(int)ScriptedEntityTweener::EasingType::InOut>("ScriptedEntityTweenerEasingType_InOut");
        }
    }

    void ScriptedEntityTweenerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptedEntityTweenerService"));
    }

    void ScriptedEntityTweenerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ScriptedEntityTweenerService"));
    }

    void ScriptedEntityTweenerSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void ScriptedEntityTweenerSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void ScriptedEntityTweenerSystemComponent::Init()
    {
    }

    void ScriptedEntityTweenerSystemComponent::Activate()
    {
        ScriptedEntityTweenerBus::Handler::BusConnect();

        AZ::TickBus::Handler::BusConnect();
    }

    void ScriptedEntityTweenerSystemComponent::Deactivate()
    {
        ScriptedEntityTweenerBus::Handler::BusDisconnect();

        AZ::TickBus::Handler::BusDisconnect();
    }

    void ScriptedEntityTweenerSystemComponent::AnimateEntity(const AZ::EntityId& entityId, const AnimationParameters& params)
    {
        auto animationTask = m_animationTasks.find(ScriptedEntityTweenerTask(entityId));
        if (animationTask == m_animationTasks.end())
        {
            animationTask = m_animationTasks.insert(ScriptedEntityTweenerTask(entityId)).first;
        }

        animationTask->AddAnimation(params);
    }

    void ScriptedEntityTweenerSystemComponent::SetOptionalParams(float timeIntoAnimation, float duration, int easingMethod, int easingType, float delayTime, int timesToPlay, bool isFrom, bool isPlayingBackward, const AZ::Uuid& animationId, int timelineId, int onCompleteCallbackId, int onUpdateCallbackId, int onLoopCallbackId)
    {
        AnimationParameters& params = m_tempParams;
        params.m_animationProperties.m_easeMethod = static_cast<EasingMethod>(easingMethod);
        params.m_animationProperties.m_easeType = static_cast<EasingType>(easingType);
        params.m_animationProperties.m_timeIntoAnimation = timeIntoAnimation;
        params.m_animationProperties.m_timeDuration = duration;
        params.m_animationProperties.m_timeToDelayAnim = delayTime;
        params.m_animationProperties.m_timesToPlay = timesToPlay;
        params.m_animationProperties.m_isFrom = isFrom;
        params.m_animationProperties.m_isPlayingBackward = isPlayingBackward;
        params.m_animationProperties.m_animationId = animationId;
        params.m_animationProperties.m_timelineId = timelineId;
        params.m_animationProperties.m_onCompleteCallbackId = onCompleteCallbackId;
        params.m_animationProperties.m_onUpdateCallbackId = onUpdateCallbackId;
        params.m_animationProperties.m_onLoopCallbackId = onLoopCallbackId;
    }

    void ScriptedEntityTweenerSystemComponent::AnimateEntityScript(const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName, const AZStd::any& paramTarget)
    {
        AnimationParameters& params = m_tempParams;
        AnimationParameterAddressData data(componentName, virtualPropertyName);
        params.m_animationParameters.emplace(data, paramTarget);
        AnimateEntity(entityId, params);
        m_tempParams.Reset();
    }

    void ScriptedEntityTweenerSystemComponent::Stop(int timelineId, const AZ::EntityId& entityId)
    {
        auto animationTask = m_animationTasks.find(ScriptedEntityTweenerTask(entityId));
        if (animationTask == m_animationTasks.end())
        {
            return;
        }

        animationTask->Stop(timelineId);
    }

    void ScriptedEntityTweenerSystemComponent::Pause(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName)
    {
        auto animationTask = m_animationTasks.find(ScriptedEntityTweenerTask(entityId));
        if (animationTask == m_animationTasks.end())
        {
            return;
        }

        AnimationParameterAddressData data(componentName, virtualPropertyName);
        animationTask->SetPaused(data, timelineId, true);
    }

    void ScriptedEntityTweenerSystemComponent::Resume(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName)
    {
        auto animationTask = m_animationTasks.find(ScriptedEntityTweenerTask(entityId));
        if (animationTask == m_animationTasks.end())
        {
            return;
        }

        AnimationParameterAddressData data(componentName, virtualPropertyName);
        animationTask->SetPaused(data, timelineId, false);
    }

    void ScriptedEntityTweenerSystemComponent::SetPlayDirectionReversed(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName, bool rewind)
    {
        auto animationTask = m_animationTasks.find(ScriptedEntityTweenerTask(entityId));
        if (animationTask == m_animationTasks.end())
        {
            return;
        }

        AnimationParameterAddressData data(componentName, virtualPropertyName);
        animationTask->SetPlayDirectionReversed(data, timelineId, rewind);
    }

    void ScriptedEntityTweenerSystemComponent::SetSpeed(int timelineId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName, float speed)
    {
        auto animationTask = m_animationTasks.find(ScriptedEntityTweenerTask(entityId));
        if (animationTask == m_animationTasks.end())
        {
            return;
        }

        AnimationParameterAddressData data(componentName, virtualPropertyName);
        animationTask->SetSpeed(data, timelineId, speed);
    }

    void ScriptedEntityTweenerSystemComponent::SetInitialValue(const AZ::Uuid& animationId, const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName, const AZStd::any& initialValue)
    {
        auto animationTask = m_animationTasks.find(ScriptedEntityTweenerTask(entityId));
        if (animationTask == m_animationTasks.end())
        {
            return;
        }

        AnimationParameterAddressData data(componentName, virtualPropertyName);
        animationTask->SetInitialValue(data, animationId, initialValue);
    }

    AZStd::any ScriptedEntityTweenerSystemComponent::GetVirtualPropertyValue(const AZ::EntityId& entityId, const AZStd::string& componentName, const AZStd::string& virtualPropertyName)
    {
        AZStd::any toReturn;
        AnimationParameterAddressData data(componentName, virtualPropertyName);

        auto animationTask = m_animationTasks.find(ScriptedEntityTweenerTask(entityId));
        if (animationTask != m_animationTasks.end())
        {
            animationTask->GetVirtualPropertyValue(toReturn, data);
        }
        else
        {
            ScriptedEntityTweenerTask task(entityId);
            task.GetVirtualPropertyValue(toReturn, data);
        }
        return toReturn;
    }

    void ScriptedEntityTweenerSystemComponent::Reset()
    {
        m_animationTasks.clear();
    }

    void ScriptedEntityTweenerSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/)
    {
        for (auto& animTask : m_animationTasks)
        {
            animTask.Update(deltaTime);
        }

        //TODO: Possible optimization: Logic to remove "stale" animation tasks, use a "garbage collection" method to defer set removal operations (which is O(n))
        for (auto it = m_animationTasks.begin(); it != m_animationTasks.end(); )
        {
            if (!((*it).GetIsActive()))
            {
                it = m_animationTasks.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    int ScriptedEntityTweenerSystemComponent::GetTickOrder()
    {
        return AZ::TICK_LAST;
    }
}
