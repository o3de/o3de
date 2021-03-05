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

// Description : Manage async pak files

#ifndef CRYINCLUDE_CRYSYSTEM_ASYNCPAKMANAGER_H
#define CRYINCLUDE_CRYSYSTEM_ASYNCPAKMANAGER_H
#pragma once

#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <IResourceManager.h>
#include <IStreamEngine.h>

namespace AZ::IO
{
    struct MemoryBlock;
}

class CAsyncPakManager
    : public IStreamCallback
{
protected:

    struct SAsyncPak
    {
        enum EState
        {
            STATE_UNLOADED,
            STATE_REQUESTED,
            STATE_REQUESTUNLOAD,
            STATE_LOADED,
        };

        enum ELifeTime
        {
            LIFETIME_LOAD_ONLY,
            LIFETIME_LEVEL_COMPLETE,
            LIFETIME_PERMANENT
        };

        SAsyncPak()
            : nRequestCount(0)
            , eState(STATE_UNLOADED)
            , eLifeTime(LIFETIME_LOAD_ONLY)
            , nSize(0)
            , pData(0)
            , bStreaming(false)
            , bPakAlreadyOpen(false)
            , bClosePakOnRelease(false)
            , pReadStream(0) {}

        string& GetStatus(string&) const;

        string layername;
        string filename;
        size_t nSize;
        AZStd::intrusive_ptr<AZ::IO::MemoryBlock> pData;
        EState eState;
        ELifeTime eLifeTime;
        bool bStreaming;
        bool bPakAlreadyOpen;
        bool bClosePakOnRelease;
        int nRequestCount;
        IReadStreamPtr pReadStream;
    };
    typedef std::map<string, SAsyncPak> TPakMap;

public:

    CAsyncPakManager();
    ~CAsyncPakManager();

    void ParseLayerPaks(const string& levelCachePath);

    bool LoadPakToMemAsync(const char* pPath, bool bLevelLoadOnly);
    void UnloadLevelLoadPaks();
    bool LoadLayerPak(const char* sLayerName);
    void UnloadLayerPak(const char* sLayerName);
    void CancelPendingJobs();

    void GetLayerPakStats(SLayerPakStats& stats, bool bCollectAllStats) const;

    void Clear();
    void Update();

protected:

    bool LoadPak(SAsyncPak& layerPak);

    void StartStreaming(SAsyncPak* pLayerPak);
    void ReleaseData(SAsyncPak* pLayerPak);

    //////////////////////////////////////////////////////////////////////////
    // IStreamCallback interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);
    virtual void StreamOnComplete (IReadStream* pStream, unsigned nError);
    virtual void* StreamOnNeedStorage(IReadStream* pStream, unsigned nSize, bool& bAbortOnFailToAlloc);
    //////////////////////////////////////////////////////////////////////////

    TPakMap m_paks;
    size_t m_nTotalOpenLayerPakSize;
    bool m_bRequestLayerUpdate;
};

#endif // CRYINCLUDE_CRYSYSTEM_ASYNCPAKMANAGER_H
