/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>


namespace EMotionFX
{
    /**
     * EMotion FX Actor Editor Request Bus
     * Used for making requests to actor editor.
     */
    class ActorEditorRequests
        : public AZ::EBusTraits
    {
    public:
        /**
         * Get the actor that is currently selected in the editor.
         * @result The currently selected actor.
         */
        virtual Actor* GetSelectedActor() { return nullptr; }

        /**
         * Get the actor instance that is currently selected in the editor.
         * @result The currently selected actor instance.
         */
        virtual ActorInstance* GetSelectedActorInstance() { return nullptr; }
    };

    using ActorEditorRequestBus = AZ::EBus<ActorEditorRequests>;

    /**
     * EMotion FX Actor Notification Bus
     * Used for monitoring events from the actor editor.
     */
    class ActorEditorNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual void ActorSelectionChanged([[maybe_unused]] Actor* actor) {}
        virtual void ActorInstanceSelectionChanged([[maybe_unused]] ActorInstance* actorInstance) {}
    };

    using ActorEditorNotificationBus = AZ::EBus<ActorEditorNotifications>;
} // namespace EMotionFX
