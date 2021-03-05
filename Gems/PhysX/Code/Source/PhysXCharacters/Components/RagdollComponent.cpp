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
#include <PhysXCharacters/API/CharacterUtils.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Physics/CharacterPhysicsDataBus.h>
#include <PhysXCharacters/Components/RagdollComponent.h>

namespace PhysX
{
    bool RagdollComponent::VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        // The element "PhysXRagdoll" was changed from a shared pointer to a unique pointer, but a version converter was
        // not added at the time.  This means there may be serialized data with either the shared or unique pointer, but
        // both labelled version 1.  This converter was added later and needs to deal with either eventuality, producing
        // a valid version 2 in either case.
        if (classElement.GetVersion() <= 1)
        {
            int ragdollElementIndex = classElement.FindElement(AZ_CRC("PhysXRagdoll", 0xe081b8b0));

            if (ragdollElementIndex >= 0)
            {
                AZ::SerializeContext::DataElementNode& ragdollElement = classElement.GetSubElement(ragdollElementIndex);

                // If we find a shared pointer, change it to a unique pointer.  If we don't, we already have a unique
                // pointer and it's fine to do nothing but bump the version number from 1 to 2.
                AZStd::shared_ptr<Ragdoll> ragdollShared = nullptr;
                bool isShared = ragdollElement.GetData<AZStd::shared_ptr<Ragdoll>>(ragdollShared);
                if (isShared)
                {
                    // The shared pointer doesn't actually contain any serialized data - it is a runtime only object and
                    // should probably never have been serialized, but removing it may cause issues with slices.  So
                    // there is no need to extract any data and it can be replaced with an empty unique pointer.
                    classElement.RemoveElement(ragdollElementIndex);
                    classElement.AddElement<AZStd::unique_ptr<Ragdoll>>(context, "PhysXRagdoll");
                }
            }
        }

        return true;
    }

    void RagdollComponent::Reflect(AZ::ReflectContext* context)
    {
        Ragdoll::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<RagdollComponent, AZ::Component>()
                ->Version(2, &VersionConverter)
                ->Field("PhysXRagdoll", &RagdollComponent::m_ragdoll)
                ->Field("PositionIterations", &RagdollComponent::m_positionIterations)
                ->Field("VelocityIterations", &RagdollComponent::m_velocityIterations)
                ->Field("EnableJointProjection", &RagdollComponent::m_enableJointProjection)
                ->Field("ProjectionLinearTol", &RagdollComponent::m_jointProjectionLinearTolerance)
                ->Field("ProjectionAngularTol", &RagdollComponent::m_jointProjectionAngularToleranceDegrees)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<RagdollComponent>(
                    "PhysX Ragdoll", "Provides simulation of characters in PhysX.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PhysXRagdoll.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/PhysXRagdoll.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RagdollComponent::m_positionIterations, "Position Iteration Count",
                        "A higher iteration count generally improves fidelity at the cost of performance, but note that very high "
                        "values may lead to severe instability if ragdoll colliders interfere with satisfying joint constraints")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RagdollComponent::m_velocityIterations, "Velocity Iteration Count",
                        "A higher iteration count generally improves fidelity at the cost of performance, but note that very high "
                        "values may lead to severe instability if ragdoll colliders interfere with satisfying joint constraints")
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RagdollComponent::m_enableJointProjection,
                        "Enable Joint Projection", "Whether to use joint projection to preserve joint constraints "
                        "in demanding situations at the expense of potentially reducing physical correctness")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RagdollComponent::m_jointProjectionLinearTolerance,
                        "Joint Projection Linear Tolerance", "Linear joint error above which projection will be applied")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 1e-3f)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &RagdollComponent::IsJointProjectionVisible)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RagdollComponent::m_jointProjectionAngularToleranceDegrees,
                        "Joint Projection Angular Tolerance", "Angular joint error (in degrees) above which projection will be applied")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &RagdollComponent::IsJointProjectionVisible)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
        }
    }

    bool RagdollComponent::IsJointProjectionVisible()
    {
        return m_enableJointProjection;
    }

    // AZ::Component
    void RagdollComponent::Init()
    {
    }

    Physics::RagdollState GetBindPoseWorld(const Physics::RagdollState& bindPose, const AZ::Transform& worldTransform)
    {
        const AZ::Quaternion worldRotation = worldTransform.GetRotation();
        const AZ::Vector3 worldTranslation = worldTransform.GetTranslation();

        Physics::RagdollState bindPoseWorld;
        const size_t numNodes = bindPose.size();
        bindPoseWorld.resize(numNodes);
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            bindPoseWorld[nodeIndex].m_orientation = worldRotation * bindPose[nodeIndex].m_orientation;
            bindPoseWorld[nodeIndex].m_position = worldRotation.TransformVector(bindPose[nodeIndex].m_position) + worldTranslation;
        }

        return bindPoseWorld;
    }

    void RagdollComponent::Activate()
    {
        AzFramework::CharacterPhysicsDataNotificationBus::Handler::BusConnect(GetEntityId());

        Reinit();
    }

    void RagdollComponent::Reinit()
    {
        bool ragdollConfigValid = false;
        Physics::RagdollConfiguration ragdollConfiguration;
        AzFramework::CharacterPhysicsDataRequestBus::EventResult(ragdollConfigValid, GetEntityId(),
            &AzFramework::CharacterPhysicsDataRequests::GetRagdollConfiguration, ragdollConfiguration);

        // Reinit is called in two cases:
        // 1. When activating the ragdoll component
        //    In this case the ragdoll configuration might not be available yet as the actor asset hasn't finished loading.
        //    Early out in this case and wait for the OnRagdollConfigurationReady() callback.
        // 2. OnRagdollConfigurationReady() callback
        if (!ragdollConfigValid)
        {
            m_ragdoll.reset();
            return;
        }

        const size_t numNodes = ragdollConfiguration.m_nodes.size();

        if (numNodes == 0)
        {
            AZ_Error("PhysX Ragdoll Component", false,
                "Ragdoll configuration has 0 nodes, ragdoll will not be created for entity \"%s\".",
                GetEntity()->GetName().c_str());
            return;
        }

        ParentIndices parentIndices;
        parentIndices.resize(numNodes);
        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            AZStd::string parentName;
            AZStd::string nodeName = ragdollConfiguration.m_nodes[nodeIndex].m_debugName;
            AzFramework::CharacterPhysicsDataRequestBus::EventResult(parentName, GetEntityId(),
                &AzFramework::CharacterPhysicsDataRequests::GetParentNodeName, nodeName);
            AZ::Outcome<size_t> parentIndex = Utils::Characters::GetNodeIndex(ragdollConfiguration, parentName);
            parentIndices[nodeIndex] = parentIndex ? parentIndex.GetValue() : SIZE_MAX;

            ragdollConfiguration.m_nodes[nodeIndex].m_entityId = GetEntityId();
        }

        Physics::RagdollState bindPose;
        AzFramework::CharacterPhysicsDataRequestBus::EventResult(bindPose, GetEntityId(),
            &AzFramework::CharacterPhysicsDataRequests::GetBindPose, ragdollConfiguration);

        AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(entityTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        Physics::RagdollState bindPoseWorld = GetBindPoseWorld(bindPose, entityTransform);

        m_ragdoll = Utils::Characters::CreateRagdoll(ragdollConfiguration, bindPoseWorld, parentIndices);
        if (!m_ragdoll)
        {
            AZ_Error("PhysX Ragdoll Component", false, "Failed to create ragdoll.");
            return;
        }

        for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
        {
            if (auto pxRigidBody = m_ragdoll->GetPxRigidDynamic(nodeIndex))
            {
                pxRigidBody->setSolverIterationCounts(m_positionIterations, m_velocityIterations);
            }
        }

        if (m_enableJointProjection)
        {
            const float linearTolerance = AZ::GetMax(0.0f, m_jointProjectionLinearTolerance);
            const float angularTolerance = AZ::DegToRad(AZ::GetMax(0.0f, m_jointProjectionAngularToleranceDegrees));

            for (size_t nodeIndex = 0; nodeIndex < numNodes; nodeIndex++)
            {
                if (auto joint = m_ragdoll->GetNode(nodeIndex)->GetJoint())
                {
                    if (auto pxJoint = static_cast<physx::PxD6Joint*>(joint->GetNativePointer()))
                    {
                        pxJoint->setConstraintFlag(physx::PxConstraintFlag::ePROJECTION, true);
                        pxJoint->setConstraintFlag(physx::PxConstraintFlag::ePROJECT_TO_ACTOR0, true);
                        pxJoint->setProjectionLinearTolerance(linearTolerance);
                        pxJoint->setProjectionAngularTolerance(angularTolerance);
                    }
                }
            }
        }

        AzFramework::RagdollPhysicsRequestBus::Handler::BusConnect(GetEntityId());
        Physics::WorldBodyRequestBus::Handler::BusConnect(GetEntityId());

        AzFramework::RagdollPhysicsNotificationBus::Event(GetEntityId(),
            &AzFramework::RagdollPhysicsNotifications::OnRagdollActivated);
    }

    void RagdollComponent::Deactivate()
    {
        Physics::WorldBodyRequestBus::Handler::BusDisconnect();
        AzFramework::RagdollPhysicsRequestBus::Handler::BusDisconnect();
        DisableSimulation();
        AzFramework::RagdollPhysicsNotificationBus::Event(GetEntityId(),
            &AzFramework::RagdollPhysicsNotifications::OnRagdollDeactivated);
        AzFramework::CharacterPhysicsDataNotificationBus::Handler::BusDisconnect();
    }

    // RagdollPhysicsBus
    void RagdollComponent::EnableSimulation(const Physics::RagdollState& initialState)
    {
        m_ragdoll->EnableSimulation(initialState);
    }

    void RagdollComponent::EnableSimulationQueued(const Physics::RagdollState& initialState)
    {
        m_ragdoll->EnableSimulationQueued(initialState);
    }

    void RagdollComponent::DisableSimulation()
    {
        if (m_ragdoll)
        {
            m_ragdoll->DisableSimulation();
        }
    }

    void RagdollComponent::DisableSimulationQueued()
    {
        if (m_ragdoll)
        {
            m_ragdoll->DisableSimulationQueued();
        }
    }

    Physics::Ragdoll* RagdollComponent::GetRagdoll()
    {
        return m_ragdoll.get();
    }

    void RagdollComponent::GetState(Physics::RagdollState& ragdollState) const
    {
        m_ragdoll->GetState(ragdollState);
    }

    void RagdollComponent::SetState(const Physics::RagdollState& ragdollState)
    {
        m_ragdoll->SetState(ragdollState);
    }

    void RagdollComponent::SetStateQueued(const Physics::RagdollState& ragdollState)
    {
        m_ragdoll->SetStateQueued(ragdollState);
    }

    void RagdollComponent::GetNodeState(size_t nodeIndex, Physics::RagdollNodeState& nodeState) const
    {
        m_ragdoll->GetNodeState(nodeIndex, nodeState);
    }

    void RagdollComponent::SetNodeState(size_t nodeIndex, const Physics::RagdollNodeState& nodeState)
    {
        m_ragdoll->SetNodeState(nodeIndex, nodeState);
    }

    Physics::RagdollNode* RagdollComponent::GetNode(size_t nodeIndex) const
    {
        return m_ragdoll->GetNode(nodeIndex);
    }

    void RagdollComponent::EnablePhysics()
    {
        // do nothing here, ragdolls are enabled via EnableSimulation
        // don't raise an error though, because the character controller component may also be handling the world body
        // request bus and it would be legitimate to call this function on this entity ID
    }

    void RagdollComponent::DisablePhysics()
    {
        // do nothing here, ragdolls are disabled via DisableSimulation
        // don't raise an error though, because the character controller component may also be handling the world body
        // request bus and it would be legitimate to call this function on this entity ID
    }

    bool RagdollComponent::IsPhysicsEnabled() const
    {
        return m_ragdoll && m_ragdoll->IsSimulated();
    }

    AZ::Aabb RagdollComponent::GetAabb() const
    {
        if (m_ragdoll)
        {
            return m_ragdoll->GetAabb();
        }
        return AZ::Aabb::CreateNull();
    }

    Physics::WorldBody* RagdollComponent::GetWorldBody()
    {
        return m_ragdoll.get();
    }

    Physics::RayCastHit RagdollComponent::RayCast(const Physics::RayCastRequest& request)
    {
        if (m_ragdoll)
        {
            return m_ragdoll->RayCast(request);
        }
        return Physics::RayCastHit();
    }

    void RagdollComponent::OnRagdollConfigurationReady()
    {
        // The ragdoll configuration currently is stored within the actor component and loaded along with the actor asset.
        // We'll have to wait until the actor asset it loaded and then trigger a reinit.
        Reinit();
    }

    // deprecated Cry functions
    void RagdollComponent::EnterRagdoll()
    {
        AZ_WarningOnce("PhysX Ragdoll", false, "Legacy Cry character function not supported in PhysX.");
    }

    void RagdollComponent::ExitRagdoll()
    {
        AZ_WarningOnce("PhysX Ragdoll", false, "Legacy Cry character function not supported in PhysX.");
    }
} // namespace PhysX
