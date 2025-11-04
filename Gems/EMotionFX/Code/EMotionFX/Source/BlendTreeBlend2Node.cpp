/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include "BlendTreeBlend2Node.h"
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
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeBlend2Node, AnimGraphAllocator)


    BlendTreeBlend2Node::BlendTreeBlend2Node()
        : BlendTreeBlend2NodeBase()
    {
    }


    BlendTreeBlend2Node::~BlendTreeBlend2Node()
    {
    }


    const char* BlendTreeBlend2Node::GetPaletteName() const
    {
        return "Blend Two";
    }


    void BlendTreeBlend2Node::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AZ_PROFILE_SCOPE(Animation, "BlendTreeBlend2Node::Update");

        if (m_disabled)
        {
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        AnimGraphNode* weightNode = GetInputNode(INPUTPORT_WEIGHT);
        if (weightNode)
        {
            UpdateIncomingNode(animGraphInstance, weightNode, timePassedInSeconds);
        }

        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, false, false);

        if (!nodeA)
        {
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        animGraphInstance->SetObjectFlags(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_IS_SYNCLEADER, true);
        UpdateIncomingNode(animGraphInstance, nodeA, timePassedInSeconds);

        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        uniqueData->Init(animGraphInstance, nodeA);

        if (nodeB && nodeA != nodeB)
        {
            UpdateIncomingNode(animGraphInstance, nodeB, timePassedInSeconds);

            float factorA;
            float factorB;
            float playSpeed;
            AnimGraphNode::CalcSyncFactors(animGraphInstance, nodeA, nodeB, m_syncMode, weight, &factorA, &factorB, &playSpeed);
            uniqueData->SetPlaySpeed(playSpeed * factorA);
        }
    }

    void BlendTreeBlend2Node::Output(AnimGraphInstance* animGraphInstance)
    {
        AZ_PROFILE_SCOPE(Animation, "BlendTreeBlend2Node::Output");

        if (m_disabled)
        {
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        AnimGraphNode* weightNode = GetInputNode(INPUTPORT_WEIGHT);
        if (weightNode)
        {
            OutputIncomingNode(animGraphInstance, weightNode);
        }

        const size_t numNodes = uniqueData->m_mask.size();
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
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }


    void BlendTreeBlend2Node::OutputNoFeathering(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        AnimGraphPose* outputPose;

        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, false, true);

        if (!nodeA)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(actorInstance);
            return;
        }

        if (!nodeB || weight < MCore::Math::epsilon)
        {
            OutputIncomingNode(animGraphInstance, nodeA);

            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
            return;
        }

        if (weight < 1.0f - MCore::Math::epsilon)
        {
            OutputIncomingNode(animGraphInstance, nodeA);
            OutputIncomingNode(animGraphInstance, nodeB);

            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
            outputPose->GetPose().Blend(&nodeB->GetMainOutputPose(animGraphInstance)->GetPose(), weight);
        }
        else
        {
            OutputIncomingNode(animGraphInstance, nodeB);
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *nodeB->GetMainOutputPose(animGraphInstance);
        }
    }


    void BlendTreeBlend2Node::OutputFeathering(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData)
    {
        AnimGraphPose* outputPose;

        float blendWeight;
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &blendWeight, false, true);

        if (!nodeA)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        OutputIncomingNode(animGraphInstance, nodeA);
        if (!nodeB || blendWeight < MCore::Math::epsilon)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
            return;
        }

        OutputIncomingNode(animGraphInstance, nodeB);
        const AnimGraphPose* maskPose = nodeB->GetMainOutputPose(animGraphInstance);
        const Pose& localMaskPose = maskPose->GetPose();

        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *nodeA->GetMainOutputPose(animGraphInstance);
        Pose& outputLocalPose = outputPose->GetPose();

        const size_t numNodes = uniqueData->m_mask.size();
        if (numNodes > 0)
        {
            Transform transform;
            for (size_t n = 0; n < numNodes; ++n)
            {
                const float finalWeight = blendWeight /* * uniqueData->m_weights[n]*/;
                const size_t nodeIndex = uniqueData->m_mask[n];
                transform = outputLocalPose.GetLocalSpaceTransform(nodeIndex);
                transform.Blend(localMaskPose.GetLocalSpaceTransform(nodeIndex), finalWeight);
                outputLocalPose.SetLocalSpaceTransform(nodeIndex, transform);
            }
        }
    }


    void BlendTreeBlend2Node::UpdateMotionExtraction(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float weight, UniqueData* uniqueData)
    {
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ZeroTrajectoryDelta();

        const ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        const Actor* actor = actorInstance->GetActor();
        AnimGraphRefCountedData* nodeAData = nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
        AnimGraphRefCountedData* nodeBData = nodeB ? nodeB->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData() : nullptr;

        if (!nodeAData)
        {
            AZ_Assert(false, "BlendTreeBlend2Node::UpdateMotionExtraction %s has no RefCountedData", nodeA->GetName());
            return;
        }

        Transform delta = Transform::CreateIdentityWithZeroScale();
        Transform deltaMirrored = Transform::CreateIdentityWithZeroScale();
        const bool hasMotionExtractionNodeInMask = (uniqueData->m_mask.size() == 0) || (uniqueData->m_mask.size() > 0 && AZStd::find(uniqueData->m_mask.begin(), uniqueData->m_mask.end(), actor->GetMotionExtractionNodeIndex()) != uniqueData->m_mask.end());
        CalculateMotionExtractionDelta(m_extractionMode, nodeAData, nodeBData, weight, hasMotionExtractionNodeInMask, delta, deltaMirrored);

        data->SetTrajectoryDelta(delta);
        data->SetTrajectoryDeltaMirrored(deltaMirrored);
    }


    void BlendTreeBlend2Node::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (m_disabled)
        {
            return;
        }

        UniqueData* uniqueData = static_cast<BlendTreeBlend2Node::UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        const BlendTreeConnection* con = GetInputPort(INPUTPORT_WEIGHT).m_connection;
        if (con)
        {
            AnimGraphNodeData* sourceNodeUniqueData = con->GetSourceNode()->FindOrCreateUniqueNodeData(animGraphInstance);
            sourceNodeUniqueData->SetGlobalWeight(uniqueData->GetGlobalWeight());
            sourceNodeUniqueData->SetLocalWeight(1.0f);
            TopDownUpdateIncomingNode(animGraphInstance, con->GetSourceNode(), timePassedInSeconds);
        }

        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, false, false);

        if (!nodeA)
        {
            return;
        }

        // Both nodes need to have their weight updated and their TopDownUpdate be called
        // Then one of the node can be removed for the synching if it has zero weight
        // Please Note: the TopDownUpdate needs to be called afer the synching
        AnimGraphNode* nodeWeightUpdateA = nodeA;
        AnimGraphNode* nodeWeightUpdateB = nodeB;

        if (m_syncMode != SYNCMODE_DISABLED)
        {
            const bool resync = (uniqueData->m_syncTrackNode != nodeA);
            if (resync)
            {
                nodeA->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_RESYNC, true);
                if (nodeB)
                {
                    nodeB->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_RESYNC, true);
                }

                uniqueData->m_syncTrackNode = nodeA;
            }

            nodeA->AutoSync(animGraphInstance, this, 0.0f, SYNCMODE_TRACKBASED, false);

            for (uint32 i = 0; i < 2; ++i)
            {
                BlendTreeConnection* connection = m_inputPorts[INPUTPORT_POSE_A + i].m_connection;
                if (!connection)
                {
                    continue;
                }

                if (animGraphInstance->GetIsObjectFlagEnabled(m_objectIndex, AnimGraphInstance::OBJECTFLAGS_SYNCED) == false)
                {
                    connection->GetSourceNode()->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, true);
                }

                AnimGraphNode* nodeToSync = connection->GetSourceNode();
                if (nodeToSync == nodeA)
                {
                    continue;
                }

                nodeToSync->AutoSync(animGraphInstance, nodeA, weight, m_syncMode, false);
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
        if (!nodeWeightUpdateB)
        {
            uniqueDataNodeA->SetGlobalWeight(uniqueData->GetGlobalWeight());
            uniqueDataNodeA->SetLocalWeight(1.0f);
        }
        else
        {
            uniqueDataNodeA->SetGlobalWeight(uniqueData->GetGlobalWeight() * (1.0f - weight));
            uniqueDataNodeA->SetLocalWeight(1.0f - weight);
            if (nodeWeightUpdateB)
            {
                AnimGraphNodeData* uniqueDataNodeB = nodeWeightUpdateB->FindOrCreateUniqueNodeData(animGraphInstance);
                uniqueDataNodeB->SetGlobalWeight(uniqueData->GetGlobalWeight() * weight);
                uniqueDataNodeB->SetLocalWeight(weight);
                TopDownUpdateIncomingNode(animGraphInstance, nodeWeightUpdateB, timePassedInSeconds);
            }
        }
        TopDownUpdateIncomingNode(animGraphInstance, nodeWeightUpdateA, timePassedInSeconds);
    }


    void BlendTreeBlend2Node::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (m_disabled)
        {
            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        const BlendTreeConnection* con = GetInputPort(INPUTPORT_WEIGHT).m_connection;
        if (con)
        {
            PostUpdateIncomingNode(animGraphInstance, con->GetSourceNode(), timePassedInSeconds);
        }

        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        float weight;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &weight, false, false);

        if (!nodeA)
        {
            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        PostUpdateIncomingNode(animGraphInstance, nodeA, timePassedInSeconds);
        if (nodeB && nodeA != nodeB)
        {
            PostUpdateIncomingNode(animGraphInstance, nodeB, timePassedInSeconds);
        }

        RequestRefDatas(animGraphInstance);
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ClearEventBuffer();
        data->ZeroTrajectoryDelta();

        FilterEvents(animGraphInstance, m_eventMode, nodeA, nodeB, weight, data);

        if (animGraphInstance->GetActorInstance()->GetActor()->GetMotionExtractionNodeIndex() != InvalidIndex)
        {
            UpdateMotionExtraction(animGraphInstance, nodeA, nodeB, weight, uniqueData);
        }
    }


    void BlendTreeBlend2Node::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeBlend2Node, BlendTreeBlend2NodeBase>()
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeBlend2Node>("Blend 2", "Blend 2 attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }
} // namespace EMotionFX
