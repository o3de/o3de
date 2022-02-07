/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ATLAudioObject.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/chrono/clocks.h>

#if !defined(AUDIO_RELEASE)
    #include <AzCore/std/string/conversions.h>
#endif // !AUDIO_RELEASE

#include <MathConversion.h>

#include <AudioInternalInterfaces.h>
#include <SoundCVars.h>
#include <ATLUtils.h>

#include <IRenderer.h>
#include <IRenderAuxGeom.h>

namespace Audio
{
    extern CAudioLogger g_audioLogger;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportStartingTriggerInstance(const TAudioTriggerInstanceID audioTriggerInstanceID, const TAudioControlID audioControlID)
    {
        SATLTriggerInstanceState& rTriggerInstState = m_cTriggers.emplace(audioTriggerInstanceID, SATLTriggerInstanceState()).first->second;
        rTriggerInstState.nTriggerID = audioControlID;
        rTriggerInstState.nFlags |= eATS_STARTING;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportStartedTriggerInstance(
        const TAudioTriggerInstanceID audioTriggerInstanceID,
        void* const pOwnerOverride,
        void* const pUserData,
        void* const pUserDataOwner,
        const TATLEnumFlagsType nFlags)
    {
        TObjectTriggerStates::iterator Iter(m_cTriggers.find(audioTriggerInstanceID));

        if (Iter != m_cTriggers.end())
        {
            SATLTriggerInstanceState& rTriggerInstState = Iter->second;
            if (rTriggerInstState.numPlayingEvents > 0 || rTriggerInstState.numLoadingEvents > 0)
            {
                rTriggerInstState.pOwnerOverride = pOwnerOverride;
                rTriggerInstState.pUserData = pUserData;
                rTriggerInstState.pUserDataOwner = pUserDataOwner;
                rTriggerInstState.nFlags &= ~eATS_STARTING;

                if ((nFlags & eARF_SYNC_FINISHED_CALLBACK) == 0)
                {
                    rTriggerInstState.nFlags |= eATS_CALLBACK_ON_AUDIO_THREAD;
                }
            }
            else
            {
                // All of the events have either finished before we got here or never started.
                // So we report this trigger as finished immediately.
                ReportFinishedTriggerInstance(Iter);
            }
        }
        else
        {
            g_audioLogger.Log(eALT_WARNING, "Reported a started instance %u that couldn't be found on an object %u",
                audioTriggerInstanceID, GetID());
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportStartedEvent(const CATLEvent* const pEvent)
    {
        m_cActiveEvents.insert(pEvent->GetID());
        m_cTriggerImpls.emplace(pEvent->m_nTriggerImplID, SATLTriggerImplState());

        const TObjectTriggerStates::iterator Iter(m_cTriggers.find(pEvent->m_nTriggerInstanceID));

        if (Iter != m_cTriggers.end())
        {
            SATLTriggerInstanceState& rTriggerInstState = Iter->second;

            switch (pEvent->m_audioEventState)
            {
                case eAES_PLAYING:
                {
                    ++(rTriggerInstState.numPlayingEvents);
                    break;
                }
                case eAES_PLAYING_DELAYED:
                {
                    AZ_Assert(rTriggerInstState.numLoadingEvents > 0, "Event state is PLAYING_DELAYED but there are no loading events!");
                    --(rTriggerInstState.numLoadingEvents);
                    ++(rTriggerInstState.numPlayingEvents);
                    break;
                }
                case eAES_LOADING:
                {
                    ++(rTriggerInstState.numLoadingEvents);
                    break;
                }
                case eAES_UNLOADING:
                {
                    // not handled currently
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Unknown event state in ReportStartedEvent (%d)", pEvent->m_audioEventState);
                    break;
                }
            }
        }
        else
        {
            AZ_Assert(false, "ATL Event must exist and was not found in ReportStartedEvent!");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportFinishedEvent(const CATLEvent* const pEvent, const bool bSuccess)
    {
        m_cActiveEvents.erase(pEvent->GetID());

        TObjectTriggerStates::iterator Iter(m_cTriggers.begin());

        if (FindPlace(m_cTriggers, pEvent->m_nTriggerInstanceID, Iter))
        {
            switch (pEvent->m_audioEventState)
            {
                case eAES_PLAYING:
                case eAES_PLAYING_DELAYED: // intentional fall-through
                {
                    SATLTriggerInstanceState& rTriggerInstState = Iter->second;
                    AZ_Assert(rTriggerInstState.numPlayingEvents > 0, "ReportFinishedEvent - Trigger instances being decremented too many times!");

                    if (--(rTriggerInstState.numPlayingEvents) == 0 &&
                        rTriggerInstState.numLoadingEvents == 0 &&
                        (rTriggerInstState.nFlags & eATS_STARTING) == 0)
                    {
                        ReportFinishedTriggerInstance(Iter);
                    }

                    DecrementRefCount();
                    break;
                }
                case eAES_LOADING:
                {
                    if (bSuccess)
                    {
                        ReportPrepUnprepTriggerImpl(pEvent->m_nTriggerImplID, true);
                    }

                    DecrementRefCount();
                    break;
                }
                case eAES_UNLOADING:
                {
                    if (bSuccess)
                    {
                        ReportPrepUnprepTriggerImpl(pEvent->m_nTriggerImplID, false);
                    }

                    DecrementRefCount();
                    break;
                }
                default:
                {
                    AZ_Assert(false, "Unknown event state in ReportFinishedEvent (%d)", pEvent->m_audioEventState);
                    break;
                }
            }
        }
        else
        {
            g_audioLogger.Log(eALT_WARNING, "Reported finished event %u on an inactive trigger %u", pEvent->GetID(), pEvent->m_nTriggerID);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportPrepUnprepTriggerImpl(const TAudioTriggerImplID nTriggerImplID, const bool bPrepared)
    {
        if (bPrepared)
        {
            m_cTriggerImpls[nTriggerImplID].nFlags |= eATS_PREPARED;
        }
        else
        {
            m_cTriggerImpls[nTriggerImplID].nFlags &= ~eATS_PREPARED;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::SetSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID)
    {
        m_cSwitchStates[nSwitchID] = nStateID;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::SetRtpc(const TAudioControlID nRtpcID, const float fValue)
    {
        m_cRtpcs[nRtpcID] = fValue;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::SetEnvironmentAmount(const TAudioEnvironmentID nEnvironmentID, const float fAmount)
    {
        if (fAmount > 0.0f)
        {
            m_cEnvironments[nEnvironmentID] = fAmount;
        }
        else
        {
            m_cEnvironments.erase(nEnvironmentID);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const TObjectTriggerImplStates& CATLAudioObjectBase::GetTriggerImpls() const
    {
        return m_cTriggerImpls;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const TObjectRtpcMap& CATLAudioObjectBase::GetRtpcs() const
    {
        return m_cRtpcs;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const TObjectEnvironmentMap& CATLAudioObjectBase::GetEnvironments() const
    {
        return m_cEnvironments;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ClearRtpcs()
    {
        m_cRtpcs.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ClearEnvironments()
    {
        m_cEnvironments.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLAudioObjectBase::HasActiveEvents() const
    {
        for (const auto& triggerInstance : m_cTriggers)
        {
            if (triggerInstance.second.numPlayingEvents != 0)
            {
                return true;
            }
        }

        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    TObjectTriggerInstanceSet CATLAudioObjectBase::GetTriggerInstancesByOwner(void* const owner) const
    {
        AZ_Assert(owner, "Retrieving a filtered list of trigger instances requires a non-null Owner pointer!");
        TObjectTriggerInstanceSet filteredTriggers;

        for (auto& triggerInstanceState : m_cTriggers)
        {
            if (triggerInstanceState.second.pOwnerOverride == owner)
            {
                filteredTriggers.insert(triggerInstanceState.first);
            }
        }

        return filteredTriggers;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::Update([[maybe_unused]] const float fUpdateIntervalMS, [[maybe_unused]] const SATLWorldPosition& rListenerPosition)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::Clear()
    {
        m_cActiveEvents.clear();

        m_cTriggers.clear();
        m_cTriggerImpls.clear();
        m_cSwitchStates.clear();
        m_cRtpcs.clear();
        m_cEnvironments.clear();

        m_nRefCounter = 0;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::ReportFinishedTriggerInstance(TObjectTriggerStates::iterator& iTriggerEntry)
    {
        SATLTriggerInstanceState& rTriggerInstState = iTriggerEntry->second;
        SAudioRequest oRequest;
        SAudioCallbackManagerRequestData<eACMRT_REPORT_FINISHED_TRIGGER_INSTANCE> oRequestData(rTriggerInstState.nTriggerID);
        oRequest.nFlags = (eARF_PRIORITY_HIGH | eARF_THREAD_SAFE_PUSH | eARF_SYNC_CALLBACK);
        oRequest.nAudioObjectID = GetID();
        oRequest.pData = &oRequestData;
        oRequest.pOwner = rTriggerInstState.pOwnerOverride;
        oRequest.pUserData = rTriggerInstState.pUserData;
        oRequest.pUserDataOwner = rTriggerInstState.pUserDataOwner;

        if ((rTriggerInstState.nFlags & eATS_CALLBACK_ON_AUDIO_THREAD) != 0)
        {
            oRequest.nFlags &= ~eARF_SYNC_CALLBACK;
        }

        AudioSystemThreadSafeRequestBus::Broadcast(&AudioSystemThreadSafeRequestBus::Events::PushRequestThreadSafe, oRequest);

        if ((rTriggerInstState.nFlags & eATS_PREPARED) != 0)
        {
            // if the trigger instance was manually prepared -- keep it
            rTriggerInstState.nFlags &= ~eATS_PLAYING;
        }
        else
        {
            //if the trigger instance wasn't prepared -- kill it
            m_cTriggers.erase(iTriggerEntry);
        }
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::Clear()
    {
        CATLAudioObjectBase::Clear();
        m_oPosition = SATLWorldPosition();
        m_raycastProcessor.Reset();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::Update(const float fUpdateIntervalMS, const SATLWorldPosition& rListenerPosition)
    {
        CATLAudioObjectBase::Update(fUpdateIntervalMS, rListenerPosition);

        if (CanRunRaycasts())
        {
            m_raycastProcessor.Update(fUpdateIntervalMS);
            m_raycastProcessor.Run(rListenerPosition);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::SetPosition(const SATLWorldPosition& oNewPosition)
    {
        m_oPosition = oNewPosition;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::SetVelocityTracking(const bool bTrackingOn)
    {
        if (bTrackingOn)
        {
            m_oPreviousPosition = m_oPosition;
            m_nFlags |= eAOF_TRACK_VELOCITY;
        }
        else
        {
            m_nFlags &= ~eAOF_TRACK_VELOCITY;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::UpdateVelocity(float const fUpdateIntervalMS)
    {
        const AZ::Vector3 cPositionDelta = m_oPosition.GetPositionVec() - m_oPreviousPosition.GetPositionVec();
        const float fCurrentVelocity = (1000.0f * cPositionDelta.GetLength()) / fUpdateIntervalMS; // fCurrentVelocity is given in units per second

        if (AZ::GetAbs(fCurrentVelocity - m_fPreviousVelocity) > Audio::CVars::s_VelocityTrackingThreshold)
        {
            m_fPreviousVelocity = fCurrentVelocity;
            SAudioRequest oRequest;
            SAudioObjectRequestData<eAORT_SET_RTPC_VALUE> oRequestData(ATLInternalControlIDs::ObjectSpeedRtpcID, fCurrentVelocity);

            oRequest.nAudioObjectID = GetID();
            oRequest.nFlags = eARF_THREAD_SAFE_PUSH;
            oRequest.pData = &oRequestData;
            AudioSystemThreadSafeRequestBus::Broadcast(&AudioSystemThreadSafeRequestBus::Events::PushRequestThreadSafe, oRequest);
        }

        m_oPreviousPosition = m_oPosition;
    }


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::SetRaycastCalcType(const EAudioObjectObstructionCalcType calcType)
    {
        m_raycastProcessor.SetType(calcType);
        switch (calcType)
        {
        case eAOOCT_IGNORE:
            AudioRaycastNotificationBus::Handler::BusDisconnect();
            break;
        case eAOOCT_SINGLE_RAY:
            [[fallthrough]];
        case eAOOCT_MULTI_RAY:
            AudioRaycastNotificationBus::Handler::BusConnect(GetID());
            break;
        default:
            break;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::RunRaycasts(const SATLWorldPosition& listenerPos)
    {
        m_raycastProcessor.Run(listenerPos);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLAudioObject::CanRunRaycasts() const
    {
        return Audio::CVars::s_EnableRaycasts      // This is the CVar to enable/disable audio raycasts.
            && Audio::CVars::s_RaycastMinDistance < Audio::CVars::s_RaycastMaxDistance
            && m_raycastProcessor.CanRun();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::GetObstOccData(SATLSoundPropagationData& data) const
    {
        data.fObstruction = m_raycastProcessor.GetObstruction();
        data.fOcclusion = m_raycastProcessor.GetOcclusion();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::OnAudioRaycastResults(const AudioRaycastResult& result)
    {
        // Pull in the results to the raycast processor...
        AZ_Assert(result.m_audioObjectId != INVALID_AUDIO_OBJECT_ID, "Audio Raycast Results - audio object id is invalid!\n");
        AZ_Assert(result.m_rayIndex < s_maxRaysPerObject, "Audio Raycast Results - ray index is out of bounds (index: %zu)!\n", result.m_rayIndex);
        AZ_Assert(result.m_result.size() <= s_maxHitResultsPerRaycast, "Audio Raycast Results - too many hits returned (hits: %zu)!\n", result.m_result.size());

        RaycastInfo& info = m_raycastProcessor.m_rayInfos[result.m_rayIndex];
        if (!info.m_pending)
        {
            // This may mean that an audio object was recycled (reset) and then reused.
            // Need to investigate this further.
            return;
        }

        info.m_pending = false;
        info.m_hits.clear();
        info.m_numHits = 0;

        for (auto& hit : result.m_result)
        {
            if (hit.m_distance > 0.f)
            {
                info.m_hits.push_back(hit);
                info.m_numHits++;
            }
        }

        info.UpdateContribution();
        info.m_cached = true;
        info.m_cacheTimerMs = Audio::CVars::s_RaycastCacheTimeMs;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastInfo::UpdateContribution()
    {
        // This calculates the contribution of a single raycast.  This calculation can be updated
        // as needed to suit a user's needs.  This is provided as a first example.
        // Based on the number of hits reported, add values from the sequence: 1/2 + 1/4 + 1/8 + ...
        float newContribution = 0.f;

        for (AZ::u16 hit = 0; hit < m_numHits; ++hit)
        {
            newContribution += std::pow(2.f, -aznumeric_cast<float>(hit + 1));
        }

        m_contribution = newContribution;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    float RaycastInfo::GetDistanceScaledContribution() const
    {
        // Max extent is the s_RaycastMaxDistance, and use the distance embedded in the raycast request as a percent (inverse).
        // Objects closer to the listener will have greater contribution amounts.
        // Objects farther away will contribute less obstruction/occlusion, but distance attenuation will be the larger contributing factor.
        const float maxDistance = static_cast<float>(Audio::CVars::s_RaycastMaxDistance);
        float clampedDistance = AZ::GetClamp(m_raycastRequest.m_distance, 0.f, maxDistance);
        float distanceScale = 1.f - (clampedDistance / maxDistance);

        // Scale the contribution amount by (inverse) distance.
        return distanceScale * m_contribution;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    float RaycastInfo::GetNearestHitDistance() const
    {
        float minDistance = m_raycastRequest.m_distance;
        for (const auto& hit : m_hits)
        {
            minDistance = AZ::GetMin(minDistance, hit.m_distance);
        }

        return minDistance;
    }


    // static
    bool RaycastProcessor::s_raycastsEnabled = false;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    RaycastProcessor::RaycastProcessor(const TAudioObjectID objectId, const SATLWorldPosition& objectPosition)
        : m_rayInfos(s_maxRaysPerObject, RaycastInfo())
        , m_position(objectPosition)
        , m_obstructionValue(Audio::CVars::s_RaycastSmoothFactor, s_epsilon)
        , m_occlusionValue(Audio::CVars::s_RaycastSmoothFactor, s_epsilon)
        , m_audioObjectId(objectId)
        , m_obstOccType(eAOOCT_IGNORE)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::Update(float elapsedMs)
    {
        if (m_obstOccType == eAOOCT_SINGLE_RAY || m_obstOccType == eAOOCT_MULTI_RAY)
        {
            // First ray is direct-path obstruction value...
            m_obstructionValue.SetNewTarget(m_rayInfos[0].GetDistanceScaledContribution());

            if (m_obstOccType == eAOOCT_MULTI_RAY)
            {
                float occlusion = 0.f;
                for (size_t i = 1; i < s_maxRaysPerObject; ++i)
                {
                    occlusion += m_rayInfos[i].GetDistanceScaledContribution();
                }

                // Average of the occlusion rays' contributions...
                occlusion /= aznumeric_cast<float>(s_maxRaysPerObject - 1);

                m_occlusionValue.SetNewTarget(occlusion);
            }

            // Tick down the cache timers, when expired mark them dirty...
            for (auto& rayInfo : m_rayInfos)
            {
                if (rayInfo.m_cached)
                {
                    rayInfo.m_cacheTimerMs -= elapsedMs;
                    rayInfo.m_cached = (rayInfo.m_cacheTimerMs > 0.f);
                }
            }
        }

        m_obstructionValue.Update(Audio::CVars::s_RaycastSmoothFactor);
        m_occlusionValue.Update(Audio::CVars::s_RaycastSmoothFactor);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::Reset()
    {
        m_obstructionValue.Reset();
        m_occlusionValue.Reset();

        for (auto& rayInfo : m_rayInfos)
        {
            rayInfo.Reset();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::SetType(EAudioObjectObstructionCalcType calcType)
    {
        if (calcType == m_obstOccType)
        {
            // No change to type, no need to reset any data.
            return;
        }

        if (calcType == eAOOCT_IGNORE)
        {
            // Reset the target values when turning off raycasts (set to IGNORE).
            m_obstructionValue.Reset();
            m_occlusionValue.Reset();
        }

        // Otherwise, switching to a new type we can allow the obst/occ values from before to smooth
        // to new targets as they become available.  Hence no reset.

        for (auto& rayInfo : m_rayInfos)
        {
            rayInfo.Reset();
        }

        m_obstOccType = calcType;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool RaycastProcessor::CanRun() const
    {
        return s_raycastsEnabled    // This enable/disable is set via ISystem events.
            && m_obstOccType != eAOOCT_IGNORE;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::Run(const SATLWorldPosition& listenerPosition)
    {
        const AZ::Vector3 listener = listenerPosition.GetPositionVec();
        const AZ::Vector3 source = m_position.GetPositionVec();
        const AZ::Vector3 ray = source - listener;

        const float distance = ray.GetLength();

        // Prevent raycast when individual sources are not within the allowed distance range...
        if (Audio::CVars::s_RaycastMinDistance >= distance || distance >= Audio::CVars::s_RaycastMaxDistance)
        {
            Reset();
            return;
        }

        const AZ::Vector3 up = AZ::Vector3::CreateAxisZ();
        const AZ::Vector3 side = ray.GetNormalized().Cross(up);

        // Spread out the side rays based on the percentage the ray distance is of the maximum distance.
        // The begin of the rays spread by [0.f, 1.f] in the side direction.
        // The end of the rays spread by [1.f, 10.f] in the side direction.
        constexpr float spreadDistanceMinExtent = 1.f;
        constexpr float spreadDistanceMaxExtent = 10.f;
        constexpr float spreadDistanceDelta = spreadDistanceMaxExtent - spreadDistanceMinExtent;

        const float rayDistancePercent = (distance / Audio::CVars::s_RaycastMaxDistance);

        const float spreadDist = spreadDistanceMinExtent + rayDistancePercent * spreadDistanceDelta;

        // Cast ray 0, the direct obstruction ray.
        CastRay(listener, source, 0);

        if (m_obstOccType == eAOOCT_MULTI_RAY)
        {
            // Cast ray 1, an indirect occlusion ray.
            CastRay(listener, source + up, 1);

            // Cast ray 2, an indirect occlusion ray.
            CastRay(listener, source - up, 2);

            // Cast ray 3, an indirect occlusion ray.
            CastRay(listener + side * rayDistancePercent, source + side * spreadDist, 3);

            // Cast ray 4, an indirect occlusion ray.
            CastRay(listener - side * rayDistancePercent, source - side * spreadDist, 4);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::CastRay(const AZ::Vector3& origin, const AZ::Vector3& dest, const AZ::u16 rayIndex)
    {
        AZ_Assert(rayIndex < s_maxRaysPerObject, "RaycastProcessor::CastRay - ray index is out of bounds!\n");

        RaycastInfo& rayInfo = m_rayInfos[rayIndex];
        if (rayInfo.m_pending || rayInfo.m_cached)
        {
            // A raycast is already in flight OR
            // A raycast result was received recently and is still considered valid.
            return;
        }

        rayInfo.m_raycastRequest.m_start = origin;
        rayInfo.m_raycastRequest.m_direction = dest - origin;
        rayInfo.m_raycastRequest.m_distance = rayInfo.m_raycastRequest.m_direction.NormalizeSafeWithLength();
        rayInfo.m_raycastRequest.m_maxResults = s_maxHitResultsPerRaycast;
        rayInfo.m_raycastRequest.m_reportMultipleHits = true;

        // Mark as pending
        rayInfo.m_pending = true;

        AudioRaycastRequest request(rayInfo.m_raycastRequest, m_audioObjectId, rayIndex);
        AudioRaycastRequestBus::Broadcast(&AudioRaycastRequestBus::Events::PushAudioRaycastRequest, request);
    }

    void RaycastProcessor::SetupTestRay(AZ::u16 rayIndex)
    {
        if (rayIndex < s_maxRaysPerObject)
        {
            // Set the pending flag to true, so the results aren't discarded.
            m_rayInfos[rayIndex].m_pending = true;
            // Set the distance in the request structure so it doesn't have the default.
            m_rayInfos[rayIndex].m_raycastRequest.m_distance = (Audio::CVars::s_RaycastMaxDistance / 4.f);
        }
    }



#if !defined(AUDIO_RELEASE)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::CheckBeforeRemoval(const CATLDebugNameStore* const pDebugNameStore)
    {
        // warn if there is activity on an object being cleared
        if (!m_cActiveEvents.empty())
        {
            const char* const sEventString = GetEventIDs("; ").c_str();
            g_audioLogger.Log(eALT_WARNING, "Active events on an object (ID: %u) being released!  #Events: %u   EventIDs: %s", GetID(), m_cActiveEvents.size(), sEventString);
        }

        if (!m_cTriggers.empty())
        {
            const char* const sTriggerString = GetTriggerNames("; ", pDebugNameStore).c_str();
            g_audioLogger.Log(eALT_WARNING, "Active triggers on an object (ID: %u) being released!  #Triggers: %u   TriggerNames: %s", GetID(), m_cTriggers.size(), sTriggerString);
        }
    }


    using TTriggerCountMap = ATLMapLookupType<TAudioControlID, size_t>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string CATLAudioObjectBase::GetTriggerNames(const char* const sSeparator, const CATLDebugNameStore* const pDebugNameStore)
    {
        AZStd::string triggersString;

        TTriggerCountMap cTriggerCounts;

        for (auto& trigger : m_cTriggers)
        {
            ++cTriggerCounts[trigger.second.nTriggerID];
        }

        for (auto& triggerCount : cTriggerCounts)
        {
            const char* const pName = pDebugNameStore->LookupAudioTriggerName(triggerCount.first);

            if (pName)
            {
                const size_t nInstances = triggerCount.second;

                if (nInstances == 1)
                {
                    triggersString = AZStd::string::format("%s%s%s", triggersString.c_str(), pName, sSeparator);
                }
                else
                {
                    triggersString = AZStd::string::format("%s%s(%zu inst.)%s", triggersString.c_str(), pName, nInstances, sSeparator);
                }
            }
        }

        return triggersString;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string CATLAudioObjectBase::GetEventIDs(const char* const sSeparator)
    {
        AZStd::string eventsString;
        for (auto activeEvent : m_cActiveEvents)
        {
            eventsString = AZStd::string::format("%s%llu%s", eventsString.c_str(), activeEvent, sSeparator);
        }

        return eventsString;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const float CATLAudioObjectBase::CStateDebugDrawData::fMinAlpha = 0.5f;
    const float CATLAudioObjectBase::CStateDebugDrawData::fMaxAlpha = 1.0f;
    const int CATLAudioObjectBase::CStateDebugDrawData::nMaxToMinUpdates = 100;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLAudioObjectBase::CStateDebugDrawData::CStateDebugDrawData(const TAudioSwitchStateID nState)
        : nCurrentState(nState)
        , fCurrentAlpha(fMaxAlpha)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLAudioObjectBase::CStateDebugDrawData::~CStateDebugDrawData()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::CStateDebugDrawData::Update(const TAudioSwitchStateID nNewState)
    {
        if ((nNewState == nCurrentState) && (fCurrentAlpha > fMinAlpha))
        {
            fCurrentAlpha -= (fMaxAlpha - fMinAlpha) / nMaxToMinUpdates;
        }
        else if (nNewState != nCurrentState)
        {
            nCurrentState = nNewState;
            fCurrentAlpha = fMaxAlpha;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, const AZ::Vector3& vListenerPos, const CATLDebugNameStore* const pDebugNameStore) const
    {
        m_raycastProcessor.DrawObstructionRays(auxGeom);

        if (!m_cTriggers.empty())
        {
            // Inspect triggers and apply filter (if set)...
            TTriggerCountMap cTriggerCounts;

            auto triggerFilter = static_cast<AZ::CVarFixedString>(Audio::CVars::s_AudioTriggersDebugFilter);
            AZStd::to_lower(triggerFilter.begin(), triggerFilter.end());

            for (auto& trigger : m_cTriggers)
            {
                AZStd::string triggerName(pDebugNameStore->LookupAudioTriggerName(trigger.second.nTriggerID));
                AZStd::to_lower(triggerName.begin(), triggerName.end());

                if (AudioDebugDrawFilter(triggerName, triggerFilter))
                {
                    ++cTriggerCounts[trigger.second.nTriggerID];
                }
            }

            // Early out for this object if all trigger names were filtered out.
            if (cTriggerCounts.empty())
            {
                return;
            }

            const AZ::Vector3 vPos(m_oPosition.GetPositionVec());
            AZ::Vector3 vScreenPos(0.f);

            // ToDo: Update to work with Atom? LYN-3677
            /*{
                float screenProj[3];
                ???->ProjectToScreen(vPos.GetX(), vPos.GetY(), vPos.GetZ(), &screenProj[0], &screenProj[1], &screenProj[2]);

                screenProj[0] *= 0.01f * static_cast<float>(???->GetWidth());
                screenProj[1] *= 0.01f * static_cast<float>(???->GetHeight());
                vScreenPos.Set(screenProj);
            }
            else*/
            {
                vScreenPos.SetZ(-1.0f);
            }

            if ((0.0f <= vScreenPos.GetZ()) && (vScreenPos.GetZ() <= 1.0f))
            {
                const float fDist = vPos.GetDistance(vListenerPos);

                if (CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::DrawObjects))
                {
                    const SAuxGeomRenderFlags nPreviousRenderFlags = auxGeom.GetRenderFlags();
                    SAuxGeomRenderFlags nNewRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
                    nNewRenderFlags.SetCullMode(e_CullModeNone);
                    auxGeom.SetRenderFlags(nNewRenderFlags);
                    const float fRadius = 0.15f;
                    const AZ::Color sphereColor(1.f, 0.1f, 0.1f, 1.f);
                    auxGeom.DrawSphere(AZVec3ToLYVec3(vPos), fRadius, AZColorToLYColorB(sphereColor));
                    auxGeom.SetRenderFlags(nPreviousRenderFlags);
                }

                const float fFontSize = 1.3f;
                const float fLineHeight = 12.0f;

                if (CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::ObjectStates))
                {
                    AZ::Vector3 vSwitchPos(vScreenPos);

                    for (auto& switchState : m_cSwitchStates)
                    {
                        const TAudioControlID nSwitchID = switchState.first;
                        const TAudioSwitchStateID nStateID = switchState.second;

                        const char* const pSwitchName = pDebugNameStore->LookupAudioSwitchName(nSwitchID);
                        const char* const pStateName = pDebugNameStore->LookupAudioSwitchStateName(nSwitchID, nStateID);

                        if (pSwitchName && pStateName)
                        {
                            CStateDebugDrawData& oDrawData = m_cStateDrawInfoMap[nSwitchID];
                            oDrawData.Update(nStateID);
                            const AZ::Color switchTextColor(0.8f, 0.8f, 0.8f, oDrawData.fCurrentAlpha);

                            vSwitchPos -= AZ::Vector3(0.f, fLineHeight, 0.f);
                            auxGeom.Draw2dLabel(
                                vSwitchPos.GetX(),
                                vSwitchPos.GetY(),
                                fFontSize,
                                AZColorToLYColorF(switchTextColor),
                                false,
                                "%s: %s\n",
                                pSwitchName,
                                pStateName);
                        }
                    }
                }

                const AZ::Color brightTextColor(0.9f, 0.9f, 0.9f, 1.f);
                const AZ::Color normalTextColor(0.75f, 0.75f, 0.75f, 1.f);
                const AZ::Color dimmedTextColor(0.5f, 0.5f, 0.5f, 1.f);

                if (CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::ObjectLabels))
                {
                    const TAudioObjectID nObjectID = GetID();
                    auxGeom.Draw2dLabel(
                        vScreenPos.GetX(),
                        vScreenPos.GetY(),
                        fFontSize,
                        AZColorToLYColorF(brightTextColor),
                        false,
                        "%s  ID: %llu  RefCnt: %2zu  Dist: %4.1fm",
                        pDebugNameStore->LookupAudioObjectName(nObjectID),
                        nObjectID,
                        GetRefCount(),
                        fDist);

                    SATLSoundPropagationData obstOccData;
                    GetObstOccData(obstOccData);

                    auxGeom.Draw2dLabel(
                        vScreenPos.GetX(),
                        vScreenPos.GetY() + fLineHeight,
                        fFontSize,
                        AZColorToLYColorF(brightTextColor),
                        false,
                        "Obst: %.3f  Occl: %.3f",
                        obstOccData.fObstruction,
                        obstOccData.fOcclusion
                    );
                }

                if (CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::ObjectTriggers))
                {
                    AZStd::string triggerStringFormatted;

                    for (auto& triggerCount : cTriggerCounts)
                    {
                        const char* const pName = pDebugNameStore->LookupAudioTriggerName(triggerCount.first);
                        if (pName)
                        {
                            const size_t nInstances = triggerCount.second;
                            if (nInstances == 1)
                            {
                                triggerStringFormatted += pName;
                                triggerStringFormatted += "\n";
                            }
                            else
                            {
                                triggerStringFormatted += AZStd::string::format("%s: %zu\n", pName, nInstances);
                            }
                        }
                    }

                    auxGeom.Draw2dLabel(
                        vScreenPos.GetX(),
                        vScreenPos.GetY() + (2.0f * fLineHeight),
                        fFontSize,
                        AZColorToLYColorF(normalTextColor),
                        false,
                        "%s",
                        triggerStringFormatted.c_str());
                }

                if (CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::ObjectRtpcs))
                {
                    AZ::Vector3 vRtpcPos(vScreenPos);

                    for (auto& rtpc : m_cRtpcs)
                    {
                        const float fRtpcValue = rtpc.second;
                        const char* const pRtpcName = pDebugNameStore->LookupAudioRtpcName(rtpc.first);

                        if (pRtpcName)
                        {
                            const float xOffset = 5.f;

                            vRtpcPos -= AZ::Vector3(0.f, fLineHeight, 0.f);     // list grows up
                            auxGeom.Draw2dLabelCustom(
                                vRtpcPos.GetX() - xOffset,
                                vRtpcPos.GetY(),
                                fFontSize,
                                AZColorToLYColorF(normalTextColor),
                                eDrawText_Right,        // right-justified
                                "%s: %2.2f\n",
                                pRtpcName,
                                fRtpcValue);
                        }
                    }
                }

                if (CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::ObjectEnvironments))
                {
                    AZ::Vector3 vEnvPos(vScreenPos);

                    for (auto& environment : m_cEnvironments)
                    {
                        const float fEnvValue = environment.second;
                        const char* const pEnvName = pDebugNameStore->LookupAudioEnvironmentName(environment.first);

                        if (pEnvName)
                        {
                            const float xOffset = 5.f;

                            vEnvPos += AZ::Vector3(0.f, fLineHeight, 0.f);      // list grows down
                            auxGeom.Draw2dLabelCustom(
                                vEnvPos.GetX() - xOffset,
                                vEnvPos.GetY(),
                                fFontSize,
                                AZColorToLYColorF(normalTextColor),
                                eDrawText_Right,        // right-justified
                                "%s: %.2f\n",
                                pEnvName,
                                fEnvValue);
                        }
                    }
                }
            }
        }
    }


    void RaycastProcessor::DrawObstructionRays(IRenderAuxGeom& auxGeom) const
    {
        static const AZ::Color obstructedRayColor(0.8f, 0.08f, 0.f, 1.f);
        static const AZ::Color freeRayColor(0.08f, 0.8f, 0.f, 1.f);
        static const AZ::Color hitSphereColor(1.f, 0.27f, 0.f, 0.8f);
        static const AZ::Color obstructedRayLabelColor(1.f, 0.f, 0.02f, 0.9f);
        static const AZ::Color freeRayLabelColor(0.f, 1.f, 0.02f, 0.9f);

        static const float hitSphereRadius = 0.02f;

        if (!CanRun())
        {
            return;
        }

        const SAuxGeomRenderFlags previousRenderFlags = auxGeom.GetRenderFlags();
        SAuxGeomRenderFlags newRenderFlags(e_Def3DPublicRenderflags | e_AlphaBlended);
        newRenderFlags.SetCullMode(e_CullModeNone);
        auxGeom.SetRenderFlags(newRenderFlags);

        const bool drawRays = CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::DrawRays);
        // ToDo: Update to work with Atom? LYN-3677
        //const bool drawLabels = CVars::s_debugDrawOptions.AreAllFlagsActive(DebugDraw::Options::RayLabels);

        size_t numRays = m_obstOccType == eAOOCT_SINGLE_RAY ? 1 : s_maxRaysPerObject;
        for (size_t rayIndex = 0; rayIndex < numRays; ++rayIndex)
        {
            const RaycastInfo& rayInfo = m_rayInfos[rayIndex];

            const AZ::Vector3 rayEnd = rayInfo.m_raycastRequest.m_start + rayInfo.m_raycastRequest.m_direction * rayInfo.GetNearestHitDistance();

            if (drawRays)
            {
                const bool rayObstructed = (rayInfo.m_numHits > 0);
                const AZ::Color& rayColor = (rayObstructed ? obstructedRayColor : freeRayColor);

                if (rayObstructed)
                {
                    auxGeom.DrawSphere(
                        AZVec3ToLYVec3(rayEnd),
                        hitSphereRadius,
                        AZColorToLYColorB(hitSphereColor)
                    );
                }

                auxGeom.DrawLine(
                    AZVec3ToLYVec3(rayInfo.m_raycastRequest.m_start),
                    AZColorToLYColorB(freeRayColor),
                    AZVec3ToLYVec3(rayEnd),
                    AZColorToLYColorB(rayColor),
                    1.f
                );
            }

            // ToDo: Update to work with Atom? LYN-3677
            /*if (drawLabels)
            {
                float screenProj[3];
                renderer->ProjectToScreen(rayEnd.GetX(), rayEnd.GetY(), rayEnd.GetZ(),
                    &screenProj[0], &screenProj[1], &screenProj[2]);

                screenProj[0] *= 0.01f * aznumeric_cast<float>(renderer->GetWidth());
                screenProj[1] *= 0.01f * aznumeric_cast<float>(renderer->GetHeight());

                AZ::Vector3 screenPos = AZ::Vector3::CreateFromFloat3(screenProj);

                if ((0.f <= screenPos.GetZ()) && (screenPos.GetZ() <= 1.f))
                {
                    float lerpValue = rayInfo.m_contribution;
                    AZ::Color labelColor = freeRayLabelColor.Lerp(obstructedRayLabelColor, lerpValue);

                    auxGeom.Draw2dLabel(
                        screenPos.GetX(),
                        screenPos.GetY() - 12.f,
                        1.6f,
                        AZColorToLYColorF(labelColor),
                        true,
                        (rayIndex == 0) ? "OBST: %.2f" : "OCCL: %.2f",
                        rayInfo.GetDistanceScaledContribution()
                    );
                }
            }*/
        }

        auxGeom.SetRenderFlags(previousRenderFlags);
    }

#endif // !AUDIO_RELEASE

} // namespace Audio
