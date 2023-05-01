/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/BlendSpaceNode.h>
#include <EMotionFX/Source/BlendSpaceManager.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionEventTable.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <MCore/Source/Config.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpaceNode, AnimGraphAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpaceNode::BlendSpaceMotion, AnimGraphAllocator)

    BlendSpaceNode::BlendSpaceMotion::BlendSpaceMotion()
        : m_coordinates(0.0f, 0.0f)
        , m_typeFlags(TypeFlags::None)
    {
    }


    BlendSpaceNode::BlendSpaceMotion::BlendSpaceMotion(const AZStd::string& motionId)
        : BlendSpaceMotion()
    {
        m_motionId = motionId;
    }


    BlendSpaceNode::BlendSpaceMotion::BlendSpaceMotion(const AZStd::string& motionId, const AZ::Vector2& coordinates, TypeFlags typeFlags)
        : m_motionId(motionId)
        , m_coordinates(coordinates)
        , m_typeFlags(typeFlags)
    {
    }


    BlendSpaceNode::BlendSpaceMotion::~BlendSpaceMotion()
    {
    }


    void BlendSpaceNode::BlendSpaceMotion::Set(const AZStd::string& motionId, const AZ::Vector2& coordinates, TypeFlags typeFlags)
    {
        m_motionId = motionId;
        m_coordinates = coordinates;
        m_typeFlags = typeFlags;
    }


    const AZStd::string& BlendSpaceNode::BlendSpaceMotion::GetMotionId() const
    {
        return m_motionId;
    }


    const AZ::Vector2& BlendSpaceNode::BlendSpaceMotion::GetCoordinates() const
    {
        return m_coordinates;
    }


    float BlendSpaceNode::BlendSpaceMotion::GetXCoordinate() const
    {
        return m_coordinates.GetX();
    }


    float BlendSpaceNode::BlendSpaceMotion::GetYCoordinate() const
    {
        return m_coordinates.GetY();
    }


    void BlendSpaceNode::BlendSpaceMotion::SetXCoordinate(float x)
    {
        m_coordinates.SetX(x);
    }


    void BlendSpaceNode::BlendSpaceMotion::SetYCoordinate(float y)
    {
        m_coordinates.SetY(y);
    }


    void BlendSpaceNode::BlendSpaceMotion::MarkXCoordinateSetByUser(bool setByUser)
    {
        if (setByUser)
        {
            SetFlag(TypeFlags::UserSetCoordinateX);
        }
        else
        {
            UnsetFlag(TypeFlags::UserSetCoordinateX);
        }
    }


    void BlendSpaceNode::BlendSpaceMotion::MarkYCoordinateSetByUser(bool setByUser)
    {
        if (setByUser)
        {
            SetFlag(TypeFlags::UserSetCoordinateY);
        }
        else
        {
            UnsetFlag(TypeFlags::UserSetCoordinateY);
        }
    }


    bool BlendSpaceNode::BlendSpaceMotion::IsXCoordinateSetByUser() const
    {
        return TestFlag(TypeFlags::UserSetCoordinateX);
    }


    bool BlendSpaceNode::BlendSpaceMotion::IsYCoordinateSetByUser() const
    {
        return TestFlag(TypeFlags::UserSetCoordinateY);
    }


    int BlendSpaceNode::BlendSpaceMotion::GetDimension() const
    {
        if (TestFlag(TypeFlags::BlendSpace1D))
        {
            return 1;
        }

        if (TestFlag(TypeFlags::BlendSpace2D))
        {
            return 2;
        }

        return 0;
    }


    void BlendSpaceNode::BlendSpaceMotion::SetDimension(int dimension)
    {
        switch (dimension)
        {
        case 1:
            SetFlag(TypeFlags::BlendSpace1D);
            break;
        case 2:
            SetFlag(TypeFlags::BlendSpace2D);
            break;
        default:
            AZ_Assert(false, "Unexpected value for dimension");
        }
    }

    BlendSpaceNode::BlendSpaceMotion::TypeFlags operator~(BlendSpaceNode::BlendSpaceMotion::TypeFlags a) { return static_cast<BlendSpaceNode::BlendSpaceMotion::TypeFlags>(~static_cast<AZ::u8>(a)); }
    BlendSpaceNode::BlendSpaceMotion::TypeFlags operator| (BlendSpaceNode::BlendSpaceMotion::TypeFlags a, BlendSpaceNode::BlendSpaceMotion::TypeFlags b) { return static_cast<BlendSpaceNode::BlendSpaceMotion::TypeFlags>(static_cast<AZ::u8>(a) | static_cast<AZ::u8>(b)); }
    BlendSpaceNode::BlendSpaceMotion::TypeFlags operator& (BlendSpaceNode::BlendSpaceMotion::TypeFlags a, BlendSpaceNode::BlendSpaceMotion::TypeFlags b) { return static_cast<BlendSpaceNode::BlendSpaceMotion::TypeFlags>(static_cast<AZ::u8>(a) & static_cast<AZ::u8>(b)); }
    BlendSpaceNode::BlendSpaceMotion::TypeFlags operator^ (BlendSpaceNode::BlendSpaceMotion::TypeFlags a, BlendSpaceNode::BlendSpaceMotion::TypeFlags b) { return static_cast<BlendSpaceNode::BlendSpaceMotion::TypeFlags>(static_cast<AZ::u8>(a) ^ static_cast<AZ::u8>(b)); }
    BlendSpaceNode::BlendSpaceMotion::TypeFlags& operator|= (BlendSpaceNode::BlendSpaceMotion::TypeFlags& a, BlendSpaceNode::BlendSpaceMotion::TypeFlags b) { (AZ::u8&)(a) |= static_cast<AZ::u8>(b); return a; }
    BlendSpaceNode::BlendSpaceMotion::TypeFlags& operator&= (BlendSpaceNode::BlendSpaceMotion::TypeFlags& a, BlendSpaceNode::BlendSpaceMotion::TypeFlags b) { (AZ::u8&)(a) &= static_cast<AZ::u8>(b); return a; }
    BlendSpaceNode::BlendSpaceMotion::TypeFlags& operator^= (BlendSpaceNode::BlendSpaceMotion::TypeFlags& a, BlendSpaceNode::BlendSpaceMotion::TypeFlags b) { (AZ::u8&)(a) ^= static_cast<AZ::u8>(b); return a; }

    void BlendSpaceNode::BlendSpaceMotion::SetFlag(TypeFlags flag)
    {
        m_typeFlags |= flag;
    }


    void BlendSpaceNode::BlendSpaceMotion::UnsetFlag(TypeFlags flag)
    {
        m_typeFlags &= ~flag;
    }


    bool BlendSpaceNode::BlendSpaceMotion::TestFlag(TypeFlags flag) const
    {
        return (m_typeFlags & flag) == flag;
    }

    //-----------------------------------------------------------------------------------------------------------------

    const char* BlendSpaceNode::s_calculationModeAuto = "Automatically calculate motion coordinates";
    const char* BlendSpaceNode::s_calculationModeManual = "Manually enter motion coordinates";
    const char* BlendSpaceNode::s_eventModeAllActiveMotions = "All Currently Active Motions";
    const char* BlendSpaceNode::s_eventModeMostActiveMotion = "Most Active Motion Only";
    const char* BlendSpaceNode::s_eventModeNone = "None";


    BlendSpaceNode::MotionInfo::MotionInfo()
        : m_motionInstance(nullptr)
        , m_syncIndex(MCore::InvalidIndex)
        , m_playSpeed(1.0f)
        , m_currentTime(0.0f)
        , m_preSyncTime(0.0f)
    {
    }

    BlendSpaceNode::BlendSpaceNode() = default;

    BlendSpaceNode::BlendSpaceNode(AnimGraph* animGraph, const char* name)
        : AnimGraphNode(animGraph, name)
    {
    }


    void BlendSpaceNode::DoUpdate(float timePassedInSeconds, const BlendInfos& blendInfos, ESyncMode syncMode, AZ::u32 leaderIdx, MotionInfos& motionInfos)
    {
        float blendedDuration = 0;
        for (const BlendInfo& blendInfo : blendInfos)
        {
            const MotionInfo& motionInfo = motionInfos[blendInfo.m_motionIndex];
            blendedDuration += blendInfo.m_weight * motionInfo.m_motionInstance->GetDuration();
        }
        if (blendedDuration < MCore::Math::epsilon)
        {
            return;
        }

        const size_t numMotions = motionInfos.size();
        for (size_t i = 0; i < numMotions; ++i)
        {
            MotionInfo& motionInfo = motionInfos[i];
            MotionInstance* motionInstance = motionInfo.m_motionInstance;
            motionInstance->SetFreezeAtLastFrame(!motionInstance->GetIsPlayingForever());
            motionInstance->SetPlaySpeed(motionInfo.m_playSpeed);
            motionInstance->SetRetargetingEnabled(m_retarget && m_animGraph->GetRetargetingEnabled());
            motionInfo.m_preSyncTime = motionInstance->GetCurrentTime();

            // If syncing is enabled, we are going to update the current play time (m_currentTime) of all motions later based
            // on the leader's. Otherwise, we need to update them now itself.
            if ((syncMode == SYNCMODE_DISABLED) || (i == leaderIdx))
            {
                const MotionInstance::PlayStateOut newPlayState = motionInstance->CalcPlayStateAfterUpdate(timePassedInSeconds);
                motionInfo.m_currentTime = newPlayState.m_currentTime;
            }

            motionInstance->SetPause(false);
            motionInfo.m_playSpeed = (i == leaderIdx) ? motionInstance->GetDuration() / blendedDuration : 1.0f;
        }
    }

    void BlendSpaceNode::DoTopDownUpdate(AnimGraphInstance* animGraphInstance, ESyncMode syncMode,
        AZ::u32 leaderIdx, MotionInfos& motionInfos, bool motionsHaveSyncTracks)
    {
        if (motionInfos.empty() || (syncMode == SYNCMODE_DISABLED))
        {
            return;
        }

        SyncMotionToNode(animGraphInstance, syncMode, motionInfos[leaderIdx], this);

        ESyncMode motionSyncMode = syncMode;
        if ((motionSyncMode == SYNCMODE_TRACKBASED) && !motionsHaveSyncTracks)
        {
            motionSyncMode = SYNCMODE_CLIPBASED;
        }

        if (motionSyncMode == SYNCMODE_CLIPBASED)
        {
            DoClipBasedSyncOfMotionsToLeader(leaderIdx, motionInfos);
        }
        else
        {
            DoEventBasedSyncOfMotionsToLeader(leaderIdx, motionInfos);
        }
    }

    void BlendSpaceNode::DoPostUpdate(AnimGraphInstance* animGraphInstance, AZ::u32 leaderIdx, BlendInfos& blendInfos, MotionInfos& motionInfos,
        EBlendSpaceEventMode eventFilterMode, AnimGraphRefCountedData* data, bool inPlace)
    {
        MCORE_UNUSED(animGraphInstance);
        MCORE_UNUSED(leaderIdx);

        const size_t numMotions = motionInfos.size();
        for (size_t i = 0; i < numMotions; ++i)
        {
            MotionInfo& motionInfo = motionInfos[i];
            MotionInstance* motionInstance = motionInfo.m_motionInstance;
            motionInstance->SetIsInPlace(inPlace);

            const size_t blendInfoIndex = GetIndexOfMotionInBlendInfos(blendInfos, i);

            // Default to not adding events which represents BSEVENTMODE_NONE.
            AnimGraphEventBuffer* eventBuffer = nullptr;
            if (blendInfoIndex != InvalidIndex32)
            {
                // Skip emitting events for motions that hardly have any influence.
                const float blendWeight = blendInfos[blendInfoIndex].m_weight;
                if (blendWeight > 0.001f)
                {
                    switch (eventFilterMode)
                    {
                        case EMotionFX::BlendSpaceNode::BSEVENTMODE_ALL_ACTIVE_MOTIONS:
                        {
                            eventBuffer = &data->GetEventBuffer();
                            break;
                        }
                        case EMotionFX::BlendSpaceNode::BSEVENTMODE_MOST_ACTIVE_MOTION:
                        {
                            // Only emit the events for the first motion as the first one has the highest weight and thus is the most active.
                            if (blendInfoIndex == 0)
                            {
                                eventBuffer = &data->GetEventBuffer();
                            }
                            break;
                        }
                        default:
                        {
                            // Nothing to do here as we defaulted to no events.
                            break;
                        }
                    }
                }
            }

            // In case the event buffer is nullptr, we update the time values to stay in sync without emitting events.
            motionInstance->UpdateByTimeValues(motionInfo.m_preSyncTime, motionInfo.m_currentTime, eventBuffer);
        }

        if (eventFilterMode == BSEVENTMODE_NONE)
        {
            data->GetEventBuffer().Clear();
        }
        else
        {
            data->GetEventBuffer().UpdateEmitters(this);
        }

        Transform trajectoryDelta;
        Transform trajectoryDeltaAMirrored;
        if (blendInfos.empty())
        {
            trajectoryDelta.IdentityWithZeroScale();
            trajectoryDeltaAMirrored.IdentityWithZeroScale();
        }
        else
        {
            trajectoryDelta.Zero();
            trajectoryDeltaAMirrored.Zero();
            for (const BlendInfo& blendInfo : blendInfos)
            {
                MotionInstance* motionInstance = motionInfos[blendInfo.m_motionIndex].m_motionInstance;
                Transform instanceDelta = Transform::CreateIdentityWithZeroScale();
                const bool isMirrored = motionInstance->GetMirrorMotion();
                motionInstance->ExtractMotion(instanceDelta);
                trajectoryDelta.Add(instanceDelta, blendInfo.m_weight);

                // extract mirrored version of the current delta
                motionInstance->SetMirrorMotion(!isMirrored);
                motionInstance->ExtractMotion(instanceDelta);
                trajectoryDeltaAMirrored.Add(instanceDelta, blendInfo.m_weight);
                motionInstance->SetMirrorMotion(isMirrored); // restore current mirrored flag
            }
            trajectoryDelta.m_rotation.Normalize();
            trajectoryDeltaAMirrored.m_rotation.Normalize();
        }

        data->SetTrajectoryDelta(trajectoryDelta);
        data->SetTrajectoryDeltaMirrored(trajectoryDeltaAMirrored);
    }

    void BlendSpaceNode::ClearMotionInfos(MotionInfos& motionInfos)
    {
        MotionInstancePool& motionInstancePool = GetMotionInstancePool();

        for (MotionInfo& motionInfo : motionInfos)
        {
            if (motionInfo.m_motionInstance)
            {
                motionInstancePool.Free(motionInfo.m_motionInstance);
            }
        }
        motionInfos.clear();
    }

    void BlendSpaceNode::AddMotionInfo(MotionInfos& motionInfos, MotionInstance* motionInstance)
    {
        AZ_Assert(motionInstance, "Invalid MotionInstance pointer");

        motionInfos.push_back(MotionInfo());
        MotionInfo& motionInfo = motionInfos.back();

        motionInfo.m_motionInstance = motionInstance;
        motionInstance->SetFreezeAtLastFrame(!motionInstance->GetIsPlayingForever());

        MotionEventTable* eventTable = motionInstance->GetMotion()->GetEventTable();
        motionInfo.m_syncTrack = eventTable->GetSyncTrack();

        motionInfo.m_playSpeed = motionInstance->GetPlaySpeed();
    }

    bool BlendSpaceNode::DoAllMotionsHaveSyncTracks(const MotionInfos& motionInfos)
    {
        for (const MotionInfo& motionInfo : motionInfos)
        {
            if (!motionInfo.m_syncTrack || motionInfo.m_syncTrack->GetNumEvents() == 0)
            {
                return false;
            }
        }
        return true;
    }

    void BlendSpaceNode::DoClipBasedSyncOfMotionsToLeader(AZ::u32 leaderIdx, MotionInfos& motionInfos)
    {
        const AZ::u32 numMotionInfos = (AZ::u32)motionInfos.size();
        if (leaderIdx >= numMotionInfos)
        {
            return;
        }
        const MotionInfo& leaderInfo = motionInfos[leaderIdx];
        const float leaderDuration = leaderInfo.m_motionInstance->GetDuration();
        if (leaderDuration < MCore::Math::epsilon)
        {
            return;
        }
        const float normalizedTime = leaderInfo.m_currentTime / leaderDuration;

        for (AZ::u32 motionIdx = 0; motionIdx < numMotionInfos; ++motionIdx)
        {
            if (motionIdx != leaderIdx)
            {
                MotionInfo& info = motionInfos[motionIdx];
                const float duration = info.m_motionInstance->GetDuration();
                info.m_playSpeed = (leaderInfo.m_playSpeed * duration) / leaderDuration;
                info.m_currentTime = normalizedTime * duration;
            }
        }
    }

    void BlendSpaceNode::DoEventBasedSyncOfMotionsToLeader(AZ::u32 leaderIdx, MotionInfos& motionInfos)
    {
        const AZ::u32 numMotionInfos = (AZ::u32)motionInfos.size();
        if (leaderIdx >= numMotionInfos)
        {
            return;
        }
        MotionInfo& srcMotion = motionInfos[leaderIdx];
        const AnimGraphSyncTrack* srcTrack = srcMotion.m_syncTrack;

        const float srcCurrentTime = srcMotion.m_currentTime;
        const bool forward = srcMotion.m_motionInstance->GetPlayMode() != PLAYMODE_BACKWARD;

        size_t srcIndexA;
        size_t srcIndexB;
        if (!srcTrack->FindEventIndices(srcCurrentTime, &srcIndexA, &srcIndexB))
        {
            return;
        }
        const bool srcSyncIndexChanged = srcMotion.m_syncIndex != srcIndexA;
        srcMotion.m_syncIndex = srcIndexA;
        const float srcDuration = srcTrack->CalcSegmentLength(srcIndexA, srcIndexB);

        // calculate the normalized offset inside the segment
        float normalizedOffset;
        if (srcIndexA < srcIndexB) // normal case
        {
            normalizedOffset = (srcDuration > MCore::Math::epsilon) ? (srcCurrentTime - srcTrack->GetEvent(srcIndexA).GetStartTime()) / srcDuration : 0.0f;
        }
        else // looping case
        {
            float timeOffset;
            if (srcCurrentTime > srcTrack->GetEvent(0).GetStartTime())
            {
                timeOffset = srcCurrentTime - srcTrack->GetEvent(srcIndexA).GetStartTime();
            }
            else
            {
                const float srcMotionDuration = srcMotion.m_motionInstance->GetDuration();
                timeOffset = (srcMotionDuration - srcTrack->GetEvent(srcIndexA).GetStartTime()) + srcCurrentTime;
            }

            normalizedOffset = (srcDuration > MCore::Math::epsilon) ? timeOffset / srcDuration : 0.0f;
        }

        for (AZ::u32 motionIdx = 0; motionIdx < numMotionInfos; ++motionIdx)
        {
            if (motionIdx == leaderIdx)
            {
                continue;
            }
            MotionInfo& targetMotion = motionInfos[motionIdx];
            const AnimGraphSyncTrack* targetTrack = targetMotion.m_syncTrack;
            size_t startEventIndex = targetMotion.m_syncIndex;
            if (srcSyncIndexChanged)
            {
                if (forward)
                {
                    startEventIndex++;
                    if (startEventIndex >= targetTrack->GetNumEvents())
                    {
                        startEventIndex = 0;
                    }
                }
                else
                {
                    if (startEventIndex == 0)
                    {
                        startEventIndex = targetTrack->GetNumEvents() - 1;
                    }
                    else
                    {
                        startEventIndex--;
                    }
                }
            }

            // Find the matching indices in the target track.
            size_t targetIndexA;
            size_t targetIndexB;
            if (!targetMotion.m_syncTrack->FindMatchingEvents(startEventIndex, srcTrack->GetEvent(srcIndexA).HashForSyncing(srcMotion.m_motionInstance->GetMirrorMotion()), srcTrack->GetEvent(srcIndexB).HashForSyncing(srcMotion.m_motionInstance->GetMirrorMotion()), &targetIndexA, &targetIndexB, forward, targetMotion.m_motionInstance->GetMirrorMotion()))
            {
                continue;
            }

            targetMotion.m_syncIndex = targetIndexA;

            // calculate the segment lengths
            const float targetDuration = targetTrack->CalcSegmentLength(targetIndexA, targetIndexB);

            // calculate the new time in the motion
            float newTargetTime;
            if (targetIndexA < targetIndexB) // if the second segment is a non-wrapping one, so a regular non-looping case
            {
                newTargetTime = targetTrack->GetEvent(targetIndexA).GetStartTime() + targetDuration * normalizedOffset;
            }
            else // looping case
            {
                // calculate the new play time
                const float unwrappedTime = targetTrack->GetEvent(targetIndexA).GetStartTime() + targetDuration * normalizedOffset;

                // if it is past the motion duration, we need to wrap around
                const float targetMotionDuration = targetMotion.m_motionInstance->GetDuration();
                if (unwrappedTime > targetMotionDuration)
                {
                    // the new wrapped time
                    newTargetTime = MCore::Math::SafeFMod(unwrappedTime, targetMotionDuration);
                }
                else
                {
                    newTargetTime = unwrappedTime;
                }
            }

            targetMotion.m_currentTime = newTargetTime;
            targetMotion.m_playSpeed = (srcDuration > MCore::Math::epsilon) ? targetDuration / srcDuration : 0.0f;
        }
    }


    size_t BlendSpaceNode::FindMotionIndexByMotionId(const AZStd::vector<BlendSpaceMotion>& motions, const AZStd::string& motionId) const
    {
        const size_t motionCount = motions.size();
        for (size_t i = 0; i < motionCount; ++i)
        {
            if (motions[i].GetMotionId() == motionId)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    void BlendSpaceNode::SyncMotionToNode(AnimGraphInstance* animGraphInstance, ESyncMode syncMode, MotionInfo& motionInfo, AnimGraphNode* srcNode)
    {
        MCORE_UNUSED(syncMode);

        motionInfo.m_currentTime = srcNode->GetCurrentPlayTime(animGraphInstance);
        motionInfo.m_playSpeed  = srcNode->GetPlaySpeed(animGraphInstance);
    }


    void BlendSpaceNode::RewindMotions(MotionInfos& motionInfos)
    {
        for (MotionInfo& motionInfo : motionInfos)
        {
            MotionInstance* motionInstance = motionInfo.m_motionInstance;
            if (!motionInstance)
            {
                continue;
            }

            motionInstance->Rewind();

            motionInfo.m_currentTime = motionInstance->GetCurrentTime();
            motionInfo.m_preSyncTime = motionInfo.m_currentTime;
            motionInfo.m_syncIndex = MCore::InvalidIndex;
        }
    }


    void BlendSpaceNode::BlendSpaceMotion::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendSpaceMotion>()
            ->Version(1)
            ->Field("motionId", &BlendSpaceMotion::m_motionId)
            ->Field("coordinates", &BlendSpaceMotion::m_coordinates)
            ->Field("typeFlags", &BlendSpaceMotion::m_typeFlags)
        ;
    }


    void BlendSpaceNode::Reflect(AZ::ReflectContext* context)
    {
        BlendSpaceMotion::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendSpaceNode, AnimGraphNode>()
            ->Version(2)
            ->Field("retarget", &BlendSpaceNode::m_retarget)
            ->Field("inPlace", &BlendSpaceNode::m_inPlace)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Enum<ECalculationMethod>("", "")
            ->Value(s_calculationModeAuto, ECalculationMethod::AUTO)
            ->Value(s_calculationModeManual, ECalculationMethod::MANUAL)
        ;

        editContext->Enum<EBlendSpaceEventMode>("Event Filter Mode", "The event filter mode, which controls which events are passed further up the hierarchy.")
            ->Value(s_eventModeAllActiveMotions, EBlendSpaceEventMode::BSEVENTMODE_ALL_ACTIVE_MOTIONS)
            ->Value(s_eventModeMostActiveMotion, EBlendSpaceEventMode::BSEVENTMODE_MOST_ACTIVE_MOTION)
            ->Value(s_eventModeNone, EBlendSpaceEventMode::BSEVENTMODE_NONE)
        ;

        editContext->Class<BlendSpaceNode>("BlendSpaceNode", "Blend space attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendSpaceNode::m_retarget, "Retarget", "Are the motions allowed to be retargeted?")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendSpaceNode::m_inPlace, "In place", "Is the motion in place? When enabled it will stay at the same spot and motion extractio will not have any impact.")
        ;
    }
} // namespace EMotionFX
