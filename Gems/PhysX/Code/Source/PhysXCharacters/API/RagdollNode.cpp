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

#include <PhysX_precompiled.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/Casts.h>
#include <PhysXCharacters/API/RagdollNode.h>
#include <PhysX/NativeTypeIdentifiers.h>

namespace PhysX
{
    // PhysX::RagdollNode
    void RagdollNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RagdollNode>()
                ->Version(1)
                ;
        }
    }

    RagdollNode::RagdollNode(AZStd::unique_ptr<Physics::RigidBody> rigidBody)
        : m_rigidBody(AZStd::move(rigidBody))
    {
        physx::PxRigidDynamic* pxRigidDynamic = static_cast<physx::PxRigidDynamic*>(m_rigidBody->GetNativePointer());
        m_actorUserData = PhysX::ActorData(pxRigidDynamic);
        m_actorUserData.SetRagdollNode(this);
        m_actorUserData.SetEntityId(m_rigidBody->GetEntityId());
    }

    void RagdollNode::SetJoint(const AZStd::shared_ptr<Physics::Joint>& joint)
    {
        m_joint = joint;
    }

    // Physics::RagdollNode
    Physics::RigidBody& RagdollNode::GetRigidBody()
    {
        return *m_rigidBody;
    }

    const AZStd::shared_ptr<Physics::Joint>& RagdollNode::GetJoint() const
    {
        return m_joint;
    }

    // Physics::WorldBody
    Physics::World* RagdollNode::GetWorld() const
    {
        return m_rigidBody ? m_rigidBody->GetWorld() : nullptr;
    }

    AZ::EntityId RagdollNode::GetEntityId() const
    {
        return m_rigidBody ? m_rigidBody->GetEntityId() : AZ::EntityId();
    }

    AZ::Transform RagdollNode::GetTransform() const
    {
        return m_rigidBody ? m_rigidBody->GetTransform() : AZ::Transform::CreateIdentity();
    }

    void RagdollNode::SetTransform([[maybe_unused]] const AZ::Transform& transform)
    {
        AZ_Warning("PhysX Ragdoll Node", false, "Setting the transform for an individual ragdoll node is not supported.  "
            "Please use the Ragdoll interface to modify ragdoll poses.");
    }

    AZ::Vector3 RagdollNode::GetPosition() const
    {
        return m_rigidBody ? m_rigidBody->GetPosition() : AZ::Vector3::CreateZero();
    }

    AZ::Quaternion RagdollNode::GetOrientation() const
    {
        return m_rigidBody ? m_rigidBody->GetOrientation() : AZ::Quaternion::CreateIdentity();
    }

    AZ::Aabb RagdollNode::GetAabb() const
    {
        return m_rigidBody ? m_rigidBody->GetAabb() : AZ::Aabb::CreateNull();
    }

    Physics::RayCastHit RagdollNode::RayCast(const Physics::RayCastRequest& request)
    {
        if (m_rigidBody)
        {
            return m_rigidBody->RayCast(request);
        }
        return Physics::RayCastHit();
    }

    AZ::Crc32 RagdollNode::GetNativeType() const
    {
        return NativeTypeIdentifiers::RagdollNode;
    }

    void* RagdollNode::GetNativePointer() const
    {
        return m_rigidBody ? m_rigidBody->GetNativePointer() : nullptr;
    }

    void RagdollNode::AddToWorld(Physics::World&)
    {
        AZ_WarningOnce("RagdollNode", false, "Not allowed to add individual ragdoll nodes to a world");
    }

    void RagdollNode::RemoveFromWorld(Physics::World&)
    {
        AZ_WarningOnce("RagdollNode", false, "Not allowed to remove individual ragdoll nodes to a world");
    }
} // namespace PhysX
