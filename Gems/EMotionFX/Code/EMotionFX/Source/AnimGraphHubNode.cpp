/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <EMotionFX/Source/AnimGraphHubNode.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphAttributeTypes.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphRefCountedData.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/EMotionFXManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphHubNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphHubNode::UniqueData, AnimGraphObjectUniqueDataAllocator);

    AnimGraphHubNode::AnimGraphHubNode()
        : AnimGraphNode()
    {
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    AnimGraphHubNode::~AnimGraphHubNode()
    {
    }
 

    bool AnimGraphHubNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();       
        return true;
    }


    const char* AnimGraphHubNode::GetPaletteName() const
    {
        return "Hub";
    }


    AnimGraphObject::ECategory AnimGraphHubNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }


    void AnimGraphHubNode::OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* previousState, AnimGraphStateTransition* usedTransition)
    {
        MCORE_UNUSED(usedTransition);
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));

        uniqueData->m_sourceNode = (previousState != this) ? previousState : nullptr;

        // Find our way through hub nodes connected to hub nodes.
        while (uniqueData->m_sourceNode && azrtti_typeid(uniqueData->m_sourceNode) == azrtti_typeid<AnimGraphHubNode>())
        {
            AnimGraphHubNode* sourceHubNode = static_cast<AnimGraphHubNode*>(uniqueData->m_sourceNode);
            UniqueData* sourceHubNodeUniqueData = static_cast<UniqueData*>(sourceHubNode->FindOrCreateUniqueNodeData(animGraphInstance));
            uniqueData->m_sourceNode = sourceHubNodeUniqueData->m_sourceNode;
        }
    }

    void AnimGraphHubNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        uniqueData->m_sourceNode = nullptr;
    }

    AnimGraphNode* AnimGraphHubNode::GetSourceNode(AnimGraphInstance* animGraphInstance) const
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        return uniqueData->m_sourceNode;
    }

    void AnimGraphHubNode::Output(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        AnimGraphPose* outputPose;
        AnimGraphNode* sourceNode = uniqueData->m_sourceNode;
        if (!sourceNode)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
        }
        else
        {
            if (GetEMotionFX().GetIsInEditorMode())
            {
                SetHasError(uniqueData, false);
            }

            // Get the output pose of the source node into our output pose.
            sourceNode->PerformOutput(animGraphInstance);
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            *outputPose = *sourceNode->GetMainOutputPose(animGraphInstance);
            sourceNode->DecreaseRef(animGraphInstance);
        }

        // Visualize the output pose.
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }


    void AnimGraphHubNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        AnimGraphNode* sourceNode = uniqueData->m_sourceNode;
        if (!sourceNode)
        {
            uniqueData->Clear();
            return;
        }

        sourceNode->IncreasePoseRefCount(animGraphInstance);
        sourceNode->IncreaseRefDataRefCount(animGraphInstance);
        UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);
        uniqueData->Init(animGraphInstance, sourceNode);
    }


    void AnimGraphHubNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        AnimGraphNode* sourceNode = uniqueData->m_sourceNode;
        if (!sourceNode)
        {
            return;
        }

        HierarchicalSyncInputNode(animGraphInstance, sourceNode, uniqueData);
        sourceNode->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
    }


    void AnimGraphHubNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        AnimGraphNode* sourceNode = uniqueData->m_sourceNode;
        if (!sourceNode)
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        sourceNode->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

        // Copy over the events and trajectory.
        RequestRefDatas(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        AnimGraphRefCountedData* sourceData = sourceNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
        if (sourceData)
        {
            data->SetEventBuffer(sourceData->GetEventBuffer());
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
        }

        sourceNode->DecreaseRefDataRef(animGraphInstance);
    }


    // When a node is about to be removed, make sure we reset any pointers that point to it.
    void AnimGraphHubNode::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        const size_t numAnimGraphInstances = animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numAnimGraphInstances; ++i)
        {
            AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);
            
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));          
            if (uniqueData->m_sourceNode == nodeToRemove)
            {
                uniqueData->m_sourceNode = nullptr;
            }
        }
    }


    void AnimGraphHubNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphHubNode, AnimGraphNode>()
            ->Version(1);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphHubNode>("Hub node", "Hub node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
    }


}   // namespace EMotionFX
