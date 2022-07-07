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
#include "AnimGraphStateTransition.h"
#include "AnimGraphStateMachine.h"
#include "AnimGraphEntryNode.h"
#include "AnimGraphExitNode.h"
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphTriggerAction.h>
#include "AnimGraphTransitionCondition.h"
#include "AnimGraphManager.h"
#include "AnimGraphRefCountedData.h"
#include "AnimGraph.h"
#include "EMotionFXManager.h"
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Source/AnimGraphBus.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphStateTransition, AnimGraphAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphStateTransition::UniqueData, AnimGraphObjectUniqueDataAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphStateTransition::StateFilterLocal, AnimGraphAllocator, 0)

    bool AnimGraphStateTransition::StateFilterLocal::IsEmpty() const
    {
        return m_stateIds.empty() && m_nodeGroupNames.empty();
    }

    void AnimGraphStateTransition::StateFilterLocal::Clear()
    {
        m_stateIds.clear();
        m_nodeGroupNames.clear();
    }

    size_t AnimGraphStateTransition::StateFilterLocal::GetNumStates() const
    {
        return m_stateIds.size();
    }

    AnimGraphNodeId AnimGraphStateTransition::StateFilterLocal::GetStateId(size_t index) const
    {
        return AnimGraphNodeId(m_stateIds[index]);
    }

    AZStd::vector<AnimGraphNodeId> AnimGraphStateTransition::StateFilterLocal::CollectStateIds() const
    {
        AZStd::vector<AnimGraphNodeId> result;

        result.reserve(m_stateIds.size());
        for (const AZ::u64 stateId : m_stateIds)
        {
            result.emplace_back(stateId);
        }

        return result;
    }

    void AnimGraphStateTransition::StateFilterLocal::SetStateIds(const AZStd::vector<AnimGraphNodeId>& stateIds)
    {
        const size_t numStates = stateIds.size();
        m_stateIds.resize(numStates);
        for (size_t i = 0; i < numStates; ++i)
        {
            m_stateIds[i] = stateIds[i];
        }
    }

    size_t AnimGraphStateTransition::StateFilterLocal::GetNumGroups() const
    {
        return m_nodeGroupNames.size();
    }

    const AZStd::string& AnimGraphStateTransition::StateFilterLocal::GetGroupName(size_t index) const
    {
        return m_nodeGroupNames[index];
    }

    void AnimGraphStateTransition::StateFilterLocal::SetGroups(const AZStd::vector<AZStd::string>& groups)
    {
        m_nodeGroupNames = groups;
    }

    const AZStd::vector<AZStd::string>& AnimGraphStateTransition::StateFilterLocal::GetGroups() const
    {
        return m_nodeGroupNames;
    }

    AZStd::vector<AnimGraphNodeId> AnimGraphStateTransition::StateFilterLocal::CollectStates(AnimGraphStateMachine* stateMachine) const
    {
        AZStd::vector<AnimGraphNodeId> result;

        // Add individual states.
        result.reserve(m_stateIds.size());
        for (const AZ::u64 stateId : m_stateIds)
        {
            result.emplace_back(stateId);
        }

        // Add nodes from the groups.
        const AnimGraph* animGraph = stateMachine->GetAnimGraph();
        for (const AZStd::string& groupName : m_nodeGroupNames)
        {
            AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupByName(groupName.c_str());
            if (!nodeGroup)
            {
                continue;
            }

            const size_t numNodes = nodeGroup->GetNumNodes();
            for (size_t i = 0; i < numNodes; ++i)
            {
                AnimGraphNodeId nodeId = nodeGroup->GetNode(i);
                AnimGraphNode* node = stateMachine->FindChildNodeById(nodeId);
                if (!node)
                {
                    // Skip the node in case it is not part of the given state machine.
                    continue;
                }

                if (AZStd::find(result.begin(), result.end(), nodeId) == result.end())
                {
                    // Add the node in case it is not yet in the array.
                    result.emplace_back(nodeId);
                }
            }
        }

        return result;
    }

    bool AnimGraphStateTransition::StateFilterLocal::Contains(AnimGraph* animGraph, AnimGraphNodeId stateId) const
    {
        for (AZ::u64 currentStateId : m_stateIds)
        {
            if (stateId == AnimGraphNodeId(currentStateId))
            {
                return true;
            }
        }

        for (const AZStd::string& nodeGroupName : m_nodeGroupNames)
        {
            AnimGraphNodeGroup* nodeGroup = animGraph->FindNodeGroupByName(nodeGroupName.c_str());
            if (!nodeGroup)
            {
                continue;
            }

            if (nodeGroup->Contains(stateId))
            {
                return true;
            }
        }

        return false;
    }

    //-----------------------------------------------------------------------------------------------------------------

    AnimGraphStateTransition::AnimGraphStateTransition()
        : AnimGraphObject(nullptr)
    {
    }

    AnimGraphStateTransition::AnimGraphStateTransition(
        AnimGraphNode* source,
        AnimGraphNode* target,
        AZStd::vector<AnimGraphTransitionCondition*> conditions,
        float duration
    )
        : AnimGraphObject()
        , m_conditions(AZStd::move(conditions))
        , m_sourceNode(source)
        , m_targetNode(target)
        , m_sourceNodeId(source ? source->GetId() : ObjectId::InvalidId)
        , m_targetNodeId(target ? target->GetId() : ObjectId::InvalidId)
        , m_transitionTime(duration)
    {
    }

    AnimGraphStateTransition::~AnimGraphStateTransition()
    {
        RemoveAllConditions(true);
        if (m_animGraph)
        {
            m_animGraph->RemoveObject(this);
        }
    }

    void AnimGraphStateTransition::Reinit()
    {
        if (!m_animGraph)
        {
            m_sourceNode = nullptr;
            m_targetNode = nullptr;
            return;
        }

        // Re-link the source node.
        if (GetSourceNodeId().IsValid())
        {
            m_sourceNode = m_animGraph->RecursiveFindNodeById(GetSourceNodeId());
        }

        // Re-link the target node.
        if (GetTargetNodeId().IsValid())
        {
            m_targetNode = m_animGraph->RecursiveFindNodeById(GetTargetNodeId());
        }

        AnimGraphObject::Reinit();
    }

    void AnimGraphStateTransition::RecursiveReinit()
    {
        Reinit();

        for (AnimGraphTransitionCondition* condition : m_conditions)
        {
            condition->Reinit();
        }
    }

    bool AnimGraphStateTransition::InitAfterLoading(AnimGraph* animGraph)
    {
        SetAnimGraph(animGraph);

        if (animGraph)
        {
            animGraph->AddObject(this);
        }

        InitInternalAttributesForAllInstances();

        for (AnimGraphTransitionCondition* condition : m_conditions)
        {
            condition->SetTransition(this);
            condition->InitAfterLoading(animGraph);
        }

        for (AnimGraphTriggerAction* action : m_actionSetup.GetActions())
        {
            action->InitAfterLoading(animGraph);
        }

        Reinit();

        return true;
    }

    // calculate the transition output, this is the main function
    void AnimGraphStateTransition::CalcTransitionOutput(AnimGraphInstance* animGraphInstance, const AnimGraphPose& from, const AnimGraphPose& to, AnimGraphPose* outputPose) const
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));

        // calculate the blend weight, based on the type of smoothing
        const float weight = uniqueData->m_blendWeight;

        // blend the two poses
        *outputPose = from;
        outputPose->GetPose().Blend(&to.GetPose(), weight);
    }

    void AnimGraphStateTransition::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        const bool gotInterrupted = GotInterrupted(animGraphInstance);
        const bool updateTimeAndProgress = (!gotInterrupted || !m_interruptionBlendBehavior);

        if (updateTimeAndProgress)
        {
            const float blendTime = GetBlendTime(animGraphInstance);

            uniqueData->m_totalSeconds += timePassedInSeconds;
            if (uniqueData->m_totalSeconds >= blendTime)
            {
                uniqueData->m_totalSeconds = blendTime;
                uniqueData->m_isDone = true;
            }
            else
            {
                uniqueData->m_isDone = false;
            }

            // calculate the blend weight
            if (blendTime > MCore::Math::epsilon)
            {
                uniqueData->m_blendProgress = uniqueData->m_totalSeconds / blendTime;
            }
            else
            {
                uniqueData->m_blendProgress = 1.0f;
            }

            uniqueData->m_blendWeight = CalculateWeight(uniqueData->m_blendProgress);
        }
    }

    void AnimGraphStateTransition::ExtractMotion(AnimGraphInstance* animGraphInstance, AnimGraphRefCountedData* sourceData, Transform* outTransform, Transform* outTransformMirrored) const
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        const float weight = uniqueData->m_blendWeight;

        AnimGraphRefCountedData* targetData = m_targetNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
        CalculateMotionExtractionDelta(m_extractionMode, sourceData, targetData, weight, true, *outTransform, *outTransformMirrored);
    }

    void AnimGraphStateTransition::OnStartTransition(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));

        uniqueData->m_blendWeight    = 0.0f;
        uniqueData->m_isDone         = false;
        uniqueData->m_totalSeconds   = 0.0f;
        uniqueData->m_blendProgress  = 0.0f;

        m_targetNode->SetSyncIndex(animGraphInstance, InvalidIndex);

        // Trigger action
        for (AnimGraphTriggerAction* action : m_actionSetup.GetActions())
        {
            if (action->GetTriggerMode() == AnimGraphTriggerAction::MODE_TRIGGERONENTER)
            {
                action->TriggerAction(animGraphInstance);
            }
        }
    }

    // check and return if the transition is transitioning or already done
    bool AnimGraphStateTransition::GetIsDone(AnimGraphInstance* animGraphInstance) const
    {
        // get the unique data and return the is done flag
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        return uniqueData->m_isDone;
    }

    float AnimGraphStateTransition::GetBlendWeight(AnimGraphInstance* animGraphInstance) const
    {
        // get the unique data and return the is done flag
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        return uniqueData->m_blendWeight;
    }

    void AnimGraphStateTransition::OnEndTransition(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data
        UniqueData* uniqueData      = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        uniqueData->m_blendWeight    = 1.0f;
        uniqueData->m_blendProgress  = 1.0f;
        uniqueData->m_isDone         = true;

        // Trigger action
        for (AnimGraphTriggerAction* action : m_actionSetup.GetActions())
        {
            if (action->GetTriggerMode() == AnimGraphTriggerAction::MODE_TRIGGERONEXIT)
            {
                action->TriggerAction(animGraphInstance);
            }
        }
    }

    void AnimGraphStateTransition::AddCondition(AnimGraphTransitionCondition* condition)
    {
        condition->SetTransition(this);
        m_conditions.push_back(condition);
    }

    void AnimGraphStateTransition::InsertCondition(AnimGraphTransitionCondition* condition, size_t index)
    {
        condition->SetTransition(this);
        m_conditions.insert(m_conditions.begin() + index, condition);
    }

    void AnimGraphStateTransition::ReserveConditions(size_t numConditions)
    {
        m_conditions.reserve(numConditions);
    }

    void AnimGraphStateTransition::RemoveCondition(size_t index, bool delFromMem)
    {
        if (delFromMem)
        {
            delete m_conditions[index];
        }

        m_conditions.erase(m_conditions.begin() + index);
    }

    void AnimGraphStateTransition::RemoveAllConditions(bool delFromMem)
    {
        // delete them all from memory
        if (delFromMem)
        {
            for (AnimGraphTransitionCondition* condition : m_conditions)
            {
                delete condition;
            }
        }

        m_conditions.clear();
    }

    // check if all conditions are tested positive
    bool AnimGraphStateTransition::CheckIfIsReady(AnimGraphInstance* animGraphInstance) const
    {
        if (m_conditions.empty())
        {
            return false;
        }

        if (!GetEMotionFX().GetIsInEditorMode())
        {
            // If we are not in editor mode, we can early out for the first failed condition
            for (AnimGraphTransitionCondition* condition : m_conditions)
            {
                const bool testResult = condition->TestCondition(animGraphInstance);

                // return directly in case one condition is not ready yet
                if (!testResult)
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            // If we are in editor mode, we need to execute all the conditions so the UI can reflect properly which ones
            // passed and which ones didn't
            bool isReady = true;
            for (AnimGraphTransitionCondition* condition : m_conditions)
            {
                const bool testResult = condition->TestCondition(animGraphInstance);

                if (!testResult)
                {
                    isReady = false;
                }
            }
            return isReady;
        }
    }

    void AnimGraphStateTransition::SetIsWildcardTransition(bool isWildcardTransition)
    {
        m_isWildcardTransition = isWildcardTransition;
    }

    void AnimGraphStateTransition::SetSourceNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* sourceNode)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        uniqueData->m_sourceNode = sourceNode;
    }

    // get the source node of the transition
    AnimGraphNode* AnimGraphStateTransition::GetSourceNode(AnimGraphInstance* animGraphInstance) const
    {
        // return the normal source node in case we are not dealing with a wildcard transition
        if (m_isWildcardTransition == false)
        {
            return m_sourceNode;
        }

        // wildcard transition special case handling
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        return uniqueData->m_sourceNode;
    }

    void AnimGraphStateTransition::SetBlendTime(float blendTime)
    {
        m_transitionTime = blendTime;
    }

    float AnimGraphStateTransition::GetBlendTime(AnimGraphInstance* animGraphInstance) const
    {
        MCORE_UNUSED(animGraphInstance);

        // Use a blend time of zero in case this transition is connected to aan entry or exit state.
        if ((m_sourceNode && (azrtti_typeid(m_sourceNode) == azrtti_typeid<AnimGraphExitNode>() || azrtti_typeid(m_sourceNode) == azrtti_typeid<AnimGraphEntryNode>())) ||
            (m_targetNode && (azrtti_typeid(m_targetNode) == azrtti_typeid<AnimGraphExitNode>() || azrtti_typeid(m_targetNode) == azrtti_typeid<AnimGraphEntryNode>())))
        {
            return 0.0f;
        }

        return m_transitionTime;
    }

    // callback that gets called before a node gets removed
    void AnimGraphStateTransition::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        for (AnimGraphTransitionCondition* condition : m_conditions)
        {
            condition->OnRemoveNode(animGraph, nodeToRemove);
        }
    }

    void AnimGraphStateTransition::ResetConditions(AnimGraphInstance* animGraphInstance)
    {
        for (AnimGraphTransitionCondition* condition : m_conditions)
        {
            condition->Reset(animGraphInstance);
        }
    }

    void AnimGraphStateTransition::SetPriority(AZ::u32 priority)
    {
        m_priority = priority;
    }

    AZ::u32 AnimGraphStateTransition::GetPriority() const
    {
        return m_priority;
    }

    bool AnimGraphStateTransition::GetIsDisabled() const
    {
        return m_isDisabled;
    }

    void AnimGraphStateTransition::SetIsDisabled(bool isDisabled)
    {
        m_isDisabled = isDisabled;
    }

    void AnimGraphStateTransition::SetCanBeInterrupted(bool canBeInterrupted)
    {
        m_canBeInterruptedByOthers = canBeInterrupted;
    }

    void AnimGraphStateTransition::SetCanInterruptOtherTransitions(bool canInterruptOtherTransitions)
    {
        m_canInterruptOtherTransitions = canInterruptOtherTransitions;
    }

    bool AnimGraphStateTransition::GetCanInterruptOtherTransitions() const
    {
        return m_canInterruptOtherTransitions;
    }

    bool AnimGraphStateTransition::GotInterrupted(AnimGraphInstance* animGraphInstance) const
    {
        const AnimGraphStateMachine* stateMachine = GetStateMachine();
        if (stateMachine)
        {
            const AZStd::vector<AnimGraphStateTransition*>& activeTransitions = stateMachine->GetActiveTransitions(animGraphInstance);

            // The transition only got interrupted in case there are multiple on the transition stack.
            if (activeTransitions.size() > 1)
            {
                const auto found = AZStd::find(activeTransitions.begin(), activeTransitions.end(), this);

                // The transition got interrupted in case it is active but also not the latest active transition on the transition stack.
                if (found != activeTransitions.end() && found != activeTransitions.begin())
                {
                    return true;
                }
            }
        }

        return false;
    }

    void AnimGraphStateTransition::SetCanInterruptItself(bool canInterruptItself)
    {
        m_allowSelfInterruption = canInterruptItself;
    }

    bool AnimGraphStateTransition::GetCanInterruptItself() const
    {
        return m_allowSelfInterruption;
    }

    void AnimGraphStateTransition::SetCanBeInterruptedBy(const AZStd::vector<AnimGraphConnectionId>& transitionIds)
    {
        m_canBeInterruptedByTransitionIds.clear();
        m_canBeInterruptedByTransitionIds.reserve(transitionIds.size());
        for (const AnimGraphConnectionId& transitionId : transitionIds)
        {
            m_canBeInterruptedByTransitionIds.emplace_back(transitionId);
        }
    }

    bool AnimGraphStateTransition::CanBeInterruptedBy(const AnimGraphStateTransition* transition, AnimGraphInstance* animGraphInstance) const
    {
        if (m_canBeInterruptedByOthers &&
            transition != this &&
            (GetIsWildcardTransition() || transition->GetIsWildcardTransition() || transition->GetSourceNode() == m_sourceNode))
        {
            // Allow all in case the transition candidate list is empty, otherwise only allow transitions from the possible interruption candidate list.
            if (m_canBeInterruptedByTransitionIds.empty() ||
                AZStd::find(m_canBeInterruptedByTransitionIds.begin(), m_canBeInterruptedByTransitionIds.end(), transition->GetId()) != m_canBeInterruptedByTransitionIds.end())
            {
                switch (m_interruptionMode)
                {
                case AlwaysAllowed:
                {
                    return true;
                }
                case MaxBlendWeight:
                {
                    if (animGraphInstance)
                    {
                        const float currentBlendWeight = GetBlendWeight(animGraphInstance);
                        if (currentBlendWeight < m_maxInterruptionBlendWeight)
                        {
                            return true;
                        }
                    }
                    else
                    {
                        return true;
                    }

                    return false;
                }
                default:
                {
                    AZ_Assert(false, "Unhandled transition interruption mode.");
                    return false;
                }
                }
            }
        }

        return false;
    }

    void AnimGraphStateTransition::SetGroups(const AZStd::vector<AZStd::string>& groups)
    {
        m_allowTransitionsFrom.SetGroups(groups);
    }

    void AnimGraphStateTransition::SetStateIds(const AZStd::vector<AnimGraphNodeId>& stateIds)
    {
        m_allowTransitionsFrom.SetStateIds(stateIds);
    }

    void AnimGraphStateTransition::SetInterpolationType(AnimGraphStateTransition::EInterpolationType interpolationType)
    {
        m_interpolationType = interpolationType;
    }

    void AnimGraphStateTransition::SetEaseInSmoothness(float easeInSmoothness)
    {
        m_easeInSmoothness = easeInSmoothness;
    }

    void AnimGraphStateTransition::SetEaseOutSmoothness(float easeOutSmoothness)
    {
        m_easeOutSmoothness = easeOutSmoothness;
    }

    // add all sub objects
    void AnimGraphStateTransition::RecursiveCollectObjects(AZStd::vector<AnimGraphObject*>& outObjects) const
    {
        for (const AnimGraphTransitionCondition* condition : m_conditions)
        {
            condition->RecursiveCollectObjects(outObjects);
        }

        outObjects.emplace_back(const_cast<AnimGraphStateTransition*>(this));
    }

    // calculate the blend weight, based on the type of smoothing
    float AnimGraphStateTransition::CalculateWeight(float linearWeight) const
    {
        switch (m_interpolationType)
        {
        case INTERPOLATIONFUNCTION_LINEAR:
        {
            return linearWeight;
        }

        case INTERPOLATIONFUNCTION_EASECURVE:
        {
            return MCore::SampleEaseInOutCurveWithSmoothness(linearWeight, m_easeInSmoothness, m_easeOutSmoothness);
        }

        default:
        {
            AZ_Assert(false, "Interpolation type undefined.");
        }
        }

        return linearWeight;
    }

    AnimGraphStateTransition::UniqueData::UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
        : AnimGraphObjectData(object, animGraphInstance)
    {
    }

    void AnimGraphStateTransition::InvalidateUniqueData(AnimGraphInstance* animGraphInstance)
    {
        AnimGraphObject::InvalidateUniqueData(animGraphInstance);

        for (AnimGraphTransitionCondition* condition : m_conditions)
        {
            condition->InvalidateUniqueData(animGraphInstance);
        }

        const size_t numActions = m_actionSetup.GetNumActions();
        for (size_t a = 0; a < numActions; ++a)
        {
            AnimGraphTriggerAction* action = m_actionSetup.GetAction(a);
            action->InvalidateUniqueData(animGraphInstance);
        }
    }

    uint32 AnimGraphStateTransition::GetVisualColor() const
    {
        return MCore::RGBA(150, 150, 150);
    }

    bool AnimGraphStateTransition::GetIsStateTransitionNode() const
    {
        return true;
    }

    const char* AnimGraphStateTransition::GetPaletteName() const
    {
        return "Transition";
    }

    AnimGraphObject::ECategory AnimGraphStateTransition::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_TRANSITIONS;
    }

    void AnimGraphStateTransition::SetSyncMode(AnimGraphStateTransition::ESyncMode syncMode)
    {
        m_syncMode = syncMode;
    }

    AnimGraphStateTransition::ESyncMode AnimGraphStateTransition::GetSyncMode() const
    {
        return m_syncMode;
    }

    void AnimGraphStateTransition::SetEventFilterMode(AnimGraphObject::EEventMode eventMode)
    {
        m_eventMode = eventMode;
    }

    AnimGraphObject::EEventMode AnimGraphStateTransition::GetEventFilterMode() const
    {
        return m_eventMode;
    }

    void AnimGraphStateTransition::SetSourceNode(AnimGraphNode* node)
    {
        m_sourceNode = node;

        if (m_sourceNode)
        {
            m_sourceNodeId = m_sourceNode->GetId();
        }
        else
        {
            m_sourceNodeId = AnimGraphNodeId::InvalidId;
        }
    }

    AnimGraphNode* AnimGraphStateTransition::GetSourceNode() const
    {
        AZ_Assert(!m_sourceNode || (m_sourceNode && m_sourceNode->GetId() == GetSourceNodeId()), "Source node not in sync with node id.");
        return m_sourceNode;
    }

    void AnimGraphStateTransition::SetTargetNode(AnimGraphNode* node)
    {
        m_targetNode = node;

        if (m_targetNode)
        {
            m_targetNodeId = m_targetNode->GetId();
        }
        else
        {
            m_targetNodeId = AnimGraphNodeId::InvalidId;
        }
    }

    AnimGraphNode* AnimGraphStateTransition::GetTargetNode() const
    {
        AZ_Assert(m_targetNode && m_targetNode->GetId() == GetTargetNodeId(), "Target node not in sync with node id.");
        return m_targetNode;
    }

    void AnimGraphStateTransition::SetVisualOffsets(int32 startX, int32 startY, int32 endX, int32 endY)
    {
        m_startOffsetX   = startX;
        m_startOffsetY   = startY;
        m_endOffsetX     = endX;
        m_endOffsetY     = endY;
    }

    int32 AnimGraphStateTransition::GetVisualStartOffsetX() const
    {
        return m_startOffsetX;
    }

    int32 AnimGraphStateTransition::GetVisualStartOffsetY() const
    {
        return m_startOffsetY;
    }

    int32 AnimGraphStateTransition::GetVisualEndOffsetX() const
    {
        return m_endOffsetX;
    }

    int32 AnimGraphStateTransition::GetVisualEndOffsetY() const
    {
        return m_endOffsetY;
    }

    bool AnimGraphStateTransition::CanWildcardTransitionFrom(AnimGraphNode* sourceNode) const
    {
        if (m_allowTransitionsFrom.IsEmpty())
        {
            // An empty state filter means we can transition from any other state.
            return true;
        }

        if (sourceNode)
        {
            if (m_allowTransitionsFrom.Contains(m_animGraph, sourceNode->GetId()))
            {
                // In case the given source node is part of the filter (either as individual state or part of a node group), return success.
                return true;
            }
        }

        return false;
    }

    AZ::Outcome<size_t> AnimGraphStateTransition::FindConditionIndex(AnimGraphTransitionCondition* condition) const
    {
        const auto iterator = AZStd::find(m_conditions.begin(), m_conditions.end(), condition);
        if (iterator == m_conditions.end())
        {
            return AZ::Failure();
        }

        return AZ::Success(static_cast<size_t>(AZStd::distance(m_conditions.begin(), iterator)));
    }

    AnimGraphStateMachine* AnimGraphStateTransition::GetStateMachine() const
    {
        if (!m_targetNode)
        {
            return nullptr;
        }

        return azdynamic_cast<AnimGraphStateMachine*>(m_targetNode->GetParentNode());
    }

    AZ::Crc32 AnimGraphStateTransition::GetEaseInOutSmoothnessVisibility() const
    {
        if (GetVisibilityHideWhenExitOrEntry() == AZ::Edit::PropertyVisibility::Hide)
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return m_interpolationType == INTERPOLATIONFUNCTION_EASECURVE ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 AnimGraphStateTransition::GetVisibilityHideWhenExitOrEntry() const
    {
        // Hide when the transition is connected to an entry or an exit state.
        if ((m_sourceNode && (azrtti_typeid(m_sourceNode) == azrtti_typeid<AnimGraphExitNode>() || azrtti_typeid(m_sourceNode) == azrtti_typeid<AnimGraphEntryNode>())) ||
            (m_targetNode && (azrtti_typeid(m_targetNode) == azrtti_typeid<AnimGraphExitNode>() || azrtti_typeid(m_targetNode) == azrtti_typeid<AnimGraphEntryNode>())))
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return AZ::Edit::PropertyVisibility::Show;
    }

    AZ::Crc32 AnimGraphStateTransition::GetVisibilityAllowedStates() const
    {
        // Show the allow transitions from states for wildcard transitions only.
        if (GetIsWildcardTransition())
        {
            return AZ::Edit::PropertyVisibility::Show;
        }

        return AZ::Edit::PropertyVisibility::Hide;
    }

    AnimGraphObject::EExtractionMode AnimGraphStateTransition::GetExtractionMode() const
    {
        return m_extractionMode;
    }

    void AnimGraphStateTransition::SetExtractionMode(AnimGraphObject::EExtractionMode mode)
    {
        m_extractionMode = mode;
    }

    void AnimGraphStateTransition::StateFilterLocal::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphStateTransition::StateFilterLocal>()
            ->Version(1)
            ->Field("stateIds", &AnimGraphStateTransition::StateFilterLocal::m_stateIds)
            ->Field("nodeGroupNames", &AnimGraphStateTransition::StateFilterLocal::m_nodeGroupNames)
        ;
    }

    void AnimGraphStateTransition::GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const
    {
        auto itConvertedIds = convertedIds.find(m_sourceNodeId);
        if (itConvertedIds != convertedIds.end())
        {
            // need to convert
            attributesString = AZStd::string::format("-sourceNodeId %llu", itConvertedIds->second);
        }
        itConvertedIds = convertedIds.find(m_targetNodeId);
        if (itConvertedIds != convertedIds.end())
        {
            // need to convert
            attributesString += AZStd::string::format(" -targetNodeId %llu", itConvertedIds->second);
        }

        bool someThingPatched = false;
        AZStd::vector<AnimGraphNodeId> patchedTransitionsFrom = m_allowTransitionsFrom.CollectStateIds();
        for (AnimGraphNodeId& nodeId : patchedTransitionsFrom)
        {
            itConvertedIds = convertedIds.find(nodeId);
            if (itConvertedIds != convertedIds.end())
            {
                nodeId = itConvertedIds->second;
                someThingPatched = true;
            }
        }
        if (someThingPatched)
        {
            StateFilterLocal sf = m_allowTransitionsFrom;
            sf.SetStateIds(patchedTransitionsFrom);

            attributesString += AZStd::string::format(" -allowTransitionsFrom {%s}", MCore::ReflectionSerializer::Serialize(&sf).GetValue().c_str());
        }
    }

    AZ::Crc32 AnimGraphStateTransition::GetVisibilityInterruptionProperties() const
    {
        if (m_canBeInterruptedByOthers &&
            GetVisibilityHideWhenExitOrEntry() == AZ::Edit::PropertyVisibility::Show)
        {
            return AZ::Edit::PropertyVisibility::Show;
        }

        return AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 AnimGraphStateTransition::GetVisibilityCanBeInterruptedBy() const
    {
        if (GetVisibilityInterruptionProperties() == AZ::Edit::PropertyVisibility::Show)
        {
            return AZ::Edit::PropertyVisibility::HideChildren;
        }

        return AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 AnimGraphStateTransition::GetVisibilityMaxInterruptionBlendWeight() const
    {
        if (m_interruptionMode == MaxBlendWeight &&
            GetVisibilityInterruptionProperties() == AZ::Edit::PropertyVisibility::Show)
        {
            return AZ::Edit::PropertyVisibility::Show;
        }

        return AZ::Edit::PropertyVisibility::Hide;
    }

    void AnimGraphStateTransition::Reflect(AZ::ReflectContext* context)
    {
        AnimGraphStateTransition::StateFilterLocal::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphStateTransition, AnimGraphObject>()
            ->Version(4)
            ->Field("id", &AnimGraphStateTransition::m_id)
            ->Field("sourceNodeId", &AnimGraphStateTransition::m_sourceNodeId)
            ->Field("targetNodeId", &AnimGraphStateTransition::m_targetNodeId)
            ->Field("isWildcard", &AnimGraphStateTransition::m_isWildcardTransition)
            ->Field("isDisabled", &AnimGraphStateTransition::m_isDisabled)
            ->Field("priority", &AnimGraphStateTransition::m_priority)
            ->Field("canBeInterruptedByOthers", &AnimGraphStateTransition::m_canBeInterruptedByOthers)
            ->Field("canBeInterruptedByTransitionIds", &AnimGraphStateTransition::m_canBeInterruptedByTransitionIds)
            ->Field("interruptionMode", &AnimGraphStateTransition::m_interruptionMode)
            ->Field("maxInterruptionBlendWeight", &AnimGraphStateTransition::m_maxInterruptionBlendWeight)
            ->Field("interruptionBlendBehavior", &AnimGraphStateTransition::m_interruptionBlendBehavior)
            ->Field("canInterruptOtherTransitions", &AnimGraphStateTransition::m_canInterruptOtherTransitions)
            ->Field("allowSelfInterruption", &AnimGraphStateTransition::m_allowSelfInterruption)
            ->Field("allowTransitionsFrom", &AnimGraphStateTransition::m_allowTransitionsFrom)
            ->Field("transitionTime", &AnimGraphStateTransition::m_transitionTime)
            ->Field("syncMode", &AnimGraphStateTransition::m_syncMode)
            ->Field("eventMode", &AnimGraphStateTransition::m_eventMode)
            ->Field("interpolationType", &AnimGraphStateTransition::m_interpolationType)
            ->Field("easeInSmoothness", &AnimGraphStateTransition::m_easeInSmoothness)
            ->Field("easeOutSmoothness", &AnimGraphStateTransition::m_easeOutSmoothness)
            ->Field("startOffsetX", &AnimGraphStateTransition::m_startOffsetX)
            ->Field("startOffsetY", &AnimGraphStateTransition::m_startOffsetY)
            ->Field("endOffsetX", &AnimGraphStateTransition::m_endOffsetX)
            ->Field("endOffsetY", &AnimGraphStateTransition::m_endOffsetY)
            ->Field("conditions", &AnimGraphStateTransition::m_conditions)
            ->Field("actionSetup", &AnimGraphStateTransition::m_actionSetup)
            ->Field("extractionMode", &AnimGraphStateTransition::m_extractionMode)
        ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphStateTransition>("State Transition", "State transition attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphStateTransition::m_isDisabled, "Disabled", "Is disabled? If yes the transition will not be used by the state machine.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphStateTransition::SyncVisualObject)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphStateTransition::m_priority, "Priority", "The priority level of the transition.")
            ->Attribute(AZ::Edit::Attributes::Min, 0)
            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<AZ::s32>::max())
            ->DataElement(AZ_CRC("TransitionStateFilterLocal", 0x7c4000ff), &AnimGraphStateTransition::m_allowTransitionsFrom, "Allow transitions from", "States and groups of states from which the wildcard transition can get activated.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityAllowedStates)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->Attribute(AZ_CRC("StateMachine", 0xe5f2e7a8), &AnimGraphStateTransition::GetStateMachine)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphStateTransition::m_transitionTime, "Transition time", "The transition time, in seconds.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityHideWhenExitOrEntry)
            ->Attribute(AZ::Edit::Attributes::Suffix, " seconds")
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphStateTransition::m_syncMode)
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityHideWhenExitOrEntry)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphStateTransition::m_eventMode)
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityHideWhenExitOrEntry)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphStateTransition::m_extractionMode)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphStateTransition::m_interpolationType, "Interpolation", "The interpolation type to use.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityHideWhenExitOrEntry)
            ->EnumAttribute(INTERPOLATIONFUNCTION_LINEAR, "Linear")
            ->EnumAttribute(INTERPOLATIONFUNCTION_EASECURVE, "Ease curve")
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphStateTransition::m_easeInSmoothness, "Ease-in smoothness", "The smoothness of the ease-in, where 0 means linear and 1 means fully smooth.\nInterpolation type has to be Ease Curve.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetEaseInOutSmoothnessVisibility)
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphStateTransition::m_easeOutSmoothness, "Ease-out smoothness", "The smoothness of the ease-out, where 0 means linear and 1 means fully smooth.\nInterpolation type has to be Ease Curve.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetEaseInOutSmoothnessVisibility)
            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
            ->ClassElement(AZ::Edit::ClassElements::Group, "Interruption")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphStateTransition::m_canBeInterruptedByOthers, "Can be interrupted by others", "Can be interrupted? If enabled the transition can be interrupted by other transitions, while it is already transitioning.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityHideWhenExitOrEntry)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC("AnimGraphStateTransitionIds", 0x7b2468f7), &AnimGraphStateTransition::m_canBeInterruptedByTransitionIds, "Can be interrupted by", "List of candidates that can interrupt the transition.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityCanBeInterruptedBy)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphStateTransition::Reinit)
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphStateTransition::m_interruptionMode, "Interruption mode", "Allow interruption until a given point or always allow it.")
            ->EnumAttribute(AlwaysAllowed, "Always allowed")
            ->EnumAttribute(MaxBlendWeight, "Allowed until blend weight")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityInterruptionProperties)
            ->DataElement(AZ::Edit::UIHandlers::Slider, &AnimGraphStateTransition::m_maxInterruptionBlendWeight, "Max blend weight", "Allow interruption until a given blend weight, e.g. set it to 0.5 in case you want to allow interruption in the first half of the transition.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityMaxInterruptionBlendWeight)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphStateTransition::m_interruptionBlendBehavior, "Interruption behavior", "Continue transitioning to the target state in case of a interruption or stop transitioning at the current position.")
            ->EnumAttribute(Continue, "Continue transitioning")
            ->EnumAttribute(Stop, "Stop transitioning")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityInterruptionProperties)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphStateTransition::m_canInterruptOtherTransitions, "Can interrupt other transitions", "Can interrupt other transitions? If enabled the transition can be activated while another one is already transitioning.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityHideWhenExitOrEntry)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphStateTransition::m_allowSelfInterruption, "Allow self interruption", "Can interrupt itself? If enabled the transition can interrupt and restart itself.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphStateTransition::GetVisibilityHideWhenExitOrEntry)
        ;
    }
} // namespace EMotionFX
