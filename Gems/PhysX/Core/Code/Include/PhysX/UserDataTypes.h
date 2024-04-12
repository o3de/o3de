/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/SimulatedBodies/StaticRigidBody.h>
#include <PxActor.h>

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace PhysX
{
    ///PxActor.userData is the custom data pointer that NVIDIA PhysX provides for applications to attach
    ///private data. The PhysX Gem requires that this userData points to ActorData objects.
    class ActorData
    {
    public:
        ActorData() = default;
        ActorData(physx::PxActor* actor);
        ActorData(ActorData&& actorData);
        ActorData& operator=(ActorData&& actorData);

        void Invalidate();
        AZ::EntityId GetEntityId() const;
        void SetEntityId(AZ::EntityId entityId);

        AzPhysics::SimulatedBodyHandle GetBodyHandle() const;

        AzPhysics::RigidBody* GetRigidBody() const;
        void SetRigidBody(AzPhysics::RigidBody* rigidBody);

        AzPhysics::StaticRigidBody* GetRigidBodyStatic() const;
        void SetRigidBodyStatic(AzPhysics::StaticRigidBody* rigidBody);

        Physics::Character* GetCharacter() const;
        void SetCharacter(Physics::Character* character);

        Physics::RagdollNode* GetRagdollNode() const;
        void SetRagdollNode(Physics::RagdollNode* ragdollNode);

        AzPhysics::SimulatedBody* GetArticulationLink();
        void SetArticulationLink(AzPhysics::SimulatedBody* articulationLink);

        AzPhysics::SimulatedBody* GetSimulatedBody() const;

        bool IsValid() const;

    private:
        using PxActorUniquePtr = AZStd::unique_ptr<physx::PxActor, AZStd::function<void(physx::PxActor*)> >;

        ///This is an arbitary value used to verify the cast from void* userdata pointer on a pxActor to ActorData
        ///is safe. If m_sanity does not have this value, then it is not safe to use the casted pointer.
        ///Helps to debug if someone is setting userData pointer to something other than this class during development
        static constexpr AZ::u32 SanityValue = 0xba5eba11;

        AZ::u32 m_sanity = SanityValue;
        PxActorUniquePtr m_actor;

        struct Payload
        {
            AZ::EntityId m_entityId;
            // Possible references, only one of them is not nullptr
            AzPhysics::RigidBody* m_rigidBody = nullptr;
            AzPhysics::StaticRigidBody* m_staticRigidBody = nullptr;
            Physics::Character* m_character = nullptr;
            Physics::RagdollNode* m_ragdollNode = nullptr;
            AzPhysics::SimulatedBody* m_articulationLink = nullptr;
            void* m_externalUserData = nullptr;
        };

        Payload m_payload;
    };
} // namespace PhysX

#include <PhysX/UserDataTypes.inl>
