/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

} // namespace LmbrCentral
