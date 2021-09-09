/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

#include <AzCore/Component/EntityId.h>

namespace LmbrCentral
{
    /*!
     * AudioSystemComponentRequests
     * Requests serviced by the Audio System Component.
     */
    class AudioSystemComponentRequests
        : public AZ::EBusTraits
    {
    public:
        /**
         * EBus Traits overrides
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual bool IsAudioSystemInitialized()
        {
            return false;
        }

        virtual void GlobalStopAllSounds() = 0;
        virtual void GlobalMuteAudio() = 0;
        virtual void GlobalUnmuteAudio() = 0;
        virtual void GlobalRefreshAudio(AZStd::string_view levelName) = 0;
        virtual void GlobalExecuteAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId) = 0;
        virtual void GlobalKillAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId) = 0;
        virtual void GlobalSetAudioRtpc(const char* rtpcName, float value) = 0;
        virtual void GlobalResetAudioRtpcs() = 0;
        virtual void GlobalSetAudioSwitchState(const char* switchName, const char* stateName) = 0;

        virtual void LevelLoadAudio(AZStd::string_view levelName) = 0;
        virtual void LevelUnloadAudio() = 0;
    };

    using AudioSystemComponentRequestBus = AZ::EBus<AudioSystemComponentRequests>;

    /*!
     * AudioSystemComponentNotifications
     * Notifications sent by the Audio System Component.
     */
    class AudioSystemComponentNotifications
        : public AZ::EBusTraits
    {
    public:
        /**
         * EBus Traits
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnGamePaused() = 0;
        virtual void OnGameUnpaused() = 0;
    };

    using AudioSystemComponentNotificationBus = AZ::EBus<AudioSystemComponentNotifications>;

} // namespace LmbrCentral
