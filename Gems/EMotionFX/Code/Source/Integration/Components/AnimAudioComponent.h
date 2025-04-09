/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>

#include <Integration/AnimationBus.h>
#include <Integration/AnimAudioComponentBus.h>
#include <IAudioSystem.h>
#include <Integration/System/SystemCommon.h>

namespace EMotionFX
{
    namespace Integration
    {
        struct AudioTriggerEvent
        {
            AZ_RTTI(AudioTriggerEvent, "{1AA35052-477B-4F8D-9DE3-6411E96B871D}");
            AZ_CLASS_ALLOCATOR(AudioTriggerEvent, EMotionFXAllocator);

            AudioTriggerEvent() = default;
            virtual ~AudioTriggerEvent() = default;

            AudioTriggerEvent(const AZStd::string& eventName, const AZStd::string& triggerName, const AZStd::string& jointName)
                : m_eventName(eventName)
                , m_triggerName(triggerName)
                , m_jointName(jointName)
            {
            }

            static void Reflect(AZ::ReflectContext* context);

            AZStd::string m_eventName;
            AZStd::string m_triggerName;
            AZStd::string m_jointName;
        };

        class AnimAudioComponent
            : public AZ::Component
            , protected AZ::TickBus::Handler
            , protected AZ::TransformNotificationBus::Handler
            , protected ActorNotificationBus::Handler
            , protected AnimAudioComponentRequestBus::Handler
            , protected Audio::AudioTriggerNotificationBus::Handler
        {
        public:
            AZ_COMPONENT(AnimAudioComponent, "{E39F772F-FE4C-405E-9008-A5B8F27CB57D}");

            void Init() override;
            void Activate() override;
            void Deactivate() override;

            // AZ::TickBus implementation
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            // AZ::TransformNotificationBus interface implementation
            void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

            // ActorNotificationBus interface implementation
            void OnMotionEvent(EMotionFX::Integration::MotionEvent motionEvent) override;

            // AnimAudioComponentRequestBus interface implementation
            void AddTriggerEvent(const AZStd::string& eventName, const AZStd::string& triggerName, const AZStd::string& jointName) override;
            void ClearTriggerEvents() override;
            void RemoveTriggerEvent(const AZStd::string& eventName) override;
            bool ExecuteSourceTrigger(
                const Audio::TAudioControlID triggerID,
                const Audio::TAudioControlID& sourceId,
                const AZStd::string& jointName) override;
            bool ExecuteTrigger(
                const Audio::TAudioControlID triggerID,
                const AZStd::string& jointName) override;
            void KillTrigger(const Audio::TAudioControlID triggerID, const AZStd::string* jointName) override;
            void KillAllTriggers(const AZStd::string* jointName) override;
            void SetRtpcValue(const Audio::TAudioControlID rtpcID, float value, const AZStd::string* jointName) override;
            void SetSwitchState(const Audio::TAudioControlID switchID, const Audio::TAudioSwitchStateID stateID, const AZStd::string* jointName) override;
            void SetEnvironmentAmount(const Audio::TAudioEnvironmentID environmentID, float amount, const AZStd::string* jointName) override;

            // Audio::AudioTriggerNotificationBus interface implementation
            void ReportTriggerStarted(Audio::TAudioControlID triggerId) override;
            void ReportTriggerFinished(Audio::TAudioControlID triggerId) override;

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("AnimationAudioService"));
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC_CE("EMotionFXActorService"));
                required.push_back(AZ_CRC_CE("AudioProxyService"));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("AnimationAudioService"));
            }

        private:
            class TriggerEventData
            {
            public:
                TriggerEventData(const AZ::Entity& entity, Audio::TAudioControlID triggerId, AZ::s32 jointId);

                AZ::s32 GetJointId() const;
                Audio::TAudioControlID GetTriggerId() const;

            private:
                AZ::s32 m_jointId = -1;
                Audio::TAudioControlID m_triggerId = INVALID_AUDIO_CONTROL_ID;
            };

            void AddTriggerEventInternal(const AZStd::string& eventName, const AZStd::string& triggerName, const AZStd::string& jointName);
            void RemoveTriggerEventInternal(const AZ::Crc32& eventName);

            void ActivateJointProxies();
            void DeactivateJointProxies();

            AZ::u32 m_activeVoices = 0;

            AZStd::vector<AudioTriggerEvent> m_eventsToAdd;
            AZStd::vector<AZ::Crc32> m_eventsToRemove;

            AZStd::unordered_map<AZ::Crc32, TriggerEventData> m_eventTriggerMap;
            AZStd::unordered_map<AZ::s32, Audio::IAudioProxy*> m_jointProxies;

            AZ::Transform m_transform;
        };
    } // namespace Integration
} // namespace EMotionFX
