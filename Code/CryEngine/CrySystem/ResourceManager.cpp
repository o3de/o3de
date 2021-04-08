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


#include "CrySystem_precompiled.h"
#include "ResourceManager.h"
#include "System.h"
#include "MaterialUtils.h"
#include <CryPath.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/Archive/Archive.h>
#include <AzFramework/Archive/INestedArchive.h>
#include <Pak/CryPakUtils.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define RESOURCEMANAGER_CPP_SECTION_1 1
#define RESOURCEMANAGER_CPP_SECTION_2 2
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION RESOURCEMANAGER_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(ResourceManager_cpp)
#endif

#define LEVEL_PAK_FILENAME "level.pak"
#define LEVEL_PAK_INMEMORY_MAXSIZE 10 * 1024 * 1024

#define ENGINE_PAK_FILENAME "engine.pak"
#define LEVEL_CACHE_PAK_FILENAME "xml.pak"

#define GAME_DATA_PAK_FILENAME "gamedata.pak"
#define FAST_LOADING_PAKS_SRC_FOLDER "_fastload/"
#define FRONTEND_COMMON_PAK_FILENAME_SP "modes/menucommon_sp.pak"
#define FRONTEND_COMMON_PAK_FILENAME_MP "modes/menucommon_mp.pak"
#define FRONTEND_COMMON_LIST_FILENAME "menucommon"
#define LEVEL_CACHE_SRC_FOLDER "_levelcache/"
#define LEVEL_CACHE_BIND_ROOT "LevelCache"
#define LEVEL_RESOURCE_LIST   "resourcelist.txt"
#define AUTO_LEVEL_RESOURCE_LIST "auto_resourcelist.txt"
#define AUTO_LEVEL_SEQUENCE_RESOURCE_LIST "auto_resources_sequence.txt"
#define AUTO_LEVEL_TOTAL_RESOURCE_LIST "auto_resourcelist_total.txt"
#define AUTO_LEVEL_TOTAL_SEQUENCE_RESOURCE_LIST "auto_resources_total_sequence.txt"

//////////////////////////////////////////////////////////////////////////
// IResourceList implementation class.
//////////////////////////////////////////////////////////////////////////
class CLevelResourceList
    : public AZ::IO::IResourceList
{
public:
    CLevelResourceList()
    {
        m_pFileBuffer = 0;
        m_nBufferSize = 0;
        m_nCurrentLine = 0;
    };
    ~CLevelResourceList()
    {
        Clear();
    };

    uint32 GetFilenameHash(const char* sResourceFile)
    {
        char filename[512];
        azstrcpy(filename, AZ_ARRAY_SIZE(filename), sResourceFile);
        MaterialUtils::UnifyMaterialName(filename);

        uint32 code = CCrc32::ComputeLowercase(filename);
        return code;
    }

    virtual void Add([[maybe_unused]] AZStd::string_view sResourceFile)
    {
        assert(0); // Not implemented.
    }
    virtual void Clear()
    {
        delete [] m_pFileBuffer;
        m_pFileBuffer = 0;
        m_nBufferSize = 0;
        stl::free_container(m_lines);
        stl::free_container(m_resources_crc32);
        m_nCurrentLine = 0;
    }

    struct ComparePredicate
    {
        bool operator()(const char* s1, const char* s2)
        {
            return strcmp(s1, s2) < 0;
        }
    };

    virtual bool IsExist(AZStd::string_view sResourceFile)
    {
        uint32 nHash = GetFilenameHash(sResourceFile.data());
        if (stl::binary_find(m_resources_crc32.begin(), m_resources_crc32.end(), nHash) != m_resources_crc32.end())
        {
            return true;
        }
        return false;
    }
    virtual bool Load(AZStd::string_view sResourceListFilename)
    {
        Clear();
        CCryFile file;
        if (file.Open(sResourceListFilename.data(), "rb", AZ::IO::IArchive::FOPEN_ONDISK)) // File access can happen from disk as well.
        {
            m_nBufferSize = file.GetLength();
            if (m_nBufferSize > 0)
            {
                m_pFileBuffer = new char[m_nBufferSize];
                size_t numBytesRead = file.ReadRaw(m_pFileBuffer, file.GetLength());

                if (numBytesRead <= 0 || numBytesRead != file.GetLength())
                {
                    AZ_Error("ResourceManager", false, "Unable to read data for: %.*s", aznumeric_cast<int>(sResourceListFilename.size()), sResourceListFilename.data());
                    return false;
                }
                m_pFileBuffer[m_nBufferSize - 1] = 0;

                char seps[] = "\r\n";

                m_lines.reserve(5000);

                // Parse file, every line in a file represents a resource filename.
                char* nextToken = nullptr;
                char* token = azstrtok(m_pFileBuffer, 0, seps, &nextToken);
                while (token != NULL)
                {
                    m_lines.push_back(token);
                    token = azstrtok(NULL, 0, seps, &nextToken);
                }

                m_resources_crc32.resize(m_lines.size());
                for (int i = 0, numlines = m_lines.size(); i < numlines; i++)
                {
                    MaterialUtils::UnifyMaterialName(const_cast<char*>(m_lines[i]));
                    m_resources_crc32[i] = CCrc32::ComputeLowercase(m_lines[i]);
                }
                std::sort(m_resources_crc32.begin(), m_resources_crc32.end());
            }
            return true;
        }
        return false;
    }
    virtual const char* GetFirst()
    {
        m_nCurrentLine = 0;
        if (!m_lines.empty())
        {
            return m_lines[0];
        }
        return NULL;
    }
    virtual const char* GetNext()
    {
        m_nCurrentLine++;
        if (m_nCurrentLine < (int)m_lines.size())
        {
            return m_lines[m_nCurrentLine];
        }
        return NULL;
    }

    void GetMemoryStatistics(ICrySizer* pSizer)
    {
        pSizer->Add(this, sizeof(*this));
        pSizer->Add(m_pFileBuffer, m_nBufferSize);
        pSizer->AddContainer(m_lines);
        pSizer->AddContainer(m_resources_crc32);
    }

public:
    char* m_pFileBuffer;
    int m_nBufferSize;
    typedef std::vector<const char*> Lines;
    Lines m_lines;
    int m_nCurrentLine;
    std::vector<uint32> m_resources_crc32;
};


//////////////////////////////////////////////////////////////////////////
CResourceManager::CResourceManager()
{
    m_bRegisteredFileOpenSink = false;
    m_bOwnResourceList = false;
    m_bLevelTransitioning = false;

    m_fastLoadPakPaths.reserve(8);
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::PrepareLevel(const char* sLevelFolder, const char* sLevelName)
{
    LOADING_TIME_PROFILE_SECTION;

    m_sLevelFolder = sLevelFolder;
    m_sLevelName = sLevelName;
    m_bLevelTransitioning = false;
    m_currentLevelCacheFolder = CryPathString(LEVEL_CACHE_SRC_FOLDER) + sLevelName;

    if (g_cvars.archiveVars.nLoadCache)
    {
        bool usePrefabSystemForLevels = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

        // The prefab system doesn't use level.pak
        if (!usePrefabSystemForLevels)
        {
            CryPathString levelpak = PathUtil::Make(sLevelFolder, LEVEL_PAK_FILENAME);
            size_t nPakFileSize = gEnv->pCryPak->FGetSize(levelpak.c_str());
            if (nPakFileSize < LEVEL_PAK_INMEMORY_MAXSIZE) // 10 megs.
            {
                // Force level.pak from this level in memory.
                gEnv->pCryPak->LoadPakToMemory(LEVEL_PAK_FILENAME, AZ::IO::IArchive::eInMemoryPakLocale_GPU);
            }
        }

        gEnv->pCryPak->LoadPakToMemory(ENGINE_PAK_FILENAME, AZ::IO::IArchive::eInMemoryPakLocale_GPU);

        //
        // Load _levelCache paks in the order they are stored on the disk - reduce seek time
        //

        if (gEnv->pConsole->GetCVar("e_StreamCgf") && gEnv->pConsole->GetCVar("e_StreamCgf")->GetIVal() != 0)
        {
            LoadLevelCachePak("cga.pak", "", true);
            LoadLevelCachePak("cgf.pak", "", true);

            if (g_cvars.archiveVars.nStreamCache)
            {
                LoadLevelCachePak("cgf_cache.pak", "", false);
            }
        }

        LoadLevelCachePak("chr.pak", "", true);

        if (g_cvars.archiveVars.nStreamCache)
        {
            LoadLevelCachePak("chr_cache.pak", "", false);
        }

        LoadLevelCachePak("dds0.pak", "", true);

        if (g_cvars.archiveVars.nStreamCache)
        {
            LoadLevelCachePak("dds_cache.pak", "", false);
        }

        LoadLevelCachePak("skin.pak", "", true);

        if (g_cvars.archiveVars.nStreamCache)
        {
            LoadLevelCachePak("skin_cache.pak", "", false);
        }

        LoadLevelCachePak(LEVEL_CACHE_PAK_FILENAME, "", true);
    }

    AZStd::intrusive_ptr<CLevelResourceList> pResList = new CLevelResourceList;
    gEnv->pCryPak->SetResourceList(AZ::IO::IArchive::RFOM_Level, pResList.get());
    m_bOwnResourceList = true;

    // Load resourcelist.txt, TODO: make sure there are no duplicates
    if (g_cvars.archiveVars.nSaveLevelResourceList == 0)
    {
        string filename = PathUtil::Make(sLevelFolder, AUTO_LEVEL_RESOURCE_LIST);
        if (!pResList->Load(filename.c_str()))   // If we saving resource list do not use auto_resourcelist.txt
        {
            // Try resource list created by the editor.
            filename = PathUtil::Make(sLevelFolder, LEVEL_RESOURCE_LIST);
            pResList->Load(filename.c_str());
        }
    }
    //LoadFastLoadPaks();

    if (g_cvars.archiveVars.nStreamCache)
    {
        m_AsyncPakManager.ParseLayerPaks(GetCurrentLevelCacheFolder());
    }
}

//////////////////////////////////////////////////////////////////////////
bool CResourceManager::LoadFastLoadPaks(bool bToMemory)
{
    if (g_cvars.archiveVars.nSaveFastloadResourceList != 0)
    {
        // Record a file list for _FastLoad/startup.pak
        m_recordedFiles.clear();
        gEnv->pCryPak->RegisterFileAccessSink(this);
        m_bRegisteredFileOpenSink = true;
        return false;
    }
    else
    {
        LOADING_TIME_PROFILE_SECTION;

        // Load a special _fastload paks
        int nPakPreloadFlags = AZ::IO::IArchive::FLAGS_FILENAMES_AS_CRC32 | AZ::IO::INestedArchive::FLAGS_OVERRIDE_PAK;
        if (bToMemory && g_cvars.archiveVars.nLoadCache)
        {
            nPakPreloadFlags |= AZ::IO::IArchive::FLAGS_PAK_IN_MEMORY;
        }

        const char* const assetsDir = "@assets@";
        const char* shaderPakDir = assetsDir;
        const char* shaderPakPath = "ShaderCacheStartup.pak";

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION RESOURCEMANAGER_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(ResourceManager_cpp)
#endif

        gEnv->pCryPak->OpenPacks(assetsDir, AZ::IO::PathString(FAST_LOADING_PAKS_SRC_FOLDER) + "*.pak", nPakPreloadFlags, &m_fastLoadPakPaths);
        gEnv->pCryPak->OpenPack(assetsDir, shaderPakPath, AZ::IO::IArchive::FLAGS_PAK_IN_MEMORY | AZ::IO::INestedArchive::FLAGS_OVERRIDE_PAK);
        gEnv->pCryPak->OpenPack(assetsDir, "Engine.pak", AZ::IO::IArchive::FLAGS_PAK_IN_MEMORY);
        return !m_fastLoadPakPaths.empty();
    }
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::UnloadFastLoadPaks()
{
    for (uint32 i = 0; i < m_fastLoadPakPaths.size(); i++)
    {
        // Unload a special _fastload paks
        gEnv->pCryPak->ClosePack(m_fastLoadPakPaths[i].c_str(), AZ::IO::IArchive::FLAGS_PATH_REAL);
    }
    m_fastLoadPakPaths.clear();

    if (gEnv && gEnv->pRenderer)
    {
        gEnv->pRenderer->UnloadShaderStartupCache();
    }
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::UnloadLevel()
{
    gEnv->pCryPak->SetResourceList(AZ::IO::IArchive::RFOM_Level, NULL);

    if (m_bRegisteredFileOpenSink)
    {
        if (g_cvars.archiveVars.nSaveTotalResourceList)
        {
            SaveRecordedResources(true);
            m_recordedFiles.clear();
        }
    }

    stl::free_container(m_sLevelFolder);
    stl::free_container(m_sLevelName);
    stl::free_container(m_currentLevelCacheFolder);

    // should always be empty, since it is freed at the end of
    // the level loading process, if it is not
    // something went wrong and we have a levelheap leak
    assert(m_openedPaks.capacity() == 0);

    m_pSequenceResourceList = NULL;
}

//////////////////////////////////////////////////////////////////////////
AZ::IO::IResourceList* CResourceManager::GetLevelResourceList()
{
    auto pResList = gEnv->pCryPak->GetResourceList(AZ::IO::IArchive::RFOM_Level);
    return pResList;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceManager::LoadLevelCachePak(const char* sPakName, const char* sBindRoot, bool bOnlyDuringLevelLoading)
{
    LOADING_TIME_PROFILE_SECTION;
    CryPathString pakPath = GetCurrentLevelCacheFolder() + "/" + sPakName;

    pakPath.MakeLower();
    pakPath.replace(AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);

    // Check if pak is already loaded
    for (int i = 0; i < (int)m_openedPaks.size(); i++)
    {
        if (strstr(m_openedPaks[i].filename.c_str(), pakPath.c_str()))
        {
            return true;
        }
    }

    // check pak file size.
    size_t nFileSize = gEnv->pCryPak->FGetSize(pakPath.c_str(), true);

    if (nFileSize <= 0)
    {
        // Cached file does not exist
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Level cache pak file %s does not exist", pakPath.c_str());
        return false;
    }

    //set these flags as DLC LevelCache Paks are found via the mod paths,
    //and the paks can never be inside other paks so we optimise the search
    uint32 nOpenPakFlags = AZ::IO::IArchive::FLAGS_FILENAMES_AS_CRC32 | AZ::IO::IArchive::FLAGS_CHECK_MOD_PATHS | AZ::IO::IArchive::FLAGS_NEVER_IN_PAK;

    if (nFileSize < LEVEL_PAK_INMEMORY_MAXSIZE) // 10 megs.
    {
        if (!(nOpenPakFlags & AZ::IO::IArchive::FLAGS_PAK_IN_MEMORY_CPU))
        {
            nOpenPakFlags |= AZ::IO::IArchive::FLAGS_PAK_IN_MEMORY;
        }
    }

    SOpenedPak op;

    if (gEnv->pCryPak->OpenPack(sBindRoot, { pakPath.c_str(), pakPath.size() }, nOpenPakFlags | AZ::IO::IArchive::FOPEN_HINT_QUIET, NULL, &op.filename))
    {
        op.bOnlyDuringLevelLoading = bOnlyDuringLevelLoading;
        m_openedPaks.push_back(op);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceManager::LoadModeSwitchPak(const char* sPakName, const bool multiplayer)
{
    if (g_cvars.archiveVars.nSaveLevelResourceList)
    {
        //Don't load the pak if we're trying to save a resourcelist in order to build it.
        m_recordedFiles.clear();
        gEnv->pCryPak->RegisterFileAccessSink(this);
        m_bRegisteredFileOpenSink = true;
        return true;
    }
    else
    {
        if (g_cvars.archiveVars.nLoadModePaks)
        {
            // Unload SP common pak if switching to multiplayer
            if (multiplayer)
            {
                UnloadMenuCommonPak(FRONTEND_COMMON_PAK_FILENAME_SP, FRONTEND_COMMON_LIST_FILENAME "_sp");
            }
            else
            {
                UnloadMenuCommonPak(FRONTEND_COMMON_PAK_FILENAME_MP, FRONTEND_COMMON_LIST_FILENAME "_mp");
            }
            //Load the mode switching pak. If this is available and up to date it speeds up this process considerably
            bool bOpened = gEnv->pCryPak->OpenPack("@assets@", sPakName, 0);
            bool bLoaded = gEnv->pCryPak->LoadPakToMemory(sPakName, AZ::IO::IArchive::eInMemoryPakLocale_GPU);
            return (bOpened && bLoaded);
        }
        else
        {
            return true;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::UnloadModeSwitchPak(const char* sPakName, const char* sResourceListName, const bool multiplayer)
{
    if (g_cvars.archiveVars.nSaveLevelResourceList && m_bRegisteredFileOpenSink)
    {
        m_sLevelFolder = sResourceListName;
        SaveRecordedResources();
        gEnv->pCryPak->UnregisterFileAccessSink(this);
        m_bRegisteredFileOpenSink = false;
    }
    else
    {
        if (g_cvars.archiveVars.nLoadModePaks)
        {
            //Unload the mode switching pak.
            gEnv->pCryPak->LoadPakToMemory(sPakName, AZ::IO::IArchive::eInMemoryPakLocale_Unload);
            gEnv->pCryPak->ClosePack(sPakName, 0);
            //Load the frontend common mode switch pak, this can considerably reduce the time spent switching especially from disc, currently SP only
            if (!multiplayer && LoadMenuCommonPak(FRONTEND_COMMON_PAK_FILENAME_SP) == false)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Could not load %s during init. This file can significantly reduce frontend loading times.\n", FRONTEND_COMMON_PAK_FILENAME_SP);
            }
            else if (multiplayer && LoadMenuCommonPak(FRONTEND_COMMON_PAK_FILENAME_MP) == false)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Could not load %s during init. This file can significantly reduce frontend loading times.\n", FRONTEND_COMMON_PAK_FILENAME_MP);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CResourceManager::LoadMenuCommonPak(const char* sPakName)
{
    if (g_cvars.archiveVars.nSaveMenuCommonResourceList)
    {
        //Don't load the pak if we're trying to save a resourcelist in order to build it.
        m_recordedFiles.clear();
        gEnv->pCryPak->RegisterFileAccessSink(this);
        m_bRegisteredFileOpenSink = true;
        return true;
    }
    else
    {
        //Load the mode switching pak. If this is available and up to date it speeds up this process considerably
        bool bOpened = gEnv->pCryPak->OpenPack("@assets@", sPakName, 0);
        bool bLoaded = gEnv->pCryPak->LoadPakToMemory(sPakName, AZ::IO::IArchive::eInMemoryPakLocale_GPU);
        return (bOpened && bLoaded);
    }
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::UnloadMenuCommonPak(const char* sPakName, const char* sResourceListName)
{
    if (g_cvars.archiveVars.nSaveMenuCommonResourceList)
    {
        m_sLevelFolder = sResourceListName;
        SaveRecordedResources();
        gEnv->pCryPak->UnregisterFileAccessSink(this);
        m_bRegisteredFileOpenSink = false;
    }
    else
    {
        //Unload the mode switching pak.
        gEnv->pCryPak->LoadPakToMemory(sPakName, AZ::IO::IArchive::eInMemoryPakLocale_Unload);
        gEnv->pCryPak->ClosePack(sPakName, 0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::UnloadLevelCachePak(const char* sPakName)
{
    LOADING_TIME_PROFILE_SECTION;
    CryPathString pakPath = GetCurrentLevelCacheFolder() + "/" + sPakName;
    pakPath.MakeLower();
    pakPath.replace(AZ_WRONG_FILESYSTEM_SEPARATOR, AZ_CORRECT_FILESYSTEM_SEPARATOR);

    for (int i = 0; i < (int)m_openedPaks.size(); i++)
    {
        if (strstr(m_openedPaks[i].filename.c_str(), pakPath.c_str()))
        {
            gEnv->pCryPak->ClosePack(m_openedPaks[i].filename.c_str(), AZ::IO::IArchive::FLAGS_PATH_REAL);
            m_openedPaks.erase(m_openedPaks.begin() + i);
            break;
        }
    }

    if (m_openedPaks.empty())
    {
        stl::free_container(m_openedPaks);
    }
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::UnloadAllLevelCachePaks(bool bLevelLoadEnd)
{
    LOADING_TIME_PROFILE_SECTION;

    if (!bLevelLoadEnd)
    {
        m_AsyncPakManager.Clear();
        UnloadFastLoadPaks();
    }
    else
    {
        m_AsyncPakManager.UnloadLevelLoadPaks();
    }

    uint32 nClosePakFlags = AZ::IO::IArchive::FLAGS_PATH_REAL; //AZ::IO::IArchive::FLAGS_CHECK_MOD_PATHS | AZ::IO::IArchive::FLAGS_NEVER_IN_PAK | AZ::IO::IArchive::FLAGS_PATH_REAL;

    for (int i = 0; i < (int)m_openedPaks.size(); i++)
    {
        if ((m_openedPaks[i].bOnlyDuringLevelLoading && bLevelLoadEnd) ||
            !bLevelLoadEnd)
        {
            gEnv->pCryPak->ClosePack(m_openedPaks[i].filename.c_str(), nClosePakFlags);
        }
    }

    if (g_cvars.archiveVars.nLoadCache)
    {
        gEnv->pCryPak->LoadPakToMemory(ENGINE_PAK_FILENAME, AZ::IO::IArchive::eInMemoryPakLocale_Unload);

        bool usePrefabSystemForLevels = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

        if (!usePrefabSystemForLevels)
        {
            // Force level.pak out of memory.
            gEnv->pCryPak->LoadPakToMemory(LEVEL_PAK_FILENAME, AZ::IO::IArchive::eInMemoryPakLocale_Unload);
        }
    }
    if (!bLevelLoadEnd)
    {
        stl::free_container(m_openedPaks);
    }
}

//////////////////////////////////////////////////////////////////////////

bool CResourceManager::LoadPakToMemAsync(const char* pPath, bool bLevelLoadOnly)
{
    return m_AsyncPakManager.LoadPakToMemAsync(pPath, bLevelLoadOnly);
}

bool CResourceManager::LoadLayerPak(const char* sLayerName)
{
    return m_AsyncPakManager.LoadLayerPak(sLayerName);
}

void CResourceManager::UnloadLayerPak(const char* sLayerName)
{
    m_AsyncPakManager.UnloadLayerPak(sLayerName);
}

void CResourceManager::GetLayerPakStats(SLayerPakStats& stats, bool bCollectAllStats) const
{
    m_AsyncPakManager.GetLayerPakStats(stats, bCollectAllStats);
}

void CResourceManager::UnloadAllAsyncPaks()
{
    m_AsyncPakManager.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::Update()
{
    m_AsyncPakManager.Update();
}
//////////////////////////////////////////////////////////////////////////
void CResourceManager::Init()
{
    GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::Shutdown()
{
    UnloadAllLevelCachePaks(false);
    if (GetISystem() && GetISystem()->GetISystemEventDispatcher())
    {
        GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CResourceManager::IsStreamingCachePak(const char* filename) const
{
    const char* cachePaks[] = {
        "dds_cache.pak",
        "cgf_cache.pak",
        "skin_cache.pak",
        "chr_cache.pak"
    };

    for (int i = 0; i < sizeof(cachePaks) / sizeof(cachePaks[0]); ++i)
    {
        if (strstr(filename, cachePaks[i]))
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_FRONTEND_INITIALISED:
    {
        GetISystem()->GetStreamEngine()->PauseStreaming(false, -1);
    }
    break;

    case ESYSTEM_EVENT_GAME_POST_INIT_DONE:
    {
        if (g_cvars.archiveVars.nSaveFastloadResourceList != 0)
        {
            SaveRecordedResources();

            if (g_cvars.archiveVars.nSaveLevelResourceList == 0 && g_cvars.archiveVars.nSaveTotalResourceList == 0)
            {
                m_recordedFiles.clear();
            }
        }
        // Unload all paks from memory, after game init.
        UnloadAllLevelCachePaks(false);
        gEnv->pCryPak->LoadPaksToMemory(0, false);

        if (g_cvars.archiveVars.nLoadCache)
        {
            //Load the frontend common mode switch pak, this can considerably reduce the time spent switching especially from disc
            if (!gEnv->bMultiplayer && LoadMenuCommonPak(FRONTEND_COMMON_PAK_FILENAME_SP) == false)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Could not load %s during init. This file can significantly reduce frontend loading times.\n", FRONTEND_COMMON_PAK_FILENAME_SP);
            }
        }

        //Load and precache frontend shader cache
        if (g_cvars.archiveVars.nLoadFrontendShaderCache)
        {
            gEnv->pRenderer->LoadShaderLevelCache();
            gEnv->pRenderer->EF_Query(EFQ_SetShaderCombinations);
        }

        break;
    }

    case ESYSTEM_EVENT_LEVEL_LOAD_PREPARE:
    {
        if (g_cvars.archiveVars.nLoadFrontendShaderCache)
        {
            gEnv->pRenderer->UnloadShaderLevelCache();
        }

        if (!gEnv->bMultiplayer)
        {
            UnloadMenuCommonPak(FRONTEND_COMMON_PAK_FILENAME_SP, FRONTEND_COMMON_LIST_FILENAME "_sp");
        }
        else
        {
            UnloadMenuCommonPak(FRONTEND_COMMON_PAK_FILENAME_MP, FRONTEND_COMMON_LIST_FILENAME "_mp");
        }

        m_bLevelTransitioning = !m_sLevelName.empty();

        m_lastLevelLoadTime.SetValue(0);
        m_beginLevelLoadTime = gEnv->pTimer->GetAsyncTime();
        if (g_cvars.archiveVars.nSaveLevelResourceList || g_cvars.archiveVars.nSaveTotalResourceList)
        {
            if (!g_cvars.archiveVars.nSaveTotalResourceList)
            {
                m_recordedFiles.clear();
            }

            if (!m_bRegisteredFileOpenSink)
            {
                gEnv->pCryPak->RegisterFileAccessSink(this);
                m_bRegisteredFileOpenSink = true;
            }
        }

        // Cancel any async pak loading, it will fight with the impending sync IO
        m_AsyncPakManager.CancelPendingJobs();

        // Pause streaming engine for anything but sound, music, video and flash.
        uint32 nMask = (1 << eStreamTaskTypeFlash) | (1 << eStreamTaskTypeVideo) | STREAM_TASK_TYPE_AUDIO_ALL; // Unblock specified streams
        nMask = ~nMask;     // Invert mask, bit set means blocking type.
        GetISystem()->GetStreamEngine()->PauseStreaming(true, nMask);
    }
    break;

    case ESYSTEM_EVENT_LEVEL_LOAD_END:
    {
        if (m_bOwnResourceList)
        {
            m_bOwnResourceList = false;
            // Clear resource list, after level loading.
            auto pResList = gEnv->pCryPak->GetResourceList(AZ::IO::IArchive::RFOM_Level);
            if (pResList)
            {
                pResList->Clear();
            }
        }
    }

    break;

    case ESYSTEM_EVENT_LEVEL_UNLOAD:
        UnloadAllLevelCachePaks(false);

        break;

    case ESYSTEM_EVENT_LEVEL_PRECACHE_START:
    {
        // Unpause all streams in streaming engine.
        GetISystem()->GetStreamEngine()->PauseStreaming(false, -1);
    }
    break;

    case ESYSTEM_EVENT_LEVEL_PRECACHE_FIRST_FRAME:
    {
        UnloadAllLevelCachePaks(true);
    }
    break;

    case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
    {
        CTimeValue t = gEnv->pTimer->GetAsyncTime();
        m_lastLevelLoadTime = t - m_beginLevelLoadTime;

        if (g_cvars.archiveVars.nSaveLevelResourceList && m_bRegisteredFileOpenSink)
        {
            SaveRecordedResources();

            if (!g_cvars.archiveVars.nSaveTotalResourceList)
            {
                gEnv->pCryPak->UnregisterFileAccessSink(this);
                m_bRegisteredFileOpenSink = false;
            }
        }

        UnloadAllLevelCachePaks(true);
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::GetMemoryStatistics(ICrySizer* pSizer)
{
    auto pResList = gEnv->pCryPak->GetResourceList(AZ::IO::IArchive::RFOM_Level);

    pSizer->AddContainer(m_openedPaks);
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::ReportFileOpen([[maybe_unused]] AZ::IO::HandleType inFileHandle, AZStd::string_view szFullPath)
{
    if (!g_cvars.archiveVars.nSaveLevelResourceList && !g_cvars.archiveVars.nSaveFastloadResourceList && !g_cvars.archiveVars.nSaveMenuCommonResourceList && !g_cvars.archiveVars.nSaveTotalResourceList)
    {
        return;
    }

    string file = PathUtil::MakeGamePath(string(szFullPath.data(), szFullPath.size()));
    file.replace('\\', '/');
    file.MakeLower();
    {
        CryAutoCriticalSection lock(recordedFilesLock);
        m_recordedFiles.push_back(file);
    }
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::SaveRecordedResources(bool bTotalList)
{
    CryAutoCriticalSection lock(recordedFilesLock);

    std::set<string> fileset;

    // eliminate duplicate values
    std::vector<string>::iterator endLocation = std::unique(m_recordedFiles.begin(), m_recordedFiles.end());
    m_recordedFiles.erase(endLocation, m_recordedFiles.end());

    fileset.insert(m_recordedFiles.begin(), m_recordedFiles.end());

    string sSequenceFilename = PathUtil::AddSlash(m_sLevelFolder) + (bTotalList ? AUTO_LEVEL_TOTAL_SEQUENCE_RESOURCE_LIST : AUTO_LEVEL_SEQUENCE_RESOURCE_LIST);
    {
        AZ::IO::HandleType fileHandle = fxopen(sSequenceFilename, "wb", true);
        if (fileHandle != AZ::IO::InvalidHandle)
        {
            for (std::vector<string>::iterator it = m_recordedFiles.begin(); it != m_recordedFiles.end(); ++it)
            {
                const char* str = it->c_str();
                AZ::IO::Print(fileHandle, "%s\n", str);
            }
            gEnv->pFileIO->Close(fileHandle);
        }
    }

    string sResourceSetFilename = PathUtil::AddSlash(m_sLevelFolder) + (bTotalList ? AUTO_LEVEL_TOTAL_RESOURCE_LIST : AUTO_LEVEL_RESOURCE_LIST);
    {
        AZ::IO::HandleType fileHandle = fxopen(sResourceSetFilename, "wb", true);
        if (fileHandle != AZ::IO::InvalidHandle)
        {
            for (std::set<string>::iterator it = fileset.begin(); it != fileset.end(); ++it)
            {
                const char* str = it->c_str();
                AZ::IO::Print(fileHandle, "%s\n", str);
            }
            gEnv->pFileIO->Close(fileHandle);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CTimeValue CResourceManager::GetLastLevelLoadTime() const
{
    return m_lastLevelLoadTime;
}
