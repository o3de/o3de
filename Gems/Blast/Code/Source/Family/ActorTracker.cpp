/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Transform.h>
#include <Family/ActorTracker.h>
#include <algorithm>

namespace Blast
{
    void ActorTracker::AddActor(BlastActor* actor)
    {
        m_actors.emplace(actor);
        m_entityIdToActor.emplace(actor->GetEntity()->GetId(), actor);
        m_bodyToActor.emplace(actor->GetSimulatedBody(), actor);
    }

    void ActorTracker::RemoveActor(BlastActor* actor)
    {
        m_bodyToActor.erase(actor->GetSimulatedBody());
        m_entityIdToActor.erase(actor->GetEntity()->GetId());
        m_actors.erase(actor);
    }

    BlastActor* ActorTracker::GetActorById(AZ::EntityId entityId)
    {
        if (const auto it = m_entityIdToActor.find(entityId); it != m_entityIdToActor.end())
        {
            return it->second;
        }
        return nullptr;
    }

    BlastActor* ActorTracker::GetActorByBody(const AzPhysics::SimulatedBody* body)
    {
        if (const auto it = m_bodyToActor.find(body); it != m_bodyToActor.end())
        {
            return it->second;
        }
        return nullptr;
    }

    BlastActor* ActorTracker::FindClosestActor(const AZ::Vector3& position)
    {
        AZ_PROFILE_FUNCTION(Physics);

        const auto candidate = std::min_element(
            m_actors.begin(), m_actors.end(),
            [&position](BlastActor* a, BlastActor* b)
            {
                return a->GetTransform().GetTranslation().GetDistanceSq(position) <
                    b->GetTransform().GetTranslation().GetDistanceSq(position);
            });
        return candidate == m_actors.end() ? nullptr : *candidate;
    }

    const AZStd::unordered_set<BlastActor*>& ActorTracker::GetActors() const
    {
        return m_actors;
    }
} // namespace Blast
