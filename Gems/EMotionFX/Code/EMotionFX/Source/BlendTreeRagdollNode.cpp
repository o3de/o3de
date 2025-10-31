/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <MCore/Source/AzCoreConversions.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphRefCountedData.h>
#include <EMotionFX/Source/BlendTreeRagdollNode.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/PoseDataRagdoll.h>
#include <EMotionFX/Source/RagdollInstance.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRagdollNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRagdollNode::UniqueData, AnimGraphAllocator)

    BlendTreeRagdollNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
        , m_isRagdollRootNodeSimulated(false)
    {
    }

    void BlendTreeRagdollNode::UniqueData::Update()
    {
        BlendTreeRagdollNode* ragdollNode = azdynamic_cast<BlendTreeRagdollNode*>(m_object);
        AZ_Assert(ragdollNode, "Unique data linked to incorrect node type.");

        const Actor* actor = m_animGraphInstance->GetActorInstance()->GetActor();
        const Skeleton* skeleton = actor->GetSkeleton();
        const size_t jointCount = skeleton->GetNumNodes();

        // Fill in the flags to indicate which of the joints are added to the physics simulation by this node.
        // This information prevents runtime searches as we need to update the target pose transforms only for the
        // joints selected by this node and not for all dynamic ones.
        m_simulatedJointStates.resize(jointCount);
        m_simulatedJointStates.assign(m_simulatedJointStates.size(), false);

        const AZStd::vector<AZStd::string>& simulatedJointNames = ragdollNode->GetSimulatedJointNames();
        for (const AZStd::string& jointName : simulatedJointNames)
        {
            const Node* node = skeleton->FindNodeByName(jointName);
            if (node)
            {
                m_simulatedJointStates[node->GetNodeIndex()] = true;
            }
        }

        // Check if we selected the ragdoll root node to be added to the simulation.
        const ActorInstance* actorInstance = m_animGraphInstance->GetActorInstance();
        const RagdollInstance* ragdollInstance = actorInstance->GetRagdollInstance();
        m_isRagdollRootNodeSimulated = false;
        if (ragdollInstance)
        {
            const Node* ragdollRootNode = ragdollInstance->GetRagdollRootNode();
            if (ragdollRootNode)
            {
                if (AZStd::find(simulatedJointNames.begin(), simulatedJointNames.end(), ragdollRootNode->GetNameString()) != simulatedJointNames.end())
                {
                    m_isRagdollRootNodeSimulated = true;
                }
            }
        }

    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    BlendTreeRagdollNode::BlendTreeRagdollNode()
        : AnimGraphNode()
    {
        InitInputPorts(2);
        SetupInputPort("Target Pose", INPUTPORT_TARGETPOSE, AttributePose::TYPE_ID, PORTID_TARGETPOSE);
        SetupInputPortAsNumber("Activate", INPUTPORT_ACTIVATE, PORTID_ACTIVATE);

        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }

    bool BlendTreeRagdollNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    void BlendTreeRagdollNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);

        if (HasConnectionAtInputPort(INPUTPORT_TARGETPOSE))
        {
            AnimGraphNode* inputNodeTargetPose = GetInputNode(INPUTPORT_TARGETPOSE);
            UpdateIncomingNode(animGraphInstance, inputNodeTargetPose, timePassedInSeconds);

            // Forward the duration, playspeed etc. from the input target pose.
            uniqueData->Init(animGraphInstance, inputNodeTargetPose);
        }
        else
        {
            uniqueData->Clear();
        }

        if (HasConnectionAtInputPort(INPUTPORT_ACTIVATE))
        {
            UpdateIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_ACTIVATE), timePassedInSeconds);
        }
    }

    void BlendTreeRagdollNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        RequestRefDatas(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

        if (m_disabled)
        {
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        if (HasConnectionAtInputPort(INPUTPORT_ACTIVATE))
        {
            PostUpdateIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_ACTIVATE), timePassedInSeconds);
        }

        if (HasConnectionAtInputPort(INPUTPORT_TARGETPOSE))
        {
            AnimGraphNode* inputNodeTargetPose = GetInputNode(INPUTPORT_TARGETPOSE);
            PostUpdateIncomingNode(animGraphInstance, inputNodeTargetPose, timePassedInSeconds);

            // Forward the event buffer from the target pose.
            AnimGraphRefCountedData* sourceData = inputNodeTargetPose->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
            data->SetEventBuffer(sourceData->GetEventBuffer());
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
        }
        else
        {
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
        }

        if (IsActivated(animGraphInstance))
        {
            const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            const RagdollInstance* ragdollInstance = actorInstance->GetRagdollInstance();

            // Apply the motion extraction delta from the ragdoll only in case the ragdoll root node is simulated.
            if (ragdollInstance && uniqueData->m_isRagdollRootNodeSimulated)
            {
                const Actor* actor = actorInstance->GetActor();
                const Node* motionExtractionNode = actor->GetMotionExtractionNode();

                Transform trajectoryDelta = Transform::CreateIdentityWithZeroScale();
                if (ragdollInstance && motionExtractionNode)
                {
                    // Move the trajectory node based on the ragdoll's movement.
                    trajectoryDelta.m_position = ragdollInstance->GetTrajectoryDeltaPos();

                    // Do the same for rotation, but extract and apply z rotation only to the trajectory node.
                    trajectoryDelta.m_rotation = ragdollInstance->GetTrajectoryDeltaRot();
                    trajectoryDelta.m_rotation.SetX(0.0f);
                    trajectoryDelta.m_rotation.SetY(0.0f);
                    trajectoryDelta.m_rotation.Normalize();
                }

                data->SetTrajectoryDelta(trajectoryDelta);
                data->SetTrajectoryDeltaMirrored(trajectoryDelta);
            }
        }
    }

    void BlendTreeRagdollNode::Output(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        RequestPoses(animGraphInstance);
        AnimGraphPose* animGraphOutputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();

        const Pose* targetPose = nullptr;
        if (HasConnectionAtInputPort(INPUTPORT_TARGETPOSE))
        {
            // Forward the input (target) pose to the output pose in case there is a connection.
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_TARGETPOSE));
            const AnimGraphPose* animGraphInputPose = GetInputPose(animGraphInstance, INPUTPORT_TARGETPOSE)->GetValue();
            *animGraphOutputPose = *animGraphInputPose;
            targetPose = &animGraphInputPose->GetPose();
        }
        else
        {
            // In case no target pose is connected, use the bind pose as base.
            animGraphOutputPose->InitFromBindPose(actorInstance);
        }

        // As we already forwarded the target pose at this point, we can just return in case the node is disabled.
        if (m_disabled)
        {
            return;
        }

        Pose& outputPose = animGraphOutputPose->GetPose();
        if (GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose, m_visualizeColor);
        }

        if (HasConnectionAtInputPort(INPUTPORT_ACTIVATE))
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_ACTIVATE));
        }
        const bool isActivated = IsActivated(animGraphInstance);

        RagdollInstance* ragdollInstance = actorInstance->GetRagdollInstance();
        if (isActivated && ragdollInstance && !m_simulatedJointNames.empty())
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

            // Make sure the output pose contains a ragdoll pose data linked to our actor instance (assures enough space for the ragdoll node state array).
            PoseDataRagdoll* outputPoseData = outputPose.GetAndPreparePoseData<PoseDataRagdoll>(actorInstance);

            const Actor* actor = actorInstance->GetActor();
            const Skeleton* skeleton = actor->GetSkeleton();
            const Physics::RagdollState& currentRagdollState = ragdollInstance->GetCurrentState();
            Node* ragdollRootNode = ragdollInstance->GetRagdollRootNode();
            const AZ::Outcome<size_t> ragdollRootNodeIndex = ragdollInstance->GetRootRagdollNodeIndex();

            // Copy ragdoll transforms (world space) and reconstruct the rest of the skeleton using the target input pose.
            // If the current node is part of the ragdoll, copy the world transforms from the ragdoll node to the pose and recalculate the local transform.
            // In case the current node is not part of the ragdoll, update the world transforms based on the local transform from the bind pose.
            const size_t jointCount = skeleton->GetNumNodes();
            for (size_t jointIndex = 0; jointIndex < jointCount; ++jointIndex)
            {
                Node* joint = skeleton->GetNode(jointIndex);
                const AZ::Outcome<size_t> ragdollNodeIndex = ragdollInstance->GetRagdollNodeIndex(jointIndex);

                // Special case handling for the motion extraction joint. The motion extraction joint transform will be a projected to the ground version of
                // the ragdoll root with rotation only around the z axis.
                // NOTE: This assumes the motion extraction node is a direct parent of the ragdoll root node.
                if (ragdollRootNode && joint == ragdollRootNode->GetParentNode() && ragdollRootNodeIndex.IsSuccess())
                {
                    Physics::RagdollNodeState& targetRagdollRootNodeState = outputPoseData->GetRagdollNodeState(ragdollRootNodeIndex.GetValue());

                    // Only move along joints parented to the ragdoll root in case the ragdoll root is actually driven by physics (simulated).
                    if (targetRagdollRootNodeState.m_simulationType == Physics::SimulationType::Simulated)
                    {
                        const Physics::RagdollNodeState& currentRagdollRootNodeState = currentRagdollState[ragdollRootNodeIndex.GetValue()];

                        // Construct a world space transform for the ragdoll root. Preserve the scale of current node.
                        #ifndef EMFX_SCALE_DISABLED
                            Transform newGlobalTransform(
                                currentRagdollRootNodeState.m_position,
                                currentRagdollRootNodeState.m_orientation,
                                outputPose.GetWorldSpaceTransform(jointIndex).m_scale);
                        #else
                            Transform newGlobalTransform(
                                currentRagdollRootNodeState.m_position,
                                currentRagdollRootNodeState.m_orientation);
                        #endif

                        // Project it to the ground and only keep rotation around the z axis.
                        newGlobalTransform.ApplyMotionExtractionFlags(EMotionExtractionFlags(0));

                        outputPose.SetWorldSpaceTransform(jointIndex, newGlobalTransform, /*invalidateChildGlobalTransforms*/ false);
                    }
                }
                // Is the joint part of the ragdoll as well as added by this ragdoll node?
                else if (ragdollNodeIndex.IsSuccess() && uniqueData->m_simulatedJointStates[jointIndex])
                {
                    const Physics::RagdollNodeState& currentRagdollNodeState = currentRagdollState[ragdollNodeIndex.GetValue()];
                    Physics::RagdollNodeState& targetRagdollNodeState = outputPoseData->GetRagdollNodeState(ragdollNodeIndex.GetValue());

                    // The joint is part of the ragdoll as well as added and selected by this ragdoll node.
                    targetRagdollNodeState.m_simulationType = Physics::SimulationType::Simulated;

                    // Go up the chain and find the next joint that is part of the ragdoll (Parent of the ragdoll node).
                    AZ::Outcome<size_t> ragdollParentJointIndex = AZ::Failure();
                    Node* ragdollParentJoint = nullptr;
                    ragdollInstance->FindNextRagdollParentForJoint(joint, ragdollParentJoint, ragdollParentJointIndex);

                    if (!ragdollParentJoint)
                    {
                        // No parent found, we're dealing with the ragdoll root.
                        #ifndef EMFX_SCALE_DISABLED
                            Transform newGlobalTransform = Transform(
                                    currentRagdollNodeState.m_position,
                                    currentRagdollNodeState.m_orientation,
                                    outputPose.GetWorldSpaceTransform(jointIndex).m_scale);
                        #else
                            Transform newGlobalTransform = Transform(
                                    currentRagdollNodeState.m_position,
                                    currentRagdollNodeState.m_orientation);
                        #endif

                        outputPose.SetWorldSpaceTransform(jointIndex, newGlobalTransform, /*invalidateChildGlobalTransforms*/ false);
                    }
                    else
                    {
                        const Physics::RagdollNodeState& currentParentRagdollNodeState = currentRagdollState[ragdollParentJointIndex.GetValue()];

                        #ifndef EMFX_SCALE_DISABLED
                            Transform globalTransform = Transform(
                                    currentRagdollNodeState.m_position,
                                    currentRagdollNodeState.m_orientation,
                                    outputPose.GetWorldSpaceTransform(jointIndex).m_scale);

                            Transform parentGlobalTransform = Transform(
                                    currentParentRagdollNodeState.m_position,
                                    currentParentRagdollNodeState.m_orientation,
                                    outputPose.GetWorldSpaceTransform(ragdollParentJoint->GetNodeIndex()).m_scale);
                        #else
                            Transform globalTransform = Transform(
                                    currentRagdollNodeState.m_position,
                                    currentRagdollNodeState.m_orientation);

                            Transform parentGlobalTransform = Transform(
                                    currentParentRagdollNodeState.m_position,
                                    currentParentRagdollNodeState.m_orientation);
                        #endif

                        // Calculate the local transform based of the current ragdoll node transform and its parent.
                        // NOTE: This does not yet account for joints in between in the animation skeleton that are not part of the ragdoll.
                        const Transform localTransform = globalTransform * parentGlobalTransform.Inversed();

                        outputPose.SetLocalSpaceTransform(jointIndex, localTransform, true);
                    }

                    if (targetPose)
                    {
                        // Set the target pose for the selected and thus simulated joints in the anim graph node has a target pose connected to its input port.
                        // Set the local space transform for powered ragdoll nodes.
                        const Transform& localTransform = targetPose->GetLocalSpaceTransform(jointIndex);
                        targetRagdollNodeState.m_position = localTransform.m_position;
                        targetRagdollNodeState.m_orientation = localTransform.m_rotation;
                    }
                    else
                    {
                        // We do not have a target pose connected to the input port, just forward what is currently in the output pose (bind pose).
                        const Transform& localTransform = outputPose.GetLocalSpaceTransform(jointIndex);
                        targetRagdollNodeState.m_position = localTransform.m_position;
                        targetRagdollNodeState.m_orientation = localTransform.m_rotation;
                    }
                }
                else
                {
                    // There are several reasons for this case:
                    // 1. The joint is kinematic:
                    //    Kinematic joints are driven by the animation system and thus we don't have to retrieve anything from the ragdoll. The world space transfroms
                    //    to drive the ragdoll will be set in the RagdollInstance::PostUpdate() based on the final animation pose after evaluating the anim graph.
                    // 2. The joint is dynamic but got added by another ragdoll node:
                    //    Joints that are dynamic but have been added by another ragdoll node baked their transforms as well as the target pose into the pose that
                    //    gets fed through the anim graph .
                    // 3. The current joint is not part of the ragdoll:
                    //    This means that we might be dealing with a leaf joint (e.g. finger joint). The output pose has been initialized with either the input target
                    //    pose or the bind pose (no connection).
                    // We'll just invalidate the global transform in all of these cases.
                    outputPose.InvalidateModelSpaceTransform(jointIndex);
                }
            }

            ragdollInstance->SetRagdollUsed();
        }
    }

    bool BlendTreeRagdollNode::IsActivated(AnimGraphInstance* animGraphInstance) const
    {
        if (HasConnectionAtInputPort(INPUTPORT_ACTIVATE))
        {
            return GetInputNumberAsBool(animGraphInstance, INPUTPORT_ACTIVATE);
        }

        return true;
    }

    AZStd::string BlendTreeRagdollNode::GetSimulatedJointName(int index) const
    {
        return m_simulatedJointNames[index];
    }

    void BlendTreeRagdollNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BlendTreeRagdollNode, AnimGraphNode>()
                ->Version(1)
                ->Field("simulatedJoints", &BlendTreeRagdollNode::m_simulatedJointNames)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<BlendTreeRagdollNode>("Ragdoll", "Ragdoll node properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ_CRC_CE("ActorRagdollJoints"), &BlendTreeRagdollNode::m_simulatedJointNames, "Simulated Joints", "The selected joints will be simulated as part of the ragdoll.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeRagdollNode::Reinit)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &BlendTreeRagdollNode::GetSimulatedJointName)
                        ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ_CRC_CE("ActorJointElement"))
                ;
            }
        }
    }
} // namespace EMotionFX
