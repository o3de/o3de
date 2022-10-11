/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysXCharacters/API/RagdollNode.h>

#include <PhysX/NativeTypeIdentifiers.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>
#include <PhysX/MathConversion.h>
#include <PxPhysicsAPI.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/EditContext.h>


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

    RagdollNode::RagdollNode(AzPhysics::SceneHandle sceneHandle, const Physics::RagdollNodeConfiguration& nodeConfig)
    {
        CreatePhysicsBody(sceneHandle, nodeConfig);
    }

    RagdollNode::~RagdollNode()
    {
        DestroyJoint();
        DestroyPhysicsBody();
    }

    void RagdollNode::SetJoint(AzPhysics::Joint* joint)
    {
        if (m_joint)
        {
            return;
        }

        m_joint = joint;
    }

    // Physics::RagdollNode
    AzPhysics::RigidBody& RagdollNode::GetRigidBody()
    {
        return *m_rigidBody;
    }

    AzPhysics::Joint* RagdollNode::GetJoint()
    {
        return m_joint;
    }

    bool RagdollNode::IsSimulating() const
    {
        if (m_rigidBody)
        {
            return m_rigidBody->m_simulating;
        }
        return false;
    }

    AzPhysics::Scene* RagdollNode::GetScene()
    {
        return m_rigidBody ? m_rigidBody->GetScene() : nullptr;
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

    AzPhysics::SceneQueryHit RagdollNode::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (m_rigidBody)
        {
            return m_rigidBody->RayCast(request);
        }
        return AzPhysics::SceneQueryHit();
    }

    AZ::Crc32 RagdollNode::GetNativeType() const
    {
        return NativeTypeIdentifiers::RagdollNode;
    }

    void* RagdollNode::GetNativePointer() const
    {
        return m_rigidBody ? m_rigidBody->GetNativePointer() : nullptr;
    }

    AzPhysics::SimulatedBodyHandle RagdollNode::GetRigidBodyHandle() const
    {
        return m_rigidBodyHandle;
    }

    void RagdollNode::CreatePhysicsBody(AzPhysics::SceneHandle sceneHandle, const Physics::RagdollNodeConfiguration& nodeConfig)
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_rigidBodyHandle = sceneInterface->AddSimulatedBody(sceneHandle, &nodeConfig);
            if (m_rigidBodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
            {
                AZ_Error("PhysX RagdollNode", false, "Failed to create rigid body for ragdoll node %s", nodeConfig.m_debugName.c_str());
                return;
            }
            m_rigidBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, m_rigidBodyHandle));
        }
        if (m_rigidBody == nullptr)
        {
            AZ_Error("PhysX RagdollNode", false, "Failed to create rigid body for ragdoll node %s", nodeConfig.m_debugName.c_str());
            return;
        }
        m_sceneOwner = sceneHandle;

        physx::PxRigidDynamic* pxRigidDynamic = static_cast<physx::PxRigidDynamic*>(m_rigidBody->GetNativePointer());
        physx::PxTransform transform(PxMathConvert(nodeConfig.m_position), PxMathConvert(nodeConfig.m_orientation));
        pxRigidDynamic->setGlobalPose(transform);

        m_actorUserData = PhysX::ActorData(pxRigidDynamic);
        m_actorUserData.SetRagdollNode(this);
        m_actorUserData.SetEntityId(m_rigidBody->GetEntityId());
    }

    void RagdollNode::DestroyPhysicsBody()
    {
        if (m_rigidBody != nullptr)
        {
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                sceneInterface->RemoveSimulatedBody(m_sceneOwner, m_rigidBodyHandle);
            }
            m_rigidBody = nullptr;
            m_sceneOwner = AzPhysics::InvalidSceneHandle;
        }
    }

    void RagdollNode::DestroyJoint()
    {
        if (m_joint != nullptr && m_sceneOwner != AzPhysics::InvalidSceneHandle)
        {
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                sceneInterface->RemoveJoint(m_sceneOwner, m_joint->m_jointHandle);
            }
            m_joint = nullptr;
        }
    }

} // namespace PhysX
