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
#include "BlendTreeMirrorPoseNode.h"
#include "AnimGraphInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "EventManager.h"
#include "TransformData.h"

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeMirrorPoseNode, AnimGraphAllocator)

    BlendTreeMirrorPoseNode::BlendTreeMirrorPoseNode()
        : AnimGraphNode()
    {
        // setup the output ports
        InitInputPorts(2);
        SetupInputPort("Input Pose", INPUTPORT_POSE, AttributePose::TYPE_ID, PORTID_INPUT_POSE);
        SetupInputPortAsNumber("Enabled", INPUTPORT_ENABLED, PORTID_INPUT_ENABLED);

        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_RESULT, PORTID_OUTPUT_POSE);
    }


    BlendTreeMirrorPoseNode::~BlendTreeMirrorPoseNode()
    {
    }


    bool BlendTreeMirrorPoseNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeMirrorPoseNode::GetPaletteName() const
    {
        return "Mirror Pose";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeMirrorPoseNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MISC;
    }


    void BlendTreeMirrorPoseNode::HierarchicalSyncInputNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* inputNode, AnimGraphNodeData* uniqueDataOfThisNode)
    {
        MCORE_UNUSED(animGraphInstance);
        MCORE_UNUSED(inputNode);
        MCORE_UNUSED(uniqueDataOfThisNode);
    }

    // check if mirroring is enabled
    bool BlendTreeMirrorPoseNode::GetIsMirroringEnabled(AnimGraphInstance* animGraphInstance) const
    {
        // check the enabled state
        bool isEnabled = true;
        if (m_inputPorts[INPUTPORT_ENABLED].m_connection)
        {
            isEnabled = GetInputNumberAsBool(animGraphInstance, INPUTPORT_ENABLED);
        }

        return isEnabled;
    }


    // perform the update
    void BlendTreeMirrorPoseNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AnimGraphNode* sourceNode = GetInputNode(INPUTPORT_POSE);
        if (sourceNode == nullptr)
        {
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        // update the source node
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // init the unique data
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        uniqueData->Init(animGraphInstance, sourceNode);

        // apply mirroring to the sync track
        if (GetIsMirroringEnabled(animGraphInstance) && !m_disabled)
        {
            EMotionFX::AnimGraphNodeData* sourceNodeData = sourceNode->FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->SetSyncTrack(sourceNodeData->GetSyncTrack());
            uniqueData->SetIsMirrorMotion(sourceNodeData->GetIsMirrorMotion());
        }
    }


    // perform the calculations / actions
    void BlendTreeMirrorPoseNode::Output(AnimGraphInstance* animGraphInstance)
    {
        if (m_inputPorts[INPUTPORT_POSE].m_connection == nullptr)
        {
            // get the output pose
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        // if we're disabled just forward the input pose
        if (m_disabled)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
            const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
            // output the pose
            RequestPoses(animGraphInstance);
            AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
            *outputPose = *inputPose;
            return;
        }

        // get the input pose
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_POSE));
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_ENABLED));

        const AnimGraphPose* inputPose = GetInputPose(animGraphInstance, INPUTPORT_POSE)->GetValue();
        const Actor* actor = animGraphInstance->GetActorInstance()->GetActor();

        // output the pose
        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_RESULT)->GetValue();
        *outputPose = *inputPose; // TODO: is this really needed?

        // check the enabled state
        if (GetIsMirroringEnabled(animGraphInstance) && actor->GetHasMirrorInfo())
        {
            const Pose&             inPose          = inputPose->GetPose();
            Pose&                   outPose         = outputPose->GetPose();
            const ActorInstance*    actorInstance   = animGraphInstance->GetActorInstance();
            const TransformData*    transformData   = actorInstance->GetTransformData();
            const Pose*             bindPose        = transformData->GetBindPose();

            Transform outputTransform;

            // for all enabled nodes
            const size_t numNodes = actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                // get the node index that we sample the motion data from
                const uint16 nodeIndex = actorInstance->GetEnabledNode(i);
                const Actor::NodeMirrorInfo& mirrorInfo = actor->GetNodeMirrorInfo(nodeIndex);

                // build the mirror plane normal, based on the mirror axis for this node
                AZ::Vector3 mirrorPlaneNormal(0.0f, 0.0f, 0.0f);
                mirrorPlaneNormal.SetElement(mirrorInfo.m_axis, 1.0f);

                // apply the mirrored delta to the bind pose of the current node
                outputTransform = bindPose->GetLocalSpaceTransform(nodeIndex);
                outputTransform.ApplyDeltaMirrored(bindPose->GetLocalSpaceTransform(mirrorInfo.m_sourceNode), inPose.GetLocalSpaceTransform(mirrorInfo.m_sourceNode), mirrorPlaneNormal, mirrorInfo.m_flags);

                // update the pose with the new transform
                outPose.SetLocalSpaceTransform(nodeIndex, outputTransform);
            }
        }

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }


    // motion extraction
    void BlendTreeMirrorPoseNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // check if we have three incoming connections, if not, we can't really continue
        if (m_connections.size() == 0 || m_inputPorts[INPUTPORT_POSE].m_connection == nullptr)
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // perform motion extraction on the input node
        AnimGraphNode* inputNode = GetInputNode(INPUTPORT_POSE);
        PostUpdateIncomingNode(animGraphInstance, inputNode, timePassedInSeconds);

        RequestRefDatas(animGraphInstance);
        AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

        AnimGraphRefCountedData* sourceData = inputNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
        data->SetEventBuffer(sourceData->GetEventBuffer());
        if (GetIsMirroringEnabled(animGraphInstance) && m_disabled == false)
        {
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDeltaMirrored());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDelta());
        }
        else
        {
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
        }
    }


    void BlendTreeMirrorPoseNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeMirrorPoseNode, AnimGraphNode>()
            ->Version(1);


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeMirrorPoseNode>("Mirror Pose", "Mirror pose attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }
} // namespace EMotionFX
