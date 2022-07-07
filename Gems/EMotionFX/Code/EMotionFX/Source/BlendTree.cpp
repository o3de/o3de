/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <EMotionFX/Source/AnimGraph.h>
#include "EMotionFXConfig.h"
#include "BlendTree.h"
#include "AnimGraphNode.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "AnimGraphMotionNode.h"
#include "MotionInstance.h"
#include "AnimGraphInstance.h"
#include "AnimGraphManager.h"
#include "AnimGraphSyncTrack.h"
#include <EMotionFX/Source/AnimGraphBus.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTree, AnimGraphAllocator, 0)

    BlendTree::BlendTree()
        : AnimGraphNode()
        , m_finalNodeId(AnimGraphNodeId::InvalidId)
        , m_finalNode(nullptr)
        , m_virtualFinalNode(nullptr)
    {
        // setup output ports
        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }


    BlendTree::~BlendTree()
    {
        // NOTE: child nodes get removed by the base class already
    }


    void BlendTree::Reinit()
    {
        m_finalNode = nullptr;

        if (m_finalNodeId == AnimGraphNodeId::InvalidId)
        {
            // Double-check if a final node exists and the final node id is just not set.
            AnimGraphNode* finalNode = FindFirstChildNodeOfType(azrtti_typeid<BlendTreeFinalNode>());
            if (!finalNode)
            {
                // No final node exists in the blend tree.
                return;
            }

            // Safety fallback: Relink the final node id to the existing final node
            m_finalNodeId = finalNode->GetId();
            m_finalNode = static_cast<BlendTreeFinalNode*>(finalNode);
        }
        else
        {
            AnimGraphNode* finalNodeCandidate = FindChildNodeById(m_finalNodeId);
            if (finalNodeCandidate)
            {
                if (azrtti_typeid(finalNodeCandidate) == azrtti_typeid<BlendTreeFinalNode>())
                {
                    m_finalNode = static_cast<BlendTreeFinalNode*>(finalNodeCandidate);
                }
            }
        }
    }


    bool BlendTree::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        // Relink input and output ports for all nodes in the blend tree with their corresponding connections.
        // This has to be done after all child nodes called InitAfterLoading() and RegisterPorts(). We're depending on the node load order here
        // and a given node might be connected to one that has not been loaded yet and thus the ports have not been created yet.
        for (AnimGraphNode* childNode : m_childNodes)
        {
            childNode->RelinkPortConnections();
        }

        Reinit();
        return true;
    }


    // get the real final node
    AnimGraphNode* BlendTree::GetRealFinalNode() const
    {
        // if there is a virtual final node, use that one
        if (m_virtualFinalNode)
        {
            return m_virtualFinalNode;
        }

        // otherwise get the real final node
        if (m_finalNode)
        {
            return (m_finalNode->GetNumConnections() > 0) ? m_finalNode : nullptr;
        }

        return nullptr;
    }

    // process the blend tree and calculate its output
    void BlendTree::Output(AnimGraphInstance* animGraphInstance)
    {
        AZ_PROFILE_SCOPE(Animation, "BlendTree::Output");

        AZ_Assert(m_finalNode, "There should always be a final node. Something seems to be wrong with the blend tree creation.");

        // get the output pose
        AnimGraphPose* outputPose;

        // if this node is disabled, output the bind pose
        if (m_disabled)
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
            return;
        }

        // output final node
        AnimGraphNode* finalNode = GetRealFinalNode();
        if (finalNode)
        {
            OutputIncomingNode(animGraphInstance, finalNode);

            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            *outputPose = *finalNode->GetMainOutputPose(animGraphInstance);

            finalNode->DecreaseRef(animGraphInstance);
        }
        else
        {
            RequestPoses(animGraphInstance);
            outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
            outputPose->InitFromBindPose(animGraphInstance->GetActorInstance());
        }

        // visualize it
        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }


    // post sync update
    void BlendTree::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AZ_PROFILE_SCOPE(Animation, "AnimGraphStateMachine::PostUpdate");

        // if this node is disabled, exit
        if (m_disabled)
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        // update the final node
        AnimGraphNode* finalNode = GetRealFinalNode();
        if (finalNode)
        {
            finalNode->PerformPostUpdate(animGraphInstance, timePassedInSeconds);

            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();

            AnimGraphNodeData* finalNodeUniqueData = finalNode->FindOrCreateUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* sourceData = finalNodeUniqueData->GetRefCountedData();

            // TODO: this happens somehow for 1 frame when transitioning towards a blend tree (the nullptr)
            if (sourceData)
            {
                data->SetEventBuffer(sourceData->GetEventBuffer());
                data->SetTrajectoryDelta(sourceData->GetTrajectoryDelta());
                data->SetTrajectoryDeltaMirrored(sourceData->GetTrajectoryDeltaMirrored());
            }

            finalNode->DecreaseRefDataRef(animGraphInstance);
        }
        else
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ZeroTrajectoryDelta();
            data->ClearEventBuffer();
        }
    }


    // update all nodes
    void BlendTree::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AZ_PROFILE_SCOPE(Animation, "BlendTree::Update");

        // if this node is disabled, output the bind pose
        if (m_disabled)
        {
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
            return;
        }

        // if we have no virtual final node, use the real final node
        AnimGraphNode* finalNode = GetRealFinalNode();

        // update the final node
        if (finalNode)
        {
            finalNode->IncreasePoseRefCount(animGraphInstance);
            finalNode->IncreaseRefDataRefCount(animGraphInstance);
            finalNode->PerformUpdate(animGraphInstance, timePassedInSeconds);
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Init(animGraphInstance, finalNode);
        }
        else
        {
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            uniqueData->Clear();
        }
    }


    // rewind the nodes in the tree
    void BlendTree::Rewind(AnimGraphInstance* animGraphInstance)
    {
        for (AnimGraphNode* childNode : m_childNodes)
        {
            childNode->Rewind(animGraphInstance);
        }

        // call the base class rewind
        AnimGraphNode::Rewind(animGraphInstance);
    }


    // top down update
    void BlendTree::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AZ_PROFILE_SCOPE(Animation, "BlendTree::TopDownUpdate");

        // get the final node
        AnimGraphNode* finalNode = GetRealFinalNode();

        // update the final node
        if (finalNode)
        {
            // hierarhical sync update
            AnimGraphNodeData* uniqueData = FindOrCreateUniqueNodeData(animGraphInstance);
            HierarchicalSyncInputNode(animGraphInstance, finalNode, uniqueData);

            // pass the global weight along to the child nodes
            finalNode->PerformTopDownUpdate(animGraphInstance, timePassedInSeconds);
        }
    }


    // recursively set object data flag
    void BlendTree::RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled)
    {
        // set flag for this node
        animGraphInstance->SetObjectFlags(m_objectIndex, flag, enabled);

        // get the final node
        AnimGraphNode* finalNode = GetRealFinalNode();

        // update the final node recursively
        if (finalNode)
        {
            finalNode->RecursiveSetUniqueDataFlag(animGraphInstance, flag, enabled);
        }
    }


    // callback that gets called before a node gets removed
    void BlendTree::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        if (GetVirtualFinalNode() == nodeToRemove)
        {
            SetVirtualFinalNode(nullptr);
        }

        if (GetFinalNode() == nodeToRemove)
        {
            m_finalNodeId = AnimGraphNodeId::InvalidId;
            m_finalNode = nullptr;
        }

        // call it for all children
        AnimGraphNode::OnRemoveNode(animGraph, nodeToRemove);
    }


    void BlendTree::SetVirtualFinalNode(AnimGraphNode* node)
    {
        m_virtualFinalNode = node;

        AnimGraphNotificationBus::Broadcast(&AnimGraphNotificationBus::Events::OnVirtualFinalNodeSet, this);
    }


    void BlendTree::SetFinalNodeId(const AnimGraphNodeId finalNodeId)
    {
        m_finalNodeId = finalNodeId;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    void BlendTree::GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const
    {
        auto itConvertedIds = convertedIds.find(m_finalNodeId);
        if (itConvertedIds != convertedIds.end())
        {
            // need to convert
            attributesString = AZStd::string::format("-finalNodeId %llu", itConvertedIds->second);
        }
    }


    AZStd::unordered_set<AZStd::pair<BlendTreeConnection*, AnimGraphNode*> > BlendTree::FindCycles() const
    {
        AZStd::unordered_set<AnimGraphNode*> visitedNodes;
        AZStd::unordered_set<AZStd::pair<BlendTreeConnection*, AnimGraphNode*> > cycleConnections;

        for (AnimGraphNode* childNode : m_childNodes)
        {
            visitedNodes.clear();
            visitedNodes.emplace(childNode);
            RecursiveFindCycles(childNode, visitedNodes, cycleConnections);
        }

        return cycleConnections;
    }


    void BlendTree::RecursiveFindCycles(AnimGraphNode* nextNode, AZStd::unordered_set<AnimGraphNode*>& visitedNodes, AZStd::unordered_set<AZStd::pair<BlendTreeConnection*, AnimGraphNode*> >& cycleConnections) const
    {
        AZStd::unordered_map<AnimGraphNode*, AZStd::vector<BlendTreeConnection*> > sourceNodesAndConnections;
        const size_t numConnections = nextNode->GetNumConnections();
        for (size_t j = 0; j < numConnections; ++j)
        {
            AnimGraphNode* sourceNode = nextNode->GetConnection(j)->GetSourceNode();
            sourceNodesAndConnections[sourceNode].emplace_back(nextNode->GetConnection(j));
        }

        for (const auto& sourceNodeAndConnections : sourceNodesAndConnections)
        {
            if (visitedNodes.emplace(sourceNodeAndConnections.first).second)
            {
                // was able to insert it, so we didn't visit it yet
                RecursiveFindCycles(sourceNodeAndConnections.first, visitedNodes, cycleConnections);
            }
            else
            {
                // Trying to visit again, this is a cycle
                for (BlendTreeConnection* connection : sourceNodeAndConnections.second)
                {
                    cycleConnections.emplace(connection, nextNode);
                }
            }
        }

        visitedNodes.erase(nextNode);
    }


    bool BlendTree::ConnectionWillProduceCycle(AnimGraphNode* sourceNode, AnimGraphNode* targetNode) const
    {
        AZStd::unordered_set<AnimGraphNode*> visitedNodes;
        AZStd::unordered_set<AZStd::pair<BlendTreeConnection*, AnimGraphNode*> > cycleConnections;

        visitedNodes.emplace(targetNode);
        RecursiveFindCycles(sourceNode, visitedNodes, cycleConnections);

        return !cycleConnections.empty();
    }


    void BlendTree::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTree, AnimGraphNode>()
            ->Version(1)
            ->Field("finalNodeId", &BlendTree::m_finalNodeId)
        ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTree>("Blend Tree", "Blend tree attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }
} // namespace EMotionFX
