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
#include "BlendTreePoseSwitchNode.h"
#include "AnimGraphInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphManager.h"
#include "AnimGraphRefCountedData.h"
#include "EMotionFXManager.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreePoseSwitchNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreePoseSwitchNode::UniqueData, AnimGraphObjectUniqueDataAllocator)


    BlendTreePoseSwitchNode::BlendTreePoseSwitchNode()
        : AnimGraphNode()
    {
        // setup input ports
        InitInputPorts(11);
        SetupInputPort("Pose 0", INPUTPORT_POSE_0, AttributePose::TYPE_ID, PORTID_INPUT_POSE_0);
        SetupInputPort("Pose 1", INPUTPORT_POSE_1, AttributePose::TYPE_ID, PORTID_INPUT_POSE_1);
        SetupInputPort("Pose 2", INPUTPORT_POSE_2, AttributePose::TYPE_ID, PORTID_INPUT_POSE_2);
        SetupInputPort("Pose 3", INPUTPORT_POSE_3, AttributePose::TYPE_ID, PORTID_INPUT_POSE_3);
        SetupInputPort("Pose 4", INPUTPORT_POSE_4, AttributePose::TYPE_ID, PORTID_INPUT_POSE_4);
        SetupInputPort("Pose 5", INPUTPORT_POSE_5, AttributePose::TYPE_ID, PORTID_INPUT_POSE_5);
        SetupInputPort("Pose 6", INPUTPORT_POSE_6, AttributePose::TYPE_ID, PORTID_INPUT_POSE_6);
        SetupInputPort("Pose 7", INPUTPORT_POSE_7, AttributePose::TYPE_ID, PORTID_INPUT_POSE_7);
        SetupInputPort("Pose 8", INPUTPORT_POSE_8, AttributePose::TYPE_ID, PORTID_INPUT_POSE_8);
        SetupInputPort("Pose 9", INPUTPORT_POSE_9, AttributePose::TYPE_ID, PORTID_INPUT_POSE_9);
        SetupInputPortAsNumber("Decision Value", INPUTPORT_DECISIONVALUE, PORTID_INPUT_DECISIONVALUE); // accept float/int/bool values

                                                                                                       // setup output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    BlendTreePoseSwitchNode::~BlendTreePoseSwitchNode()
    {
    }


    bool BlendTreePoseSwitchNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // get the palette name
    const char* BlendTreePoseSwitchNode::GetPaletteName() const
    {
        return "Pose Switch";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreePoseSwitchNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_LOGIC;
    }


    // perform the calculations / actions
    void BlendTreePoseSwitchNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // if the decision port has no incomming connection, there is nothing we can do
        if (m_inputPorts[INPUTPORT_DECISIONVALUE].m_connection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        // get the index we choose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_DECISIONVALUE));
        const int32 decisionValue = MCore::Clamp<int32>(GetInputNumberAsInt32(animGraphInstance, INPUTPORT_DECISIONVALUE), 0, 9); // max 10 cases

        // check if there is an incoming connection from this port
        if (m_inputPorts[INPUTPORT_POSE_0 + decisionValue].m_connection == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());

            // visualize it
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
            }
            return;
        }

        // get the local pose from the port
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE_0 + decisionValue));
        const AnimGraphPose* pose = GetInputPose(animGraphInstance, INPUTPORT_POSE_0 + decisionValue)->GetValue();

        // output the pose
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *pose;

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }


    // update the blend tree node (update timer values etc)
    void BlendTreePoseSwitchNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if the decision port has no incomming connection, there is nothing we can do
        if (m_inputPorts[INPUTPORT_DECISIONVALUE].m_connection == nullptr)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            uniqueData->Clear();
            return;
        }

        // update the node that plugs into the decision value port
        UpdateIncomingNode(animGraphInstance, m_inputPorts[INPUTPORT_DECISIONVALUE].m_connection->GetSourceNode(), timePassedInSeconds);

        // get the index we choose
        const int32 decisionValue = MCore::Clamp<int32>(GetInputNumberAsInt32(animGraphInstance, INPUTPORT_DECISIONVALUE), 0, 9); // max 10 cases

        // check if there is an incoming connection from this port
        if (m_inputPorts[INPUTPORT_POSE_0 + decisionValue].m_connection == nullptr)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            uniqueData->Clear();
            return;
        }

        // pass through the motion extraction of the selected node
        AnimGraphNode* sourceNode = m_inputPorts[INPUTPORT_POSE_0 + decisionValue].m_connection->GetSourceNode();

        // if our decision value changed since last time, specify that we want to resync
        // this basically means that the motion extraction delta will be zero for one frame
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        if (uniqueData->m_decisionIndex != decisionValue)
        {
            uniqueData->m_decisionIndex = decisionValue;
            //sourceNode->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphObjectData::FLAGS_RESYNC, true);
        }

        // update the source node and init the unique data
        UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);
        uniqueData->Init(animGraphInstance, sourceNode);
    }


    // post update
    void BlendTreePoseSwitchNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if the decision port has no incomming connection, there is nothing we can do
        if (m_inputPorts[INPUTPORT_DECISIONVALUE].m_connection == nullptr)
        {
            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // update the node that plugs into the decision value port
        PostUpdateIncomingNode(animGraphInstance, m_inputPorts[INPUTPORT_DECISIONVALUE].m_connection->GetSourceNode(), timePassedInSeconds);

        // get the index we choose
        const int32 decisionValue = MCore::Clamp<int32>(GetInputNumberAsInt32(animGraphInstance, INPUTPORT_DECISIONVALUE), 0, 9); // max 10 cases

        // check if there is an incoming connection from this port
        if (m_inputPorts[INPUTPORT_POSE_0 + decisionValue].m_connection == nullptr)
        {
            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // pass through the motion extraction of the selected node
        AnimGraphNode* sourceNode = m_inputPorts[INPUTPORT_POSE_0 + decisionValue].m_connection->GetSourceNode();
        PostUpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);

        // output the events of the source node we picked
        RequestRefDatas(animGraphInstance);
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        AnimGraphRefCountedData* sourceData = sourceNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
        data->SetEventBuffer(sourceData->GetEventBuffer());
        data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
        data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
    }



    // post update
    void BlendTreePoseSwitchNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if the decision port has no incomming connection, there is nothing we can do
        if (m_inputPorts[INPUTPORT_DECISIONVALUE].m_connection == nullptr)
        {
            return;
        }

        // get the index we choose
        const int32 decisionValue = MCore::Clamp<int32>(GetInputNumberAsInt32(animGraphInstance, INPUTPORT_DECISIONVALUE), 0, 9); // max 10 cases

        // check if there is an incoming connection from this port
        if (m_inputPorts[INPUTPORT_POSE_0 + decisionValue].m_connection == nullptr)
        {
            return;
        }

        // sync all the incoming connections
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        HierarchicalSyncAllInputNodes(animGraphInstance, uniqueData);

        // top down update all incoming connections
        for (BlendTreeConnection* connection : m_connections)
        {
            TopDownUpdateIncomingNode(animGraphInstance, connection->GetSourceNode(), timePassedInSeconds);
        }
    }

    void BlendTreePoseSwitchNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreePoseSwitchNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreePoseSwitchNode>("Pose Switch", "Pose switch attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX
