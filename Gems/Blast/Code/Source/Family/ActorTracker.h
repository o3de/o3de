/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
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
        [[nodiscard]] BlastActor* GetActorByBody(const AzPhysics::SimulatedBody* body);
        [[nodiscard]] BlastActor* FindClosestActor(const AZ::Vector3& position);
        [[nodiscard]] const AZStd::unordered_set<BlastActor*>& GetActors() const;

    private:
        AZStd::unordered_set<BlastActor*> m_actors;
        AZStd::unordered_map<AZ::EntityId, BlastActor*> m_entityIdToActor;
        AZStd::unordered_map<const AzPhysics::SimulatedBody*, BlastActor*> m_bodyToActor;
    };
} // namespace Blast
