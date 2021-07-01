/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    enum class BaseActorType : AZ::u32
    {
        PHYSX_DEFAULT = 0,
        TOUCHBENDING_TRIGGER,
    };

    ///PxActor.userData is the custom data pointer that NVIDIA PhysX provides for applications to attach
    ///private data. The PhysX Gem requires that this userData points to objects that subclass BaseActorData.
    ///For Example:
    ///The TouchBending Gem defines "struct TouchBendingInstanceHandle : public PhysX::BaseActorData",
    ///While regular PhysX Gem Components use "class ActorData : public BaseActorData".
    class BaseActorData
    {
    protected:
        using PxActorUniquePtr = AZStd::unique_ptr<physx::PxActor, AZStd::function<void(physx::PxActor*)> >;

        ///This is an arbitary value used to verify the cast from void* userdata pointer on a pxActor to BaseActorData
        ///is safe. If m_sanity does not have this value, then it is not safe to use the casted pointer.
        ///Helps to debug if someone is setting userData pointer to something other than this class during development
        static const AZ::u32 s_sanityValue = 0xba5eba11;

        AZ::u32 m_sanity = s_sanityValue;
        BaseActorType m_actorType = BaseActorType::PHYSX_DEFAULT;
        PxActorUniquePtr m_actor;

        BaseActorData() = default;
        BaseActorData(BaseActorType type, physx::PxActor* actor);
        BaseActorData(BaseActorData&& other);
        BaseActorData& operator=(BaseActorData&& other);

    public:
        bool IsValid() const;
        BaseActorType GetType() const;
    };


    class ActorData : public BaseActorData
    {
    public:
        ActorData() = default;
        ActorData(physx::PxActor* actor);
        ActorData(ActorData&& actorData) = default;
        ActorData& operator=(ActorData&& actorData) = default;

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

        AzPhysics::SimulatedBody* GetSimulatedBody() const;

    private:

        struct Payload
        {
            AZ::EntityId m_entityId;
            // Possible references, only one of them is not nullptr
            AzPhysics::RigidBody* m_rigidBody = nullptr;
            AzPhysics::StaticRigidBody* m_staticRigidBody = nullptr;
            Physics::Character* m_character = nullptr;
            Physics::RagdollNode* m_ragdollNode = nullptr;
            void* m_externalUserData = nullptr;
        };

        Payload m_payload;
    };
} // namespace PhysX

#include <PhysX/UserDataTypes.inl>
