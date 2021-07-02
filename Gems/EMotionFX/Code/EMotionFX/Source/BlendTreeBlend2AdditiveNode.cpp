/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include "BlendTreeBlend2AdditiveNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "AnimGraphMotionNode.h"
#include "ActorInstance.h"
#include "MotionInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "TransformData.h"
#include "Node.h"
#include "AnimGraph.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeBlend2AdditiveNode, AnimGraphAllocator, 0)

    BlendTreeBlend2AdditiveNode::BlendTreeBlend2AdditiveNode()
        : BlendTreeBlend2NodeBase()
    {
        m_eventMode = EVENTMODE_BOTHNODES;
    }


    BlendTreeBlend2AdditiveNode::~BlendTreeBlend2AdditiveNode()
    {
    }


    const char* BlendTreeBlend2AdditiveNode::GetPaletteName() const
    {
        return "Blend Two Additive";
    }


    void BlendTreeBlend2AdditiveNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (mDisabled)
        {
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        // Update the weight node.
        AnimGraphNode* weightNode = GetInputNode(INPUTPORT_WEIGHT);
        if (weightNode)
        {
            UpdateIncomingNode(animGraphInstance, weightNode, timePassedInSeconds);
        }

        // Get the input nodes.
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, true, false);

        if (!nodeA)
        {
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        // Update the first node.
        animGraphInstance->SetObjectFlags(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_IS_SYNCLEADER, true);
        UpdateIncomingNode(animGraphInstance, nodeA, timePassedInSeconds);

        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        uniqueData->Init(animGraphInstance, nodeA);

        // Update the second node.
        if (nodeB && nodeA != nodeB)
        {
            UpdateIncomingNode(animGraphInstance, nodeB, timePassedInSeconds);
        }
    }



    void BlendTreeBlend2AdditiveNode::Output(AnimGraphInstance* animGraphInstance)
    {
        // If we disabled this blend node, simply output a bind pose.
        if (mDisabled)
        {
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        // Output the weight node.
        AnimGraphNode* weightNode = GetInputNode(INPUTPORT_WEIGHT);
        if (weightNode)
        {
            OutputIncomingNode(animGraphInstance, weightNode);
        }

        const size_t numNodes = uniqueData->mMask.size();
        if (numNodes == 0)
        {
            OutputNoFeathering(animGraphInstance);
        }
        else
        {
            OutputFeathering(animGraphInstance, uniqueData);
        }

        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            AnimGraphPose* outPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            animGraphInstance->GetActorInstance()->DrawSkeleton(outPose->GetPose(), mVisualizeColor);
        }
    }


    void BlendTreeBlend2AdditiveNode::OutputNoFeathering(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // Get the input nodes.
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, true, true);

        // Check if we have an incoming connection. If not, just output the bind pose.
        AnimGraphPose* outputPose;
        if (!nodeA)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        // If there is only one pose, we just output that one.
        if (!nodeB || weight < MCore::Math::epsilon)
        {
            OutputIncomingNode(animGraphInstance, nodeA);

            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
            return;
        }

        // Output the nodes.
        OutputIncomingNode(animGraphInstance, nodeA);
        OutputIncomingNode(animGraphInstance, nodeB);

        // Apply the additive blend.
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
        outputPose->GetPose().ApplyAdditive(nodeB->GetMainOutputPose(animGraphInstance)->GetPose(), weight);
    }


    void BlendTreeBlend2AdditiveNode::OutputFeathering(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        AnimGraphPose* outputPose;

        // Get the input nodes.
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float blendWeight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &blendWeight, true, true);

        // Check if we have connected something to an input port.
        if (!nodeA)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        // If we only input one pose, output that one.
        OutputIncomingNode(animGraphInstance, nodeA);
        if (!nodeB || blendWeight < MCore::Math::epsilon)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
            return;
        }

        OutputIncomingNode(animGraphInstance, nodeB);
        const Pose& additivePose = nodeB->GetMainOutputPose(animGraphInstance)->GetPose();
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
        Pose& outputLocalPose = outputPose->GetPose();

        // If we use a mask, overwrite those nodes.
        const size_t numNodes = uniqueData->mMask.size();
        if (numNodes > 0)
        {
            Transform transform;
            for (size_t n = 0; n < numNodes; ++n)
            {
                const float finalWeight = blendWeight;// * uniqueData->mWeights[n];
                const uint32 nodeIndex = uniqueData->mMask[n];
                transform = outputLocalPose.GetLocalSpaceTransform(nodeIndex);
                transform.ApplyAdditive(additivePose.GetLocalSpaceTransform(nodeIndex), finalWeight);
                outputLocalPose.SetLocalSpaceTransform(nodeIndex, transform);
            }
        }
    }


    void BlendTreeBlend2AdditiveNode::UpdateMotionExtraction(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float weight, UniqueData* uniqueData)
    {
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

        const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        const Actor* actor = actorInstance->GetActor();
        AnimGraphRefCountedData* nodeAData = nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
        AnimGraphRefCountedData* nodeBData = nodeB ? nodeB->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData() : nullptr;

        if (!nodeAData)
        {
            // nodeAData is assumed to be present
            data->ZeroTrajectoryDelta();
            return;
        }

        Transform delta = Transform::CreateIdentityWithZeroScale();
        Transform deltaMirrored = Transform::CreateIdentityWithZeroScale();
        const bool hasMotionExtractionNodeInMask = (uniqueData->mMask.size() == 0) || (uniqueData->mMask.size() > 0 && AZStd::find(uniqueData->mMask.begin(), uniqueData->mMask.end(), actor->GetMotionExtractionNodeIndex()) != uniqueData->mMask.end());
        if (!hasMotionExtractionNodeInMask || !nodeBData || m_extractionMode == EXTRACTIONMODE_SOURCEONLY)
        {
            delta = nodeAData->GetTrajectoryDelta();
            deltaMirrored = nodeAData->GetTrajectoryDeltaMirrored();
        }
        else if (m_extractionMode == EXTRACTIONMODE_TARGETONLY)
        {
            if (nodeBData)
            {
                delta = nodeAData->GetTrajectoryDelta();
                deltaMirrored = nodeAData->GetTrajectoryDeltaMirrored();
                delta.ApplyAdditive(nodeBData->GetTrajectoryDelta());
                deltaMirrored.ApplyAdditive(nodeBData->GetTrajectoryDeltaMirrored());
            }
            else
            {
                delta = nodeAData->GetTrajectoryDelta();
                deltaMirrored = nodeAData->GetTrajectoryDeltaMirrored();
            }
        }
        else if (m_extractionMode == EXTRACTIONMODE_BLEND)
        {
            AZ_Assert(nodeAData && nodeBData, "There should be data for both input nodes.");
            delta = nodeAData->GetTrajectoryDelta();
            deltaMirrored = nodeAData->GetTrajectoryDeltaMirrored();
            delta.ApplyAdditive(nodeBData->GetTrajectoryDelta(), weight);
            deltaMirrored.ApplyAdditive(nodeBData->GetTrajectoryDeltaMirrored(), weight);
        }

        data->SetTrajectoryDelta(delta);
        data->SetTrajectoryDeltaMirrored(deltaMirrored);
    }


    void BlendTreeBlend2AdditiveNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (mDisabled)
        {
            return;
        }

        UniqueData* uniqueData = static_cast<BlendTreeBlend2AdditiveNode::UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        const BlendTreeConnection* con = GetInputPort(INPUTPORT_WEIGHT).mConnection;
        if (con)
        {
            AnimGraphNodeData* sourceNodeUniqueData = con->GetSourceNode()->FindOrCreateUniqueNodeData(animGraphInstance);
            sourceNodeUniqueData->SetGlobalWeight(uniqueData->GetGlobalWeight());
            sourceNodeUniqueData->SetLocalWeight(1.0f);
            con->GetSourceNode()->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        }

        // Get the input nodes.
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, true, false);
        if (!nodeA)
        {
            return;
        }

        // Both nodes need to have their weight updated and their TopDownUpdate be called
        // Then one of the node can be removed for the synching if it has zero weight
        // Please Note: the TopDownUpdate needs to be called afer the synching
        AnimGraphNode* nodeWeightUpdateA = nodeA;
        AnimGraphNode* nodeWeightUpdateB = nodeB;
        float weightBeforeOptimization = weight;

        if (weight < MCore::Math::epsilon)
        {
            nodeB = nullptr;
        }

        // If we want to sync the motions.
        if (m_syncMode != SYNCMODE_DISABLED)
        {
            const bool resync = (uniqueData->mSyncTrackNode != nodeA);
            if (resync)
            {
                nodeA->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_RESYNC, true);
                if (nodeB)
                {
                    nodeB->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_RESYNC, true);
                }

                uniqueData->mSyncTrackNode = nodeA;
            }

            // Sync the leader to this node.
            nodeA->AutoSync(animGraphInstance, this, 0.0f, SYNCMODE_TRACKBASED, false);

            // Sync the motion's to the leader.
            for (uint32 i = 0; i < 2; ++i)
            {
                BlendTreeConnection* connection = mInputPorts[INPUTPORT_POSE_A + i].mConnection;
                if (!connection)
                {
                    continue;
                }

                if (animGraphInstance->GetIsObjectFlagEnabled(mObjectIndex, AnimGraphInstance::OBJECTFLAGS_SYNCED) == false)
                {
                    connection->GetSourceNode()->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, true);
                }

                AnimGraphNode* nodeToSync = connection->GetSourceNode();
                if (nodeToSync == nodeA)
                {
                    continue;
                }

                nodeToSync->AutoSync(animGraphInstance, nodeA, 0.0f, m_syncMode, false);
            }
        }
        else
        {
            if (animGraphInstance->GetIsObjectFlagEnabled(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCED))
            {
                nodeA->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, false);
            }

            if (nodeB && animGraphInstance->GetIsObjectFlagEnabled(nodeB->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCED))
            {
                nodeB->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, false);
            }
        }

        AnimGraphNodeData* uniqueDataNodeA = nodeWeightUpdateA->FindOrCreateUniqueNodeData(animGraphInstance);

        uniqueDataNodeA->SetGlobalWeight(uniqueData->GetGlobalWeight());
        uniqueDataNodeA->SetLocalWeight(1.0f);
        if (nodeWeightUpdateB)
        {
            AnimGraphNodeData* uniqueDataNodeB = nodeWeightUpdateB->FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueDataNodeB->SetGlobalWeight(uniqueData->GetGlobalWeight() * weightBeforeOptimization);
            uniqueDataNodeB->SetLocalWeight(weightBeforeOptimization);
            nodeWeightUpdateB->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        }

        nodeWeightUpdateA->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
    }



    // post sync update
    void BlendTreeBlend2AdditiveNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (mDisabled)
        {
            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        const BlendTreeConnection* con = GetInputPort(INPUTPORT_WEIGHT).mConnection;
        if (con)
        {
            con->GetSourceNode()->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        // Get the input nodes.
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        const bool canOptimize = (m_extractionMode == EXTRACTIONMODE_BLEND);    // Don't optimize when using the source or target motion extraction modes.
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, true, canOptimize);

        if (!nodeA)
        {
            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        nodeA->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        if (nodeB && nodeA != nodeB)
        {
            nodeB->PerformPostUpdate(animGraphInstance, timePassedInSeconds);
        }

        RequestRefDatas(animGraphInstance);
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ClearEventBuffer();
        data->ZeroTrajectoryDelta();

        // Output events.
        EEventMode eventMode = m_eventMode;
        if (eventMode == EVENTMODE_MOSTACTIVE)
        {
            eventMode = EVENTMODE_BOTHNODES;
        }
        FilterEvents(animGraphInstance, eventMode, nodeA, nodeB, weight, data);

        // Output motion extraction deltas.
        if (animGraphInstance->GetActorInstance()->GetActor()->GetMotionExtractionNodeIndex() != MCORE_INVALIDINDEX32)
        {
            UpdateMotionExtraction(animGraphInstance, nodeA, nodeB, weight, uniqueData);
        }
    }


    void BlendTreeBlend2AdditiveNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeBlend2AdditiveNode, BlendTreeBlend2NodeBase>()
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeBlend2AdditiveNode>("Blend 2 Additive", "Blend 2 additive attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }
} // namespace EMotionFX
