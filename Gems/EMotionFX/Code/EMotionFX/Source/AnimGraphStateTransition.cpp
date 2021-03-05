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

            const AZ::u32 numNodes = nodeGroup->GetNumNodes();
            for (AZ::u32 i = 0; i < numNodes; ++i)
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
        , mConditions(AZStd::move(conditions))
        , mSourceNode(source)
        , mTargetNode(target)
        , m_sourceNodeId(source ? source->GetId() : ObjectId::InvalidId)
        , m_targetNodeId(target ? target->GetId() : ObjectId::InvalidId)
        , m_transitionTime(duration)
    {
    }

    AnimGraphStateTransition::~AnimGraphStateTransition()
    {
        RemoveAllConditions(true);
        if (mAnimGraph)
        {
            mAnimGraph->RemoveObject(this);
        }
    }

    void AnimGraphStateTransition::Reinit()
    {
        if (!mAnimGraph)
        {
            mSourceNode = nullptr;
            mTargetNode = nullptr;
            return;
        }

        // Re-link the source node.
        if (GetSourceNodeId().IsValid())
        {
            mSourceNode = mAnimGraph->RecursiveFindNodeById(GetSourceNodeId());
        }

        // Re-link the target node.
        if (GetTargetNodeId().IsValid())
        {
            mTargetNode = mAnimGraph->RecursiveFindNodeById(GetTargetNodeId());
        }

        AnimGraphObject::Reinit();
    }

    void AnimGraphStateTransition::RecursiveReinit()
    {
        Reinit();

        for (AnimGraphTransitionCondition* condition : mConditions)
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

        for (AnimGraphTransitionCondition* condition : mConditions)
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
        const float weight = uniqueData->mBlendWeight;

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

            uniqueData->mTotalSeconds += timePassedInSeconds;
            if (uniqueData->mTotalSeconds >= blendTime)
            {
                uniqueData->mTotalSeconds = blendTime;
                uniqueData->mIsDone = true;
            }
            else
            {
                uniqueData->mIsDone = false;
            }

            // calculate the blend weight
            if (blendTime > MCore::Math::epsilon)
            {
                uniqueData->mBlendProgress = uniqueData->mTotalSeconds / blendTime;
            }
            else
            {
                uniqueData->mBlendProgress = 1.0f;
            }

            uniqueData->mBlendWeight = CalculateWeight(uniqueData->mBlendProgress);
        }
    }

    void AnimGraphStateTransition::ExtractMotion(AnimGraphInstance* animGraphInstance, AnimGraphRefCountedData* sourceData, Transform* outTransform, Transform* outTransformMirrored) const
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        const float weight = uniqueData->mBlendWeight;

        AnimGraphRefCountedData* targetData = mTargetNode->FindOrCreateUniqueNodeData(animGraphInstance)->GetRefCountedData();
        CalculateMotionExtractionDelta(m_extractionMode, sourceData, targetData, weight, true, *outTransform, *outTransformMirrored);
    }

    void AnimGraphStateTransition::OnStartTransition(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));

        uniqueData->mBlendWeight    = 0.0f;
        uniqueData->mIsDone         = false;
        uniqueData->mTotalSeconds   = 0.0f;
        uniqueData->mBlendProgress  = 0.0f;

        mTargetNode->SetSyncIndex(animGraphInstance, MCORE_INVALIDINDEX32);

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
        return uniqueData->mIsDone;
    }

    float AnimGraphStateTransition::GetBlendWeight(AnimGraphInstance* animGraphInstance) const
    {
        // get the unique data and return the is done flag
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        return uniqueData->mBlendWeight;
    }

    void AnimGraphStateTransition::OnEndTransition(AnimGraphInstance* animGraphInstance)
    {
        // get the unique data
        UniqueData* uniqueData      = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        uniqueData->mBlendWeight    = 1.0f;
        uniqueData->mBlendProgress  = 1.0f;
        uniqueData->mIsDone         = true;

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
        mConditions.push_back(condition);
    }

    void AnimGraphStateTransition::InsertCondition(AnimGraphTransitionCondition* condition, size_t index)
    {
        condition->SetTransition(this);
        mConditions.insert(mConditions.begin() + index, condition);
    }

    void AnimGraphStateTransition::ReserveConditions(size_t numConditions)
    {
        mConditions.reserve(numConditions);
    }

    void AnimGraphStateTransition::RemoveCondition(size_t index, bool delFromMem)
    {
        if (delFromMem)
        {
            delete mConditions[index];
        }

        mConditions.erase(mConditions.begin() + index);
    }

    void AnimGraphStateTransition::RemoveAllConditions(bool delFromMem)
    {
        // delete them all from memory
        if (delFromMem)
        {
            for (AnimGraphTransitionCondition* condition : mConditions)
            {
                delete condition;
            }
        }

        mConditions.clear();
    }

    // check if all conditions are tested positive
    bool AnimGraphStateTransition::CheckIfIsReady(AnimGraphInstance* animGraphInstance) const
    {
        if (mConditions.empty())
        {
            return false;
        }

        if (!GetEMotionFX().GetIsInEditorMode())
        {
            // If we are not in editor mode, we can early out for the first failed condition
            for (AnimGraphTransitionCondition* condition : mConditions)
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
            for (AnimGraphTransitionCondition* condition : mConditions)
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
        mIsWildcardTransition = isWildcardTransition;
    }

    void AnimGraphStateTransition::SetSourceNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* sourceNode)
    {
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        uniqueData->mSourceNode = sourceNode;
    }

    // get the source node of the transition
    AnimGraphNode* AnimGraphStateTransition::GetSourceNode(AnimGraphInstance* animGraphInstance) const
    {
        // return the normal source node in case we are not dealing with a wildcard transition
        if (mIsWildcardTransition == false)
        {
            return mSourceNode;
        }

        // wildcard transition special case handling
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        return uniqueData->mSourceNode;
    }

    void AnimGraphStateTransition::SetBlendTime(float blendTime)
    {
        m_transitionTime = blendTime;
    }

    float AnimGraphStateTransition::GetBlendTime(AnimGraphInstance* animGraphInstance) const
    {
        MCORE_UNUSED(animGraphInstance);

        // Use a blend time of zero in case this transition is connected to aan entry or exit state.
        if ((mSourceNode && (azrtti_typeid(mSourceNode) == azrtti_typeid<AnimGraphExitNode>() || azrtti_typeid(mSourceNode) == azrtti_typeid<AnimGraphEntryNode>())) ||
            (mTargetNode && (azrtti_typeid(mTargetNode) == azrtti_typeid<AnimGraphExitNode>() || azrtti_typeid(mTargetNode) == azrtti_typeid<AnimGraphEntryNode>())))
        {
            return 0.0f;
        }

        return m_transitionTime;
    }

    // callback that gets called before a node gets removed
    void AnimGraphStateTransition::OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove)
    {
        for (AnimGraphTransitionCondition* condition : mConditions)
        {
            condition->OnRemoveNode(animGraph, nodeToRemove);
        }
    }

    void AnimGraphStateTransition::ResetConditions(AnimGraphInstance* animGraphInstance)
    {
        for (AnimGraphTransitionCondition* condition : mConditions)
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
            (GetIsWildcardTransition() || transition->GetIsWildcardTransition() || transition->GetSourceNode() == mSourceNode))
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
    void AnimGraphStateTransition::RecursiveCollectObjects(MCore::Array<AnimGraphObject*>& outObjects) const
    {
        for (const AnimGraphTransitionCondition* condition : mConditions)
        {
            condition->RecursiveCollectObjects(outObjects);
        }

        outObjects.Add(const_cast<AnimGraphStateTransition*>(this));
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

        for (AnimGraphTransitionCondition* condition : mConditions)
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
        mSourceNode = node;

        if (mSourceNode)
        {
            m_sourceNodeId = mSourceNode->GetId();
        }
        else
        {
            m_sourceNodeId = AnimGraphNodeId::InvalidId;
        }
    }

    AnimGraphNode* AnimGraphStateTransition::GetSourceNode() const
    {
        AZ_Assert(!mSourceNode || (mSourceNode && mSourceNode->GetId() == GetSourceNodeId()), "Source node not in sync with node id.");
        return mSourceNode;
    }

    void AnimGraphStateTransition::SetTargetNode(AnimGraphNode* node)
    {
        mTargetNode = node;

        if (mTargetNode)
        {
            m_targetNodeId = mTargetNode->GetId();
        }
        else
        {
            m_targetNodeId = AnimGraphNodeId::InvalidId;
        }
    }

    AnimGraphNode* AnimGraphStateTransition::GetTargetNode() const
    {
        AZ_Assert(mTargetNode && mTargetNode->GetId() == GetTargetNodeId(), "Target node not in sync with node id.");
        return mTargetNode;
    }

    void AnimGraphStateTransition::SetVisualOffsets(int32 startX, int32 startY, int32 endX, int32 endY)
    {
        mStartOffsetX   = startX;
        mStartOffsetY   = startY;
        mEndOffsetX     = endX;
        mEndOffsetY     = endY;
    }

    int32 AnimGraphStateTransition::GetVisualStartOffsetX() const
    {
        return mStartOffsetX;
    }

    int32 AnimGraphStateTransition::GetVisualStartOffsetY() const
    {
        return mStartOffsetY;
    }

    int32 AnimGraphStateTransition::GetVisualEndOffsetX() const
    {
        return mEndOffsetX;
    }

    int32 AnimGraphStateTransition::GetVisualEndOffsetY() const
    {
        return mEndOffsetY;
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
            if (m_allowTransitionsFrom.Contains(mAnimGraph, sourceNode->GetId()))
            {
                // In case the given source node is part of the filter (either as individual state or part of a node group), return success.
                return true;
            }
        }

        return false;
    }

    AZ::Outcome<size_t> AnimGraphStateTransition::FindConditionIndex(AnimGraphTransitionCondition* condition) const
    {
        const auto iterator = AZStd::find(mConditions.begin(), mConditions.end(), condition);
        if (iterator == mConditions.end())
        {
            return AZ::Failure();
        }

        return AZ::Success(static_cast<size_t>(AZStd::distance(mConditions.begin(), iterator)));
    }

    AnimGraphStateMachine* AnimGraphStateTransition::GetStateMachine() const
    {
        if (!mTargetNode)
        {
            return nullptr;
        }

        return azdynamic_cast<AnimGraphStateMachine*>(mTargetNode->GetParentNode());
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
        if ((mSourceNode && (azrtti_typeid(mSourceNode) == azrtti_typeid<AnimGraphExitNode>() || azrtti_typeid(mSourceNode) == azrtti_typeid<AnimGraphEntryNode>())) ||
            (mTargetNode && (azrtti_typeid(mTargetNode) == azrtti_typeid<AnimGraphExitNode>() || azrtti_typeid(mTargetNode) == azrtti_typeid<AnimGraphEntryNode>())))
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
            ->Field("isWildcard", &AnimGraphStateTransition::mIsWildcardTransition)
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
            ->Field("startOffsetX", &AnimGraphStateTransition::mStartOffsetX)
            ->Field("startOffsetY", &AnimGraphStateTransition::mStartOffsetY)
            ->Field("endOffsetX", &AnimGraphStateTransition::mEndOffsetX)
            ->Field("endOffsetY", &AnimGraphStateTransition::mEndOffsetY)
            ->Field("conditions", &AnimGraphStateTransition::mConditions)
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
