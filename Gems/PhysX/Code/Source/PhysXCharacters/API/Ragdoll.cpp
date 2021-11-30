/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/SystemBus.h>
#include <PhysXCharacters/API/Ragdoll.h>
#include <PhysXCharacters/API/CharacterUtils.h>
#include <PhysX/NativeTypeIdentifiers.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/MathConversion.h>
#include <Scene/PhysXScene.h>

namespace PhysX
{
    namespace Internal
    {
        physx::PxScene* GetPxScene(AzPhysics::SceneHandle sceneHandle)
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                if (AzPhysics::Scene* scene = physicsSystem->GetScene(sceneHandle))
                {
                    return static_cast<physx::PxScene*>(scene->GetNativePointer());
                }
            }
            return nullptr;
        }
    } // namespace Internal

    void Ragdoll::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PhysX::Ragdoll, Physics::Ragdoll>()
                ->Version(1)
                ;
        }
    }

    void Ragdoll::AddNode(AZStd::unique_ptr<RagdollNode> node)
    {
        m_nodes.push_back(AZStd::move(node));
    }

    void Ragdoll::SetParentIndices(const Physics::ParentIndices& parentIndices)
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
            AzPhysics::RigidBody& rigidBody = m_nodes[nodeIndex]->GetRigidBody();
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

    Ragdoll::Ragdoll(AzPhysics::SceneHandle sceneHandle)
        : m_sceneStartSimHandler([this](
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            [[maybe_unused]] float fixedDeltaTime)
            {
                this->ApplyQueuedEnableSimulation();
                this->ApplyQueuedSetState();
                this->ApplyQueuedDisableSimulation();
            })
    {
        m_sceneOwner = sceneHandle;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RegisterSceneSimulationStartHandler(m_sceneOwner, m_sceneStartSimHandler);
        }
    }

    Ragdoll::~Ragdoll()
    {
        m_sceneStartSimHandler.Disconnect();

        m_nodes.clear(); //the nodes destructor will remove the simulated body from the scene.
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
        if (m_simulating)
        {
            return;
        }

        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (sceneInterface == nullptr)
        {
            AZ_Error("PhysX Ragdoll", false, "Unable to Enable Ragdoll, Physics Scene Interface is missing.");
            return;
        }

        physx::PxScene* pxScene = Internal::GetPxScene(m_sceneOwner);

        if (pxScene == nullptr)
        {
            AZ_Error("PhysX Ragdoll", false, "Unable to Enable Ragdoll, Unable to retrieve Physics Scene Interface is missing.");
            return;
        }       

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

                sceneInterface->EnableSimulationOfBody(m_sceneOwner, m_nodes[nodeIndex]->GetRigidBodyHandle());
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
                    sceneInterface->SuppressCollisionEvents(m_sceneOwner,
                        m_nodes[nodeIndex]->GetRigidBodyHandle(), m_nodes[parentIndex]->GetRigidBodyHandle());
                }
            }
        }

        sceneInterface->EnableSimulationOfBody(m_sceneOwner, m_bodyHandle);
    }

    void Ragdoll::EnableSimulationQueued(const Physics::RagdollState& initialState)
    {
        if (m_simulating)
        {
            return;
        }

        m_queuedInitialState = initialState;
    }

    void Ragdoll::DisableSimulation()
    {
        if (!m_simulating)
        {
            return;
        }
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (sceneInterface == nullptr)
        {
            AZ_Error("PhysX Ragdoll", false, "Unable to Disable Ragdoll, Physics Scene Interface is missing.");
            return;
        }

        physx::PxScene* pxScene = Internal::GetPxScene(m_sceneOwner);
        const size_t numNodes = m_nodes.size();

        PHYSX_SCENE_WRITE_LOCK(pxScene);

        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            sceneInterface->DisableSimulationOfBody(m_sceneOwner, m_nodes[nodeIndex]->GetRigidBodyHandle());

            if (nodeIndex < m_parentIndices.size())
            {
                size_t parentIndex = m_parentIndices[nodeIndex];
                if (parentIndex < numNodes)
                {
                    sceneInterface->UnsuppressCollisionEvents(m_sceneOwner,
                        m_nodes[nodeIndex]->GetRigidBodyHandle(), m_nodes[parentIndex]->GetRigidBodyHandle());
                }
            }
        }

        sceneInterface->DisableSimulationOfBody(m_sceneOwner, m_bodyHandle);
    }

    void Ragdoll::DisableSimulationQueued()
    {
        m_queuedDisableSimulation = true;
    }

    bool Ragdoll::IsSimulated() const
    {
        return m_simulating;
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

            if (AzPhysics::Joint* joint = m_nodes[nodeIndex]->GetJoint())
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

    AZ::EntityId Ragdoll::GetEntityId() const
    {
        AZ_Warning("PhysX Ragdoll", false, "Not yet supported.");
        return AZ::EntityId(AZ::EntityId::InvalidEntityId);
    }

    AzPhysics::Scene* Ragdoll::GetScene()
    {
        return m_nodes.empty() ? nullptr : m_nodes[0]->GetScene();
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

    AzPhysics::SceneQueryHit Ragdoll::RayCast(const AzPhysics::RayCastRequest& request)
    {
        AzPhysics::SceneQueryHit closestHit;
        float closestHitDist = FLT_MAX;
        for (int i = 0; i < m_nodes.size(); ++i)
        {
            AzPhysics::SceneQueryHit hit = m_nodes[i]->RayCast(request);
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
} // namespace PhysX
