/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace Blast
{
    class BlastActor;
    class BlastFamily;

    /// Interface for classes that want to listen to Blast events.
    class BlastListener
    {
    public:
        virtual ~BlastListener() = default;

        /// Called when BlastFamily creates a new actor.
        /// @param family Corresponding BlastFamily that created the new actor.
        /// @param actor The newly created actor.
        virtual void OnActorCreated(const BlastFamily& family, const BlastActor& actor) = 0;

        /// Called before a BlastFamily destroys an actor.
        /// @param family Corresponding BlastFamily that will destroy the actor.
        /// @param actor The actor to be destroyed.
        virtual void OnActorDestroyed(const BlastFamily& family, const BlastActor& actor) = 0;
    };
} // namespace Blast
