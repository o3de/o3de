/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        virtual void GlobalStopAllSounds() = 0;
        virtual void GlobalExecuteAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId) = 0;
        virtual void GlobalKillAudioTrigger(const char* triggerName, AZ::EntityId callbackOwnerEntityId) = 0;
        virtual void GlobalSetAudioRtpc(const char* rtpcName, float value) = 0;
        virtual void GlobalResetAudioRtpcs() = 0;
        virtual void GlobalSetAudioSwitchState(const char* switchName, const char* stateName) = 0;
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
