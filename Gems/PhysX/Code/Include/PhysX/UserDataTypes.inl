/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

namespace PhysX
{
    inline ActorData::ActorData(physx::PxActor* actor)
    {
        auto nullUserData = [](physx::PxActor* actorToSet)
        {
            // making sure userData never dangles
            actorToSet->userData = nullptr;
        };

        m_actor = PxActorUniquePtr(actor, nullUserData);
        actor->userData = this;
    }

    inline ActorData::ActorData(ActorData&& other)
        : m_sanity(other.m_sanity)
        , m_actor(AZStd::move(other.m_actor))
        , m_payload(AZStd::move(other.m_payload))
    {
        m_actor->userData = this;
    }

    inline ActorData& ActorData::operator=(ActorData&& other)
    {
        m_sanity = other.m_sanity;
        m_actor = AZStd::move(other.m_actor);
        m_actor->userData = this;
        m_payload = AZStd::move(other.m_payload);
        return *this;
    }

    inline bool ActorData::IsValid() const
    {
        return m_sanity == SanityValue;
    }

    inline void ActorData::Invalidate()
    {
        m_actor = nullptr;
        m_payload = Payload();
    }

    inline AZ::EntityId ActorData::GetEntityId() const
    {
        return m_payload.m_entityId;
    }

    inline void ActorData::SetEntityId(AZ::EntityId entityId)
    {
        m_payload.m_entityId = entityId;
    }

    inline AzPhysics::SimulatedBodyHandle ActorData::GetBodyHandle() const
    {
        AzPhysics::SimulatedBody* body = GetSimulatedBody();
        if (body)
        {
            return body->m_bodyHandle;
        }
        return AzPhysics::InvalidSimulatedBodyHandle;
    }

    inline AzPhysics::RigidBody* ActorData::GetRigidBody() const
    {
        return m_payload.m_rigidBody;
    }

    inline void ActorData::SetRigidBody(AzPhysics::RigidBody* rigidBody)
    {
        m_payload.m_rigidBody = rigidBody;
    }

    inline AzPhysics::StaticRigidBody* ActorData::GetRigidBodyStatic() const
    {
        return m_payload.m_staticRigidBody;
    }

    inline void ActorData::SetRigidBodyStatic(AzPhysics::StaticRigidBody* rigidBody)
    {
        m_payload.m_staticRigidBody = rigidBody;
    }

    inline Physics::Character* ActorData::GetCharacter() const
    {
        return m_payload.m_character;
    }

    inline void ActorData::SetCharacter(Physics::Character* character)
    {
        m_payload.m_character = character;
    }

    inline Physics::RagdollNode* ActorData::GetRagdollNode() const
    {
        return m_payload.m_ragdollNode;
    }

    inline void ActorData::SetRagdollNode(Physics::RagdollNode* ragdollNode)
    {
        m_payload.m_ragdollNode = ragdollNode;
    }

    inline AzPhysics::SimulatedBody* ActorData::GetArticulationLink()
    {
        return m_payload.m_articulationLink;
    }

    inline void ActorData::SetArticulationLink(AzPhysics::SimulatedBody* articulationLink)
    {
        m_payload.m_articulationLink = articulationLink;
    }

    inline AzPhysics::SimulatedBody* ActorData::GetSimulatedBody() const
    {
        if (m_payload.m_rigidBody)
        {
            return m_payload.m_rigidBody;
        }

        else if (m_payload.m_staticRigidBody)
        {
            return m_payload.m_staticRigidBody;
        }

        else if (m_payload.m_character)
        {
            return m_payload.m_character;
        }

        else if (m_payload.m_ragdollNode)
        {
            return m_payload.m_ragdollNode;
        }

        else if (m_payload.m_articulationLink)
        {
            return m_payload.m_articulationLink;
        }

        else
        {
            AZ_Error("PhysX Actor User Data", false, "Invalid user data");
            return nullptr;
        }
    }
} //namespace PhysX
