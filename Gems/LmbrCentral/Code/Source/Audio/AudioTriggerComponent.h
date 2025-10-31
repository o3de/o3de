/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <IAudioSystem.h>
#include <LmbrCentral/Audio/AudioTriggerComponentBus.h>

namespace LmbrCentral
{
    /*!
     * AudioTriggerComponent
     *  Allows controlling ATL Triggers, executing and stopping them.
     *  Trigger name can be serialized with the component, or manually specified
     *  at runtime for use in scripting.
     *  There is only 1 AudioTriggerComponent allowed on an Entity, but the interface
     *  supports firing multiple ATL Triggers.
     */
    class AudioTriggerComponent
        : public AZ::Component
        , public AudioTriggerComponentRequestBus::Handler
    {
    public:
        /*!
         * AZ::Component
         */
        AZ_COMPONENT(AudioTriggerComponent, "{8CBBB54B-7435-4D33-844D-E7F201BD581A}");
        void Activate() override;
        void Deactivate() override;

        AudioTriggerComponent() = default;
        AudioTriggerComponent(const AZStd::string& playTriggerName, const AZStd::string& stopTriggerName,
            Audio::ObstructionType obstructionType, bool playsImmediately);
        ~AudioTriggerComponent() = default;
        AZ_DISABLE_COPY(AudioTriggerComponent);

        /*!
         * AudioTriggerComponentRequestBus::Handler Interface
         */
        void Play() override;
        void Stop() override;

        void ExecuteTrigger(const char* triggerName) override;
        void KillTrigger(const char* triggerName) override;

        void KillAllTriggers() override;

        void SetMovesWithEntity(bool shouldTrackEntity) override;

        void SetObstructionType(Audio::ObstructionType obstructionType) override;

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("AudioPreloadService"));
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AudioTriggerService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("AudioProxyService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("AudioTriggerService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Editor callbacks
        void OnPlayTriggerChanged();
        void OnStopTriggerChanged();
        void OnObstructionTypeChanged();

        //! Transient data
        Audio::TAudioControlID m_defaultPlayTriggerID = INVALID_AUDIO_CONTROL_ID;
        Audio::TAudioControlID m_defaultStopTriggerID = INVALID_AUDIO_CONTROL_ID;

        //! Serialized data
        AZStd::string m_defaultPlayTriggerName;
        AZStd::string m_defaultStopTriggerName;
        Audio::ObstructionType m_obstructionType = Audio::ObstructionType::Ignore;
        bool m_playsImmediately = false;
    };

} // namespace LmbrCentral
