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

#include <IAudioInterfacesCommonData.h>
#include <AzCore/Component/ComponentBus.h>

namespace LmbrCentral
{
    /*!
     * AudioTriggerComponentRequests EBus Interface
     * Messages serviced by AudioTriggerComponents.
     */
    class AudioTriggerComponentRequests
        : public AZ::ComponentBus
    {
    public:
        //! Executes the play trigger if set.
        virtual void Play() = 0;

        //! Kills the play trigger if no stop trigger is set.
        //! Otherwise Executes the stop trigger.
        virtual void Stop() = 0;

        //! Execute function where you can specify the ATL Trigger name manually.
        virtual void ExecuteTrigger(const char* triggerName) = 0;

        //! Kill function where you can specify the ATL Trigger name manually.
        virtual void KillTrigger(const char* triggerName) = 0;

        //! Forces a kill of any triggers active on the underlying proxy.
        virtual void KillAllTriggers() = 0;

        //! Specify whether the Trigger should be repositioned as the Entity moves or not.
        virtual void SetMovesWithEntity(bool shouldTrackEntity) = 0;

        //! Set the occlusion / obstruction calculation type.
        virtual void SetObstructionType(Audio::ObstructionType obstructionType) = 0;
    };

    using AudioTriggerComponentRequestBus = AZ::EBus<AudioTriggerComponentRequests>;

    /*!
     * AudioTriggerComponentNotifications EBus Interface
     * Messages broadcasted from AudioTriggerComponents.
     */
    class AudioTriggerComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        //! Notification when a trigger instance has finished.
        virtual void OnTriggerFinished(const Audio::TAudioControlID triggerID) = 0;
    };

    using AudioTriggerComponentNotificationBus = AZ::EBus<AudioTriggerComponentNotifications>;

} // namespace LmbrCentral
