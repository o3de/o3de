/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace EMotionFX
{
    class ActorInstance;

    /**
     * EMotion FX Actor Instance Request Bus
     * Used for making requests to actor instances.
     */
    class ActorInstanceRequests
        : public AZ::EBusTraits
    {
    public:
    };

    using ActorInstanceRequestBus = AZ::EBus<ActorInstanceRequests>;

    /**
     * EMotion FX Actor Instance Notification Bus
     * Used for monitoring events from actor instances.
     */
    class ActorInstanceNotifications
        : public AZ::EBusTraits
    {
    public:
        // Enable multi-threaded access by locking primitive using a mutex when connecting handlers to the EBus or executing events.
        using MutexType = AZStd::recursive_mutex;

        virtual void OnActorInstanceCreated([[maybe_unused]] ActorInstance* actorInstance) {}

        /**
         * Called when any of the actor instances gets destructed.
         * @param actorInstance The actorInstance that gets destructed.
         */
        virtual void OnActorInstanceDestroyed([[maybe_unused]] ActorInstance* actorInstance) {}
    };

    using ActorInstanceNotificationBus = AZ::EBus<ActorInstanceNotifications>;
} // namespace EMotionFX
