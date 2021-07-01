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
    class Actor;
    class Node;

    /**
     * EMotion FX Actor Request Bus
     * Used for making requests to actors.
     */
    class ActorRequests
        : public AZ::EBusTraits
    {
    public:
    };

    using ActorRequestBus = AZ::EBus<ActorRequests>;

    /**
     * EMotion FX Actor Notification Bus
     * Used for monitoring events from actors.
     */
    class ActorNotifications
        : public AZ::EBusTraits
    {
    public:
        // Enable multi-threaded access by locking primitive using a mutex when connecting handlers to the EBus or executing events.
        using MutexType = AZStd::recursive_mutex;

        /**
         * Called whenever the motion extraction node of an actor changed.
         */
        virtual void OnMotionExtractionNodeChanged(Actor* actor, Node* newMotionExtractionNode) { AZ_UNUSED(actor); AZ_UNUSED(newMotionExtractionNode); }

        virtual void OnActorCreated(Actor* actor) { AZ_UNUSED(actor); }
        virtual void OnActorDestroyed(Actor* actor) { AZ_UNUSED(actor); }
        virtual void OnActorReady(Actor* actor) { AZ_UNUSED(actor); }
    };

    using ActorNotificationBus = AZ::EBus<ActorNotifications>;
} // namespace EMotionFX
