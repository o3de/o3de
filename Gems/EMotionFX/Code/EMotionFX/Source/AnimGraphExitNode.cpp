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
#include "AnimGraphExitNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphManager.h"
#include "EMotionFXManager.h"
#include <EMotionFX/Source/AnimGraph.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphExitNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphExitNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    AnimGraphExitNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void AnimGraphExitNode::UniqueData::Update()
    {
        AnimGraphExitNode* exitNode = azdynamic_cast<AnimGraphExitNode*>(m_object);
        AZ_Assert(exitNode, "Unique data linked to incorrect node type.");

        if (m_previousNode && exitNode->FindChildNodeIndex(m_previousNode) == InvalidIndex32)
        {
            m_previousNode = nullptr;
        }
    }

    AnimGraphExitNode::AnimGraphExitNode()
        : AnimGraphNode()
    {
        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }

    AnimGraphExitNode::~AnimGraphExitNode()
    {
    }

    bool AnimGraphExitNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    const char* AnimGraphExitNode::GetPaletteName() const
    {
        return "Exit Node";
    }

    AnimGraphObject::ECategory AnimGraphExitNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_SOURCES;
    }

    void AnimGraphExitNode::OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* previousState, AnimGraphStateTransition* usedTransition)
    {
        MCORE_UNUSED(usedTransition);
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        uniqueData->m_previousNode = previousState;
    }

    void AnimGraphExitNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        uniqueData->m_previousNode = nullptr;
    }

    void AnimGraphExitNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));

        // if the previous node is not set, do nothing
        if (uniqueData->m_previousNode == nullptr || uniqueData->m_previousNode == this)
        {
            uniqueData->Clear();
            return;
        }

        UpdateIncomingNode(animGraphInstance, uniqueData->m_previousNode, timePassedInSeconds);

        AnimGraphStateMachine* parentStateMachine = azdynamic_cast<AnimGraphStateMachine*>(GetParentNode());
        if (parentStateMachine)
        {
            // The exit node evaluates and outputs the transforms from the previous node. Transfer ref counting ownership to the
            // parent state machine to make sure it will be decreased properly even though we're fully blended into the exit node.
            AnimGraphStateMachine::UniqueData* parentUniqueData = static_cast<AnimGraphStateMachine::UniqueData*>(parentStateMachine->FindOrCreateUniqueNodeData(animGraphInstance));
            parentUniqueData->IncreasePoseRefCountForNode(uniqueData->m_previousNode, animGraphInstance);
            parentUniqueData->IncreaseDataRefCountForNode(uniqueData->m_previousNode, animGraphInstance);
        }

        uniqueData->Init(animGraphInstance, uniqueData->m_previousNode);
    }

    void AnimGraphExitNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // if the previous node is not set, output a bind pose
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        if (uniqueData->m_previousNode == nullptr || uniqueData->m_previousNode == this)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(actorInstance);

            // visualize it
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
            }

            return;
        }

        // everything seems fine with the previous node, so just sample that one
        OutputIncomingNode(animGraphInstance, uniqueData->m_previousNode);
        RequestPoses(animGraphInstance);

        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();

        if (uniqueData->m_previousNode != this)
        {
            *outputPose = *uniqueData->m_previousNode->GetMainOutputPose(animGraphInstance);
        }
        else
        {
            outputPose->InitFromBindPose(actorInstance);
        }

        // We moved ownership of decreasing the ref to the parent parent state machine as it might happen that within one of the multiple
        // passes the state machine is doing, the entry node is transitioned over while we never reach the decrease ref point.
        //uniqueData->m_previousNode->DecreaseRef(animGraphInstance);

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }

    void AnimGraphExitNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if there is no previous node, do nothing
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        if (uniqueData->m_previousNode == nullptr || uniqueData->m_previousNode == this)
        {
            return;
        }

        // sync the previous node to this exit node
        HierarchicalSyncInputNode(animGraphInstance, uniqueData->m_previousNode, uniqueData);

        // call the top-down update of the previous node
        uniqueData->m_previousNode->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
    }

    void AnimGraphExitNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if there is no previous node, do nothing
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        if (uniqueData->m_previousNode == nullptr || uniqueData->m_previousNode == this)
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // post update the previous node, so that its event buffer is filled
        uniqueData->m_previousNode->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

        RequestRefDatas(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

        AnimGraphRefCountedData* sourceData = uniqueData->m_previousNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
        data->SetEventBuffer(sourceData->GetEventBuffer());
        data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
        data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());

        uniqueData->m_previousNode->DecreaseRefDataRef(animGraphInstance);
    }

    void AnimGraphExitNode::RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToDisable)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        animGraphInstance->DisableObjectFlags(m_objectIndex, flagsToDisable);

        // forward it to the node we came from
        if (uniqueData->m_previousNode && uniqueData->m_previousNode != this)
        {
            uniqueData->m_previousNode->RecursiveResetFlags(animGraphInstance, flagsToDisable);
        }
    }

    void AnimGraphExitNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphExitNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphExitNode>("Exit Node", "Exit node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX
