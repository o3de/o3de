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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Interface to the Resource Manager


#ifndef CRYINCLUDE_CRYSYSTEM_RESOURCEMANAGER_H
#define CRYINCLUDE_CRYSYSTEM_RESOURCEMANAGER_H
#pragma once


#include <IResourceManager.h>
#include "AsyncPakManager.h"

//////////////////////////////////////////////////////////////////////////
// IResource manager interface
//////////////////////////////////////////////////////////////////////////
class CResourceManager
    : public IResourceManager
    , public ISystemEventListener
    , public AZ::IO::IArchiveFileAccessSink
{
public:
    CResourceManager();

    void Init();
    void Shutdown();

    bool IsStreamingCachePak(const char* filename) const;

    //////////////////////////////////////////////////////////////////////////
    // IResourceManager interface implementation.
    //////////////////////////////////////////////////////////////////////////
    void PrepareLevel(const char* sLevelFolder, const char* sLevelName);
    void UnloadLevel();
    AZ::IO::IResourceList* GetLevelResourceList();
    bool LoadLevelCachePak(const char* sPakName, const char* sBindRoot, bool bOnlyDuringLevelLoading);
    void UnloadLevelCachePak(const char* sPakName);
    bool LoadModeSwitchPak(const char* sPakName, const bool multiplayer);
    void UnloadModeSwitchPak(const char* sPakName, const char* sResourceListName, const bool multiplayer);
    bool LoadMenuCommonPak(const char* sPakName);
    void UnloadMenuCommonPak(const char* sPakName, const char* sResourceListName);
    bool LoadPakToMemAsync(const char* pPath, bool bLevelLoadOnly);
    void UnloadAllAsyncPaks();
    bool LoadLayerPak(const char* sLayerName);
    void UnloadLayerPak(const char* sLayerName);
    void UnloadAllLevelCachePaks(bool bLevelLoadEnd);
    void GetMemoryStatistics(ICrySizer* pSizer);
    bool LoadFastLoadPaks(bool bToMemory);
    void UnloadFastLoadPaks();
    CTimeValue GetLastLevelLoadTime() const;
    void GetLayerPakStats(SLayerPakStats& stats, bool bCollectAllStats) const;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // ISystemEventListener interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IArchiveFileAccessSink interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void ReportFileOpen(AZ::IO::HandleType inFileHandle, AZStd::string_view szFullPath);
    //////////////////////////////////////////////////////////////////////////

    // Per frame update of the resource manager.
    void Update();

    CryPathString GetCurrentLevelCacheFolder() const { return m_currentLevelCacheFolder; };
    void SaveRecordedResources(bool bTotalList = false);

private:

    //////////////////////////////////////////////////////////////////////////

    CryPathString m_currentLevelCacheFolder;

    struct SOpenedPak
    {
        AZStd::fixed_string<AZ::IO::IArchive::MaxPath> filename;
        bool bOnlyDuringLevelLoading;
    };
    std::vector<SOpenedPak> m_openedPaks;

    CAsyncPakManager m_AsyncPakManager;

    string m_sLevelFolder;
    string m_sLevelName;
    bool m_bLevelTransitioning;

    bool m_bRegisteredFileOpenSink;
    bool m_bOwnResourceList;

    CTimeValue m_beginLevelLoadTime;
    CTimeValue m_lastLevelLoadTime;

    AZStd::intrusive_ptr<AZ::IO::IResourceList> m_pSequenceResourceList;

    CryCriticalSection recordedFilesLock;
    std::vector<string> m_recordedFiles;

    AZStd::vector< AZStd::fixed_string<AZ::IO::IArchive::MaxPath> > m_fastLoadPakPaths;
};

#endif // CRYINCLUDE_CRYSYSTEM_RESOURCEMANAGER_H
