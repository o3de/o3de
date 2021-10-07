/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraphSnapshot.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNetworkSerializer.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>


namespace EMotionFX
{
    AnimGraphSnapshot::AnimGraphSnapshot(const AnimGraphInstance& instance, bool networkAuthoritative)
        : m_bundleSerializer(nullptr)
        , m_chunkSerializer(nullptr)
        , m_networkAuthoritative(networkAuthoritative)
        , m_dirtyFlag(0)
        , m_doFullRestore(false)
    {
        Init(instance);
    }

    AnimGraphSnapshot::~AnimGraphSnapshot()
    {
        m_parameters.clear();
        m_bundleSerializer = nullptr;
        m_chunkSerializer = nullptr;
    }

    void AnimGraphSnapshot::Init(const AnimGraphInstance& instance)
    {
        m_parameters.clear();

        const size_t numValueParameters = instance.GetAnimGraph()->GetNumValueParameters();
        for (size_t i = 0; i < numValueParameters; ++i)
        {
            m_parameters.emplace_back(instance.GetParameterValue(i)->Clone());
        }
    }

    void AnimGraphSnapshot::SetParameters(const AttributeContainer& attributes)
    {
        const size_t numParameters = m_parameters.size();
        AZ_Error("EMotionFX", numParameters == attributes.size(), "Attribute size mismatch");

        for (size_t i = 0; i < numParameters; ++i)
        {
            m_parameters[i]->InitFrom(attributes[i]);
        }
        m_dirtyFlag |= LODFlag::Parameter;
    }

    const AttributeContainer& AnimGraphSnapshot::GetParameters() const
    {
        return m_parameters;
    }

    void AnimGraphSnapshot::SetActiveNodes(const NodeIndexContainer& activeNodes)
    {
        if (m_activeStateNodes != activeNodes)
        {
            m_activeStateNodes = activeNodes;
            m_dirtyFlag |= LODFlag::ActiveNodes;
        }
    }

    const NodeIndexContainer& AnimGraphSnapshot::GetActiveNodes() const
    {
        return m_activeStateNodes;
    }

    void AnimGraphSnapshot::SetMotionNodePlaytimes(const MotionNodePlaytimeContainer& motionNodePlaytimes)
    {
        m_motionNodePlaytimes = motionNodePlaytimes;
        m_dirtyFlag |= LODFlag::MotionPlaytimes;
    }

    const MotionNodePlaytimeContainer& AnimGraphSnapshot::GetMotionNodePlaytimes() const
    {
        return m_motionNodePlaytimes;
    }

    void AnimGraphSnapshot::CollectAttributes(const AnimGraphInstance& instance)
    {
        const size_t numParams = m_parameters.size();
        AZ_Assert(instance.GetAnimGraph()->GetNumValueParameters() == numParams, "Attribute size mismatch. Did you forget to call init?");

        for (size_t i = 0; i < numParams; ++i)
        {
            m_parameters[i]->InitFrom(instance.GetParameterValue(i));
        }
    }

    void AnimGraphSnapshot::CollectActiveNodes(AnimGraphInstance& instance)
    {
        m_activeStateNodes.clear();
        AZStd::vector<AnimGraphNode*> stateMachineNodes;
        instance.CollectActiveAnimGraphNodes(&stateMachineNodes, azrtti_typeid<AnimGraphStateMachine>());

        const size_t numStateMachines = stateMachineNodes.size();
        for (size_t i = 0; i < numStateMachines; ++i)
        {
            AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(stateMachineNodes[i]);
            AnimGraphNode* currentState = stateMachine->GetCurrentState(&instance);

            AZ_Assert(currentState, "There should always be a valid current state.");
            m_activeStateNodes.emplace_back(aznumeric_caster(currentState->GetNodeIndex()));
        }
    }

    void AnimGraphSnapshot::CollectMotionNodePlaytimes(AnimGraphInstance& instance)
    {
        m_motionNodePlaytimes.clear();
        AZStd::vector<AnimGraphNode*> tempGraphNodes;
        instance.CollectActiveNetTimeSyncNodes(&tempGraphNodes);

        for (const AnimGraphNode* animGraphNode : tempGraphNodes)
        {
            const size_t nodeIndex = animGraphNode->GetNodeIndex();
            float normalizedPlaytime = animGraphNode->GetCurrentPlayTime(&instance) / animGraphNode->GetDuration(&instance);
            m_motionNodePlaytimes.emplace_back(aznumeric_caster(nodeIndex), normalizedPlaytime);
        }
    }

    void AnimGraphSnapshot::RestoreAttributes(AnimGraphInstance& instance)
    {
        const size_t numParams = m_parameters.size();

        for (size_t i = 0; i < numParams; ++i)
        {
            MCore::Attribute* attribute = instance.GetParameterValue(i);
            attribute->InitFrom(m_parameters[i]);
        }
    }

    void AnimGraphSnapshot::RestoreActiveNodes(AnimGraphInstance& instance)
    {
        for (const size_t nodeIndex : m_activeStateNodes)
        {
            AnimGraphNode* node = instance.GetAnimGraph()->GetNode(nodeIndex);
            AnimGraphNode* parent = node->GetParentNode();

            if (azrtti_typeid(parent) != azrtti_typeid<AnimGraphStateMachine>())
            {
                AZ_Error("EMotionFX", false, "Parent node should be a valid state machine");
                continue;
            }

            AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(parent);
            AnimGraphStateMachine::UniqueData* uniqueData = static_cast<AnimGraphStateMachine::UniqueData*>(stateMachine->FindOrCreateUniqueNodeData(&instance));
            const AZStd::vector<AnimGraphNode*>& activeStates = stateMachine->GetActiveStates(&instance);
            if (AZStd::find(activeStates.begin(), activeStates.end(), node) == activeStates.end())
            {
                stateMachine->EndAllActiveTransitions(&instance);
                uniqueData->m_currentState = node;
            }
        }
    }

    void AnimGraphSnapshot::RestoreMotionNodePlaytimes(AnimGraphInstance& instance)
    {
        for (const MotionPlayTimeEntry& motionPlaytime : m_motionNodePlaytimes)
        {
            AnimGraphNode* node = instance.GetAnimGraph()->GetNode(motionPlaytime.first);

            if (!node || !node->GetNeedsNetTimeSync())
            {
                AZ_Error("EMotionFX", false, "index should point to a valid node that need net time sync");
                continue;
            }

            node->SetCurrentPlayTimeNormalized(&instance, motionPlaytime.second);
        }
    }

    void AnimGraphSnapshot::Restore(AnimGraphInstance& instance)
    {
        // Full restore will skip the dirty flag check and restore everything to the animgraph from the snapshot data.
        // A full restore can happen either on the inital syncing, or after certian amount of server package lost.
        if (m_doFullRestore)
        {
            RestoreAttributes(instance);
            RestoreActiveNodes(instance);
            RestoreMotionNodePlaytimes(instance);
            m_doFullRestore = false;
            return;
        }

        // If we don't need to do a full restore, we restore the snapshot LOD based on the dirty flag.
        if (m_dirtyFlag & LODFlag::Parameter)
        {
            RestoreAttributes(instance);
        }

        if (m_dirtyFlag & LODFlag::ActiveNodes)
        {
            RestoreActiveNodes(instance);
        }

        if (m_dirtyFlag & LODFlag::MotionPlaytimes)
        {
            RestoreMotionNodePlaytimes(instance);
        }

        m_dirtyFlag = 0;
    }

    void AnimGraphSnapshot::OnNetworkConnected(AnimGraphInstance& instance)
    {
        AZ_UNUSED(instance);
        if (!IsNetworkAuthoritative())
        {
            // In this function, we deserialize from the serialzier to retrieve any data that need to be synced initially.
            if (m_bundleSerializer)
            {
                m_bundleSerializer->Deserialize(*this);
            }

            m_doFullRestore = true;
        }
    }

    void AnimGraphSnapshot::SetSnapshotSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotSerializer> serializer)
    {
        m_bundleSerializer = serializer;
    }

    void AnimGraphSnapshot::SetSnapshotChunkSerializer(AZStd::shared_ptr<Network::AnimGraphSnapshotChunkSerializer> serializer)
    {
        m_chunkSerializer = serializer;
    }

    void AnimGraphSnapshot::Serialize()
    {
        if (m_bundleSerializer)
        {
            m_bundleSerializer->Serialize(*this);
        }

        if (m_chunkSerializer)
        {
            size_t i = 0;

            for (; i < m_parameters.size(); ++i)
            {
                m_chunkSerializer->Serialize(*m_parameters[i], "parameters");
            }

            for (i = 0; i < m_activeStateNodes.size(); ++i)
            {
                m_chunkSerializer->Serialize(m_activeStateNodes[i], "activeNodes");
            }

            for (i = 0; i < m_motionNodePlaytimes.size(); ++i)
            {
                m_chunkSerializer->Serialize(m_motionNodePlaytimes[i].first, "motionNodePlaytime");
                m_chunkSerializer->Serialize(m_motionNodePlaytimes[i].second, "motionNodePlaytime");
            }
        }
    }
}
