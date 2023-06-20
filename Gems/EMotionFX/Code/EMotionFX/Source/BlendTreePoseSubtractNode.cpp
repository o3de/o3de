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
#include "BlendTreePoseSubtractNode.h"
#include "AnimGraphInstance.h"
#include "ActorInstance.h"
#include "TransformData.h"
#include "Pose.h"
#include "AnimGraphPose.h"
#include "AnimGraphRefCountedData.h"
#include "Node.h"
#include "AnimGraph.h"
#include <EMotionFX/Source/EMotionFXManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreePoseSubtractNode, AnimGraphAllocator)

    BlendTreePoseSubtractNode::BlendTreePoseSubtractNode()
        : AnimGraphNode()
        , m_syncMode(SYNCMODE_DISABLED)
        , m_eventMode(EVENTMODE_MOSTACTIVE)
    {
        // Setup the input ports.
        InitInputPorts(2);
        SetupInputPort("Pose 1", INPUTPORT_POSE_A, AttributePose::TYPE_ID, PORTID_INPUT_POSE_A);
        SetupInputPort("Pose 2", INPUTPORT_POSE_B, AttributePose::TYPE_ID, PORTID_INPUT_POSE_B);

        // Setup the output ports.
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    BlendTreePoseSubtractNode::~BlendTreePoseSubtractNode()
    {
    }


    bool BlendTreePoseSubtractNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* BlendTreePoseSubtractNode::GetPaletteName() const
    {
        return "Pose Subtract";
    }


    AnimGraphObject::ECategory BlendTreePoseSubtractNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }


    void BlendTreePoseSubtractNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE_A);
        AnimGraphNode* subtractNode = GetInputNode(INPUTPORT_POSE_B);

        // If we are disabled and we have an input node, or if we are no disabled but have no subtract input.
        if ((m_disabled && inputNode) || (!m_disabled && inputNode && !subtractNode))
        {
            OutputIncomingNode(animGraphInstance, inputNode);
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *inputNode->GetMainOutputPose(animGraphInstance);                
            return;
        }
        else if (m_disabled || !inputNode) // If we are disabled or have no inputs.
        {
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        OutputAllIncomingNodes(animGraphInstance);

        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();

        // Perform the subtraction
        if (inputNode)
        {
            *outputPose = *inputNode->GetMainOutputPose(animGraphInstance);
        }
        if (subtractNode)
        {
            AnimGraphPose* subtractPose = subtractNode->GetMainOutputPose(animGraphInstance);
            if (subtractPose)
            {
                outputPose->GetPose().MakeRelativeTo(subtractPose->GetPose());
            }
        }

        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            AnimGraphPose* visualOutputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            animGraphInstance->GetActorInstance()->DrawSkeleton(visualOutputPose->GetPose(), m_visualizeColor);
        }
    }
    

    void BlendTreePoseSubtractNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE_A);
        AnimGraphNode* subtractNode = GetInputNode(INPUTPORT_POSE_B);

        // If we are disabled and we have an input node, or if we are no disabled but have no subtract input.
        if ((m_disabled && inputNode) || (!m_disabled && inputNode && !subtractNode))
        {
            UpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Init(animGraphInstance, inputNode);
            return;
        }
        else if (m_disabled || (!inputNode && !subtractNode))    // If we are disabled or have no inputs.
        {
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        if (inputNode)
        {
            UpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Init(animGraphInstance, inputNode);
        }
        if (subtractNode)
        {
            UpdateIncomingNode(animGraphInstance, subtractNode, timePassedInSeconds);
        }
    }


    void BlendTreePoseSubtractNode::UpdateMotionExtraction(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, AnimGraphNodeData* uniqueData)
    {
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

        if (!data)
        {
            return;
        }

        data->ZeroTrajectoryDelta();

        // We are disabled and have no input pose, so output no delta.
        if (m_disabled || !nodeA)
        {
            return;
        }

        // We are disabled but had an input pose, just forward that in this case.
        // Do the same if we are not disabled but have no second pose.
        AnimGraphRefCountedData* nodeAData = nodeA ? nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData() : nullptr;
        AnimGraphRefCountedData* nodeBData = nodeB ? nodeB->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData() : nullptr;
        if (nodeAData)
        {
            data->SetTrajectoryDelta(nodeAData->GetTrajectoryDelta());
        }
        if (nodeBData)
        {
            data->SetTrajectoryDeltaMirrored(nodeAData->GetTrajectoryDeltaMirrored());
        }
    }


    void BlendTreePoseSubtractNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE_A);
        AnimGraphNode* subtractNode = GetInputNode(INPUTPORT_POSE_B);

        if (m_disabled)
        {
            if (inputNode)
            {
                TopDownUpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);
            }

            if (subtractNode)
            {
                TopDownUpdateIncomingNode(animGraphInstance, subtractNode, timePassedInSeconds);
            }
            return;
        }

        if (m_syncMode != SYNCMODE_DISABLED)
        {
            if (inputNode)
            {
                // Sync the input node to this node.
                inputNode->AutoSync(animGraphInstance, this, 0.0f, SYNCMODE_TRACKBASED, false);
                if (animGraphInstance->GetIsObjectFlagEnabled(m_objectIndex, AnimGraphInstance::OBJECTFLAGS_SYNCED) == false)
                {
                    inputNode->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, true);
                }

                // Sync the subtract node to the input node.
                if (subtractNode)
                {
                    subtractNode->AutoSync(animGraphInstance, inputNode, 0.0f, m_syncMode, false);
                }
            }
        }
        else
        {
            if (inputNode && animGraphInstance->GetIsObjectFlagEnabled(inputNode->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCED))
            {
                inputNode->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, false);
            }

            if (subtractNode && animGraphInstance->GetIsObjectFlagEnabled(subtractNode->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCED))
            {
                subtractNode->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, false);
            }
        }

        if (inputNode)
        {
            TopDownUpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);
        }

        if (subtractNode)
        {
            TopDownUpdateIncomingNode(animGraphInstance, subtractNode, timePassedInSeconds);
        }
    }


    void BlendTreePoseSubtractNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE_A);
        AnimGraphNode* subtractNode = GetInputNode(INPUTPORT_POSE_B);

        // We are disabled but had an input pose, just forward that in this case.
        // Do the same if we are not disabled but have no second pose.
        if ((m_disabled && inputNode) || (!m_disabled && inputNode && !subtractNode))
        {
            PostUpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);
            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            AnimGraphRefCountedData* inputData = inputNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
            data->SetEventBuffer(inputData->GetEventBuffer());
            data->SetTrajectoryDelta(inputData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(inputData->GetTrajectoryDelta());
            return;
        }
        else if (m_disabled || !inputNode) // If we are disabled or have no inputs.
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        if (inputNode)
        {
            PostUpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);
        }
        if (subtractNode)
        {
            PostUpdateIncomingNode(animGraphInstance, subtractNode, timePassedInSeconds);
        }

        RequestRefDatas(animGraphInstance);
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ClearEventBuffer();

        if (data && inputNode && subtractNode)
        {
            FilterEvents(animGraphInstance, m_eventMode, inputNode, subtractNode, 0.0f, data);
            UpdateMotionExtraction(animGraphInstance, inputNode, subtractNode, uniqueData);
        }
    }


    void BlendTreePoseSubtractNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreePoseSubtractNode, AnimGraphNode>()
            ->Version(1)
            ->Field("syncMode", &BlendTreePoseSubtractNode::m_syncMode)
            ->Field("eventMode", &BlendTreePoseSubtractNode::m_eventMode)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreePoseSubtractNode>("Pose Subtract", "Subtract one pose from another, making its output compatible with additive nodes.")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreePoseSubtractNode::m_syncMode)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreePoseSubtractNode::m_eventMode)
        ;
    }

} // namespace EMotionFX
