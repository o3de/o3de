/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/sort.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <EMotionFX/Source/BlendSpace1DNode.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendSpaceManager.h>
#include <EMotionFX/Source/BlendSpaceParamEvaluator.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionData/MotionData.h>

namespace
{
    class MotionSortComparer
    {
    public:
        MotionSortComparer(const AZStd::vector<float>& motionCoordinates)
            : m_motionCoordinates(motionCoordinates)
        {
        }
        bool operator()(AZ::u32 a, AZ::u32 b) const
        {
            return m_motionCoordinates[a] < m_motionCoordinates[b];
        }

    private:
        const AZStd::vector<float>& m_motionCoordinates;
    };
}

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpace1DNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpace1DNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendSpace1DNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
        , m_allMotionsHaveSyncTracks(false)
        , m_currentPosition(0)
        , m_leaderMotionIdx(0)
        , m_hasOverlappingCoordinates(false)
    {
    }

    BlendSpace1DNode::UniqueData::~UniqueData()
    {
        BlendSpaceNode::ClearMotionInfos(m_motionInfos);
    }

    float BlendSpace1DNode::UniqueData::GetRangeMin() const
    {
        return m_sortedMotions.empty() ? 0.0f : m_motionCoordinates[m_sortedMotions.front()];
    }

    float BlendSpace1DNode::UniqueData::GetRangeMax() const
    {
        return m_sortedMotions.empty() ? 0.0f : m_motionCoordinates[m_sortedMotions.back()];
    }

    void BlendSpace1DNode::UniqueData::Reset()
    {
        BlendSpaceNode::ClearMotionInfos(m_motionInfos);
        m_currentSegment.m_segmentIndex = MCORE_INVALIDINDEX32;
        m_motionCoordinates.clear();
        m_sortedMotions.clear();

        Invalidate();
    }

    void BlendSpace1DNode::UniqueData::Update()
    {
        BlendSpace1DNode* blendSpaceNode = azdynamic_cast<BlendSpace1DNode*>(m_object);
        AZ_Assert(blendSpaceNode, "Unique data linked to incorrect node type.");

        blendSpaceNode->UpdateMotionInfos(this);
    }

    BlendSpace1DNode::BlendSpace1DNode()
        : BlendSpaceNode()
    {
        InitInputPorts(2);
        SetupInputPortAsNumber("X", INPUTPORT_VALUE, PORTID_INPUT_VALUE);
        SetupInputPortAsNumber("In Place", INPUTPORT_INPLACE, PORTID_INPUT_INPLACE);

        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }

    BlendSpace1DNode::~BlendSpace1DNode()
    {
    }

    void BlendSpace1DNode::Reinit()
    {
        const BlendSpaceManager* blendSpaceManager = GetAnimGraphManager().GetBlendSpaceManager();
        m_evaluator = blendSpaceManager->FindEvaluatorByType(m_evaluatorType);

        for (BlendSpaceMotion& motion : m_motions)
        {
            motion.SetDimension(1);
        }

        AnimGraphNode::Reinit();
    }

    bool BlendSpace1DNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    bool BlendSpace1DNode::GetValidCalculationMethodAndEvaluator() const
    {
        // if the evaluators is null, is in "manual" mode.
        if (m_calculationMethod == ECalculationMethod::MANUAL)
        {
            return true;
        }
        else
        {
            AZ_Assert(m_evaluator, "Expected non-null blend space param evaluator");
            return !m_evaluator->IsNullEvaluator();
        }
    }


    const char* BlendSpace1DNode::GetAxisLabel() const
    {
        if (!m_evaluator || m_evaluator->IsNullEvaluator())
        {
            return "X-Axis";
        }
        return m_evaluator->GetName();
    }

    const char* BlendSpace1DNode::GetPaletteName() const
    {
        return "Blend Space 1D";
    }


    AnimGraphObject::ECategory BlendSpace1DNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }


    void BlendSpace1DNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AZ_PROFILE_SCOPE(Animation, "BlendSpace1DNode::Output");

        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return;
        }

        // If the node is disabled, simply output a bind pose.
        if (m_disabled)
        {
            SetBindPoseAtOutput(animGraphInstance);
            return;
        }

        OutputAllIncomingNodes(animGraphInstance);

        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        outputPose->InitFromBindPose(actorInstance);
        Pose& outputLocalPose = outputPose->GetPose();
        outputLocalPose.Zero();

        const uint32 threadIndex = actorInstance->GetThreadIndex();
        AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(threadIndex)->GetPosePool();

        AnimGraphPose* bindPose = posePool.RequestPose(actorInstance);
        bindPose->InitFromBindPose(actorInstance);
        AnimGraphPose* motionOutPose = posePool.RequestPose(actorInstance);
        Pose& motionOutLocalPose = motionOutPose->GetPose();

        const bool inPlace = GetIsInPlace(animGraphInstance);

        if (uniqueData->m_currentSegment.m_segmentIndex != MCORE_INVALIDINDEX32)
        {
            const AZ::u32 segIndex = uniqueData->m_currentSegment.m_segmentIndex;
            for (int i = 0; i < 2; ++i)
            {
                MotionInstance* motionInstance = uniqueData->m_motionInfos[uniqueData->m_sortedMotions[segIndex + i]].m_motionInstance;
                motionOutPose->InitFromBindPose(actorInstance);
                motionInstance->SetIsInPlace(inPlace);
                motionInstance->GetMotion()->Update(&bindPose->GetPose(), &motionOutLocalPose, motionInstance);

                if (motionInstance->GetMotionExtractionEnabled() && actorInstance->GetMotionExtractionEnabled() && !motionInstance->GetMotion()->GetMotionData()->IsAdditive())
                {
                    motionOutLocalPose.CompensateForMotionExtractionDirect(motionInstance->GetMotion()->GetMotionExtractionFlags());
                }

                const float weight = (i == 0) ? (1 - uniqueData->m_currentSegment.m_weightForSegmentEnd) : uniqueData->m_currentSegment.m_weightForSegmentEnd;
                outputLocalPose.Sum(&motionOutLocalPose, weight);
            }
            outputLocalPose.NormalizeQuaternions();
        }
        else if (!uniqueData->m_motionInfos.empty())
        {
            const AZ::u16 motionIdx = (uniqueData->m_currentPosition < uniqueData->GetRangeMin()) ? uniqueData->m_sortedMotions.front() : uniqueData->m_sortedMotions.back();
            MotionInstance* motionInstance = uniqueData->m_motionInfos[motionIdx].m_motionInstance;
            motionInstance->SetIsInPlace(inPlace);
            motionOutPose->InitFromBindPose(actorInstance);
            motionInstance->GetMotion()->Update(&bindPose->GetPose(), &motionOutLocalPose, motionInstance);

            if (motionInstance->GetMotionExtractionEnabled() && actorInstance->GetMotionExtractionEnabled() && !motionInstance->GetMotion()->GetMotionData()->IsAdditive())
            {
                motionOutLocalPose.CompensateForMotionExtractionDirect(motionInstance->GetMotion()->GetMotionExtractionFlags());
            }

            outputLocalPose.Sum(&motionOutLocalPose, 1.0f);
            outputLocalPose.NormalizeQuaternions();
        }
        else
        {
            SetBindPoseAtOutput(animGraphInstance);
        }

        posePool.FreePose(motionOutPose);
        posePool.FreePose(bindPose);


        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }


    void BlendSpace1DNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        DoTopDownUpdate(animGraphInstance, m_syncMode, uniqueData->m_leaderMotionIdx,
            uniqueData->m_motionInfos, uniqueData->m_allMotionsHaveSyncTracks);

        EMotionFX::BlendTreeConnection* paramConnection = GetInputPort(INPUTPORT_VALUE).m_connection;
        if (paramConnection)
        {
            AnimGraphNode* paramSrcNode = paramConnection->GetSourceNode();
            if (paramSrcNode)
            {
                TopDownUpdateIncomingNode(animGraphInstance, paramSrcNode, timePassedInSeconds);
            }
        }
    }


    void BlendSpace1DNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AZ_PROFILE_SCOPE(Animation, "BlendSpace1DNode::Update");

        if (!m_disabled)
        {
            EMotionFX::BlendTreeConnection* paramConnection = GetInputPort(INPUTPORT_VALUE).m_connection;
            if (paramConnection)
            {
                UpdateIncomingNode(animGraphInstance, paramConnection->GetSourceNode(), timePassedInSeconds);
            }

            UpdateIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_INPLACE), timePassedInSeconds);
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        AZ_Assert(uniqueData, "UniqueData not found for BlendSpace1DNode");
        uniqueData->Clear();

        if (m_disabled)
        {
            return;
        }

        uniqueData->m_currentPosition = GetCurrentSamplePosition(animGraphInstance, *uniqueData);

        // Set the duration and current play time etc to the leader motion index, or otherwise just the first motion in the list if syncing is disabled.
        AZ::u32 motionIndex = (uniqueData->m_leaderMotionIdx != MCORE_INVALIDINDEX32) ? uniqueData->m_leaderMotionIdx : MCORE_INVALIDINDEX32;
        if (m_syncMode == ESyncMode::SYNCMODE_DISABLED || motionIndex == MCORE_INVALIDINDEX32)
        {
            motionIndex = 0;
        }

        UpdateBlendingInfoForCurrentPoint(*uniqueData);

        DoUpdate(timePassedInSeconds, uniqueData->m_blendInfos, m_syncMode, uniqueData->m_leaderMotionIdx, uniqueData->m_motionInfos);

        if (!uniqueData->m_motionInfos.empty())
        {
            MotionInfo& motionInfo = uniqueData->m_motionInfos[motionIndex];
            uniqueData->SetDuration(motionInfo.m_motionInstance ? motionInfo.m_motionInstance->GetDuration() : 0.0f);
            uniqueData->SetCurrentPlayTime(motionInfo.m_currentTime);
            uniqueData->SetSyncTrack(motionInfo.m_syncTrack);
            uniqueData->SetSyncIndex(motionInfo.m_syncIndex);
            uniqueData->SetPreSyncTime(motionInfo.m_preSyncTime);
            uniqueData->SetPlaySpeed(motionInfo.m_playSpeed);
        }
    }


    void BlendSpace1DNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));

        if (m_disabled)
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        EMotionFX::BlendTreeConnection* paramConnection = GetInputPort(INPUTPORT_VALUE).m_connection;
        if (paramConnection)
        {
            PostUpdateIncomingNode(animGraphInstance, paramConnection->GetSourceNode(), timePassedInSeconds);
        }

        if (uniqueData->m_motionInfos.empty())
        {
            RequestRefDatas(animGraphInstance);
            AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
            data->ClearEventBuffer();
            data->ZeroTrajectoryDelta();
            return;
        }

        RequestRefDatas(animGraphInstance);
        AnimGraphRefCountedData* data = uniqueData->GetRefCountedData();
        data->ClearEventBuffer();
        data->ZeroTrajectoryDelta();

        const bool inPlace = GetIsInPlace(animGraphInstance);
        DoPostUpdate(animGraphInstance, uniqueData->m_leaderMotionIdx, uniqueData->m_blendInfos, uniqueData->m_motionInfos, m_eventFilterMode, data, inPlace);
    }

    bool BlendSpace1DNode::UpdateMotionInfos(UniqueData* uniqueData)
    {
        AZ_Assert(uniqueData, "uniqueData is nullptr.");
        if (!uniqueData)
        {
            return false;
        }

        const AnimGraphInstance* animGraphInstance = uniqueData->GetAnimGraphInstance();
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        if (!actorInstance)
        {
            return false;
        }

        ClearMotionInfos(uniqueData->m_motionInfos);

        MotionSet* motionSet = animGraphInstance->GetMotionSet();
        if (!motionSet)
        {
            return false;
        }

        // Initialize motion instance and parameter value arrays.
        const size_t motionCount = m_motions.size();
        AZ_Assert(uniqueData->m_motionInfos.empty(), "This is assumed to have been cleared already");
        uniqueData->m_motionInfos.reserve(motionCount);

        MotionInstancePool& motionInstancePool = GetMotionInstancePool();

        uniqueData->m_leaderMotionIdx = 0;

        PlayBackInfo playInfo;// TODO: Init from attributes
        for (BlendSpaceMotion& blendSpaceMotion : m_motions)
        {
            const AZStd::string& motionId = blendSpaceMotion.GetMotionId();
            Motion* motion = motionSet->RecursiveFindMotionById(motionId);
            if (!motion)
            {
                blendSpaceMotion.SetFlag(BlendSpaceMotion::TypeFlags::InvalidMotion);
                continue;
            }
            blendSpaceMotion.UnsetFlag(BlendSpaceMotion::TypeFlags::InvalidMotion);

            MotionInstance* motionInstance = motionInstancePool.RequestNew(motion, actorInstance);
            motionInstance->InitFromPlayBackInfo(playInfo, true);
            motionInstance->SetRetargetingEnabled(animGraphInstance->GetRetargetingEnabled() && playInfo.m_retarget);
            motionInstance->UnPause();
            motionInstance->SetIsActive(true);
            motionInstance->SetWeight(1.0f, 0.0f);
            AddMotionInfo(uniqueData->m_motionInfos, motionInstance);

            if (motionId == m_syncLeaderMotionId)
            {
                uniqueData->m_leaderMotionIdx = (AZ::u32)uniqueData->m_motionInfos.size() - 1;
            }
        }
        uniqueData->m_allMotionsHaveSyncTracks = DoAllMotionsHaveSyncTracks(uniqueData->m_motionInfos);

        UpdateMotionPositions(*uniqueData);

        SortMotionInstances(*uniqueData);
        uniqueData->m_currentSegment.m_segmentIndex = MCORE_INVALIDINDEX32;

        return true;
    }


    bool BlendSpace1DNode::GetIsInPlace(AnimGraphInstance* animGraphInstance) const
    {
        EMotionFX::BlendTreeConnection* inPlaceConnection = GetInputPort(INPUTPORT_INPLACE).m_connection;
        if (inPlaceConnection)
        {
            return GetInputNumberAsBool(animGraphInstance, INPUTPORT_INPLACE);
        }

        return m_inPlace;
    }


    void BlendSpace1DNode::UpdateMotionPositions(UniqueData& uniqueData)
    {
        // Get the motion parameter evaluator.
        BlendSpaceParamEvaluator* evaluator = nullptr;
        if (m_calculationMethod == ECalculationMethod::AUTO)
        {
            evaluator = m_evaluator;
            if (evaluator && evaluator->IsNullEvaluator())
            {
                // "Null evaluator" is really not an evaluator.
                evaluator = nullptr;
            }
        }

        // the motions in the attributes could not match the ones in the unique data. The attribute could have some invalid motions
        const size_t uniqueDataMotionCount = uniqueData.m_motionInfos.size();

        // Iterate through all motions and calculate their location in the blend space.
        uniqueData.m_motionCoordinates.resize(uniqueDataMotionCount);
        size_t uniqueDataMotionIndex = 0;
        for (const BlendSpaceMotion& motion : m_motions)
        {
            if (motion.TestFlag(BlendSpaceMotion::TypeFlags::InvalidMotion))
            {
                continue;
            }

            // Calculate the position of the motion in the blend space.
            if (motion.IsXCoordinateSetByUser())
            {
                // Did the user set the values manually? If so, use that.
                uniqueData.m_motionCoordinates[uniqueDataMotionIndex] = motion.GetXCoordinate();
            }
            else if (evaluator)
            {
                // Position was not set by user. Use evaluator for automatic computation.
                MotionInstance* motionInstance = uniqueData.m_motionInfos[uniqueDataMotionIndex].m_motionInstance;
                motionInstance->SetIsInPlace(false);
                uniqueData.m_motionCoordinates[uniqueDataMotionIndex] = evaluator->ComputeParamValue(*motionInstance);
            }

            ++uniqueDataMotionIndex;
        }
    }


    void BlendSpace1DNode::SetCurrentPosition(float point)
    {
        m_currentPositionSetInteractively = point;
    }


    void BlendSpace1DNode::ComputeMotionCoordinates(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance, AZ::Vector2& position)
    {
        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        AZ_Assert(uniqueData, "Unique data not found for blend space 1D node '%s'.", GetName());

        const MotionSet* activeMotionSet = animGraphInstance->GetMotionSet();
        if (!activeMotionSet)
        {
            return;
        }

        const size_t motionIndex = FindMotionIndexByMotionId(m_motions, motionId);
        if (motionIndex == MCORE_INVALIDINDEX32)
        {
            AZ_Assert(false, "Can't find blend space motion for motion id '%s'.", motionId.c_str());
            return;
        }

        // Get the motion parameter evaluator.
        BlendSpaceParamEvaluator* evaluator = nullptr;
        if (m_calculationMethod == ECalculationMethod::AUTO)
        {
            evaluator = m_evaluator;
            if (evaluator && evaluator->IsNullEvaluator())
            {
                // "Null evaluator" is really not an evaluator.
                evaluator = nullptr;
            }
        }

        if (!evaluator)
        {
            position = AZ::Vector2::CreateZero();
            return;
        }

        // If the motion is invalid, we dont have anything to update.
        const BlendSpaceMotion& blendSpaceMotion = m_motions[motionIndex];
        if (blendSpaceMotion.TestFlag(BlendSpaceMotion::TypeFlags::InvalidMotion))
        {
            return;
        }

        // Compute the unique data motion index by skipping those motions from the attribute that are invalid
        size_t uniqueDataMotionIndex = 0;
        for (size_t i = 0; i < motionIndex; ++i)
        {
            const BlendSpaceMotion& currentBlendSpaceMotion = m_motions[i];
            if (currentBlendSpaceMotion.TestFlag(BlendSpaceMotion::TypeFlags::InvalidMotion))
            {
                continue;
            }
            else
            {
                ++uniqueDataMotionIndex;
            }
        }

        AZ_Assert(uniqueDataMotionIndex < uniqueData->m_motionInfos.size(), "Invalid amount of motion infos in unique data");
        MotionInstance* motionInstance = uniqueData->m_motionInfos[uniqueDataMotionIndex].m_motionInstance;
        motionInstance->SetIsInPlace(false);
        position.SetX(evaluator->ComputeParamValue(*motionInstance));
        position.SetY(0.0f);
    }


    void BlendSpace1DNode::RestoreMotionCoordinates(BlendSpaceMotion& motion, AnimGraphInstance* animGraphInstance)
    {
        AZ::Vector2 computedMotionCoords;
        ComputeMotionCoordinates(motion.GetMotionId(), animGraphInstance, computedMotionCoords);

        // Reset the motion coordinates in case the user manually set the value and we're in automatic mode.
        if (m_calculationMethod == ECalculationMethod::AUTO)
        {
            motion.SetXCoordinate(computedMotionCoords.GetX());
            motion.MarkXCoordinateSetByUser(false);
        }
    }


    void BlendSpace1DNode::SetMotions(const AZStd::vector<BlendSpaceMotion>& motions)
    {
        m_motions = motions;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    const AZStd::vector<BlendSpaceNode::BlendSpaceMotion>& BlendSpace1DNode::GetMotions() const
    {
        return m_motions;
    }


    void BlendSpace1DNode::SortMotionInstances(UniqueData& uniqueData)
    {
        const AZ::u16 numMotions = aznumeric_caster(uniqueData.m_motionCoordinates.size());
        uniqueData.m_sortedMotions.resize(numMotions);
        for (AZ::u16 i = 0; i < numMotions; ++i)
        {
            uniqueData.m_sortedMotions[i] = i;
        }
        MotionSortComparer comparer(uniqueData.m_motionCoordinates);
        AZStd::sort(uniqueData.m_sortedMotions.begin(), uniqueData.m_sortedMotions.end(), comparer);

        // Detect if we have coordinates overlapping
        uniqueData.m_hasOverlappingCoordinates = false;
        for (AZ::u32 i = 1; i < numMotions; ++i)
        {
            const AZ::u16 motionA = uniqueData.m_sortedMotions[i - 1];
            const AZ::u16 motionB = uniqueData.m_sortedMotions[i];
            if (AZ::IsClose(uniqueData.m_motionCoordinates[motionA],
                    uniqueData.m_motionCoordinates[motionB],
                    0.0001f))
            {
                uniqueData.m_hasOverlappingCoordinates = true;
                break;
            }
        }
    }


    float BlendSpace1DNode::GetCurrentSamplePosition(AnimGraphInstance* animGraphInstance, UniqueData& uniqueData)
    {
        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return 0.0f;
        }

        if (IsInInteractiveMode())
        {
            return m_currentPositionSetInteractively;
        }
        else
        {
            EMotionFX::BlendTreeConnection* paramConnection = GetInputPort(INPUTPORT_VALUE).m_connection;

            if (GetEMotionFX().GetIsInEditorMode())
            {
                // We do require the user to make connections into the value port.
                SetHasError(&uniqueData, (paramConnection == nullptr));
            }

            float samplePoint;
            if (paramConnection)
            {
                samplePoint = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_VALUE);
            }
            else
            {
                // Nothing connected to input port. Just return the middle of the parameter range as a default choice.
                samplePoint = (uniqueData.GetRangeMin() + uniqueData.GetRangeMax()) / 2;
            }

            return samplePoint;
        }
    }


    void BlendSpace1DNode::UpdateBlendingInfoForCurrentPoint(UniqueData& uniqueData)
    {
        uniqueData.m_currentSegment.m_segmentIndex = MCORE_INVALIDINDEX32;
        FindLineSegmentForCurrentPoint(uniqueData);

        uniqueData.m_blendInfos.clear();

        if (uniqueData.m_currentSegment.m_segmentIndex != MCORE_INVALIDINDEX32)
        {
            const AZ::u32 segIndex = uniqueData.m_currentSegment.m_segmentIndex;
            uniqueData.m_blendInfos.resize(2);
            for (int i = 0; i < 2; ++i)
            {
                BlendInfo& blendInfo = uniqueData.m_blendInfos[i];
                blendInfo.m_motionIndex = uniqueData.m_sortedMotions[segIndex + i];
                blendInfo.m_weight = (i == 0) ? (1 - uniqueData.m_currentSegment.m_weightForSegmentEnd) : uniqueData.m_currentSegment.m_weightForSegmentEnd;
            }
        }
        else if (!uniqueData.m_motionInfos.empty())
        {
            uniqueData.m_blendInfos.resize(1);
            BlendInfo& blendInfo = uniqueData.m_blendInfos[0];
            blendInfo.m_motionIndex = (uniqueData.m_currentPosition < uniqueData.GetRangeMin()) ? uniqueData.m_sortedMotions.front() : uniqueData.m_sortedMotions.back();
            blendInfo.m_weight = 1.0f;
        }

        AZStd::sort(uniqueData.m_blendInfos.begin(), uniqueData.m_blendInfos.end());
    }


    bool BlendSpace1DNode::FindLineSegmentForCurrentPoint(UniqueData& uniqueData)
    {
        const AZ::u32 numPoints = (AZ::u32)uniqueData.m_sortedMotions.size();
        if ((numPoints < 2) || (uniqueData.m_currentPosition < uniqueData.GetRangeMin()) || (uniqueData.m_currentPosition > uniqueData.GetRangeMax()))
        {
            uniqueData.m_currentSegment.m_segmentIndex = MCORE_INVALIDINDEX32;
            return false;
        }
        for (AZ::u32 i = 1; i < numPoints; ++i)
        {
            const float segStart = uniqueData.m_motionCoordinates[uniqueData.m_sortedMotions[i - 1]];
            const float segEnd = uniqueData.m_motionCoordinates[uniqueData.m_sortedMotions[i]];
            AZ_Assert(segStart <= segEnd, "The values should have been sorted");
            if ((uniqueData.m_currentPosition >= segStart) && (uniqueData.m_currentPosition <= segEnd))
            {
                uniqueData.m_currentSegment.m_segmentIndex = i - 1;
                const float segLength = segEnd - segStart;
                if (segLength <= 0)
                {
                    uniqueData.m_currentSegment.m_weightForSegmentEnd = 0;
                }
                else
                {
                    uniqueData.m_currentSegment.m_weightForSegmentEnd = (uniqueData.m_currentPosition - segStart) / segLength;
                }
                return true;
            }
        }
        uniqueData.m_currentSegment.m_segmentIndex = MCORE_INVALIDINDEX32;
        return false;
    }


    void BlendSpace1DNode::SetBindPoseAtOutput(AnimGraphInstance* animGraphInstance)
    {
        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return;
        }

        RequestPoses(animGraphInstance);
        AnimGraphPose* outputPose = GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue();
        ActorInstance* actorInstance = animGraphInstance->GetActorInstance();
        outputPose->InitFromBindPose(actorInstance);
    }


    void BlendSpace1DNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return;
        }

        UniqueData* uniqueData = static_cast<BlendSpace1DNode::UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        RewindMotions(uniqueData->m_motionInfos);
    }


    void BlendSpace1DNode::SetCalculationMethod(ECalculationMethod calculationMethod)
    {
        m_calculationMethod = calculationMethod;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    BlendSpaceNode::ECalculationMethod BlendSpace1DNode::GetCalculationMethod() const
    {
        return m_calculationMethod;
    }


    void BlendSpace1DNode::SetSyncLeaderMotionId(const AZStd::string& syncLeaderMotionId)
    {
        m_syncLeaderMotionId = syncLeaderMotionId;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    const AZStd::string& BlendSpace1DNode::GetSyncLeaderMotionId() const
    {
        return m_syncLeaderMotionId;
    }


    void BlendSpace1DNode::SetEvaluatorType(const AZ::TypeId& evaluatorType)
    {
        m_evaluatorType = evaluatorType;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    const AZ::TypeId& BlendSpace1DNode::GetEvaluatorType() const
    {
        return m_evaluatorType;
    }


    BlendSpaceParamEvaluator* BlendSpace1DNode::GetEvaluator() const
    {
        return m_evaluator;
    }


    void BlendSpace1DNode::SetSyncMode(ESyncMode syncMode)
    {
        m_syncMode = syncMode;
    }


    BlendSpaceNode::ESyncMode BlendSpace1DNode::GetSyncMode() const
    {
        return m_syncMode;
    }


    void BlendSpace1DNode::SetEventFilterMode(EBlendSpaceEventMode eventFilterMode)
    {
        m_eventFilterMode = eventFilterMode;
    }


    BlendSpaceNode::EBlendSpaceEventMode BlendSpace1DNode::GetEventFilterMode() const
    {
        return m_eventFilterMode;
    }


    AZ::Crc32 BlendSpace1DNode::GetEvaluatorVisibility() const
    {
        if (m_calculationMethod == ECalculationMethod::MANUAL)
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return AZ::Edit::PropertyVisibility::Show;
    }


    AZ::Crc32 BlendSpace1DNode::GetSyncOptionsVisibility() const
    {
        if (m_syncMode == ESyncMode::SYNCMODE_DISABLED)
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return AZ::Edit::PropertyVisibility::Show;
    }


    bool BlendSpace1DNode::NodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        const unsigned int version = classElement.GetVersion();
        if (version < 2)
        {
            // Developer code and APIs with exclusionary terms will be deprecated as we introduce replacements across this project's related
            // codebases and APIs. Please note, some instances have been retained in the current version to provide backward compatibility
            // for assets/materials created prior to the change. These will be deprecated in the future.
            int index = classElement.FindElement(AZ_CRC_CE("syncMasterMotionId"));
            if (index > 0)
            {
                AZStd::string oldValue;
                AZ::SerializeContext::DataElementNode& dataElementNode = classElement.GetSubElement(index);
                const bool result = dataElementNode.GetData<AZStd::string>(oldValue);
                if (!result)
                {
                    return false;
                }
                classElement.RemoveElement(index);
                classElement.AddElementWithData(context, "syncLeaderMotionId", oldValue);
            }
        }
        return true;
    }

    bool BlendSpace1DNode::AnimGraphInstanceExists(AnimGraphInstance* animGraphInstance)
    {
        AZ_Assert(animGraphInstance, "animGraphInstance is nullptr.");
        return animGraphInstance != nullptr;
    }


    void BlendSpace1DNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendSpace1DNode, BlendSpaceNode>()
            ->Version(2, NodeVersionConverter)
            ->Field("calculationMethod", &BlendSpace1DNode::m_calculationMethod)
            ->Field("evaluatorType", &BlendSpace1DNode::m_evaluatorType)
            ->Field("syncMode", &BlendSpace1DNode::m_syncMode)
            ->Field("syncLeaderMotionId", &BlendSpace1DNode::m_syncLeaderMotionId)
            ->Field("eventFilterMode", &BlendSpace1DNode::m_eventFilterMode)
            ->Field("motions", &BlendSpace1DNode::m_motions)
        ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendSpace1DNode>("Blend Space 1D", "Blend space 1D attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendSpace1DNode::m_calculationMethod, "Calculation method", "Calculation method.")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendSpace1DNode::Reinit)
            ->DataElement(AZ_CRC_CE("BlendSpaceEvaluator"), &BlendSpace1DNode::m_evaluatorType, "Evaluator", "Evaluator for the motions.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendSpace1DNode::GetEvaluatorVisibility)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendSpace1DNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendSpace1DNode::m_syncMode)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC_CE("BlendSpaceMotion"), &BlendSpace1DNode::m_syncLeaderMotionId, "Sync Leader Motion", "The leader motion used for motion synchronization.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendSpace1DNode::GetSyncOptionsVisibility)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendSpace1DNode::Reinit)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendSpace1DNode::m_eventFilterMode)
            ->DataElement(AZ_CRC_CE("BlendSpaceMotionContainer"), &BlendSpace1DNode::m_motions, "Motions", "Source motions for blend space")
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendSpace1DNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
        ;
    }

} // namespace EMotionFX
