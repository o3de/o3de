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


#include "CrySystem_precompiled.h"
#include "AsyncPakManager.h"
#include "System.h"
#include "IStreamEngine.h"
#include <AzFramework/Archive/Archive.h>
#include "ResourceManager.h"

#define MEGA_BYTE 1024* 1024

//////////////////////////////////////////////////////////////////////////

string& CAsyncPakManager::SAsyncPak::GetStatus(string& status) const
{
    switch (eState)
    {
    case STATE_UNLOADED:
        status = "Unloaded";
        break;
    case STATE_REQUESTED:
        status = "Requested";
        break;
    case STATE_REQUESTUNLOAD:
        status = "RequestUnload";
        break;
    case STATE_LOADED:
        status = "Loaded";
        break;
    default:
        status = "Unknown";
        break;
    }

    return status;
}

//////////////////////////////////////////////////////////////////////////

CAsyncPakManager::CAsyncPakManager()
{
    m_nTotalOpenLayerPakSize = 0;
    m_bRequestLayerUpdate = false;
}

CAsyncPakManager::~CAsyncPakManager()
{
    Clear();
}

//////////////////////////////////////////////////////////////////////////

void CAsyncPakManager::Clear()
{
    //float startTime = gEnv->pTimer->GetAsyncCurTime();

    for (TPakMap::iterator it = m_paks.begin();
         it != m_paks.end(); ++it)
    {
        SAsyncPak& layerPak = it->second;
        if (layerPak.bStreaming)
        {
            // wait until finished
            layerPak.pReadStream->Abort();
        }

        ReleaseData(&layerPak);
    }
    m_paks.clear();
    m_bRequestLayerUpdate = false;

    assert(m_nTotalOpenLayerPakSize == 0);
    m_nTotalOpenLayerPakSize = 0;

    //printf("CAsyncPakManager::Clear() %0.4f secs\n", gEnv->pTimer->GetAsyncCurTime() - startTime);
}

void CAsyncPakManager::UnloadLevelLoadPaks()
{
    for (TPakMap::iterator it = m_paks.begin();
         it != m_paks.end(); ++it)
    {
        SAsyncPak& layerPak = it->second;

        if (layerPak.eLifeTime == SAsyncPak::LIFETIME_LOAD_ONLY)
        {
            if (layerPak.bStreaming)
            {
                // wait until finished
                layerPak.pReadStream->Abort();
            }

            ReleaseData(&layerPak);
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CAsyncPakManager::ParseLayerPaks(const string& levelCachePath)
{
    string layerPath = levelCachePath + "/"; // "/layers/";
    string search = layerPath + "*";
    auto pPak = gEnv->pCryPak;


    // allow this find first to actually touch the file system
    AZ::IO::ArchiveFileIterator fileIterator= pPak->FindFirst(search.c_str(), 0, true);

    if (fileIterator)
    {
        do
        {
            if ((fileIterator.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory || fileIterator.m_filename == "." || fileIterator.m_filename == "..")
            {
                continue;
            }

            string pakName(fileIterator.m_filename.data(), fileIterator.m_filename.size());
            size_t findPos = pakName.find_last_of('.');
            if (findPos == string::npos)
            {
                continue;
            }

            string extension = pakName.substr(findPos + 1, pakName.size());
            if (extension != "pak")
            {
                continue;
            }

            SAsyncPak layerPak;
            layerPak.layername = pakName.substr(0, findPos);
            layerPak.filename = layerPath + pakName;
            layerPak.nSize = pPak->FGetSize(layerPak.filename.c_str(), true); // allow to go to disc for this access
            layerPak.bClosePakOnRelease = true;

            m_paks[layerPak.layername] = layerPak;
        } while (fileIterator = pPak->FindNext(fileIterator));

        pPak->FindClose(fileIterator);
    }
}

//////////////////////////////////////////////////////////////////////////

void CAsyncPakManager::StartStreaming(SAsyncPak* pLayerPak)
{
    StreamReadParams params;
    params.dwUserData = (DWORD_PTR) pLayerPak;
    params.nSize = 0;
    params.pBuffer = NULL;
    params.nFlags = IStreamEngine::FLAGS_FILE_ON_DISK;
    params.ePriority = estpIdle;

    pLayerPak->pReadStream = gEnv->pSystem->GetStreamEngine()->StartRead(eStreamTaskTypePak, pLayerPak->filename.c_str(), this, &params);

    if (pLayerPak->pReadStream)
    {
        pLayerPak->bStreaming = true;
    }
    else
    {
        pLayerPak->eState = SAsyncPak::STATE_UNLOADED;
        pLayerPak->pData.reset();
    }
}

void CAsyncPakManager::ReleaseData(SAsyncPak* pLayerPak)
{
    if (pLayerPak->eState == SAsyncPak::STATE_LOADED)
    {
        if (pLayerPak->bClosePakOnRelease)
        {
            gEnv->pCryPak->ClosePack(pLayerPak->filename.c_str(), 0);
            //printf("Unload pak from mem: %s\n", pLayerPak->filename.c_str());
        }
        else
        {
            gEnv->pCryPak->LoadPakToMemory(pLayerPak->filename.c_str(), AZ::IO::IArchive::eInMemoryPakLocale_Unload);
            //printf("Close pak: %s\n", pLayerPak->filename.c_str());
        }

        m_nTotalOpenLayerPakSize -= pLayerPak->nSize;
    }

    if (pLayerPak->pData)
    {
        assert(pLayerPak->pData->use_count() == 1);
    }

    assert((!pLayerPak->pData) || (pLayerPak->pData && pLayerPak->pData->use_count() == 1));
    pLayerPak->pData.reset();
    pLayerPak->eState = SAsyncPak::STATE_UNLOADED;

    m_bRequestLayerUpdate = true;
}

//////////////////////////////////////////////////////////////////////////

bool CAsyncPakManager::LoadLayerPak(const char* sLayerName)
{
    // only load layer paks from valid files
    TPakMap::iterator findResult = m_paks.find(sLayerName);
    if (findResult != m_paks.end())
    {
        return LoadPak(findResult->second);
    }

    return false;
}


bool CAsyncPakManager::LoadPakToMemAsync(const char* pPath, bool bLevelLoadOnly)
{
    //check if pak reference exists
    TPakMap::iterator findResult = m_paks.find(pPath);
    if (findResult != m_paks.end())
    {
        return LoadPak(findResult->second);
    }
    else
    {
        char szFullPathBuf[AZ::IO::IArchive::MaxPath];
        const char* szFullPath = gEnv->pCryPak->AdjustFileName(pPath, szFullPathBuf, AZ_ARRAY_SIZE(szFullPathBuf), AZ::IO::IArchive::FOPEN_HINT_QUIET | AZ::IO::IArchive::FLAGS_PATH_REAL);

        // Check if the pak file actually exists before trying to load
        if (!gEnv->pCryPak->IsFileExist(szFullPath, AZ::IO::IArchive::eFileLocation_Any))
        {
            // Cached file does not exist
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Level cache pak file %s does not exist", szFullPath);
            return false;
        }

        SAsyncPak layerPak;
        layerPak.layername = pPath;
        layerPak.filename = szFullPathBuf;
        layerPak.nSize = 0;
        layerPak.eLifeTime =  bLevelLoadOnly ? SAsyncPak::LIFETIME_LOAD_ONLY : SAsyncPak::LIFETIME_LEVEL_COMPLETE;

        m_paks[layerPak.layername] = layerPak;

        return LoadPak(m_paks[layerPak.layername]);
    }
    return false;
}

bool CAsyncPakManager::LoadPak(SAsyncPak& layerPak)
{
    layerPak.nRequestCount++;
    if (layerPak.eState == SAsyncPak::STATE_LOADED || layerPak.bStreaming ||
        layerPak.eState == SAsyncPak::STATE_REQUESTED)
    {
        return true;
    }

    layerPak.eState = SAsyncPak::STATE_REQUESTED;

    //printf("Streaming level pak: %s\n", layerPak.layername.c_str());

    StartStreaming(&layerPak);

    return false;
}

//////////////////////////////////////////////////////////////////////////

void CAsyncPakManager::UnloadLayerPak(const char* sLayerName)
{
    TPakMap::iterator findResult = m_paks.find(sLayerName);
    if (findResult == m_paks.end())
    {
        return;
    }

    SAsyncPak& layerPak = findResult->second;
    layerPak.nRequestCount--;
    assert(layerPak.nRequestCount >= 0);
    if (layerPak.nRequestCount > 0)
    {
        return;
    }

    if (layerPak.bStreaming)
    {
        if (layerPak.pReadStream)
        {
            layerPak.pReadStream->Abort();
        }
        layerPak.eState = SAsyncPak::STATE_REQUESTUNLOAD;
        return;
    }

    if (layerPak.eState == SAsyncPak::STATE_LOADED)
    {
        ReleaseData(&layerPak);
        m_bRequestLayerUpdate = true;
    }

    if (layerPak.eState == SAsyncPak::STATE_REQUESTED)
    {
        layerPak.eState = SAsyncPak::STATE_UNLOADED;
    }
}

//////////////////////////////////////////////////////////////////////////

void CAsyncPakManager::GetLayerPakStats(
    SLayerPakStats& stats, bool bCollectAllStats) const
{
    stats.m_MaxSize = (g_cvars.archiveVars.nTotalInMemoryPakSizeLimit * MEGA_BYTE);
    stats.m_UsedSize = m_nTotalOpenLayerPakSize;

    for (TPakMap::const_iterator it = m_paks.begin(); it != m_paks.end(); ++it)
    {
        const SAsyncPak& layerPak = it->second;
        if (bCollectAllStats || layerPak.eState != SAsyncPak::STATE_UNLOADED)
        {
            SLayerPakStats::SEntry entry;
            entry.name = it->first;
            entry.nSize = layerPak.nSize;
            entry.bStreaming = layerPak.bStreaming;
            layerPak.GetStatus(entry.status);

            stats.m_entries.push_back(entry);
        }
    }
}

//////////////////////////////////////////////////////////////////////////

void CAsyncPakManager::StreamAsyncOnComplete(
    IReadStream* pStream, unsigned nError)
{
    if (nError != 0)
    {
        return;
    }

    SAsyncPak* pLayerPak = (SAsyncPak*) pStream->GetUserData();

    //Check is pak is already open, if so, just assign mem
    if (gEnv->pCryPak->LoadPakToMemory(pLayerPak->filename.c_str(), AZ::IO::IArchive::eInMemoryPakLocale_GPU, pLayerPak->pData))
    {
        pLayerPak->bPakAlreadyOpen = true;
    }
    else
    {
        //
        // ugly hack - depending on the pak file pak may need special root info / open flags
        //
        if (pLayerPak->layername.find("level.pak") != string::npos)
        {
            gEnv->pCryPak->OpenPack({ pLayerPak->filename.c_str(), pLayerPak->filename.size() }, AZ::IO::IArchive::FLAGS_FILENAMES_AS_CRC32, NULL);
        }
        else if (pLayerPak->layername.find("levelshadercache.pak") != string::npos)
        {
            gEnv->pCryPak->OpenPack("@assets@", { pLayerPak->filename.c_str(), pLayerPak->filename.size() }, AZ::IO::IArchive::FLAGS_PATH_REAL, NULL);
        }
        else
        {
            gEnv->pCryPak->OpenPack("@assets@", { pLayerPak->filename.c_str(), pLayerPak->filename.size() }, AZ::IO::IArchive::FLAGS_FILENAMES_AS_CRC32, NULL);
        }
        gEnv->pCryPak->LoadPakToMemory(pLayerPak->filename.c_str(), AZ::IO::IArchive::eInMemoryPakLocale_GPU, pLayerPak->pData);
    }

    pLayerPak->eState = SAsyncPak::STATE_LOADED;

    //printf("Finished streaming level pak: %s\n", pLayerPak->layername.c_str());
}

//////////////////////////////////////////////////////////////////////////

void CAsyncPakManager::StreamOnComplete(
    IReadStream* pStream, unsigned nError)
{
    SAsyncPak* pLayerPak = (SAsyncPak*) pStream->GetUserData();

    if (nError != 0)
    {
        ReleaseData(pLayerPak);
    }

    pLayerPak->bStreaming = false;
    pLayerPak->pReadStream = NULL;

    m_bRequestLayerUpdate = true;
}

void* CAsyncPakManager::StreamOnNeedStorage(IReadStream* pStream, unsigned nSize, bool& bAbortOnFailToAlloc)
{
    SAsyncPak* pAsyncPak = (SAsyncPak*)pStream->GetUserData();

    pAsyncPak->nSize = nSize;

    if ((m_nTotalOpenLayerPakSize + nSize) > (size_t)(g_cvars.archiveVars.nTotalInMemoryPakSizeLimit * MEGA_BYTE))
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Not enough space to load in memory layer pak %s (Current: %" PRISIZE_T " Required: %d)",
            pAsyncPak->filename.c_str(), m_nTotalOpenLayerPakSize, nSize);

        //printf("Not enough space to load in memory layer pak %s (Current: %d Required: %d)\n", pAsyncPak->filename.c_str(), m_nTotalOpenLayerPakSize, nSize);

        pAsyncPak->eState = SAsyncPak::STATE_UNLOADED;
        pAsyncPak->bStreaming = false;
        pAsyncPak->pReadStream = NULL;

        bAbortOnFailToAlloc = true;

        return NULL;
    }

    if (nSize)
    {
        auto pCryPak = static_cast<AZ::IO::Archive*>(gEnv->pCryPak);

        // allocate the data
        const char* szUsage = "In Memory Zip File";
        pAsyncPak->pData = pCryPak->PoolAllocMemoryBlock(nSize, szUsage, alignof(uint8_t));

        m_nTotalOpenLayerPakSize += nSize;

        return pAsyncPak->pData->m_address.get();
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////

void CAsyncPakManager::Update()
{
    if (!m_bRequestLayerUpdate)
    {
        return;
    }

    m_bRequestLayerUpdate = false;

    for (TPakMap::iterator it = m_paks.begin();
         it != m_paks.end(); ++it)
    {
        SAsyncPak& layerPak = it->second;
        if (!layerPak.bStreaming)
        {
            if (layerPak.eState == SAsyncPak::STATE_REQUESTUNLOAD)
            {
                // done streaming and not interested in it anymore, then release it again
                ReleaseData(&layerPak);
            }
            else if (layerPak.eState == SAsyncPak::STATE_REQUESTED &&
                     (m_nTotalOpenLayerPakSize + layerPak.nSize <= ((size_t)g_cvars.archiveVars.nTotalInMemoryPakSizeLimit * MEGA_BYTE)))
            {
                // do we have enough memory now to start streaming the pak
                StartStreaming(&layerPak);
            }
        }
    }
}

// Abort streaming jobs and prevent any more requests
// Paks which are loaded remain, they will be cleaned up as usual
void CAsyncPakManager::CancelPendingJobs()
{
    for (TPakMap::iterator it = m_paks.begin(); it != m_paks.end(); ++it)
    {
        SAsyncPak& layerPak = it->second;

        if (layerPak.bStreaming)
        {
            layerPak.pReadStream->Abort();
            ReleaseData(&layerPak);

            //printf("Pak %s Aborted\n", layerPak.filename.c_str());
        }
        else if (layerPak.eState == SAsyncPak::STATE_REQUESTED)
        {
            layerPak.eState = SAsyncPak::STATE_UNLOADED;
            ReleaseData(&layerPak);

            //printf("Pak %s Cancelled\n", layerPak.filename.c_str());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
