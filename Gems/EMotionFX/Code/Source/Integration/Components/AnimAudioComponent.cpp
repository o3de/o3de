/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Integration/Components/AnimAudioComponent.h>

#include <LmbrCentral/Audio/AudioProxyComponentBus.h>
#include <LmbrCentral/Animation/SkeletalHierarchyRequestBus.h>

#include <MathConversion.h>


using namespace LmbrCentral;

namespace EMotionFX
{
    namespace Integration
    {
        void AudioTriggerEvent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AudioTriggerEvent>()
                    ->Version(0)
                    ->Field("event", &AudioTriggerEvent::m_eventName)
                    ->Field("trigger", &AudioTriggerEvent::m_triggerName)
                    ->Field("joint", &AudioTriggerEvent::m_jointName);
            }
        }

        void AnimAudioComponent::AddTriggerEvent(const AZStd::string& eventName, const AZStd::string& triggerName, const AZStd::string& jointName)
        {
            AZ::Entity* entity = GetEntity();
            AZ_Assert(entity, "Component must be added to entity prior to adding an audio trigger event.");
            if (entity->GetState() == AZ::Entity::State::Active)
            {
                AddTriggerEventInternal(eventName, triggerName, jointName);
            }
            else
            {
                m_eventsToAdd.emplace_back(eventName, triggerName, jointName);
            }
        }

        void AnimAudioComponent::ClearTriggerEvents()
        {
            m_eventsToAdd.clear();
            m_eventsToRemove.clear();
            m_eventTriggerMap.clear();
        }

        void AnimAudioComponent::RemoveTriggerEvent(const AZStd::string& eventName)
        {
            const AZ::Crc32 eventCrc(eventName.c_str());

            const AZ::Entity* entity = GetEntity();
            AZ_Assert(entity, "Component must be added to entity prior to removing an audio trigger event.");
            if (entity->GetState() == AZ::Entity::State::Active)
            {
                RemoveTriggerEventInternal(eventCrc);
            }
            else
            {
                m_eventsToRemove.push_back(eventCrc);
            }
        }

        bool AnimAudioComponent::ExecuteSourceTrigger(
            const Audio::TAudioControlID triggerID,
            const Audio::TAudioControlID& sourceID,
            const AZStd::string& jointName)
        {
            if (triggerID == INVALID_AUDIO_CONTROL_ID)
            {
                return false;
            }

            bool success = false;
            AZ::s32 jointId = -1;
            SkeletalHierarchyRequestBus::EventResult(jointId, GetEntityId(),
                &SkeletalHierarchyRequestBus::Events::GetJointIndexByName, jointName.c_str());

            if (jointId < 0)
            {
                if (jointName.empty())
                {
                    AZ_Warning("Editor", false, "'ExecuteSourceTrigger' called on default entity proxy.  If this was the intent, a more explicit practice would be requesting this via the AudioProxyComponentBus.");
                    AudioProxyComponentRequestBus::EventResult(success, GetEntityId(), &AudioProxyComponentRequests::ExecuteSourceTrigger, triggerID, sourceID);
                }
                else
                {
                    AZ_Warning("Editor", false, "Joint not found.  'ExecuteSourceTrigger' call not performed on joint '%s'", jointName.c_str());
                }

                return success;
            }

            for (auto const& iter : m_jointProxies)
            {
                if (iter.first == jointId)
                {
                    if (Audio::IAudioProxy* proxy = iter.second)
                    {
                        proxy->ExecuteSourceTrigger(triggerID, sourceID);
                        success = true;
                    }
                }
            }

            return success;
        }

        bool AnimAudioComponent::ExecuteTrigger(
            const Audio::TAudioControlID triggerID,
            const AZStd::string& jointName)
        {
            if (triggerID == INVALID_AUDIO_CONTROL_ID)
            {
                return false;
            }

            bool success = false;
            AZ::s32 jointId = -1;
            SkeletalHierarchyRequestBus::EventResult(jointId, GetEntityId(),
                &SkeletalHierarchyRequestBus::Events::GetJointIndexByName, jointName.c_str());

            if (jointId < 0)
            {
                if (jointName.empty())
                {
                    AZ_Warning("Editor", false, "'ExecuteTrigger' called on default entity proxy.  If this was the intent, a more explicit practice would be requesting this via the AudioProxyComponentBus.");
                    AudioProxyComponentRequestBus::EventResult(success, GetEntityId(), &AudioProxyComponentRequests::ExecuteTrigger, triggerID);
                }
                else
                {
                    AZ_Warning("Editor", false, "Joint not found.  'ExecuteTrigger' call not performed on joint '%s'", jointName.c_str());
                }

                return success;
            }

            for (auto const& iter : m_jointProxies)
            {
                if (iter.first == jointId)
                {
                    if (Audio::IAudioProxy* proxy = iter.second)
                    {
                        proxy->ExecuteTrigger(triggerID);
                        success = true;
                    }
                }
            }

            return success;
        }

        void AnimAudioComponent::KillTrigger(const Audio::TAudioControlID triggerId, const AZStd::string* jointName)
        {
            AZ::s32 jointId = -1;
            if (jointName)
            {
                SkeletalHierarchyRequestBus::EventResult(jointId, GetEntityId(),
                    &SkeletalHierarchyRequestBus::Events::GetJointIndexByName, jointName->c_str());

                if (jointId < 0)
                {
                    if (jointName->empty())
                    {
                        AZ_Warning("Editor", false, "'KillTrigger' called on default entity proxy.  If this was the intent, a more explicit practice would be requesting this via the AudioProxyComponentBus.");
                        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequests::KillTrigger, triggerId);
                    }
                    else
                    {
                        AZ_Warning("Editor", false, "Joint not found.  'KillTrigger' call not performed on joint '%s'", jointName->c_str());
                    }

                    return;
                }
            }

            for (auto const& iter : m_jointProxies)
            {
                if (!jointName || iter.first == jointId)
                {
                    if (Audio::IAudioProxy* proxy = iter.second)
                    {
                        proxy->StopTrigger(triggerId);
                    }
                }
            }
        }

        void AnimAudioComponent::KillAllTriggers(const AZStd::string* jointName)
        {
            AZ::s32 jointId = -1;
            if (jointName)
            {
                SkeletalHierarchyRequestBus::EventResult(jointId, GetEntityId(),
                    &SkeletalHierarchyRequestBus::Events::GetJointIndexByName, jointName->c_str());

                if (jointId < 0)
                {
                    if (jointName->empty())
                    {
                        AZ_Warning("Editor", false, "'KillAllTrigger' called on default entity proxy.  If this was the intent, a more explicit practice would be requesting this via the AudioProxyComponentBus.");
                        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequests::KillAllTriggers);
                    }
                    else
                    {
                        AZ_Warning("Editor", false, "Joint not found.  'KillAllTrigger' call not performed on joint '%s'", jointName->c_str());
                    }

                    return;
                }
            }

            for (auto const& iter : m_jointProxies)
            {
                if (!jointName || iter.first == jointId)
                {
                    if (Audio::IAudioProxy* proxy = iter.second)
                    {
                        proxy->StopAllTriggers();
                    }
                }
            }
        }

        void AnimAudioComponent::SetRtpcValue(const Audio::TAudioControlID rtpcID, float value, const AZStd::string* jointName)
        {
            AZ::s32 jointId = -1;
            if (jointName)
            {
                SkeletalHierarchyRequestBus::EventResult(jointId, GetEntityId(),
                    &SkeletalHierarchyRequestBus::Events::GetJointIndexByName, jointName->c_str());

                if (jointId < 0)
                {
                    if (jointName->empty())
                    {
                        AZ_Warning("Editor", false, "'SetRtpcValue' called on default entity proxy.  If this was the intent, a more explicit practice would be requesting this via the AudioProxyComponentBus.");
                        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequests::SetRtpcValue, rtpcID, value);
                    }
                    else
                    {
                        AZ_Warning("Editor", false, "Joint not found.  'SetRtpcValue' call not performed on joint '%s'", jointName->c_str());
                    }

                    return;
                }
            }

            for (auto const& iter : m_jointProxies)
            {
                if (!jointName || iter.first == jointId)
                {
                    if (Audio::IAudioProxy* proxy = iter.second)
                    {
                        proxy->SetRtpcValue(rtpcID, value);
                    }
                }
            }
        }

        void AnimAudioComponent::SetSwitchState(const Audio::TAudioControlID switchID, const Audio::TAudioSwitchStateID stateID, const AZStd::string* jointName)
        {
            AZ::s32 jointId = -1;
            if (jointName)
            {
                SkeletalHierarchyRequestBus::EventResult(jointId, GetEntityId(),
                    &SkeletalHierarchyRequestBus::Events::GetJointIndexByName, jointName->c_str());

                if (jointId < 0)
                {
                    if (jointName->empty())
                    {
                        AZ_Warning("Editor", false, "'SetSwitchState' called on default entity proxy.  If this was the intent, a more explicit practice would be requesting this via the AudioProxyComponentBus.");
                        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequests::SetSwitchState, switchID, stateID);
                    }
                    else
                    {
                        AZ_Warning("Editor", false, "Joint not found.  'SetSwitchState' call not performed on joint '%s'", jointName->c_str());
                    }

                    return;
                }
            }

            for (auto const& iter : m_jointProxies)
            {
                if (!jointName || iter.first == jointId)
                {
                    if (Audio::IAudioProxy* proxy = iter.second)
                    {
                        proxy->SetSwitchState(switchID, stateID);
                    }
                }
            }
        }

        void AnimAudioComponent::SetEnvironmentAmount(const Audio::TAudioEnvironmentID environmentID, float amount, const AZStd::string* jointName)
        {
            AZ::s32 jointId = -1;
            if (jointName)
            {
                SkeletalHierarchyRequestBus::EventResult(jointId, GetEntityId(),
                    &SkeletalHierarchyRequestBus::Events::GetJointIndexByName, jointName->c_str());

                if (jointId < 0)
                {
                    if (jointName->empty())
                    {
                        AZ_Warning("Editor", false, "'SetEnvironmentAmount' called on default entity proxy.  If this was the intent, a more explicit practice would be requesting this via the AudioProxyComponentBus.");
                        AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequests::SetEnvironmentAmount, environmentID, amount);
                    }
                    else
                    {
                        AZ_Warning("Editor", false, "Joint not found.  'SetEnvironmentAmount' call not performed on joint '%s'", jointName->c_str());
                    }

                    return;
                }
            }

            for (auto const& iter : m_jointProxies)
            {
                if (!jointName || iter.first == jointId)
                {
                    if (Audio::IAudioProxy* proxy = iter.second)
                    {
                        proxy->SetEnvironmentAmount(environmentID, amount);
                    }
                }
            }
        }

        void AnimAudioComponent::ReportTriggerStarted([[maybe_unused]] Audio::TAudioControlID triggerId)
        {
            if (!m_activeVoices)
            {
                AZ::TickBus::Handler::BusConnect();
                AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
            }

            ++m_activeVoices;
        }

        void AnimAudioComponent::ReportTriggerFinished([[maybe_unused]] Audio::TAudioControlID triggerId)
        {
            --m_activeVoices;

            if (!m_activeVoices)
            {
                AZ::TickBus::Handler::BusDisconnect();
                AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
            }
        }

        void AnimAudioComponent::Init()
        {
        }

        void AnimAudioComponent::Activate()
        {
            AZStd::for_each(m_eventsToAdd.begin(), m_eventsToAdd.end(), [this](const auto& triggerEvent)
            {
                AddTriggerEventInternal(triggerEvent.m_eventName, triggerEvent.m_triggerName, triggerEvent.m_jointName);
            });
            m_eventsToAdd.clear();

            AZStd::for_each(m_eventsToRemove.begin(), m_eventsToRemove.end(), [this](const auto& eventCrc)
            {
                RemoveTriggerEventInternal(eventCrc);
            });
            m_eventsToRemove.clear();

            ActivateJointProxies();

            ActorNotificationBus::Handler::BusConnect(GetEntityId());
            Audio::AudioTriggerNotificationBus::Handler::BusConnect(Audio::TriggerNotificationIdType{ GetEntityId() });
            AnimAudioComponentRequestBus::Handler::BusConnect(GetEntityId());
        }

        void AnimAudioComponent::Deactivate()
        {
            AZ::TickBus::Handler::BusDisconnect();
            AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());

            m_activeVoices = 0;

            DeactivateJointProxies();

            ActorNotificationBus::Handler::BusDisconnect(GetEntityId());
            Audio::AudioTriggerNotificationBus::Handler::BusDisconnect(Audio::TriggerNotificationIdType{ GetEntityId() });
            AnimAudioComponentRequestBus::Handler::BusDisconnect(GetEntityId());
        }

        void AnimAudioComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
        {
            AZ_UNUSED(deltaTime);
            AZ_UNUSED(time);
            for (auto& iter : m_jointProxies)
            {
                if (Audio::IAudioProxy* proxy = iter.second)
                {
                    AZ::Transform jointTransform = AZ::Transform::CreateIdentity();
                    auto getJointTransform = &SkeletalHierarchyRequestBus::Events::GetJointTransformCharacterRelative;
                    SkeletalHierarchyRequestBus::EventResult(jointTransform, GetEntityId(), getJointTransform, iter.first);

                    Audio::SATLWorldPosition atlTransform(m_transform * jointTransform);
                    proxy->SetPosition(m_transform * jointTransform);
                }
            }
        }

        void AnimAudioComponent::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
        {
            AZ_UNUSED(local);
            m_transform = world;
        }

        void AnimAudioComponent::OnMotionEvent(EMotionFX::Integration::MotionEvent motionEvent)
        {
            // 1. Check if event is registered
            auto eventIter = m_eventTriggerMap.find(AZ::Crc32(motionEvent.m_eventTypeName));
            if (!motionEvent.m_isEventStart || eventIter == m_eventTriggerMap.end())
            {
                return;
            }

            // 2. If registered but jointId is unset, play on ProxyComponent's proxy
            const AZ::s32 jointId = eventIter->second.GetJointId();
            if (jointId < 0)
            {
                AudioProxyComponentRequestBus::Event(GetEntityId(), &AudioProxyComponentRequests::ExecuteTrigger,
                    eventIter->second.GetTriggerId());
                return;
            }

            // 3. If no joint is registered with the component, then don't play anything
            // (If joints can be removed, then this would occur when event mapping and
            // event call still exist)
            auto jointIter = m_jointProxies.find(jointId);
            if (jointIter == m_jointProxies.end())
            {
                return;
            }

            // 4. If we have a joint proxy, update its position and play request.
            if (Audio::IAudioProxy* proxy = jointIter->second)
            {
                const Audio::TAudioControlID triggerId = eventIter->second.GetTriggerId();

                AZ::Transform jointTransform = AZ::Transform::CreateIdentity();
                const auto getJointTransform = &SkeletalHierarchyRequestBus::Events::GetJointTransformCharacterRelative;
                SkeletalHierarchyRequestBus::EventResult(jointTransform, GetEntityId(), getJointTransform, jointId);

                const Audio::SATLWorldPosition atlTransform(m_transform * jointTransform);
                proxy->SetPosition(atlTransform);
                proxy->ExecuteTrigger(triggerId);
            }
        }

        void AnimAudioComponent::Reflect(AZ::ReflectContext* context)
        {
            AudioTriggerEvent::Reflect(context);
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AnimAudioComponent, AZ::Component>()
                    ->Version(0)
                    ->Field("AudioTriggerEvents", &AnimAudioComponent::m_eventsToAdd);
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<EMotionFX::Integration::AnimAudioComponentRequestBus>("AnimAudioComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::Category, "Animation")
                    ->Event("AddTriggerEvent", &AnimAudioComponentRequestBus::Events::AddTriggerEvent)
                    ->Event("ClearTriggerEvents", &AnimAudioComponentRequestBus::Events::ClearTriggerEvents)
                    ->Event("RemoveTriggerEvent", &AnimAudioComponentRequestBus::Events::RemoveTriggerEvent);
            }
        }

        void AnimAudioComponent::AddTriggerEventInternal(const AZStd::string& eventName, const AZStd::string& triggerName, const AZStd::string& jointName)
        {
            Audio::TAudioControlID triggerId = INVALID_AUDIO_CONTROL_ID;
            if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get();
                audioSystem != nullptr)
            {
                triggerId = audioSystem->GetAudioTriggerID(triggerName.c_str());
            }

            if (triggerId == INVALID_AUDIO_CONTROL_ID)
            {
                AZ_Warning("Editor", false, "Audio trigger '%s' not found. Trigger not registered for motion event '%s'",
                    triggerName.c_str(), eventName.c_str());
            }
            else
            {
                AZ::s32 jointId = -1;
                if (!jointName.empty())
                {
                    SkeletalHierarchyRequestBus::EventResult(jointId, GetEntityId(),
                        &SkeletalHierarchyRequestBus::Events::GetJointIndexByName, jointName.c_str());

                    if (jointId < 0)
                    {
                        AZ_Warning("Editor", false, "Joint name '%s' not found: anim event '%s' audio trigger '%s' will be played on default proxy",
                            jointName.c_str(),
                            eventName.c_str(),
                            triggerName.c_str());
                    }
                }

                const AZ::Crc32 eventCrc(eventName.c_str());
                RemoveTriggerEventInternal(eventCrc);

                auto entity = GetEntity();
                AZ_Assert(entity, "AnimAudioComponent must be attached to entity prior to adding a trigger event");

                m_eventTriggerMap.emplace(eventCrc, TriggerEventData(*entity, triggerId, jointId));
            }
        }

        void AnimAudioComponent::RemoveTriggerEventInternal(const AZ::Crc32& eventCrc)
        {
            const auto iter = m_eventTriggerMap.find(eventCrc);
            if (iter != m_eventTriggerMap.end())
            {
                m_eventTriggerMap.erase(iter);
            }
        }

        void AnimAudioComponent::ActivateJointProxies()
        {
            const AZ::Entity* entity = GetEntity();
            AZ_Assert(entity, "Parent entity not found");
            const AZStd::string& name = entity->GetName();
            for (auto& eventIter : m_eventTriggerMap)
            {
                const AZ::s32 jointId = eventIter.second.GetJointId();
                if (jointId >= 0)
                {
                    auto jointIter = m_jointProxies.find(eventIter.second.GetJointId());
                    if (jointIter == m_jointProxies.end())
                    {
                        Audio::IAudioProxy* proxy = nullptr;
                        if (auto audioSystem = AZ::Interface<Audio::IAudioSystem>::Get();
                            audioSystem != nullptr)
                        {
                            proxy = audioSystem->GetAudioProxy();
                            AZ_Assert(proxy, "Failed to get free audio proxy");

                            AZStd::string proxyName = AZStd::string::format("%s:%d", name.c_str(), jointId);
                            proxy->Initialize(proxyName.c_str(),
                                reinterpret_cast<void*>(static_cast<uintptr_t>(static_cast<AZ::u64>(GetEntityId()))));
                            proxy->SetObstructionCalcType(Audio::ObstructionType::Ignore);
                            m_jointProxies.emplace(jointId, proxy);
                        }
                    }
                }
            }
        }

        void AnimAudioComponent::DeactivateJointProxies()
        {
            for (auto& iter : m_jointProxies)
            {
                if (Audio::IAudioProxy* proxy = iter.second)
                {
                    proxy->StopAllTriggers();
                    proxy->Release();
                }
            }
            m_jointProxies.clear();
        }

        AnimAudioComponent::TriggerEventData::TriggerEventData(const AZ::Entity& entity, Audio::TAudioControlID triggerId, AZ::s32 jointId)
            : m_jointId(jointId)
            , m_triggerId(triggerId)
        {
            AZ_UNUSED(entity);
        }

        AZ::s32 AnimAudioComponent::TriggerEventData::GetJointId() const
        {
            return m_jointId;
        }

        Audio::TAudioControlID AnimAudioComponent::TriggerEventData::GetTriggerId() const
        {
            return m_triggerId;
        }
    } // namespace Integration
} // namespace EMotionFX
