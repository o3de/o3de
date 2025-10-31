/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ATLAudioObject.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/std/chrono/chrono.h>

#include <MathConversion.h>

#include <SoundCVars.h>
#include <ATLUtils.h>

#if !defined(AUDIO_RELEASE)
    // Debug Draw
    #include <AzCore/std/string/conversions.h>
    #include <AzFramework/Entity/EntityDebugDisplayBus.h>
    #include <AzFramework/Viewport/ViewportBus.h>
    #include <AzFramework/Viewport/ViewportScreen.h>
    #include <Atom/RPI.Public/View.h>
    #include <Atom/RPI.Public/ViewportContext.h>
    #include <Atom/RPI.Public/ViewportContextBus.h>
    #include <Atom/RPI.Public/WindowContext.h>
#endif // !AUDIO_RELEASE


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::TriggerInstanceStarting(TAudioTriggerInstanceID triggerInstanceId, TAudioControlID audioControlId)
    {
        SATLTriggerInstanceState& triggerInstState = m_cTriggers.emplace(triggerInstanceId, SATLTriggerInstanceState()).first->second;
        triggerInstState.nTriggerID = audioControlId;
        triggerInstState.nFlags |= eATS_STARTING;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::TriggerInstanceStarted(TAudioTriggerInstanceID triggerInstanceId, void* owner)
    {
        auto iter = m_cTriggers.find(triggerInstanceId);
        if (iter != m_cTriggers.end())
        {
            SATLTriggerInstanceState& instState = iter->second;
            if (instState.numPlayingEvents > 0)
            {
                instState.nFlags &= ~eATS_STARTING;
                instState.pOwner = owner;

                if (instState.pOwner)
                {
                    AudioTriggerNotificationBus::QueueEvent(
                        TriggerNotificationIdType{ instState.pOwner },
                        &AudioTriggerNotificationBus::Events::ReportTriggerStarted,
                        iter->second.nTriggerID);
                }
            }
            else
            {
                TriggerInstanceFinished(iter);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::TriggerInstanceFinished(TObjectTriggerStates::const_iterator iter)
    {
        if (iter->second.pOwner)
        {
            AudioTriggerNotificationBus::QueueEvent(
                TriggerNotificationIdType{ iter->second.pOwner },
                &AudioTriggerNotificationBus::Events::ReportTriggerFinished,
                iter->second.nTriggerID);
        }
        m_cTriggers.erase(iter);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::EventStarted(const CATLEvent* const atlEvent)
    {
        m_cActiveEvents.insert(atlEvent->GetID());
        m_cTriggerImpls.emplace(atlEvent->m_nTriggerImplID, SATLTriggerImplState());

        auto iter = m_cTriggers.find(atlEvent->m_nTriggerInstanceID);
        if (iter != m_cTriggers.end())
        {
            if (atlEvent->m_audioEventState == eAES_PLAYING)
            {
                ++(iter->second.numPlayingEvents);
            }

            IncrementRefCount();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObjectBase::EventFinished(const CATLEvent* const atlEvent)
    {
        m_cActiveEvents.erase(atlEvent->GetID());
        auto iter = m_cTriggers.find(atlEvent->m_nTriggerInstanceID);
        if (iter != m_cTriggers.end())
        {
            SATLTriggerInstanceState& instState = iter->second;
            AZ_Assert(instState.numPlayingEvents > 0, "EventFinished - Trigger instances being decremented too many times!");

            if (--instState.numPlayingEvents == 0
                && (instState.nFlags & eATS_STARTING) == 0)
            {
                TriggerInstanceFinished(iter);
            }

            DecrementRefCount();
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
            if (triggerInstanceState.second.pOwner == owner)
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

            Audio::ObjectRequest::SetParameterValue setParameter;
            setParameter.m_audioObjectId = GetID();
            setParameter.m_parameterId = ATLInternalControlIDs::ObjectSpeedRtpcID;
            setParameter.m_value = fCurrentVelocity;
            AZ::Interface<IAudioSystem>::Get()->PushRequest(AZStd::move(setParameter));
        }

        m_oPreviousPosition = m_oPosition;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::SetRaycastCalcType(const ObstructionType calcType)
    {
        m_raycastProcessor.SetType(calcType);
        switch (calcType)
        {
        case ObstructionType::Ignore:
            AudioRaycastNotificationBus::Handler::BusDisconnect();
            break;
        case ObstructionType::SingleRay:
            [[fallthrough]];
        case ObstructionType::MultiRay:
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
        , m_obstOccType(ObstructionType::Ignore)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::Update(float elapsedMs)
    {
        if (m_obstOccType == ObstructionType::SingleRay || m_obstOccType == ObstructionType::MultiRay)
        {
            // First ray is direct-path obstruction value...
            m_obstructionValue.SetNewTarget(m_rayInfos[0].GetDistanceScaledContribution());

            if (m_obstOccType == ObstructionType::MultiRay)
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
    void RaycastProcessor::SetType(ObstructionType calcType)
    {
        if (calcType == m_obstOccType)
        {
            // No change to type, no need to reset any data.
            return;
        }

        if (calcType == ObstructionType::Ignore)
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
            && m_obstOccType != ObstructionType::Ignore;
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

        if (m_obstOccType == ObstructionType::MultiRay)
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
            AZLOG_NOTICE(
                "Events are active on an object (ID: %llu) being released!  #Events: %zu   EventIDs: %s", GetID(), m_cActiveEvents.size(),
                sEventString);
        }

        if (!m_cTriggers.empty())
        {
            const char* const sTriggerString = GetTriggerNames("; ", pDebugNameStore).c_str();
            AZLOG_NOTICE(
                "Triggers are active on an object (ID: %llu) being released!  #Triggers: %zu   TriggerNames: %s", GetID(),
                m_cTriggers.size(), sTriggerString);
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
    bool ConvertOjbectWorldPosToScreenCoords(AZ::Vector3& position)
    {
        auto viewportContextMgr = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (!viewportContextMgr)
        {
            return false;
        }
        AZ::RPI::ViewportContextPtr viewportContext = viewportContextMgr->GetDefaultViewportContext();
        if (!viewportContext)
        {
            return false;
        }
        AZ::RPI::ViewPtr view = viewportContext->GetDefaultView();
        if (!view)
        {
            return false;
        }
        const AZ::RHI::Viewport& viewport = viewportContext->GetWindowContext()->GetViewport();

        position = AzFramework::WorldToScreenNdc(position, view->GetWorldToViewMatrixAsMatrix3x4(), view->GetViewToClipMatrix());
        position.SetX(position.GetX() * viewport.GetWidth());
        position.SetY((1.f - position.GetY()) * viewport.GetHeight());
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLAudioObject::DrawDebugInfo(
        AzFramework::DebugDisplayRequests& debugDisplay,
        [[maybe_unused]] const AZ::Vector3& vListenerPos,
        [[maybe_unused]] const CATLDebugNameStore* const pDebugNameStore) const
    {
        if (!m_cTriggers.empty())
        {
            // Inspect triggers and apply filter (if set)...
            TTriggerCountMap triggerCounts;

            auto triggerFilter = static_cast<AZ::CVarFixedString>(CVars::s_AudioTriggersDebugFilter);
            AZStd::to_lower(triggerFilter.begin(), triggerFilter.end());

            for (auto& trigger : m_cTriggers)
            {
                AZStd::string triggerName(pDebugNameStore->LookupAudioTriggerName(trigger.second.nTriggerID));
                AZStd::to_lower(triggerName.begin(), triggerName.end());

                if (AudioDebugDrawFilter(triggerName, triggerFilter))
                {
                    ++triggerCounts[trigger.second.nTriggerID];
                }
            }

            // Early out for this object if all trigger names were filtered out.
            if (triggerCounts.empty())
            {
                return;
            }

            const AZ::Vector3 pos3d(m_oPosition.GetPositionVec());
            AZ::Vector3 screenPos{ pos3d };
            if (!ConvertOjbectWorldPosToScreenCoords(screenPos))
            {
                return;
            }

            if (screenPos.GetZ() < 0.5f)
            {
                return;
            }

            if (CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::DrawObjects)))
            {
                const float radius = 0.05f;
                const AZ::Color sphereColor{ 1.f, 0.1f, 0.1f, 1.f };
                debugDisplay.SetColor(sphereColor);
                debugDisplay.DrawWireSphere(pos3d, radius);
            }

            AZStd::string str;
            const AZ::Color brightColor{ 0.9f, 0.9f, 0.9f, 1.f };
            const AZ::Color normalColor(0.75f, 0.75f, 0.75f, 1.f);
            const AZ::Color dimmedColor(0.5f, 0.5f, 0.5f, 1.f);
            const float distance = pos3d.GetDistance(vListenerPos);
            const float fontSize = 0.75f;
            const float lineHeight = 15.f;
            float posX = screenPos.GetX();
            float posY = screenPos.GetY();

            if (CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::ObjectLabels)))
            {
                SATLSoundPropagationData obstOccData;
                GetObstOccData(obstOccData);
                str = AZStd::string::format(
                    "%s  ID: %llu  RefCnt: %2zu  Dist: %4.1f m", pDebugNameStore->LookupAudioObjectName(GetID()), GetID(), GetRefCount(),
                    distance);
                debugDisplay.SetColor(brightColor);
                debugDisplay.Draw2dTextLabel(posX, posY, fontSize, str.c_str());

                posY += lineHeight;
                str = AZStd::string::format("  Obst: %.3f  Occl: %.3f", obstOccData.fObstruction, obstOccData.fOcclusion);
                debugDisplay.SetColor(normalColor);
                debugDisplay.Draw2dTextLabel(posX, posY, fontSize, str.c_str());
            }


            if (CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::ObjectTriggers)))
            {
                posY += lineHeight;
                debugDisplay.SetColor(brightColor);
                debugDisplay.Draw2dTextLabel(posX, posY, fontSize, "Triggers:");
                debugDisplay.SetColor(normalColor);

                for (auto& triggerCount : triggerCounts)
                {
                    auto triggerName = pDebugNameStore->LookupAudioTriggerName(triggerCount.first);
                    if (triggerName)
                    {
                        posY += lineHeight;
                        str = AZStd::string::format("  %s  (count = %zu)", triggerName, triggerCount.second);
                        debugDisplay.Draw2dTextLabel(posX, posY, fontSize, str.c_str());
                    }
                }
            }

            if (CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::ObjectStates)))
            {
                posY += lineHeight;
                debugDisplay.SetColor(brightColor);
                debugDisplay.Draw2dTextLabel(posX, posY, fontSize, "Switches:");

                for (auto& switchState : m_cSwitchStates)
                {
                    const auto switchId = switchState.first;
                    const auto stateId = switchState.second;
                    auto switchName = pDebugNameStore->LookupAudioSwitchName(switchId);
                    auto stateName = pDebugNameStore->LookupAudioSwitchStateName(switchId, stateId);
                    if (switchName && stateName)
                    {
                        CStateDebugDrawData& stateDrawData = m_cStateDrawInfoMap[switchId];
                        stateDrawData.Update(stateId);
                        AZ::Color switchColor{ 0.8f, 0.8f, 0.8f, stateDrawData.fCurrentAlpha };
                        posY += lineHeight;
                        str = AZStd::string::format("  %s : %s", switchName, stateName);
                        debugDisplay.SetColor(switchColor);
                        debugDisplay.Draw2dTextLabel(posX, posY, fontSize, str.c_str());
                    }
                }
            }

            if (CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::ObjectRtpcs)))
            {
                posY += lineHeight;
                debugDisplay.SetColor(brightColor);
                debugDisplay.Draw2dTextLabel(posX, posY, fontSize, "Parameters:");
                debugDisplay.SetColor(normalColor);

                for (auto& param : m_cRtpcs)
                {
                    const float value = param.second;
                    auto paramName = pDebugNameStore->LookupAudioRtpcName(param.first);
                    if (paramName)
                    {
                        posY += lineHeight;
                        str = AZStd::string::format("  %s = %4.2f", paramName, value);
                        debugDisplay.Draw2dTextLabel(posX, posY, fontSize, str.c_str());
                    }
                }
            }

            if (CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::ObjectEnvironments)))
            {
                posY += lineHeight;
                debugDisplay.SetColor(brightColor);
                debugDisplay.Draw2dTextLabel(posX, posY, fontSize, "Environments:");
                debugDisplay.SetColor(normalColor);

                for (auto& environment : m_cEnvironments)
                {
                    const float value = environment.second;
                    auto envName = pDebugNameStore->LookupAudioEnvironmentName(environment.first);
                    if (envName)
                    {
                        posY += lineHeight;
                        str = AZStd::string::format("  %s = %.3f", envName, value);
                        debugDisplay.Draw2dTextLabel(posX, posY, fontSize, str.c_str());
                    }
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void RaycastProcessor::DrawObstructionRays(AzFramework::DebugDisplayRequests& debugDisplay) const
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

        AZStd::string str;
        const float textSize = 0.7f;
        const bool drawRays = CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::DrawRays));
        const bool drawLabels = CVars::s_debugDrawOptions.AreAllFlagsActive(static_cast<AZ::u32>(DebugDraw::Options::RayLabels));

        size_t numRays = m_obstOccType == ObstructionType::SingleRay ? 1 : s_maxRaysPerObject;
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
                    debugDisplay.SetColor(hitSphereColor);
                    debugDisplay.DrawWireSphere(rayEnd, hitSphereRadius);
                }

                debugDisplay.DrawLine(rayInfo.m_raycastRequest.m_start, rayEnd, freeRayColor.GetAsVector4(), rayColor.GetAsVector4());
            }

            if (drawLabels)
            {
                AZ::Vector3 screenPos{ rayEnd };
                if (ConvertOjbectWorldPosToScreenCoords(screenPos) && screenPos.GetZ() >= 0.5f)
                {
                    float lerpValue = rayInfo.m_contribution;
                    AZ::Color labelColor = freeRayLabelColor.Lerp(obstructedRayLabelColor, lerpValue);

                    str = AZStd::string::format((rayIndex == 0) ? "Obst: %.2f" : "Occl: %.2f", rayInfo.GetDistanceScaledContribution());
                    debugDisplay.SetColor(labelColor);
                    debugDisplay.Draw2dTextLabel(screenPos.GetX(), screenPos.GetY() - 12.0f, textSize, str.c_str());
                }
            }
        }
    }

#endif // !AUDIO_RELEASE

} // namespace Audio
