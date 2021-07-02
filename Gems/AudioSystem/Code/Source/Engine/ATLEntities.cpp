/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ATLEntities.h>

namespace Audio
{
#if !defined(AUDIO_RELEASE)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLDebugNameStore::CATLDebugNameStore()
        : m_bATLObjectsChanged(false)
        , m_bATLTriggersChanged(false)
        , m_bATLRtpcsChanged(false)
        , m_bATLSwitchesChanged(false)
        , m_bATLPreloadsChanged(false)
        , m_bATLEnvironmentsChanged(false)
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CATLDebugNameStore::~CATLDebugNameStore()
    {
        // the containers only hold numbers and strings, no ATL specific objects,
        // so there is no need to call the implementation to do the cleanup
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::SyncChanges(const CATLDebugNameStore& rOtherNameStore)
    {
        if (rOtherNameStore.m_bATLObjectsChanged)
        {
            m_cATLObjectNames.clear();
            m_cATLObjectNames.insert(rOtherNameStore.m_cATLObjectNames.begin(), rOtherNameStore.m_cATLObjectNames.end());
            rOtherNameStore.m_bATLObjectsChanged = false;
        }

        if (rOtherNameStore.m_bATLTriggersChanged)
        {
            m_cATLTriggerNames.clear();
            m_cATLTriggerNames.insert(rOtherNameStore.m_cATLTriggerNames.begin(), rOtherNameStore.m_cATLTriggerNames.end());
            rOtherNameStore.m_bATLTriggersChanged = false;
        }

        if (rOtherNameStore.m_bATLRtpcsChanged)
        {
            m_cATLRtpcNames.clear();
            m_cATLRtpcNames.insert(rOtherNameStore.m_cATLRtpcNames.begin(), rOtherNameStore.m_cATLRtpcNames.end());
            rOtherNameStore.m_bATLRtpcsChanged = false;
        }

        if (rOtherNameStore.m_bATLSwitchesChanged)
        {
            m_cATLSwitchNames.clear();
            m_cATLSwitchNames.insert(rOtherNameStore.m_cATLSwitchNames.begin(), rOtherNameStore.m_cATLSwitchNames.end());
            rOtherNameStore.m_bATLSwitchesChanged = false;
        }

        if (rOtherNameStore.m_bATLPreloadsChanged)
        {
            m_cATLPreloadRequestNames.clear();
            m_cATLPreloadRequestNames.insert(rOtherNameStore.m_cATLPreloadRequestNames.begin(), rOtherNameStore.m_cATLPreloadRequestNames.end());
            rOtherNameStore.m_bATLPreloadsChanged = false;
        }

        if (rOtherNameStore.m_bATLEnvironmentsChanged)
        {
            m_cATLEnvironmentNames.clear();
            m_cATLEnvironmentNames.insert(rOtherNameStore.m_cATLEnvironmentNames.begin(), rOtherNameStore.m_cATLEnvironmentNames.end());
            rOtherNameStore.m_bATLEnvironmentsChanged = false;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioObject(const TAudioObjectID nObjectID, const char* const sName)
    {
        m_cATLObjectNames[nObjectID] = sName;
        m_bATLObjectsChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioTrigger(const TAudioControlID nTriggerID, const char* const sName)
    {
        m_cATLTriggerNames[nTriggerID] = sName;
        m_bATLTriggersChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioRtpc(const TAudioControlID nRtpcID, const char* const sName)
    {
        m_cATLRtpcNames[nRtpcID] = sName;
        m_bATLRtpcsChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioSwitch(const TAudioControlID nSwitchID, const char* const sName)
    {
        m_cATLSwitchNames[nSwitchID] = AZStd::make_pair(sName, TAudioSwitchStateMap());
        m_bATLSwitchesChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID, const char* const sName)
    {
        m_cATLSwitchNames[nSwitchID].second[nStateID] = sName;
        m_bATLSwitchesChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioPreloadRequest(const TAudioPreloadRequestID nRequestID, const char* const sName)
    {
        m_cATLPreloadRequestNames[nRequestID] = sName;
        m_bATLPreloadsChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::AddAudioEnvironment(const TAudioEnvironmentID nEnvironmentID, const char* const sName)
    {
        m_cATLEnvironmentNames[nEnvironmentID] = sName;
        m_bATLEnvironmentsChanged = true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioObject(const TAudioObjectID nObjectID)
    {
        auto num = m_cATLObjectNames.erase(nObjectID);
        m_bATLObjectsChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioTrigger(const TAudioControlID nTriggerID)
    {
        auto num = m_cATLTriggerNames.erase(nTriggerID);
        m_bATLTriggersChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioRtpc(const TAudioControlID nRtpcID)
    {
        auto num = m_cATLRtpcNames.erase(nRtpcID);
        m_bATLRtpcsChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioSwitch(const TAudioControlID nSwitchID)
    {
        auto num = m_cATLSwitchNames.erase(nSwitchID);
        m_bATLSwitchesChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID)
    {
        TAudioSwitchMap::iterator iPlace = m_cATLSwitchNames.begin();
        if (FindPlace(m_cATLSwitchNames, nSwitchID, iPlace))
        {
            auto num = iPlace->second.second.erase(nStateID);
            m_bATLSwitchesChanged |= (num > 0);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioPreloadRequest(const TAudioPreloadRequestID nRequestID)
    {
        auto num = m_cATLPreloadRequestNames.erase(nRequestID);
        m_bATLPreloadsChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CATLDebugNameStore::RemoveAudioEnvironment(const TAudioEnvironmentID nEnvironmentID)
    {
        auto num = m_cATLEnvironmentNames.erase(nEnvironmentID);
        m_bATLEnvironmentsChanged |= (num > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioObjectName(const TAudioObjectID nObjectID) const
    {
        TAudioObjectMap::const_iterator iterObject(m_cATLObjectNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLObjectNames, nObjectID, iterObject))
        {
            sResult = iterObject->second.c_str();
        }
        else if (nObjectID == GLOBAL_AUDIO_OBJECT_ID)
        {
            sResult = "GlobalAudioObject";
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioTriggerName(const TAudioControlID nTriggerID) const
    {
        TAudioControlMap::const_iterator iterTrigger(m_cATLTriggerNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLTriggerNames, nTriggerID, iterTrigger))
        {
            sResult = iterTrigger->second.c_str();
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioRtpcName(const TAudioControlID nRtpcID) const
    {
        TAudioControlMap::const_iterator iterRtpc(m_cATLRtpcNames.begin());
        char const* sResult = nullptr;

        if (FindPlaceConst(m_cATLRtpcNames, nRtpcID, iterRtpc))
        {
            sResult = iterRtpc->second.c_str();
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioSwitchName(const TAudioControlID nSwitchID) const
    {
        TAudioSwitchMap::const_iterator iterSwitch(m_cATLSwitchNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLSwitchNames, nSwitchID, iterSwitch))
        {
            sResult = iterSwitch->second.first.c_str();
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioSwitchStateName(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID) const
    {
        TAudioSwitchMap::const_iterator iterSwitch(m_cATLSwitchNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLSwitchNames, nSwitchID, iterSwitch))
        {
            const TAudioSwitchStateMap& cStates = iterSwitch->second.second;
            TAudioSwitchStateMap::const_iterator iterState = cStates.begin();

            if (FindPlaceConst(cStates, nStateID, iterState))
            {
                sResult = iterState->second.c_str();
            }
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioPreloadRequestName(const TAudioPreloadRequestID nRequestID) const
    {
        TAudioPreloadRequestsMap::const_iterator iterPreload(m_cATLPreloadRequestNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLPreloadRequestNames, nRequestID, iterPreload))
        {
            sResult = iterPreload->second.c_str();
        }

        return sResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    const char* CATLDebugNameStore::LookupAudioEnvironmentName(const TAudioEnvironmentID nEnvironmentID) const
    {
        TAudioEnvironmentMap::const_iterator iterEnvironment(m_cATLEnvironmentNames.begin());
        const char* sResult = nullptr;

        if (FindPlaceConst(m_cATLEnvironmentNames, nEnvironmentID, iterEnvironment))
        {
            sResult = iterEnvironment->second.c_str();
        }

        return sResult;
    }
#endif // !AUDIO_RELEASE
} // namespace Audio
