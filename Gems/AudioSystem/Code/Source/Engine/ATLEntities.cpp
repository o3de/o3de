/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ATLEntities.h>
#include <AzCore/IO/Streamer/FileRequest.h>

namespace Audio
{
#if !defined(AUDIO_RELEASE)

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::AddAudioObject(const TAudioObjectID nObjectID, const char* const sName)
    {
        return (m_cATLObjectNames.insert(AZStd::make_pair(nObjectID, sName)).second);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::AddAudioTrigger(const TAudioControlID nTriggerID, const char* const sName)
    {
        return (m_cATLTriggerNames.insert(AZStd::make_pair(nTriggerID, sName)).second);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::AddAudioRtpc(const TAudioControlID nRtpcID, const char* const sName)
    {
        return (m_cATLRtpcNames.insert(AZStd::make_pair(nRtpcID, sName)).second);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::AddAudioSwitch(const TAudioControlID nSwitchID, const char* const sName)
    {
        return (m_cATLSwitchNames.insert(AZStd::make_pair(nSwitchID, AZStd::make_pair(sName, TAudioSwitchStateMap()))).second);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::AddAudioSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID, const char* const sName)
    {
        auto iter = m_cATLSwitchNames.find(nSwitchID);
        if (iter != m_cATLSwitchNames.end())
        {
            return (iter->second.second.insert(AZStd::make_pair(nStateID, sName)).second);
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::AddAudioPreloadRequest(const TAudioPreloadRequestID nRequestID, const char* const sName)
    {
        return (m_cATLPreloadRequestNames.insert({ nRequestID, sName }).second);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::AddAudioEnvironment(const TAudioEnvironmentID nEnvironmentID, const char* const sName)
    {
        return (m_cATLEnvironmentNames.insert({ nEnvironmentID, sName }).second);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::RemoveAudioObject(const TAudioObjectID nObjectID)
    {
        return (m_cATLObjectNames.erase(nObjectID) > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::RemoveAudioTrigger(const TAudioControlID nTriggerID)
    {
        return (m_cATLTriggerNames.erase(nTriggerID) > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::RemoveAudioRtpc(const TAudioControlID nRtpcID)
    {
        return (m_cATLRtpcNames.erase(nRtpcID) > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::RemoveAudioSwitch(const TAudioControlID nSwitchID)
    {
        return (m_cATLSwitchNames.erase(nSwitchID) > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::RemoveAudioSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID)
    {
        TAudioSwitchMap::iterator iPlace = m_cATLSwitchNames.begin();
        if (FindPlace(m_cATLSwitchNames, nSwitchID, iPlace))
        {
            return (iPlace->second.second.erase(nStateID) > 0);
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::RemoveAudioPreloadRequest(const TAudioPreloadRequestID nRequestID)
    {
        return (m_cATLPreloadRequestNames.erase(nRequestID) > 0);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    bool CATLDebugNameStore::RemoveAudioEnvironment(const TAudioEnvironmentID nEnvironmentID)
    {
        return (m_cATLEnvironmentNames.erase(nEnvironmentID) > 0);
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

    CATLAudioFileEntry::CATLAudioFileEntry(const char* const filePath, IATLAudioFileEntryData* const implData)
        : m_filePath(filePath)
        , m_fileSize(0)
        , m_useCount(0)
        , m_memoryBlockAlignment(AUDIO_MEMORY_ALIGNMENT)
        , m_flags(0)
        , m_dataScope(eADS_ALL)
        , m_memoryBlock(nullptr)
        , m_implData(implData)
    {
    }

    CATLAudioFileEntry::~CATLAudioFileEntry() = default;
} // namespace Audio
