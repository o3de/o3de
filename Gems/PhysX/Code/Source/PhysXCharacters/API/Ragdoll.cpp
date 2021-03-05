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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/SystemBus.h>
#include <PhysXCharacters/API/Ragdoll.h>
#include <PhysXCharacters/API/CharacterUtils.h>
#include <PhysX/NativeTypeIdentifiers.h>
#include <PhysX/PhysXLocks.h>

namespace PhysX
{
    // PhysX::Ragdoll
    void Ragdoll::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Ragdoll>()
                ->Version(1)
                ;
        }
    }

    void Ragdoll::AddNode(AZStd::unique_ptr<RagdollNode> node)
    {
        m_nodes.push_back(AZStd::move(node));
    }

    void Ragdoll::SetParentIndices(const ParentIndices& parentIndices)
    {
        m_parentIndices = parentIndices;
    }

    void Ragdoll::SetRootIndex(size_t nodeIndex)
    {
        m_rootIndex = AZ::Success(nodeIndex);
    }

    physx::PxRigidDynamic* Ragdoll::GetPxRigidDynamic(size_t nodeIndex) const
    {
        if (nodeIndex < m_nodes.size())
        {
            Physics::RigidBody& rigidBody = m_nodes[nodeIndex]->GetRigidBody();
            return static_cast<physx::PxRigidDynamic*>(rigidBody.GetNativePointer());
        }

        AZ_Error("PhysX Ragdoll", false, "Invalid node index (%i) in ragdoll with %i nodes.", nodeIndex, m_nodes.size());
        return nullptr;
    }

    physx::PxTransform Ragdoll::GetRootPxTransform() const
    {
        if (m_rootIndex && m_rootIndex.GetValue() < m_nodes.size())
        {
            physx::PxRigidDynamic* rigidDynamic = GetPxRigidDynamic(m_rootIndex.GetValue());
            if (rigidDynamic)
            {
                PHYSX_SCENE_READ_LOCK(rigidDynamic->getScene());
                return rigidDynamic->getGlobalPose();
            }
            else
            {
                AZ_Error("PhysX Ragdoll", false, "No valid PhysX actor for root node.");
                return physx::PxTransform(physx::PxIdentity);
            }
        }

        AZ_Error("PhysX Ragdoll", false, "Invalid root index.");
        return physx::PxTransform(physx::PxIdentity);
    }

    Ragdoll::Ragdoll()
        : m_isSimulated(false)
    {
    }

    Ragdoll::~Ragdoll()
    {
        Physics::WorldNotificationBus::Handler::BusDisconnect();
    }

    void Ragdoll::ApplyQueuedEnableSimulation()
    {
        if (m_queuedInitialState.empty())
        {
            return;
        }

        EnableSimulation(m_queuedInitialState);
        m_queuedInitialState.clear();
    }

    void Ragdoll::ApplyQueuedSetState()
    {
        if (m_queuedState.empty())
        {
            return;
        }

        SetState(m_queuedState);
        m_queuedState.clear();
    }

    void Ragdoll::ApplyQueuedDisableSimulation()
    {
        if (m_queuedDisableSimulation)
        {
            DisableSimulation();
        }
        m_queuedDisableSimulation = false;
    }

    // Physics::Ragdoll
    void Ragdoll::EnableSimulation(const Physics::RagdollState& initialState)
    {
        if (m_isSimulated)
        {
            return;
        }

        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
        physx::PxScene* pxScene = static_cast<physx::PxScene*>(world->GetNativePointer());

        const size_t numNodes = m_nodes.size();
        if (initialState.size() != numNodes)
        {
            AZ_Error("PhysX Ragdoll", false, "Mismatch between the number of nodes in the ragdoll initial state (%i)"
                "and the number of nodes in the ragdoll (%i).", initialState.size(), numNodes);
            return;
        }

        PHYSX_SCENE_WRITE_LOCK(pxScene);

        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            physx::PxRigidDynamic* pxActor = GetPxRigidDynamic(nodeIndex);
            if (pxActor)
            {
                const Physics::RagdollNodeState& nodeState = initialState[nodeIndex];
                physx::PxTransform pxTm(PxMathConvert(nodeState.m_position), PxMathConvert(nodeState.m_orientation));
                pxActor->setGlobalPose(pxTm);
                pxActor->setLinearVelocity(PxMathConvert(nodeState.m_linearVelocity));
                pxActor->setAngularVelocity(PxMathConvert(nodeState.m_angularVelocity));
                pxScene->addActor(*pxActor);
            }

            else
            {
                AZ_Error("PhysX Ragdoll", false, "Invalid PhysX actor for node index %i", nodeIndex);
            }

            if (nodeIndex < m_parentIndices.size())
            {
                size_t parentIndex = m_parentIndices[nodeIndex];
                if (parentIndex < numNodes)
                {
                    world->RegisterSuppressedCollision(m_nodes[nodeIndex]->GetRigidBody(), m_nodes[parentIndex]->GetRigidBody());
                }
            }
        }

        Physics::WorldNotificationBus::Handler::BusConnect(world->GetWorldId());

        m_isSimulated = true;
    }

    void Ragdoll::EnableSimulationQueued(const Physics::RagdollState& initialState)
    {
        if (m_isSimulated)
        {
            return;
        }

        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);

        Physics::WorldNotificationBus::Handler::BusConnect(world->GetWorldId());

        m_queuedInitialState = initialState;
    }

    void Ragdoll::DisableSimulation()
    {
        if (!m_isSimulated)
        {
            return;
        }

        Physics::WorldNotificationBus::Handler::BusDisconnect();

        AZStd::shared_ptr<Physics::World> world;
        Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
        physx::PxScene* pxScene = static_cast<physx::PxScene*>(world->GetNativePointer());
        const size_t numNodes = m_nodes.size();

        PHYSX_SCENE_WRITE_LOCK(pxScene);

        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            pxScene->removeActor(*GetPxRigidDynamic(nodeIndex));

            if (nodeIndex < m_parentIndices.size())
            {
                size_t parentIndex = m_parentIndices[nodeIndex];
                if (parentIndex < numNodes)
                {
                    world->UnregisterSuppressedCollision(m_nodes[nodeIndex]->GetRigidBody(), m_nodes[parentIndex]->GetRigidBody());
                }
            }
        }

        m_isSimulated = false;
    }

    void Ragdoll::DisableSimulationQueued()
    {
        m_queuedDisableSimulation = true;
    }

    bool Ragdoll::IsSimulated()
    {
        return m_isSimulated;
    }

    void Ragdoll::GetState(Physics::RagdollState& ragdollState) const
    {
        ragdollState.resize(m_nodes.size());

        const size_t numNodes = m_nodes.size();
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            GetNodeState(nodeIndex, ragdollState[nodeIndex]);
        }
    }

    void Ragdoll::SetState(const Physics::RagdollState& ragdollState)
    {
        if (ragdollState.size() != m_nodes.size())
        {
            AZ_ErrorOnce("PhysX Ragdoll", false, "Mismatch between number of nodes in desired ragdoll state (%i) and ragdoll (%i)",
                ragdollState.size(), m_nodes.size());
            return;
        }

        const size_t numNodes = m_nodes.size();
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            SetNodeState(nodeIndex, ragdollState[nodeIndex]);
        }
    }

    void Ragdoll::SetStateQueued(const Physics::RagdollState& ragdollState)
    {
        m_queuedState = ragdollState;
    }

    void Ragdoll::GetNodeState(size_t nodeIndex, Physics::RagdollNodeState& nodeState) const
    {
        if (nodeIndex >= m_nodes.size())
        {
            AZ_Error("PhysX Ragdoll", false, "Invalid node index (%i) in ragdoll with %i nodes.", nodeIndex, m_nodes.size());
            return;
        }

        const physx::PxRigidDynamic* actor = GetPxRigidDynamic(nodeIndex);
        if (!actor)
        {
            AZ_Error("Physx Ragdoll", false, "No PhysX actor associated with ragdoll node %i", nodeIndex);
            return;
        }

        PHYSX_SCENE_READ_LOCK(actor->getScene());

        nodeState.m_position = PxMathConvert(actor->getGlobalPose().p);
        nodeState.m_orientation = PxMathConvert(actor->getGlobalPose().q);
        nodeState.m_linearVelocity = PxMathConvert(actor->getLinearVelocity());
        nodeState.m_angularVelocity = PxMathConvert(actor->getAngularVelocity());
    }

    void Ragdoll::SetNodeState(size_t nodeIndex, const Physics::RagdollNodeState& nodeState)
    {
        if (nodeIndex >= m_nodes.size())
        {
            AZ_Error("PhysX Ragdoll", false, "Invalid node index (%i) in ragdoll with %i nodes.", nodeIndex, m_nodes.size());
            return;
        }

        physx::PxRigidDynamic* actor = GetPxRigidDynamic(nodeIndex);
        if (!actor)
        {
            AZ_Error("Physx Ragdoll", false, "No PhysX actor associated with ragdoll node %i", nodeIndex);
            return;
        }

        PHYSX_SCENE_WRITE_LOCK(actor->getScene());

        if (nodeState.m_simulationType == Physics::SimulationType::Kinematic)
        {
            actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
            actor->setKinematicTarget(physx::PxTransform(
                PxMathConvert(nodeState.m_position),
                PxMathConvert(nodeState.m_orientation)));
        }

        else
        {
            actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
            const AZStd::shared_ptr<Physics::Joint>& joint = m_nodes[nodeIndex]->GetJoint();
            if (joint)
            {
                if (physx::PxD6Joint* pxJoint = static_cast<physx::PxD6Joint*>(joint->GetNativePointer()))
                {
                    float forceLimit = std::numeric_limits<float>::max();
                    physx::PxD6JointDrive jointDrive = Utils::Characters::CreateD6JointDrive(nodeState.m_strength,
                        nodeState.m_dampingRatio, forceLimit);
                    pxJoint->setDrive(physx::PxD6Drive::eSWING, jointDrive);
                    pxJoint->setDrive(physx::PxD6Drive::eTWIST, jointDrive);
                    physx::PxQuat targetRotation = pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR0).q.getConjugate() *
                        PxMathConvert(nodeState.m_orientation) * pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR1).q;
                    pxJoint->setDrivePosition(physx::PxTransform(targetRotation));
                }
            }
        }
    }

    Physics::RagdollNode* Ragdoll::GetNode(size_t nodeIndex) const
    {
        if (nodeIndex < m_nodes.size())
        {
            return m_nodes[nodeIndex].get();
        }

        AZ_Error("PhysX Ragdoll", false, "Invalid node index %i in ragdoll with %i nodes.", nodeIndex, m_nodes.size());
        return nullptr;
    }

    size_t Ragdoll::GetNumNodes() const
    {
        return m_nodes.size();
    }

    AZ::Crc32 Ragdoll::GetWorldId() const
    {
        return Physics::DefaultPhysicsWorldId;
    }

    // Physics::WorldBody
    AZ::EntityId Ragdoll::GetEntityId() const
    {
        AZ_Warning("PhysX Ragdoll", false, "Not yet supported.");
        return AZ::EntityId(AZ::EntityId::InvalidEntityId);
    }

    Physics::World* Ragdoll::GetWorld() const
    {
        return m_nodes.empty() ? nullptr : m_nodes[0]->GetWorld();
    }

    AZ::Transform Ragdoll::GetTransform() const
    {
        return PxMathConvert(GetRootPxTransform());
    }

    void Ragdoll::SetTransform([[maybe_unused]] const AZ::Transform& transform)
    {
        AZ_WarningOnce("PhysX Ragdoll", false, "Directly setting the transform for the whole ragdoll is not supported.  "
            "Use SetState or SetNodeState to set transforms for individual ragdoll nodes.");
    }

    AZ::Vector3 Ragdoll::GetPosition() const
    {
        return PxMathConvert(GetRootPxTransform().p);
    }

    AZ::Quaternion Ragdoll::GetOrientation() const
    {
        return PxMathConvert(GetRootPxTransform().q);
    }

    AZ::Aabb Ragdoll::GetAabb() const
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        for (int i = 0; i < m_nodes.size(); ++i)
        {
            if (m_nodes[i]->GetRigidBody().GetShapeCount() > 0)
            {
                aabb.AddAabb(m_nodes[i]->GetAabb());
            }
        }
        return aabb;
    }

    Physics::RayCastHit Ragdoll::RayCast(const Physics::RayCastRequest& request)
    {
        Physics::RayCastHit closestHit;
        float closestHitDist = FLT_MAX;
        for (int i = 0; i < m_nodes.size(); ++i)
        {
            Physics::RayCastHit hit = m_nodes[i]->RayCast(request);
            if (hit && hit.m_distance < closestHitDist)
            {
                closestHit = hit;
                closestHitDist = hit.m_distance;
            }
        }
        return closestHit;
    }

    AZ::Crc32 Ragdoll::GetNativeType() const
    {
        return NativeTypeIdentifiers::Ragdoll;
    }

    void* Ragdoll::GetNativePointer() const
    {
        AZ_WarningOnce("PhysX Ragdoll", false, "Not yet supported.");
        return nullptr;
    }

    void Ragdoll::AddToWorld(Physics::World& world)
    {
        for (auto& node : m_nodes)
        {
            node->GetRigidBody().AddToWorld(world);
        }
    }

    void Ragdoll::RemoveFromWorld(Physics::World& world)
    {
        for (auto& node : m_nodes)
        {
            node->GetRigidBody().RemoveFromWorld(world);
        }
    }

    // Physics::WorldNotificationBus
    void Ragdoll::OnPrePhysicsSubtick([[maybe_unused]] float fixedDeltaTime)
    {
        ApplyQueuedEnableSimulation();
        ApplyQueuedSetState();
        ApplyQueuedDisableSimulation();
    }
} // namespace PhysX
