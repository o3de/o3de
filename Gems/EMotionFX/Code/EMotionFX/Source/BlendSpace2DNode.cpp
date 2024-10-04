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
#include <MCore/Source/DelaunayTriangulator.h>
#include <EMotionFX/Source/BlendSpace2DNode.h>
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
    AZ_FORCE_INLINE bool IsDegenerateTriangle(const AZ::Vector2& p0, const AZ::Vector2& p1, const AZ::Vector2& p2)
    {
        const AZ::Vector2 v01(p1 - p0);
        const AZ::Vector2 v02(p2 - p0);
        const float perpProduct = v01.GetX() * v02.GetY() - v01.GetY() * v02.GetX();
        return (::fabsf(perpProduct) < 0.001f);
    }

    AZ_FORCE_INLINE bool IsPointInTriangle(const AZ::Vector2& a, const AZ::Vector2& b,
        const AZ::Vector2& c, const AZ::Vector2& p, float& v, float& w, float epsilon)
    {
        const AZ::Vector2 v0(b - a);
        const AZ::Vector2 v1(c - a);
        const AZ::Vector2 v2(p - a);

        const float dot00 = v0.Dot(v0);
        const float dot01 = v0.Dot(v1);
        const float dot02 = v0.Dot(v2);
        const float dot11 = v1.Dot(v1);
        const float dot12 = v1.Dot(v2);

        const float denom = (dot00 * dot11 - dot01 * dot01);
        if (denom < MCore::Math::epsilon)
        {
            return false;
        }

        // Compute barycentric coordinates.
        const float invDenom = 1 / denom;
        v = (dot11 * dot02 - dot01 * dot12) * invDenom;
        w = (dot00 * dot12 - dot01 * dot02) * invDenom;

        if ((v < 0) && (v > -epsilon))
        {
            v = 0;
        }
        if ((w < 0) && (w > -epsilon))
        {
            w = 0;
        }

        return (v >= 0) && (w >= 0) && (v + w < 1 + epsilon);
    }

    AZ::Vector2 GetClosestPointOnLineSegment(const AZ::Vector2& segStart, const AZ::Vector2& segEnd, const AZ::Vector2& pt, float& u)
    {
        const AZ::Vector2 segVec(segEnd - segStart);
        const AZ::Vector2 vec(pt - segStart);

        const float d1 = segVec.Dot(vec);
        if (d1 <= 0)
        {
            u = 0;
            return segStart;
        }
        const float segLenSqr = segVec.Dot(segVec);
        if (segLenSqr <= d1)
        {
            u = 1.0f;
            return segEnd;
        }
        u = d1 / segLenSqr;
        return segStart + u * segVec;
    }
}

namespace EMotionFX
{
    const float BlendSpace2DNode::s_epsilonForBarycentricCoords = 0.001f;

    AZ_CLASS_ALLOCATOR_IMPL(BlendSpace2DNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpace2DNode::UniqueData, AnimGraphObjectUniqueDataAllocator)

    BlendSpace2DNode::Triangle::Triangle(uint16_t indexA, uint16_t indexB, uint16_t indexC)
    {
        m_vertIndices[0] = indexA;
        m_vertIndices[1] = indexB;
        m_vertIndices[2] = indexC;
    }

    bool BlendSpace2DNode::Triangle::operator==(const Triangle& other) const
    {
        return (m_vertIndices[0] == other.m_vertIndices[0] &&
            m_vertIndices[1] == other.m_vertIndices[1] &&
            m_vertIndices[2] == other.m_vertIndices[2]);
    }

    BlendSpace2DNode::UniqueData::UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance)
        : AnimGraphNodeData(node, animGraphInstance)
        , m_allMotionsHaveSyncTracks(false)
        , m_rangeMin(0, 0)
        , m_rangeMax(0, 0)
        , m_currentPosition(0, 0)
        , m_normCurrentPosition(0, 0)
        , m_leaderMotionIdx(0)
        , m_hasDegenerateTriangles(false)
    {
    }

    BlendSpace2DNode::UniqueData::~UniqueData()
    {
        BlendSpaceNode::ClearMotionInfos(m_motionInfos);
    }

    AZ::Vector2 BlendSpace2DNode::UniqueData::ConvertToNormalizedSpace(const AZ::Vector2& pt) const
    {
        return (pt - m_rangeCenter) * m_normalizationScale;
    }

    void BlendSpace2DNode::UniqueData::Reset()
    {
        BlendSpaceNode::ClearMotionInfos(m_motionInfos);
        m_currentTriangle.m_triangleIndex = MCORE_INVALIDINDEX32;
        m_currentEdge.m_edgeIndex = MCORE_INVALIDINDEX32;
        m_motionCoordinates.clear();
        m_normMotionPositions.clear();
        m_blendInfos.clear();

        Invalidate();
    }

    void BlendSpace2DNode::UniqueData::Update()
    {
        BlendSpace2DNode* blendSpaceNode = azdynamic_cast<BlendSpace2DNode*>(m_object);
        AZ_Assert(blendSpaceNode, "Unique data linked to incorrect node type.");

        blendSpaceNode->UpdateMotionInfos(this);
    }

    BlendSpace2DNode::BlendSpace2DNode()
        : BlendSpaceNode()
    {
        InitInputPorts(3);
        SetupInputPortAsNumber("X", INPUTPORT_XVALUE, PORTID_INPUT_XVALUE);
        SetupInputPortAsNumber("Y", INPUTPORT_YVALUE, PORTID_INPUT_YVALUE);
        SetupInputPortAsNumber("In Place", INPUTPORT_INPLACE, PORTID_INPUT_INPLACE);

        InitOutputPorts(1);
        SetupOutputPortAsPose("Output Pose", OUTPUTPORT_POSE, PORTID_OUTPUT_POSE);
    }

    BlendSpace2DNode::~BlendSpace2DNode()
    {
    }

    void BlendSpace2DNode::Reinit()
    {
        const BlendSpaceManager* blendSpaceManager = GetAnimGraphManager().GetBlendSpaceManager();
        m_evaluatorX = blendSpaceManager->FindEvaluatorByType(m_evaluatorTypeX);
        m_evaluatorY = blendSpaceManager->FindEvaluatorByType(m_evaluatorTypeY);

        for (BlendSpaceMotion& motion : m_motions)
        {
            motion.SetDimension(2);
        }

        AnimGraphNode::Reinit();
    }


    bool BlendSpace2DNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }

    bool BlendSpace2DNode::GetValidCalculationMethodsAndEvaluators() const
    {
        // If both calculation methods are manual, we have valid blend space param evaluators
        if (m_calculationMethodX == ECalculationMethod::MANUAL &&
            m_calculationMethodY == ECalculationMethod::MANUAL)
        {
            return true;
        }
        else
        {
            AZ_Assert(m_calculationMethodX == ECalculationMethod::MANUAL || m_evaluatorX, "Expected non-null blend space param evaluator for X-Axis with auto calculation method");
            AZ_Assert(m_calculationMethodY == ECalculationMethod::MANUAL || m_evaluatorY, "Expected non-null blend space param evaluator for Y-Axis with auto calculation method");

            if ((m_calculationMethodX == ECalculationMethod::AUTO && m_evaluatorX->IsNullEvaluator())
                || (m_calculationMethodY == ECalculationMethod::AUTO && m_evaluatorY->IsNullEvaluator()))
            {
                // If any of the calculation methods is auto and it doesnt have an evaluator, then is invalid
                return false;
            }
            else if (m_evaluatorX == m_evaluatorY)
            {
                // if both evaluators are the same, then it is invalid
                return false;
            }
            else
            {
                return true;
            }
        }
    }

    const char* BlendSpace2DNode::GetAxisLabel(int axisIndex) const
    {
        switch (axisIndex)
        {
        case 0:
        {
            if (!m_evaluatorX || m_evaluatorX->IsNullEvaluator())
            {
                return "X-Axis";
            }

            return m_evaluatorX->GetName();
        }
        case 1:
        {
            if (!m_evaluatorY || m_evaluatorY->IsNullEvaluator())
            {
                return "Y-Axis";
            }

            return m_evaluatorY->GetName();
        }
        default:
        {
            return "Invalid axis index";
        }
        }
    }


    bool BlendSpace2DNode::GetIsInPlace(AnimGraphInstance* animGraphInstance) const
    {
        EMotionFX::BlendTreeConnection* inPlaceConnection = GetInputPort(INPUTPORT_INPLACE).m_connection;
        if (inPlaceConnection)
        {
            return GetInputNumberAsBool(animGraphInstance, INPUTPORT_INPLACE);
        }

        return m_inPlace;
    }

    const char* BlendSpace2DNode::GetPaletteName() const
    {
        return "Blend Space 2D";
    }

    AnimGraphObject::ECategory BlendSpace2DNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_BLENDING;
    }


    void BlendSpace2DNode::Output(AnimGraphInstance* animGraphInstance)
    {
        AZ_PROFILE_SCOPE(Animation, "BlendSpace2DNode::Output");

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

        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_INPLACE));

        const bool inPlace = GetIsInPlace(animGraphInstance);
        if (uniqueData->m_currentTriangle.m_triangleIndex != MCORE_INVALIDINDEX32)
        {
            const Triangle& triangle = uniqueData->m_triangles[uniqueData->m_currentTriangle.m_triangleIndex];
            for (int i = 0; i < 3; ++i)
            {
                MotionInstance* motionInstance = uniqueData->m_motionInfos[triangle.m_vertIndices[i]].m_motionInstance;
                motionInstance->SetIsInPlace(inPlace);
                motionOutPose->InitFromBindPose(actorInstance);
                motionInstance->GetMotion()->Update(&bindPose->GetPose(), &motionOutLocalPose, motionInstance);

                if (motionInstance->GetMotionExtractionEnabled() && actorInstance->GetMotionExtractionEnabled() && !motionInstance->GetMotion()->GetMotionData()->IsAdditive())
                {
                    motionOutLocalPose.CompensateForMotionExtractionDirect(motionInstance->GetMotion()->GetMotionExtractionFlags());
                }

                outputLocalPose.Sum(&motionOutLocalPose, uniqueData->m_currentTriangle.m_weights[i]);
            }
        }
        else if (uniqueData->m_currentEdge.m_edgeIndex != MCORE_INVALIDINDEX32)
        {
            const Edge& edge = uniqueData->m_outerEdges[uniqueData->m_currentEdge.m_edgeIndex];
            for (int i = 0; i < 2; ++i)
            {
                MotionInstance* motionInstance = uniqueData->m_motionInfos[edge.m_vertIndices[i]].m_motionInstance;
                motionInstance->SetIsInPlace(inPlace);
                motionOutPose->InitFromBindPose(actorInstance);
                motionInstance->GetMotion()->Update(&bindPose->GetPose(), &motionOutLocalPose, motionInstance);

                if (motionInstance->GetMotionExtractionEnabled() && actorInstance->GetMotionExtractionEnabled() && !motionInstance->GetMotion()->GetMotionData()->IsAdditive())
                {
                    motionOutLocalPose.CompensateForMotionExtractionDirect(motionInstance->GetMotion()->GetMotionExtractionFlags());
                }

                const float weight = (i == 0) ? (1.0f - uniqueData->m_currentEdge.m_u) : uniqueData->m_currentEdge.m_u;
                outputLocalPose.Sum(&motionOutLocalPose, weight);
            }
        }
        else
        {
            SetBindPoseAtOutput(animGraphInstance);
        }

        outputLocalPose.NormalizeQuaternions();

        posePool.FreePose(motionOutPose);
        posePool.FreePose(bindPose);

        if (GetEMotionFX().GetIsInEditorMode() && GetCanVisualize(animGraphInstance))
        {
            animGraphInstance->GetActorInstance()->DrawSkeleton(outputPose->GetPose(), m_visualizeColor);
        }
    }

    void BlendSpace2DNode::TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return;
        }

        if (m_disabled)
        {
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        DoTopDownUpdate(animGraphInstance, m_syncMode, uniqueData->m_leaderMotionIdx,
            uniqueData->m_motionInfos, uniqueData->m_allMotionsHaveSyncTracks);

        for (int i = 0; i < 2; ++i)
        {
            const uint32 portIdx = (i == 0) ? INPUTPORT_XVALUE : INPUTPORT_YVALUE;
            EMotionFX::BlendTreeConnection* paramConnection = GetInputPort(portIdx).m_connection;
            if (paramConnection)
            {
                AnimGraphNode* paramSrcNode = paramConnection->GetSourceNode();
                if (paramSrcNode)
                {
                    TopDownUpdateIncomingNode(animGraphInstance, paramSrcNode, timePassedInSeconds);
                }
            }
        }
    }

    void BlendSpace2DNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        AZ_PROFILE_SCOPE(Animation, "BlendSpace2DNode::Update");

        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return;
        }

        if (!m_disabled)
        {
            EMotionFX::BlendTreeConnection* param1Connection = GetInputPort(INPUTPORT_XVALUE).m_connection;
            if (param1Connection)
            {
                UpdateIncomingNode(animGraphInstance, param1Connection->GetSourceNode(), timePassedInSeconds);
            }

            EMotionFX::BlendTreeConnection* param2Connection = GetInputPort(INPUTPORT_YVALUE).m_connection;
            if (param2Connection)
            {
                UpdateIncomingNode(animGraphInstance, param2Connection->GetSourceNode(), timePassedInSeconds);
            }

            UpdateIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_INPLACE), timePassedInSeconds);
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        AZ_Assert(uniqueData, "Unique data not found for blend space 2D node '%s'.", GetName());
        uniqueData->Clear();

        if (m_disabled)
        {
            return;
        }

        uniqueData->m_currentPosition = GetCurrentSamplePosition(animGraphInstance, *uniqueData);
        uniqueData->m_normCurrentPosition = uniqueData->ConvertToNormalizedSpace(uniqueData->m_currentPosition);

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
            const MotionInfo& motionInfo = uniqueData->m_motionInfos[motionIndex];
            uniqueData->SetDuration(motionInfo.m_motionInstance ? motionInfo.m_motionInstance->GetDuration() : 0.0f);
            uniqueData->SetCurrentPlayTime(motionInfo.m_currentTime);
            uniqueData->SetSyncTrack(motionInfo.m_syncTrack);
            uniqueData->SetSyncIndex(motionInfo.m_syncIndex);
            uniqueData->SetPreSyncTime(motionInfo.m_preSyncTime);
            uniqueData->SetPlaySpeed(motionInfo.m_playSpeed);
        }
    }

    void BlendSpace2DNode::PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
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

        EMotionFX::BlendTreeConnection* param1Connection = GetInputPort(INPUTPORT_XVALUE).m_connection;
        if (param1Connection)
        {
            PostUpdateIncomingNode(animGraphInstance, param1Connection->GetSourceNode(), timePassedInSeconds);
        }
        EMotionFX::BlendTreeConnection* param2Connection = GetInputPort(INPUTPORT_YVALUE).m_connection;
        if (param2Connection)
        {
            PostUpdateIncomingNode(animGraphInstance, param2Connection->GetSourceNode(), timePassedInSeconds);
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

    bool BlendSpace2DNode::UpdateMotionInfos(UniqueData* uniqueData)
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

        ComputeNormalizationInfo(*uniqueData);
        const size_t numPoints = uniqueData->m_motionCoordinates.size();
        uniqueData->m_normMotionPositions.resize(numPoints);
        for (size_t i = 0; i < numPoints; ++i)
        {
            uniqueData->m_normMotionPositions[i] = uniqueData->ConvertToNormalizedSpace(uniqueData->m_motionCoordinates[i]);
        }
        UpdateTriangulation(*uniqueData);
        uniqueData->m_currentTriangle.m_triangleIndex = MCORE_INVALIDINDEX32;
        uniqueData->m_currentEdge.m_edgeIndex = MCORE_INVALIDINDEX32;

        return true;
    }

    void BlendSpace2DNode::SetCurrentPosition(const AZ::Vector2& point)
    {
        m_currentPositionSetInteractively = point;
    }

    void BlendSpace2DNode::ComputeMotionCoordinates(const AZStd::string& motionId, AnimGraphInstance* animGraphInstance, AZ::Vector2& position)
    {
        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return;
        }

        UniqueData* uniqueData = static_cast<UniqueData*>(FindOrCreateUniqueNodeData(animGraphInstance));
        AZ_Assert(uniqueData, "Unique data not found for blend space 2D node '%s'.", GetName());

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

        // If the motion is invalid, we dont have anything to update.
        const BlendSpaceMotion& blendSpaceMotion = m_motions[motionIndex];
        if (blendSpaceMotion.TestFlag(BlendSpaceMotion::TypeFlags::InvalidMotion))
        {
            return;
        }

        // Compute the unique data motion index by skipping those motions from the attribute that are invalid
        uint32 uniqueDataMotionIndex = 0;
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

        position = AZ::Vector2::CreateZero();

        for (int i = 0; i < 2; ++i)
        {
            const ECalculationMethod calculationMethod = (i == 0) ? m_calculationMethodX : m_calculationMethodY;
            if (calculationMethod == ECalculationMethod::AUTO)
            {
                BlendSpaceParamEvaluator* evaluator = (i == 0) ? m_evaluatorX : m_evaluatorY;
                if (evaluator && !evaluator->IsNullEvaluator())
                {
                    position.SetElement(i, evaluator->ComputeParamValue(*motionInstance));
                }
            }
        }
    }


    void BlendSpace2DNode::RestoreMotionCoordinates(BlendSpaceMotion& motion, AnimGraphInstance* animGraphInstance)
    {
        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return;
        }

        AZ::Vector2 computedMotionCoords;
        ComputeMotionCoordinates(motion.GetMotionId(), animGraphInstance, computedMotionCoords);

        // Reset the motion coordinates in case the user manually set the value and we're in automatic mode.
        if (m_calculationMethodX == ECalculationMethod::AUTO)
        {
            motion.SetXCoordinate(computedMotionCoords.GetX());
            motion.MarkXCoordinateSetByUser(false);
        }

        if (m_calculationMethodY == ECalculationMethod::AUTO)
        {
            motion.SetYCoordinate(computedMotionCoords.GetY());
            motion.MarkYCoordinateSetByUser(false);
        }
    }


    void BlendSpace2DNode::SetMotions(const AZStd::vector<BlendSpaceMotion>& motions)
    {
        m_motions = motions;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    const AZStd::vector<BlendSpaceNode::BlendSpaceMotion>& BlendSpace2DNode::GetMotions() const
    {
        return m_motions;
    }


    void BlendSpace2DNode::SetSyncLeaderMotionId(const AZStd::string& syncLeaderMotionId)
    {
        m_syncLeaderMotionId = syncLeaderMotionId;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    const AZStd::string& BlendSpace2DNode::GetSyncLeaderMotionId() const
    {
        return m_syncLeaderMotionId;
    }


    void BlendSpace2DNode::SetEvaluatorTypeX(const AZ::TypeId& evaluatorType)
    {
        m_evaluatorTypeX = evaluatorType;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    const AZ::TypeId& BlendSpace2DNode::GetEvaluatorTypeX() const
    {
        return m_evaluatorTypeX;
    }


    BlendSpaceParamEvaluator* BlendSpace2DNode::GetEvaluatorX() const
    {
        return m_evaluatorX;
    }


    void BlendSpace2DNode::SetCalculationMethodX(ECalculationMethod calculationMethod)
    {
        m_calculationMethodX = calculationMethod;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    BlendSpaceNode::ECalculationMethod BlendSpace2DNode::GetCalculationMethodX() const
    {
        return m_calculationMethodX;
    }


    void BlendSpace2DNode::SetEvaluatorTypeY(const AZ::TypeId& evaluatorType)
    {
        m_evaluatorTypeY = evaluatorType;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    const AZ::TypeId& BlendSpace2DNode::GetEvaluatorTypeY() const
    {
        return m_evaluatorTypeY;
    }


    BlendSpaceParamEvaluator* BlendSpace2DNode::GetEvaluatorY() const
    {
        return m_evaluatorY;
    }


    void BlendSpace2DNode::SetCalculationMethodY(ECalculationMethod calculationMethod)
    {
        m_calculationMethodY = calculationMethod;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    BlendSpaceNode::ECalculationMethod BlendSpace2DNode::GetCalculationMethodY() const
    {
        return m_calculationMethodY;
    }



    void BlendSpace2DNode::SetSyncMode(ESyncMode syncMode)
    {
        m_syncMode = syncMode;
    }


    BlendSpaceNode::ESyncMode BlendSpace2DNode::GetSyncMode() const
    {
        return m_syncMode;
    }



    void BlendSpace2DNode::SetEventFilterMode(EBlendSpaceEventMode eventFilterMode)
    {
        m_eventFilterMode = eventFilterMode;
    }


    BlendSpaceNode::EBlendSpaceEventMode BlendSpace2DNode::GetEventFilterMode() const
    {
        return m_eventFilterMode;
    }


    void BlendSpace2DNode::UpdateMotionPositions(UniqueData& uniqueData)
    {
        // Get the motion parameter evaluators.
        BlendSpaceParamEvaluator* evaluatorX = nullptr;
        BlendSpaceParamEvaluator* evaluatorY = nullptr;

        if (m_calculationMethodX == ECalculationMethod::AUTO)
        {
            evaluatorX = m_evaluatorX;
            if (evaluatorX && evaluatorX->IsNullEvaluator())
            {
                // "Null evaluator" is really not an evaluator.
                evaluatorX = nullptr;
            }
        }

        if (m_calculationMethodY == ECalculationMethod::AUTO)
        {
            evaluatorY = m_evaluatorY;
            if (evaluatorY && evaluatorY->IsNullEvaluator())
            {
                // "Null evaluator" is really not an evaluator.
                evaluatorY = nullptr;
            }
        }

        // It is possible that the blend setup motions are not matching the ones in the unique data, some of the blend setup motions could be invalid.
        const size_t motionCount = m_motions.size();
        const size_t uniqueDataMotionCount = uniqueData.m_motionInfos.size();

        // Iterate through all motions and calculate their location in the blend space.
        uniqueData.m_motionCoordinates.resize(uniqueDataMotionCount);
        size_t iUniqueDataMotionIndex = 0;
        for (uint32 iAttributeMotionIndex = 0; iAttributeMotionIndex < motionCount; ++iAttributeMotionIndex)
        {
            const BlendSpaceMotion& blendSpaceMotion = m_motions[iAttributeMotionIndex];
            if (blendSpaceMotion.TestFlag(BlendSpaceMotion::TypeFlags::InvalidMotion))
            {
                continue;
            }

            MotionInstance* motionInstance = uniqueData.m_motionInfos[iUniqueDataMotionIndex].m_motionInstance;
            motionInstance->SetIsInPlace(false);
            AZ::Vector2& point = uniqueData.m_motionCoordinates[iUniqueDataMotionIndex];

            // X
            // Did the user set the x coordinate manually? If so, use the shared value from the attribute.
            if (blendSpaceMotion.IsXCoordinateSetByUser() || !evaluatorX)
            {
                point.SetX(blendSpaceMotion.GetXCoordinate());
            }
            else
            {
                // Motion x coordinate was not set by user. Use evaluator for automatic computation.
                const float computedXCoord = evaluatorX->ComputeParamValue(*motionInstance);
                point.SetX(computedXCoord);
            }

            // Y
            // Did the user set the y coordinate manually? If so, use the shared value from the attribute.
            if (blendSpaceMotion.IsYCoordinateSetByUser() || !evaluatorY)
            {
                point.SetY(blendSpaceMotion.GetYCoordinate());
            }
            else
            {
                // Motion y coordinate was not set by user. Use evaluator for automatic computation.
                const float computedYCoord = evaluatorY->ComputeParamValue(*motionInstance);
                point.SetY(computedYCoord);
            }

            ++iUniqueDataMotionIndex;
        }
    }

    void BlendSpace2DNode::ComputeNormalizationInfo(UniqueData& uniqueData)
    {
        float minX, minY, maxX, maxY;

        minX = minY = FLT_MAX;
        maxX = maxY = -FLT_MAX;
        for (const AZ::Vector2& point : uniqueData.m_motionCoordinates)
        {
            if (point.GetX() < minX)
            {
                minX = point.GetX();
            }
            if (point.GetY() < minY)
            {
                minY = point.GetY();
            }
            if (point.GetX() > maxX)
            {
                maxX = point.GetX();
            }
            if (point.GetY() > maxY)
            {
                maxY = point.GetY();
            }
        }
        uniqueData.m_rangeMin.Set(minX, minY);
        uniqueData.m_rangeMax.Set(maxX, maxY);
        uniqueData.m_rangeCenter = (uniqueData.m_rangeMin + uniqueData.m_rangeMax) / 2;

        for (int i = 0; i < 2; ++i)
        {
            float scale;
            if (uniqueData.m_rangeMax(i) <= uniqueData.m_rangeMin(i))
            {
                scale = 1.0f;
            }
            else
            {
                scale = 1.0f / (uniqueData.m_rangeMax(i) - uniqueData.m_rangeMin(i));
            }
            uniqueData.m_normalizationScale.SetElement(i, 2 * scale);// multiplying by 2 because the target range is 2 (-1 to 1)
        }
    }

    void BlendSpace2DNode::UpdateTriangulation(UniqueData& uniqueData)
    {
        if (uniqueData.m_normMotionPositions.empty())
        {
            uniqueData.m_triangles.clear();
            uniqueData.m_outerEdges.clear();
        }
        else
        {
            MCore::DelaunayTriangulator triangulator;

            const MCore::DelaunayTriangulator::Triangles& triangles =
                triangulator.Triangulate(uniqueData.m_normMotionPositions);

            const size_t numTriangles = triangles.size();
            AZ_Assert(numTriangles < UINT16_MAX, "More triangles than our 16 bit indices can handle");

            uniqueData.m_triangles.clear();
            uniqueData.m_triangles.reserve(numTriangles);

            // detect degenerate triangles
            uniqueData.m_hasDegenerateTriangles = false;
            for (size_t i = 0; i < numTriangles; ++i)
            {
                const MCore::DelaunayTriangulator::Triangle& srcTri = triangles[i];
                const AZStd::vector<AZ::Vector2>& normPositions = uniqueData.m_normMotionPositions;

                uniqueData.m_hasDegenerateTriangles |= IsDegenerateTriangle(normPositions[srcTri.VertIndex(0)],
                        normPositions[srcTri.VertIndex(1)],
                        normPositions[srcTri.VertIndex(2)]);

                uniqueData.m_triangles.emplace_back(Triangle(static_cast<uint16_t>(srcTri.VertIndex(0)),
                        static_cast<uint16_t>(srcTri.VertIndex(1)),
                        static_cast<uint16_t>(srcTri.VertIndex(2))));
            }
            DetermineOuterEdges(uniqueData);
        }
        //UpdateTriangleGrid(uniqueData);
    }

    // Determines the outer (i.e., boundary) edges of the triangulated region.
    // To do this, we make use of the fact that the inner edges are shared between
    // two triangles while the outer edges are not shared.
    //
    void BlendSpace2DNode::DetermineOuterEdges(UniqueData& uniqueData)
    {
        uniqueData.m_outerEdges.clear();

        AZStd::unordered_map<Edge, AZ::u32, EdgeHasher> edgeToCountMap;
        for (const Triangle& tri : uniqueData.m_triangles)
        {
            for (int i = 0; i < 3; ++i)
            {
                int j = (i + 1) % 3;
                Edge edge;
                edge.m_vertIndices[0] = tri.m_vertIndices[i];
                edge.m_vertIndices[1] = tri.m_vertIndices[j];
                if (edge.m_vertIndices[0] > edge.m_vertIndices[1])
                {
                    AZStd::swap(edge.m_vertIndices[0], edge.m_vertIndices[1]);
                }
                auto mapIt = edgeToCountMap.find(edge);
                if (mapIt == edgeToCountMap.end())
                {
                    edgeToCountMap[edge] = 1;
                }
                else
                {
                    mapIt->second++;
                }
            }
        }
        for (auto& mapEntry : edgeToCountMap)
        {
            const AZ::u32 shareCount = mapEntry.second;
            AZ_Assert((shareCount == 1) || (shareCount == 2), "Edges should be shared by at most 2 triangles");
            if (shareCount == 1)
            {
                uniqueData.m_outerEdges.push_back(mapEntry.first);
            }
        }
    }

    AZ::Vector2 BlendSpace2DNode::GetCurrentSamplePosition(AnimGraphInstance* animGraphInstance, UniqueData& uniqueData)
    {
        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return AZ::Vector2();
        }

        if (IsInInteractiveMode())
        {
            return m_currentPositionSetInteractively;
        }
        else
        {
            AZ::Vector2 samplePoint;

            EMotionFX::BlendTreeConnection* inputConnectionX = GetInputPort(INPUTPORT_XVALUE).m_connection;
            EMotionFX::BlendTreeConnection* inputConnectionY = GetInputPort(INPUTPORT_YVALUE).m_connection;

            if (GetEMotionFX().GetIsInEditorMode())
            {
                if (inputConnectionX && inputConnectionY)
                {
                    SetHasError(&uniqueData, false);
                }
                else
                {
                    // We do require the user to make connections into the value ports.
                    SetHasError(&uniqueData, true);
                }
            }

            if (inputConnectionX)
            {
                samplePoint.SetX(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_XVALUE));
            }
            else
            {
                // Nothing connected to input port. Just set the middle of the range as a default choice.
                const float value = (uniqueData.m_rangeMin(0) + uniqueData.m_rangeMax(0)) / 2;
                samplePoint.SetX(value);
            }


            if (inputConnectionY)
            {
                samplePoint.SetY(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_YVALUE));
            }
            else
            {
                // Nothing connected to input port. Just set the middle of the range
                // as a default choice.
                const float value = (uniqueData.m_rangeMin(1) + uniqueData.m_rangeMax(1)) / 2;
                samplePoint.SetY(value);
            }

            return samplePoint;
        }
    }

    void BlendSpace2DNode::UpdateBlendingInfoForCurrentPoint(UniqueData& uniqueData)
    {
        uniqueData.m_currentTriangle.m_triangleIndex = MCORE_INVALIDINDEX32;
        uniqueData.m_currentEdge.m_edgeIndex = MCORE_INVALIDINDEX32;

        if (!FindTriangleForCurrentPoint(uniqueData))
        {
            FindOuterEdgeClosestToCurrentPoint(uniqueData);
        }

        uniqueData.m_blendInfos.clear();

        if (uniqueData.m_currentTriangle.m_triangleIndex != MCORE_INVALIDINDEX32)
        {
            const Triangle& triangle = uniqueData.m_triangles[uniqueData.m_currentTriangle.m_triangleIndex];
            uniqueData.m_blendInfos.resize(3);
            for (int i = 0; i < 3; ++i)
            {
                BlendInfo& blendInfo = uniqueData.m_blendInfos[i];
                blendInfo.m_motionIndex = triangle.m_vertIndices[i];
                blendInfo.m_weight = uniqueData.m_currentTriangle.m_weights[i];
            }
        }
        else if (uniqueData.m_currentEdge.m_edgeIndex != MCORE_INVALIDINDEX32)
        {
            const Edge& edge = uniqueData.m_outerEdges[uniqueData.m_currentEdge.m_edgeIndex];
            uniqueData.m_blendInfos.resize(2);
            for (int i = 0; i < 2; ++i)
            {
                BlendInfo& blendInfo = uniqueData.m_blendInfos[i];
                blendInfo.m_motionIndex = edge.m_vertIndices[i];
                blendInfo.m_weight = (i == 0) ? (1 - uniqueData.m_currentEdge.m_u) : uniqueData.m_currentEdge.m_u;
            }
        }

        AZStd::sort(uniqueData.m_blendInfos.begin(), uniqueData.m_blendInfos.end());
    }

    bool BlendSpace2DNode::FindTriangleForCurrentPoint(UniqueData& uniqueData)
    {
        // As of now, we go over all the triangles. We can speed this up by
        // some spatial organization of the triangles.
        const AZ::u32 numTriangles = (AZ::u32)uniqueData.m_triangles.size();
        for (AZ::u32 i = 0; i < numTriangles; ++i)
        {
            float v, w;
            const uint16_t* triVerts = uniqueData.m_triangles[i].m_vertIndices;
            if (IsPointInTriangle(uniqueData.m_normMotionPositions[triVerts[0]],
                    uniqueData.m_normMotionPositions[triVerts[1]], uniqueData.m_normMotionPositions[triVerts[2]],
                    uniqueData.m_normCurrentPosition,
                    v, w, s_epsilonForBarycentricCoords))
            {
                uniqueData.m_currentTriangle.m_triangleIndex = i;
                uniqueData.m_currentTriangle.m_weights[0] = 1 - (v + w);
                if (uniqueData.m_currentTriangle.m_weights[0] < 0)
                {
                    uniqueData.m_currentTriangle.m_weights[0] = 0;
                }
                uniqueData.m_currentTriangle.m_weights[1] = v;
                uniqueData.m_currentTriangle.m_weights[2] = w;
                return true;
            }
        }
        return false;
    }

    bool BlendSpace2DNode::FindOuterEdgeClosestToCurrentPoint(UniqueData& uniqueData)
    {
        float   minDistSqr = FLT_MAX;
        AZ::u32 closestEdgeIdx = MCORE_INVALIDINDEX32;
        float   uOnClosestEdge = 0.0f;

        const AZ::u32 numEdges = (AZ::u32)uniqueData.m_outerEdges.size();
        for (AZ::u32 i = 0; i < numEdges; ++i)
        {
            const Edge& edge = uniqueData.m_outerEdges[i];
            float u;
            AZ::Vector2 pointOnEdge = GetClosestPointOnLineSegment(uniqueData.m_normMotionPositions[edge.m_vertIndices[0]],
                    uniqueData.m_normMotionPositions[edge.m_vertIndices[1]], uniqueData.m_normCurrentPosition, u);
            float distSqr = pointOnEdge.GetDistanceSq(uniqueData.m_normCurrentPosition);
            if (distSqr < minDistSqr)
            {
                minDistSqr = distSqr;
                closestEdgeIdx = i;
                uOnClosestEdge = u;
            }
        }
        if (closestEdgeIdx != MCORE_INVALIDINDEX32)
        {
            uniqueData.m_currentEdge.m_edgeIndex = closestEdgeIdx;
            uniqueData.m_currentEdge.m_u = uOnClosestEdge;
            return true;
        }
        return false;
    }

    void BlendSpace2DNode::SetBindPoseAtOutput(AnimGraphInstance* animGraphInstance)
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


    void BlendSpace2DNode::Rewind(AnimGraphInstance* animGraphInstance)
    {
        if (!AnimGraphInstanceExists(animGraphInstance))
        {
            return;
        }

        UniqueData* uniqueData = static_cast<BlendSpace2DNode::UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));
        RewindMotions(uniqueData->m_motionInfos);
    }


    AZ::Crc32 BlendSpace2DNode::GetEvaluatorXVisibility() const
    {
        if (m_calculationMethodX == ECalculationMethod::MANUAL)
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return AZ::Edit::PropertyVisibility::Show;
    }


    AZ::Crc32 BlendSpace2DNode::GetEvaluatorYVisibility() const
    {
        if (m_calculationMethodY == ECalculationMethod::MANUAL)
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return AZ::Edit::PropertyVisibility::Show;
    }


    AZ::Crc32 BlendSpace2DNode::GetSyncOptionsVisibility() const
    {
        if (m_syncMode == ESyncMode::SYNCMODE_DISABLED)
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return AZ::Edit::PropertyVisibility::Show;
    }

    bool BlendSpace2DNode::NodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
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

    bool BlendSpace2DNode::AnimGraphInstanceExists(AnimGraphInstance* animGraphInstance)
    {
        AZ_Assert(animGraphInstance, "animGraphInstance is nullptr.");
        return animGraphInstance != nullptr;
    }

    void BlendSpace2DNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendSpace2DNode, BlendSpaceNode>()
            ->Version(2, NodeVersionConverter)
            ->Field("calculationMethodX", &BlendSpace2DNode::m_calculationMethodX)
            ->Field("evaluatorTypeX", &BlendSpace2DNode::m_evaluatorTypeX)
            ->Field("calculationMethodY", &BlendSpace2DNode::m_calculationMethodY)
            ->Field("evaluatorTypeY", &BlendSpace2DNode::m_evaluatorTypeY)
            ->Field("syncMode", &BlendSpace2DNode::m_syncMode)
            ->Field("syncLeaderMotionId", &BlendSpace2DNode::m_syncLeaderMotionId)
            ->Field("eventFilterMode", &BlendSpace2DNode::m_eventFilterMode)
            ->Field("motions", &BlendSpace2DNode::m_motions)
        ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendSpace2DNode>("Blend Space 1D", "Blend space 1D attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendSpace2DNode::m_calculationMethodX, "Calculation method (X-Axis)", "Calculation method for the X Axis")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendSpace2DNode::Reinit)
            ->DataElement(AZ_CRC_CE("BlendSpaceEvaluator"), &BlendSpace2DNode::m_evaluatorTypeX, "X-Axis Evaluator", "Evaluator for the X axis value of motions")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendSpace2DNode::GetEvaluatorXVisibility)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendSpace2DNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendSpace2DNode::m_calculationMethodY, "Calculation method (Y-Axis)", "Calculation method for the Y Axis")
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendSpace2DNode::Reinit)
            ->DataElement(AZ_CRC_CE("BlendSpaceEvaluator"), &BlendSpace2DNode::m_evaluatorTypeY, "Y-Axis Evaluator", "Evaluator for the Y axis value of motions")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendSpace2DNode::GetEvaluatorYVisibility)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendSpace2DNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendSpace2DNode::m_syncMode)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ_CRC_CE("BlendSpaceMotion"), &BlendSpace2DNode::m_syncLeaderMotionId, "Sync Leader Motion", "The leader motion used for motion synchronization.")
            ->Attribute(AZ::Edit::Attributes::Visibility, &BlendSpace2DNode::GetSyncOptionsVisibility)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendSpace2DNode::Reinit)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendSpace2DNode::m_eventFilterMode)
            ->DataElement(AZ_CRC_CE("BlendSpaceMotionContainer"), &BlendSpace2DNode::m_motions, "Motions", "Source motions for blend space")
            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendSpace2DNode::Reinit)
            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::HideChildren)
        ;
    }

} // namespace EMotionFX
