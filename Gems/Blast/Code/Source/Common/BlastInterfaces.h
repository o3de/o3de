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
