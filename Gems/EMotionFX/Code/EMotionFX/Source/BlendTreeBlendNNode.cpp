/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BlendTreeBlendNNode.h"
#include "ActorInstance.h"
#include "AnimGraphAttributeTypes.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "AnimGraphMotionNode.h"
#include "EMotionFXConfig.h"
#include "MotionInstance.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/AnimGraphBus.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeBlendNNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeBlendNNode::UniqueData, AnimGraphObjectUniqueDataAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendNParamWeight, AnimGraphAllocator)

    BlendTreeBlendNNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
    {
    }

    void BlendTreeBlendNNode::UniqueData::Update()
    {
        BlendTreeBlendNNode* blendNNode = azdynamic_cast<BlendTreeBlendNNode*>(m_object);
        AZ_Assert(blendNNode, "Unique data linked to incorrect node type.");

        blendNNode->UpdateParamWeightRanges();
    }

    BlendNParamWeight::BlendNParamWeight(AZ::u32 portId, float weightRange)
        : m_portId(portId)
        , m_weightRange(weightRange)
    {
    }

    const char* BlendNParamWeight::GetPortLabel() const
    {
        return BlendTreeBlendNNode::GetPoseInputPortName(m_portId);
    }

    AZ::u32 BlendNParamWeight::GetPortId() const
    {
        return m_portId;
    }

    float BlendNParamWeight::GetWeightRange() const
    {
        return m_weightRange;
    }

    void BlendNParamWeight::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendNParamWeight>()
            ->Version(1)
            ->Field("portId", &BlendNParamWeight::m_portId)
            ->Field("weightRange", &BlendNParamWeight::m_weightRange);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendNParamWeight>("Blend N Param Weight", "Blend N Param Weight")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, &BlendNParamWeight::GetPortLabel);
    }

    BlendTreeBlendNNode::BlendTreeBlendNNode()
        : AnimGraphNode()
        , m_syncMode(SYNCMODE_DISABLED)
        , m_eventMode(EVENTMODE_MOSTACTIVE)
    {
        // setup input ports
        InitInputPorts(11);
        SetupInputPort(GetPoseInputPortName(PORTID_INPUT_POSE_0), INPUTPORT_POSE_0, AttributePose::TYPE_ID, PORTID_INPUT_POSE_0);
        SetupInputPort(GetPoseInputPortName(PORTID_INPUT_POSE_1), INPUTPORT_POSE_1, AttributePose::TYPE_ID, PORTID_INPUT_POSE_1);
        SetupInputPort(GetPoseInputPortName(PORTID_INPUT_POSE_2), INPUTPORT_POSE_2, AttributePose::TYPE_ID, PORTID_INPUT_POSE_2);
        SetupInputPort(GetPoseInputPortName(PORTID_INPUT_POSE_3), INPUTPORT_POSE_3, AttributePose::TYPE_ID, PORTID_INPUT_POSE_3);
        SetupInputPort(GetPoseInputPortName(PORTID_INPUT_POSE_4), INPUTPORT_POSE_4, AttributePose::TYPE_ID, PORTID_INPUT_POSE_4);
        SetupInputPort(GetPoseInputPortName(PORTID_INPUT_POSE_5), INPUTPORT_POSE_5, AttributePose::TYPE_ID, PORTID_INPUT_POSE_5);
        SetupInputPort(GetPoseInputPortName(PORTID_INPUT_POSE_6), INPUTPORT_POSE_6, AttributePose::TYPE_ID, PORTID_INPUT_POSE_6);
        SetupInputPort(GetPoseInputPortName(PORTID_INPUT_POSE_7), INPUTPORT_POSE_7, AttributePose::TYPE_ID, PORTID_INPUT_POSE_7);
        SetupInputPort(GetPoseInputPortName(PORTID_INPUT_POSE_8), INPUTPORT_POSE_8, AttributePose::TYPE_ID, PORTID_INPUT_POSE_8);
        SetupInputPort(GetPoseInputPortName(PORTID_INPUT_POSE_9), INPUTPORT_POSE_9, AttributePose::TYPE_ID, PORTID_INPUT_POSE_9);
        SetupInputPortAsNumber("Weight", INPUTPORT_WEIGHT, PORTID_INPUT_WEIGHT); // accept float/int/bool values

        // setup output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }

    bool BlendTreeBlendNNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();
        Reinit();
        return true;
    }

    const char* BlendTreeBlendNNode::GetPaletteName() const
    {
        return "Blend N";
    }

    AnimGraphObject::ECategory BlendTreeBlendNNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }

    void BlendTreeBlendNNode::UpdateParamWeightRanges()
    {
        // Initialize default connection custom weights
        // If this node has connections but no custom weights, it needs to set the default custom weight ranges
        if (m_paramWeights.empty())
        {
            float weightRange = 0.0f;
            const float defaultWeightStep = 1.0f;
            for (const AnimGraphNode::Port& port : m_inputPorts)
            {
                if (port.m_connection && port.m_portId != PORTID_INPUT_WEIGHT)
                {
                    m_paramWeights.emplace_back(port.m_portId, weightRange);
                    weightRange += defaultWeightStep;
                }
            }
            if (!m_paramWeights.empty())
            {
                const float maxWeightRange = m_paramWeights.back().m_weightRange;
                if (maxWeightRange > AZ::Constants::FloatEpsilon)
                {
                    for (BlendNParamWeight& paramWeight : m_paramWeights)
                    {
                        paramWeight.m_weightRange /= maxWeightRange;
                    }
                }
            }
        }
    }

    // find the two blend nodes
    void BlendTreeBlendNNode::FindBlendNodes(AnimGraphInstance* animGraphInstance, AnimGraphNode** outNodeA, AnimGraphNode** outNodeB, uint32* outIndexA, uint32* outIndexB, float* outWeight) const
    {
        if (m_paramWeights.empty())
        {
            *outNodeA = nullptr;
            *outNodeB = nullptr;
            *outIndexA = MCORE_INVALIDINDEX32;
            *outIndexB = MCORE_INVALIDINDEX32;
            *outWeight = 0.0f;
            return;
        }

        float weight = m_paramWeights.front().m_weightRange;
        if (!m_disabled)
        {
            if (m_inputPorts[INPUTPORT_WEIGHT].m_connection)
            {
                weight = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_WEIGHT);
            }
        }

        uint32 poseIndexA = m_paramWeights.front().m_portId;
        uint32 poseIndexB = m_paramWeights.front().m_portId;

        // If weight is <= minimum weight range the port id is the first for both poses (A and B) then the output weight is zero
        if (weight <= m_paramWeights.front().m_weightRange)
        {
            poseIndexA = m_paramWeights.front().m_portId;
            poseIndexB = poseIndexA;
            *outWeight = 0.0f;

            // Calculate the blend weight and get the nodes
            *outNodeA = GetInputPort(INPUTPORT_POSE_0 + poseIndexA).m_connection->GetSourceNode();
            *outNodeB = GetInputPort(INPUTPORT_POSE_0 + poseIndexB).m_connection->GetSourceNode();
            *outIndexA = poseIndexA;
            *outIndexB = poseIndexB;

            return;
        }

        // Searching for the index corresponding to the weight range in the sorted weight range array
        const size_t paramWeightCount = m_paramWeights.size();
        for (size_t i = 1; i < paramWeightCount; ++i)
        {
            if (weight <= m_paramWeights[i].m_weightRange)
            {
                poseIndexB = m_paramWeights[i].m_portId;
                poseIndexA = m_paramWeights[i - 1].m_portId;
                *outWeight = (weight - m_paramWeights[i - 1].m_weightRange)
                    / (m_paramWeights[i].m_weightRange - m_paramWeights[i - 1].m_weightRange);

                if (i == 1 && (*outWeight < MCore::Math::epsilon))
                {
                    // Snap to the minimum if for the first range check the value is near 0
                    poseIndexA = m_paramWeights.front().m_portId;
                    poseIndexB = poseIndexA;
                    *outWeight = 0.0f;
                }
                else if (i == (paramWeightCount - 1) && (*outWeight > 1.0f - MCore::Math::epsilon))
                {
                    // Snap to the maximum if for the first range check the value is near 1
                    poseIndexA = m_paramWeights.back().m_portId;
                    poseIndexB = poseIndexA;
                    *outWeight = 0.0f;
                }
                else if ((*outWeight > 1.0f - MCore::Math::epsilon))
                {
                    poseIndexA = poseIndexB;
                    *outWeight = 0.0f;
                }

                // Search complete: the input weight is between m_paramWeights[i] and m_paramWeights[i - 1]
                // Calculate the blend weight and get the nodes and then return
                *outNodeA = GetInputPort(INPUTPORT_POSE_0 + poseIndexA).m_connection->GetSourceNode();
                *outNodeB = GetInputPort(INPUTPORT_POSE_0 + poseIndexB).m_connection->GetSourceNode();
                *outIndexA = poseIndexA;
                *outIndexB = poseIndexB;

                return;
            }
        }

        // Not found in the range (nor below it) so snap to the maximum
        poseIndexA = m_paramWeights.back().m_portId;
        poseIndexB = poseIndexA;
        *outWeight = 0.0f;

        // Calculate the blend weight and get the nodes
        *outNodeA = GetInputPort(INPUTPORT_POSE_0 + poseIndexA).m_connection->GetSourceNode();
        *outNodeB = GetInputPort(INPUTPORT_POSE_0 + poseIndexB).m_connection->GetSourceNode();
        *outIndexA = poseIndexA;
        *outIndexB = poseIndexB;
    }

    void BlendTreeBlendNNode::SyncMotions(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeA, AnimGraphNode* nodeB, uint32 poseIndexA, uint32 poseIndexB, float blendWeight, ESyncMode syncMode)
    {
        if (nodeA == nullptr || nodeB == nullptr)
        {
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueNodeData(this));

        // check if we need to resync, this indicates the two motions we blend between changed
        bool resync = false;
        if (uniqueData->m_indexA != poseIndexA || uniqueData->m_indexB != poseIndexB)
        {
            resync = true;
            nodeA->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_RESYNC, true);
        }

        // Sync the primary node to this blend N node.
        nodeA->AutoSync(animGraphInstance, this, 0.0f, SYNCMODE_TRACKBASED, resync);

        // for all input ports (10 motion input poses)
        for (uint32 i = 0; i < 10; ++i)
        {
            // check if this port is used
            BlendTreeConnection* connection = m_inputPorts[i].m_connection;
            if (connection == nullptr)
            {
                continue;
            }

            // mark this node recursively as synced
            if (animGraphInstance->GetIsObjectFlagEnabled(m_objectIndex, AnimGraphInstance::OBJECTFLAGS_SYNCED) == false)
            {
                connection->GetSourceNode()->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, true);
            }

            if (connection->GetSourceNode() == nodeA)
            {
                continue;
            }

            // get the node to sync, and check the resync flag
            AnimGraphNode* nodeToSync = connection->GetSourceNode();
            if (resync)
            {
                nodeToSync->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_RESYNC, true);
            }

            // Only use the blend weight when syncing between the two active anim graph input nodes, otherwise use a blend weight of 1.
            if (i != poseIndexB)
            {
                nodeToSync->AutoSync(animGraphInstance, nodeA, 1.0f, syncMode, resync);
            }
            else
            {
                nodeToSync->AutoSync(animGraphInstance, nodeA, blendWeight, syncMode, resync);
            }
        }

        uniqueData->m_indexA = poseIndexA;
        uniqueData->m_indexB = poseIndexB;
    }

    // perform the calculations / actions
    void BlendTreeBlendNNode::Output(AnimGraphInstance* animGraphInstance)
    {
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();

        // get the output pose
        AnimGraphPose* outputPose;

        // if there are no connections, there is nothing to do
        if (m_disabled || !HasRequiredInputs())
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();

            // output a bind pose
            outputPose->InitFromBindPose(actorInstance);

            // visualize it
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
            }
            return;
        }

        // output the input weight node
        BlendTreeConnection* connection = m_inputPorts[INPUTPORT_WEIGHT].m_connection;
        if (connection)
        {
            OutputIncomingNode(animGraphInstance, connection->GetSourceNode());
        }

        // get two nodes that we receive input poses from, and get the blend weight
        float blendWeight;
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        uint32 poseIndexA;
        uint32 poseIndexB;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &poseIndexA, &poseIndexB, &blendWeight);

        // if there are no input poses, there is nothing else to do
        if (nodeA == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(actorInstance);

            // visualize it
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
            }
            return;
        }

        // if both nodes are equal we can just output the given pose
        OutputIncomingNode(animGraphInstance, nodeA);
        const AnimGraphPose* poseA = GetInputPose(animGraphInstance, INPUTPORT_POSE_0 + poseIndexA)->GetValue();
        if (nodeA == nodeB || blendWeight < MCore::Math::epsilon || nodeB == nullptr)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();

            *outputPose = *poseA;
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
            }
            return;
        }

        // get the second pose, and check if blending is still needed
        OutputIncomingNode(animGraphInstance, nodeB);
        const AnimGraphPose* poseB = GetInputPose(animGraphInstance, INPUTPORT_POSE_0 + poseIndexB)->GetValue();
        if (blendWeight > 1.0f - MCore::Math::epsilon)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *poseB;

            // visualize it
            if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
            {
                actorInstance->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
            }
            return;
        }

        // perform the blend
        RequestPoses(animGraphInstance);
        outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        *outputPose = *poseA;
        outputPose->GetPose().Blend(&poseB->GetPose(), blendWeight);

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            actorInstance->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }

    bool BlendTreeBlendNNode::HasRequiredInputs() const
    {
        if (m_connections.empty())
        {
            return false;
        }

        // If we have only one input connection and it is our weight input, that means we have no input poses.
        return !(m_connections.size() == 1 && m_inputPorts[INPUTPORT_WEIGHT].m_connection);
    }

    void BlendTreeBlendNNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (m_disabled || !HasRequiredInputs())
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            uniqueData->Clear();
            return;
        }

        const BlendTreeConnection* weightConnection = m_inputPorts[INPUTPORT_WEIGHT].m_connection;
        if (weightConnection)
        {
            UpdateIncomingNode(animGraphInstance, weightConnection->GetSourceNode(), timePassedInSeconds);
        }

        // get two nodes that we receive input poses from, and get the blend weight
        float blendWeight;
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        uint32 poseIndexA;
        uint32 poseIndexB;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &poseIndexA, &poseIndexB, &blendWeight);

        // if we have no input nodes
        if (!nodeA)
        {
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            uniqueData->Clear();
            return;
        }

        UpdateIncomingNode(animGraphInstance, nodeA, timePassedInSeconds);
        if (nodeB && nodeA != nodeB)
        {
            UpdateIncomingNode(animGraphInstance, nodeB, timePassedInSeconds);
        }

        // update the sync track
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        uniqueData->Init(animGraphInstance, nodeA);

        // output the correct play speed
        float factorA;
        float factorB;
        float playSpeed;
        AnimGraphNode::CalcSyncFactors(animGraphInstance, nodeA, nodeB, m_syncMode, blendWeight, &factorA, &factorB, &playSpeed);
        uniqueData->SetPlaySpeed(playSpeed * factorA);
    }

    void BlendTreeBlendNNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if the node is disabled
        if (m_disabled || !HasRequiredInputs())
        {
            return;
        }

        // top down update the weight input
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        const BlendTreeConnection* con = GetInputPort(INPUTPORT_WEIGHT).m_connection;
        if (con)
        {
            con->GetSourceNode()->FindOrCreateUniqueNodeData(animGraphInstance)->SetGlobalWeight(uniqueData->GetGlobalWeight());
            TopDownUpdateIncomingNode(animGraphInstance, con->GetSourceNode(), timePassedInSeconds);
        }

        // get two nodes that we receive input poses from, and get the blend weight
        float blendWeight;
        AnimGraphNode* nodeA = nullptr;
        AnimGraphNode* nodeB = nullptr;
        uint32 poseIndexA;
        uint32 poseIndexB;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &poseIndexA, &poseIndexB, &blendWeight);

        // check if we want to sync the motions
        if (nodeA)
        {
            if (m_syncMode != SYNCMODE_DISABLED)
            {
                SyncMotions(animGraphInstance, nodeA, nodeB, poseIndexA, poseIndexB, blendWeight, m_syncMode);
            }
            else
            {
                nodeA->SetPlaySpeed(animGraphInstance, uniqueData->GetPlaySpeed() /* * nodeA->GetInternalPlaySpeed(animGraphInstance)*/);
                if (animGraphInstance->GetIsObjectFlagEnabled(nodeA->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCED))
                {
                    nodeA->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, false);
                }
            }

            nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->SetGlobalWeight(uniqueData->GetGlobalWeight() * (1.0f - blendWeight));
            nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->SetLocalWeight(1.0f - blendWeight);
        }

        if (nodeB)
        {
            if (m_syncMode == SYNCMODE_DISABLED)
            {
                nodeB->SetPlaySpeed(animGraphInstance, uniqueData->GetPlaySpeed() /* * nodeB->GetInternalPlaySpeed(animGraphInstance)*/);
                if (animGraphInstance->GetIsObjectFlagEnabled(nodeB->GetObjectIndex(), AnimGraphInstance::OBJECTFLAGS_SYNCED))
                {
                    nodeB->RecursiveSetUniqueDataFlag(animGraphInstance, AnimGraphInstance::OBJECTFLAGS_SYNCED, false);
                }
            }

            nodeB->FindOrCreateUniqueNodeData(animGraphInstance)->SetGlobalWeight(uniqueData->GetGlobalWeight() * blendWeight);
            nodeB->FindOrCreateUniqueNodeData(animGraphInstance)->SetLocalWeight(blendWeight);
        }

        if (nodeA && nodeA == nodeB)
        {
            if (blendWeight < MCore::Math::epsilon)
            {
                nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->SetGlobalWeight(uniqueData->GetGlobalWeight());
                nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->SetLocalWeight(1.0f);
            }
            else
            {
                if (blendWeight > 1.0f - MCore::Math::epsilon)
                {
                    nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->SetGlobalWeight(0.0f);
                    nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->SetLocalWeight(0.0f);
                }
            }
        }

        // Top-down update the relevant nodes.
        if (nodeA)
        {
            TopDownUpdateIncomingNode(animGraphInstance, nodeA, timePassedInSeconds);
        }

        if (nodeB && nodeA != nodeB)
        {
            TopDownUpdateIncomingNode(animGraphInstance, nodeB, timePassedInSeconds);
        }
    }

    // post sync update
    void BlendTreeBlendNNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // if we don't have enough inputs or are disabled, we don't need to update anything
        if (m_disabled || !HasRequiredInputs())
        {
            // request the reference counted data inside the unique data
            RequestRefDatas(animGraphInstance);
            UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // get the input weight
        BlendTreeConnection* connection = m_inputPorts[INPUTPORT_WEIGHT].m_connection;
        if (connection)
        {
            PostUpdateIncomingNode(animGraphInstance, connection->GetSourceNode(), timePassedInSeconds);
        }

        // get two nodes that we receive input poses from, and get the blend weight
        float blendWeight;
        AnimGraphNode* nodeA;
        AnimGraphNode* nodeB;
        uint32 poseIndexA;
        uint32 poseIndexB;
        FindBlendNodes(animGraphInstance, &nodeA, &nodeB, &poseIndexA, &poseIndexB, &blendWeight);

        // if we have no input nodes
        if (nodeA == nullptr)
        {
            // request the reference counted data inside the unique data
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

        // request the reference counted data inside the unique data
        RequestRefDatas(animGraphInstance);
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

        FilterEvents(animGraphInstance, m_eventMode, nodeA, nodeB, blendWeight, data);

        // if we have just one input node
        if (nodeA == nodeB || nodeB == nullptr)
        {
            AnimGraphRefCountedData* sourceData = nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
            data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
            data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
            return;
        }

        // extract motion from both
        AnimGraphRefCountedData* nodeAData = nodeA->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
        AnimGraphRefCountedData* nodeBData = nodeB->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();

        // blend the results
        Transform delta = nodeAData->GetTrajectoryDelta();
        delta.Blend(nodeBData->GetTrajectoryDelta(), blendWeight);
        data->SetTrajectoryDelta(delta);

        // blend the mirrored results
        delta = nodeAData->GetTrajectoryDeltaMirrored();
        delta.Blend(nodeBData->GetTrajectoryDeltaMirrored(), blendWeight);
        data->SetTrajectoryDeltaMirrored(delta);
    }

    static bool BlendTreeBlendNNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        const unsigned int version = classElement.GetVersion();
        if (version < 2)
        {
            AZStd::vector<BlendNParamWeight> paramWeights;
            classElement.AddElementWithData(context, "paramWeights", paramWeights);
        }
        return true;
    }

    void BlendTreeBlendNNode::Reflect(AZ::ReflectContext* context)
    {
        BlendNParamWeight::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeBlendNNode, AnimGraphNode>()
            ->Version(2, &BlendTreeBlendNNodeVersionConverter)
            ->Field("syncMode", &BlendTreeBlendNNode::m_syncMode)
            ->Field("eventMode", &BlendTreeBlendNNode::m_eventMode)
            ->Field("paramWeights", &BlendTreeBlendNNode::m_paramWeights);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeBlendNNode>("Blend N", "Blend N attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeBlendNNode::m_syncMode)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeBlendNNode::m_eventMode)
            ->DataElement(AZ_CRC_CE("BlendNParamWeightsContainerHandler"), &BlendTreeBlendNNode::m_paramWeights, "Blend weight triggers", "The values of the input weight at which an input pose will weigh 100%")
            ->Attribute(AZ_CRC_CE("BlendTreeBlendNNodeParamWeightsElement"), "")
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeBlendNNode::UpdateParamWeightRanges)
            ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ_CRC_CE("BlendNParamWeightsElementHandler"));
    }

    void BlendTreeBlendNNode::SetSyncMode(ESyncMode syncMode)
    {
        m_syncMode = syncMode;
    }

    void BlendTreeBlendNNode::SetEventMode(EEventMode eventMode)
    {
        m_eventMode = eventMode;
    }

    void BlendTreeBlendNNode::SetParamWeightsEquallyDistributed(float min, float max)
    {
        if (m_paramWeights.empty())
        {
            return;
        }

        float weightStep = 0;
        if (m_paramWeights.size() > 1)
        {
            weightStep = (max - min) / (m_paramWeights.size() - 1);
        }
        m_paramWeights.back().m_weightRange = max;
        float weightRange = min;
        for (size_t i = 0; i < m_paramWeights.size() - 1; ++i)
        {
            m_paramWeights[i].m_weightRange = weightRange;
            weightRange += weightStep;
        }
    }

    void BlendTreeBlendNNode::UpdateParamWeights()
    {
        AZStd::unordered_map<AZ::u32, float> portToWeightRangeTable;
        for (const BlendNParamWeight& paramWeight : m_paramWeights)
        {
            portToWeightRangeTable.emplace(paramWeight.GetPortId(), paramWeight.GetWeightRange());
        }
        m_paramWeights.clear();

        const AZStd::vector<AnimGraphNode::Port>& inputPorts = GetInputPorts();
        int defaultElementsCount = 0;
        float* lastNonDefaultValue = nullptr;
        for (const AnimGraphNode::Port& port : inputPorts)
        {
            if (port.m_connection && port.m_portId != PORTID_INPUT_WEIGHT)
            {
                const float defaultRangeValue = m_paramWeights.empty() ? 0.0f : m_paramWeights.back().GetWeightRange();
                auto portToWeightRangeIterator = portToWeightRangeTable.find(port.m_portId);

                if (portToWeightRangeIterator == portToWeightRangeTable.end())
                {
                    // New connection just plugged
                    m_paramWeights.emplace_back(port.m_portId, defaultRangeValue);
                    defaultElementsCount++;
                }
                else
                {
                    // Existing connection, using existing weight range
                    m_paramWeights.emplace_back(port.m_portId, portToWeightRangeIterator->second);

                    // We want to fill the previous default values with uniformly distributed
                    // Weight ranges, if possible:
                    // Calculating the values to spread backwards to the previous default values
                    float weightRangeStep = 0;
                    if (lastNonDefaultValue)
                    {
                        weightRangeStep = (portToWeightRangeIterator->second - *lastNonDefaultValue) / (defaultElementsCount + 1);
                    }
                    float weightRange = portToWeightRangeIterator->second;
                    for (int i = 1; i <= defaultElementsCount; ++i)
                    {
                        weightRange -= weightRangeStep;
                        m_paramWeights[m_paramWeights.size() - 1 - i].m_weightRange = weightRange;
                    }
                    // Resetting the state of the default value calculator
                    defaultElementsCount = 0;
                    lastNonDefaultValue = &portToWeightRangeIterator->second;
                }
            }
        }

        AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnSyncVisualObject, this);
    }

    const AZStd::vector<BlendNParamWeight>& BlendTreeBlendNNode::GetParamWeights()
    {
        return m_paramWeights;
    }

    const char* BlendTreeBlendNNode::GetPoseInputPortName(AZ::u32 portId)
    {
        const char* name = "";
        switch (portId)
        {
            case PORTID_INPUT_POSE_0:
                name = "Pose 0";
                break;
            case PORTID_INPUT_POSE_1:
                name = "Pose 1";
                break;
            case PORTID_INPUT_POSE_2:
                name = "Pose 2";
                break;
            case PORTID_INPUT_POSE_3:
                name = "Pose 3";
                break;
            case PORTID_INPUT_POSE_4:
                name = "Pose 4";
                break;
            case PORTID_INPUT_POSE_5:
                name = "Pose 5";
                break;
            case PORTID_INPUT_POSE_6:
                name = "Pose 6";
                break;
            case PORTID_INPUT_POSE_7:
                name = "Pose 7";
                break;
            case PORTID_INPUT_POSE_8:
                name = "Pose 8";
                break;
            case PORTID_INPUT_POSE_9:
                name = "Pose 9";
                break;
            default:
                AZ_Assert(false, "Error: unknown input port id %u", portId);
                break;
        }
        return name;
    }
} // namespace EMotionFX
