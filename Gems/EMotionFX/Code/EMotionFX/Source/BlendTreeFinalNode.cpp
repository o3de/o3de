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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include "BlendTreeFinalNode.h"
#include "BlendTree.h"
#include "AnimGraphNode.h"
#include "AnimGraphAttributeTypes.h"


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeFinalNode, AnimGraphAllocator, 0)

    BlendTreeFinalNode::BlendTreeFinalNode()
        : AnimGraphNode()
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPort("Input Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    BlendTreeFinalNode::~BlendTreeFinalNode()
    {
    }


    bool BlendTreeFinalNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeFinalNode::GetPaletteName() const
    {
        return "Final Output";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFinalNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MISC;
    }


    // the main process method of the final node
    void BlendTreeFinalNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphPose* outputPose;

        // if there is no input, just output a bind pose
        if (mConnections.empty())
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        // output the source node
        AnimGraphNode* sourceNode = mConnections[0]->GetSourceNode();
        OutputIncomingNode(animGraphInstance, sourceNode);

        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
        *outputPose = *sourceNode->GetMainOutputPose(animGraphInstance);
    }


    // update the node
    void BlendTreeFinalNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if there are no connections, output nothing
        if (mConnections.empty())
        {
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        // update the source node
        AnimGraphNode* sourceNode = mConnections[0]->GetSourceNode();
        UpdateIncomingNode(animGraphInstance, sourceNode, timePassedInSeconds);

        // update the sync track
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        uniqueData->Init(animGraphInstance, sourceNode);
    }


    void BlendTreeFinalNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeFinalNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeFinalNode>("Final Node", "Final node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ;
    }
} // namespace EMotionFX
