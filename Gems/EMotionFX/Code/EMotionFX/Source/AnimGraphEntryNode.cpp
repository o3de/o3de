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
#include "AnimGraphEntryNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphRefCountedData.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphHubNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphEntryNode, AnimGraphAllocator)

    AnimGraphEntryNode::AnimGraphEntryNode()
        : AnimGraphNode()
    {
        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }

    AnimGraphEntryNode::~AnimGraphEntryNode()
    {
    }

    bool AnimGraphEntryNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    const char* AnimGraphEntryNode::GetPaletteName() const
    {
        return "Entry";
    }

    AnimGraphObject::ECategory AnimGraphEntryNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }

    AnimGraphNode* AnimGraphEntryNode::FindSourceNode(AnimGraphInstance* animGraphInstance) const
    {
        AnimGraphStateMachine* grandParentStateMachine = AnimGraphStateMachine::GetGrandParentStateMachine(this);
        if (grandParentStateMachine)
        {
            AnimGraphNode* currentState = grandParentStateMachine->GetCurrentState(animGraphInstance);
            if (currentState)
            {
                // Avoid circular dependency between a hub node coming from a state machine with our entry node being active.
                if (currentState->RTTI_GetType() == azrtti_typeid<AnimGraphHubNode>())
                {
                    AnimGraphHubNode* hubNode = static_cast<AnimGraphHubNode*>(currentState);
                    AnimGraphStateMachine* hubStateMachine = azdynamic_cast<AnimGraphStateMachine*>(hubNode->GetSourceNode(animGraphInstance));
                    if (hubStateMachine && hubStateMachine->GetCurrentState(animGraphInstance) == this)
                    {
                        return nullptr;
                    }
                }

                return currentState;
            }
        }

        return nullptr;
    }

    void AnimGraphEntryNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose = nullptr;

        // we only need to get the pose from the source node for the cases where the source node is valid (not null) and 
        // is not the parent node (since the source of the parent state machine is the binding pose)
        AnimGraphNode* sourceNode = FindSourceNode(animGraphInstance);
        if (sourceNode && sourceNode != GetParentNode())
        {
            OutputIncomingNode(animGraphInstance, sourceNode);

            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            *outputPose = *sourceNode->GetMainOutputPose(animGraphInstance);
        }
        else
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
        }
        // We moved ownership of decreasing the ref to the parent parent state machine as it might happen that within one of the multiple
        // passes the state machine is doing, the entry node is transitioned over while we never reach the decrease ref point.
        //sourceNode->DecreaseRef(animGraphInstance);

        if (outputPose && GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }

    void AnimGraphEntryNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);

        AnimGraphNode* sourceNode = FindSourceNode(animGraphInstance);
        if (!sourceNode || sourceNode == GetParentNode())
        {
            uniqueData->Clear();
            return;
        }

        AnimGraphStateMachine* grandParentStateMachine = AnimGraphStateMachine::GetGrandParentStateMachine(this);
        if (grandParentStateMachine)
        {
            // As the entry node passes the transforms from the source node of the currently active transition in the grand parent state machine,
            // we will transfer ownership of the ref counting to it to make sure it will be decreased properly.
            AnimGraphStateMachine::UniqueData* parentUniqueData = static_cast<AnimGraphStateMachine::UniqueData*>(grandParentStateMachine->FindOrCreateUniqueNodeData(animGraphInstance));
            parentUniqueData->IncreasePoseRefCountForNode(sourceNode, animGraphInstance);
            parentUniqueData->IncreaseDataRefCountForNode(sourceNode, animGraphInstance);
        }

        UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);
        uniqueData->Init(animGraphInstance, sourceNode);
    }

    void AnimGraphEntryNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // find the source node
        AnimGraphNode* sourceNode = FindSourceNode(animGraphInstance);
        if (!sourceNode || sourceNode == GetParentNode())
        {
            return;
        }

        // sync the previous node to this exit node
        //HierarchicalSyncInputNode(animGraphInstance, sourceNode, uniqueData);

        // call the top-down update of the previous node
        sourceNode->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
    }

    void AnimGraphEntryNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode* sourceNode = FindSourceNode(animGraphInstance);
        if (!sourceNode || sourceNode == GetParentNode())
        {
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // post update the source node, so that its event buffer is filled
        sourceNode->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

        // copy over the event buffer
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        RequestRefDatas(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

        AnimGraphRefCountedData* sourceData = sourceNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
        if (sourceData)
        {
            data->SetEventBuffer(sourceData->GetEventBuffer());
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
        }
        else
        {
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
        }
    }

    void AnimGraphEntryNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphEntryNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphEntryNode>("Entry Node", "Entry node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX
