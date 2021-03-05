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

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/unordered_set.h>
#include <Blast/BlastActor.h>

namespace Blast
{
    //! Class used for storing and retrieving blast actors. Convenient to be shared between different classes as a
    //! dependency.
    //! @note ActorTracker does not own or control lifecycle of BlastActors it tracks. TkFramework controls lifecycle of
    //! TkActors and sends notification to BlastFamily when actors are created/destroyed which we follow up on by
    //! creating/deleting corresponding BlastActors (and adding/removing them to/from ActorTracker). This guarantees
    //! that stored BlastActors are always valid.
    class ActorTracker
    {
    public:
        void AddActor(BlastActor* actor);
        void RemoveActor(BlastActor* actor);

        [[nodiscard]] BlastActor* GetActorById(AZ::EntityId entityId);
        [[nodiscard]] BlastActor* GetActorByBody(const Physics::WorldBody* body);
        [[nodiscard]] BlastActor* FindClosestActor(const AZ::Vector3& position);
        [[nodiscard]] const AZStd::unordered_set<BlastActor*>& GetActors() const;

    private:
        AZStd::unordered_set<BlastActor*> m_actors;
        AZStd::unordered_map<AZ::EntityId, BlastActor*> m_entityIdToActor;
        AZStd::unordered_map<const Physics::WorldBody*, BlastActor*> m_bodyToActor;
    };
} // namespace Blast
