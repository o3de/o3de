/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// [LYN-2376] Remove the entire file once legacy slice support is removed

#include "CrySystem_precompiled.h"
#include "LevelSystem.h"
#include "IMovieSystem.h"
#include <ILocalizationManager.h>
#include "CryPath.h"

#include <LoadScreenBus.h>
#include <CryCommon/StaticInstance.h>

#include <AzCore/Debug/AssetTracking.h>
#include <AzCore/Time/ITime.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Input/Buses/Requests/InputChannelRequestBus.h>
#include <AzFramework/Spawnable/RootSpawnableInterface.h>

#include "MainThreadRenderRequestBus.h"
#include <LyShine/ILyShine.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzCore/Script/ScriptSystemBus.h>

namespace LegacyLevelSystem
{
static constexpr const char* ArchiveExtension = ".pak";

//////////////////////////////////////////////////////////////////////////
bool CLevelInfo::OpenLevelPak()
{
    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

    // The prefab system doesn't use level.pak
    if (usePrefabSystemForLevels)
    {
        return false;
    }

    AZ::IO::Path levelPak(m_levelPath);
    levelPak /= "level.pak";
    AZ::IO::FixedMaxPathString fullLevelPakPath;
    bool bOk = gEnv->pCryPak->OpenPack(levelPak.Native(), nullptr, &fullLevelPakPath, false);
    m_levelPakFullPath.assign(fullLevelPakPath.c_str(), fullLevelPakPath.size());
    return bOk;
}

//////////////////////////////////////////////////////////////////////////
void CLevelInfo::CloseLevelPak()
{
    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

    // The prefab system doesn't use level.pak
    if (usePrefabSystemForLevels)
    {
        return;
    }

    if (!m_levelPakFullPath.empty())
    {
        gEnv->pCryPak->ClosePack(m_levelPakFullPath.c_str());
        m_levelPakFullPath.clear();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CLevelInfo::ReadInfo()
{
    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

    // Set up a default game type for legacy code.
    m_defaultGameTypeName = "mission0";

    if (usePrefabSystemForLevels)
    {
        return true;
    }


    AZStd::string levelPath(m_levelPath);
    AZStd::string xmlFile(levelPath);
    xmlFile += "/levelinfo.xml";
    XmlNodeRef rootNode = GetISystem()->LoadXmlFromFile(xmlFile.c_str());

    if (rootNode)
    {
        AZStd::string dataFile(levelPath);
        dataFile += "/leveldataaction.xml";
        XmlNodeRef dataNode = GetISystem()->LoadXmlFromFile(dataFile.c_str());
        if (!dataNode)
        {
            dataFile = levelPath + "/leveldata.xml";
            dataNode = GetISystem()->LoadXmlFromFile(dataFile.c_str());
        }

        if (dataNode)
        {
            XmlNodeRef gameTypesNode = dataNode->findChild("Missions");

            if ((gameTypesNode != 0) && (gameTypesNode->getChildCount() > 0))
            {
                m_defaultGameTypeName.clear();

                for (int i = 0; i < gameTypesNode->getChildCount(); i++)
                {
                    XmlNodeRef gameTypeNode = gameTypesNode->getChild(i);

                    if (gameTypeNode->isTag("Mission"))
                    {
                        const char* gameTypeName = gameTypeNode->getAttr("Name");

                        if (gameTypeName)
                        {
                            m_defaultGameTypeName = gameTypeName;
                            break;
                        }
                    }
                }
            }
        }
    }
    return rootNode != 0;
}

//////////////////////////////////////////////////////////////////////////

/// Used by console auto completion.
struct SLevelNameAutoComplete
    : public IConsoleArgumentAutoComplete
{
    AZStd::vector<AZStd::string> levels;
    virtual int GetCount() const { return static_cast<int>(levels.size()); };
    virtual const char* GetValue(int nIndex) const { return levels[nIndex].c_str(); };
};
// definition and declaration must be separated for devirtualization
static StaticInstance<SLevelNameAutoComplete, AZStd::no_destruct<SLevelNameAutoComplete>> g_LevelNameAutoComplete;

//------------------------------------------------------------------------
static void LoadMap(IConsoleCmdArgs* args)
{
    if (gEnv->pSystem && gEnv->pSystem->GetILevelSystem() && !gEnv->IsEditor())
    {
        if (args->GetArgCount() > 1)
        {
            gEnv->pSystem->GetILevelSystem()->UnloadLevel();
            gEnv->pSystem->GetILevelSystem()->LoadLevel(args->GetArg(1));
        }
    }
}

//------------------------------------------------------------------------
static void UnloadMap([[maybe_unused]] IConsoleCmdArgs* args)
{
    if (gEnv->pSystem && gEnv->pSystem->GetILevelSystem() && !gEnv->IsEditor())
    {
        gEnv->pSystem->GetILevelSystem()->UnloadLevel();
    }
}

//------------------------------------------------------------------------
CLevelSystem::CLevelSystem(ISystem* pSystem, const char* levelsFolder)
    : m_pSystem(pSystem)
    , m_pCurrentLevel(0)
    , m_pLoadingLevelInfo(0)
{
    CRY_ASSERT(pSystem);

    //if (!gEnv->IsEditor())
    Rescan(levelsFolder);

    m_fLastLevelLoadTime = 0;
    m_fLastTime = 0;
    m_bLevelLoaded = false;

    m_levelLoadStartTime.SetValue(0);

    m_nLoadedLevelsCount = 0;

    REGISTER_COMMAND("map", LoadMap, VF_BLOCKFRAME, "Load a map");
    REGISTER_COMMAND("unload", UnloadMap, 0, "Unload current map");
    gEnv->pConsole->RegisterAutoComplete("map", &(*g_LevelNameAutoComplete));

    AZ_Assert(gEnv && gEnv->pCryPak, "gEnv and CryPak must be initialized for loading levels.");
    if (!gEnv || !gEnv->pCryPak)
    {
        return;
    }
    auto archive = AZ::Interface<AZ::IO::IArchive>::Get();

    if (AZ::IO::IArchive::LevelPackOpenEvent* levelPakOpenEvent = archive->GetLevelPackOpenEvent())
    {
        m_levelPackOpenHandler = AZ::IO::IArchive::LevelPackOpenEvent::Handler([this](const AZStd::vector<AZ::IO::Path>& levelDirs)
        {
            for (AZ::IO::Path levelDir : levelDirs)
            {
                bool modFolder = false;
                PopulateLevels((levelDir / "*").Native(), levelDir.Native(), AZ::Interface<AZ::IO::IArchive>::Get(), modFolder, false);
            }
        });
        m_levelPackOpenHandler.Connect(*levelPakOpenEvent);
    }

    if (AZ::IO::IArchive::LevelPackCloseEvent* levelPakCloseEvent = archive->GetLevelPackCloseEvent())
    {
        m_levelPackCloseHandler = AZ::IO::IArchive::LevelPackCloseEvent::Handler([this](AZStd::string_view)
        {
            Rescan(ILevelSystem::LevelsDirectoryName);
        });
        m_levelPackCloseHandler.Connect(*levelPakCloseEvent);
    }
}

//------------------------------------------------------------------------
CLevelSystem::~CLevelSystem()
{
    UnloadLevel();
}

//------------------------------------------------------------------------
void CLevelSystem::Rescan(const char* levelsFolder)
{
    if (levelsFolder)
    {
        m_levelsFolder = levelsFolder;
    }

    CRY_ASSERT(!m_levelsFolder.empty());
    m_levelInfos.clear();
    m_levelInfos.reserve(64);
    ScanFolder(0, false);

    g_LevelNameAutoComplete->levels.clear();
    for (int i = 0; i < (int)m_levelInfos.size(); i++)
    {
        g_LevelNameAutoComplete->levels.push_back(AZStd::string(PathUtil::GetFileName(m_levelInfos[i].GetName()).c_str()));
    }
}

//-----------------------------------------------------------------------
void CLevelSystem::ScanFolder(const char* subfolder, bool modFolder)
{
    AZStd::string folder;
    if (subfolder && subfolder[0])
    {
        folder = subfolder;
    }

    AZStd::string search(m_levelsFolder);
    if (!folder.empty())
    {
        if (AZ::StringFunc::StartsWith(folder.c_str(), m_levelsFolder.c_str()))
        {
            search = folder;
        }
        else
        {
            search += "/" + folder;
        }
    }
    search += "/*";

    AZ_Assert(gEnv && gEnv->pCryPak, "gEnv and must be initialized for loading levels.");
    if (!gEnv || !gEnv->pCryPak)
    {
        return;
    }
    auto pPak = gEnv->pCryPak;

    AZStd::unordered_set<AZStd::string> pakList;

    AZ::IO::ArchiveFileIterator handle = pPak->FindFirst(search.c_str(), AZ::IO::FileSearchLocation::OnDisk);

    if (handle)
    {
        do
        {
            AZStd::string extension;
            AZStd::string levelName;
            AZ::StringFunc::Path::Split(handle.m_filename.data(), nullptr, nullptr, &levelName, &extension);
            if (extension == ArchiveExtension)
            {
                if (AZ::StringFunc::Equal(handle.m_filename.data(), LevelPakName))
                {
                    // level folder contain pak files like 'level.pak'
                    // which we only want to load during level loading.
                    continue;
                }

                AZStd::string levelContainerPakPath;
                AZ::StringFunc::Path::Join("@products@", m_levelsFolder.c_str(), levelContainerPakPath);
                if (subfolder && subfolder[0])
                {
                    AZ::StringFunc::Path::Join(levelContainerPakPath.c_str(), subfolder, levelContainerPakPath);
                }
                AZ::StringFunc::Path::Join(levelContainerPakPath.c_str(), handle.m_filename.data(), levelContainerPakPath);
                pakList.emplace(levelContainerPakPath);
                continue;
            }
        } while (handle = pPak->FindNext(handle));

        pPak->FindClose(handle);
    }

    // Open all the available paks found in the levels folder
    for (auto iter = pakList.begin(); iter != pakList.end(); iter++)
    {
        gEnv->pCryPak->OpenPack(iter->c_str(), nullptr, nullptr, false);
    }

    // Levels in bundles now take priority over levels outside of bundles.
    PopulateLevels(search, folder, pPak, modFolder, false);
    // Load levels outside of the bundles to maintain backward compatibility.
    PopulateLevels(search, folder, pPak, modFolder, true);

}

void CLevelSystem::PopulateLevels(
    AZStd::string searchPattern, const AZStd::string& folder, AZ::IO::IArchive* pPak, bool& modFolder, bool fromFileSystemOnly)
{
    // allow this find first to actually touch the file system
    // (causes small overhead but with minimal amount of levels this should only be around 150ms on actual DVD Emu)
    AZ::IO::ArchiveFileIterator handle = pPak->FindFirst(searchPattern.c_str(),
        fromFileSystemOnly ? AZ::IO::FileSearchLocation::OnDisk : AZ::IO::FileSearchLocation::InPak);

    if (handle)
    {
        do
        {
            if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) != AZ::IO::FileDesc::Attribute::Subdirectory ||
                handle.m_filename == "." || handle.m_filename == "..")
            {
                continue;
            }

            AZStd::string levelFolder;
            if (fromFileSystemOnly)
            {
                levelFolder =
                    (folder.empty() ? "" : (folder + "/")) + AZStd::string(handle.m_filename.data(), handle.m_filename.size());
            }
            else
            {
                AZStd::string levelName(AZ::IO::PathView(handle.m_filename).Filename().Native());
                levelFolder = (folder.empty() ? "" : (folder + "/")) + levelName;
            }

            AZStd::string levelPath;
            if (AZ::StringFunc::StartsWith(levelFolder.c_str(), m_levelsFolder.c_str()))
            {
                levelPath = levelFolder;
            }
            else
            {
                levelPath = m_levelsFolder + "/" + levelFolder;
            }

            const AZStd::string levelPakName = levelPath + "/" + LevelPakName;
            const AZStd::string levelInfoName = levelPath + "/levelinfo.xml";

            if (!pPak->IsFileExist(
                levelPakName.c_str(),
                fromFileSystemOnly ? AZ::IO::FileSearchLocation::OnDisk : AZ::IO::FileSearchLocation::InPak) &&
                !pPak->IsFileExist(
                    levelInfoName.c_str(),
                    fromFileSystemOnly ? AZ::IO::FileSearchLocation::OnDisk : AZ::IO::FileSearchLocation::InPak))
            {
                ScanFolder(levelFolder.c_str(), modFolder);
                continue;
            }

            // With the level.pak workflow, levelPath and levelName will point to a directory.
            // levelPath: levels/mylevel
            // levelName: mylevel
            CLevelInfo levelInfo;
            levelInfo.m_levelPath = levelPath;
            levelInfo.m_levelName = levelFolder;
            levelInfo.m_isPak = !fromFileSystemOnly;

            CLevelInfo* pExistingInfo = GetLevelInfoInternal(levelInfo.m_levelName);

            // Don't add the level if it is already in the list
            if (pExistingInfo == NULL)
            {
                m_levelInfos.push_back(levelInfo);
            }
            else
            {
                // Levels in bundles take priority over levels outside bundles.
                if (!pExistingInfo->m_isPak && levelInfo.m_isPak)
                {
                    *pExistingInfo = levelInfo;
                }
            }
        } while (handle = pPak->FindNext(handle));

        pPak->FindClose(handle);
    }
}

//------------------------------------------------------------------------
int CLevelSystem::GetLevelCount()
{
    return (int)m_levelInfos.size();
}

//------------------------------------------------------------------------
ILevelInfo* CLevelSystem::GetLevelInfo(int level)
{
    return GetLevelInfoInternal(level);
}

//------------------------------------------------------------------------
CLevelInfo* CLevelSystem::GetLevelInfoInternal(int level)
{
    if ((level >= 0) && (level < GetLevelCount()))
    {
        return &m_levelInfos[level];
    }

    return 0;
}

//------------------------------------------------------------------------
ILevelInfo* CLevelSystem::GetLevelInfo(const char* levelName)
{
    return GetLevelInfoInternal(levelName);
}

//------------------------------------------------------------------------
CLevelInfo* CLevelSystem::GetLevelInfoInternal(const AZStd::string& levelName)
{
    // If level not found by full name try comparing with only filename
    for (AZStd::vector<CLevelInfo>::iterator it = m_levelInfos.begin(); it != m_levelInfos.end(); ++it)
    {
        if (!azstricmp(it->GetName(), levelName.c_str()))
        {
            return &(*it);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    for (AZStd::vector<CLevelInfo>::iterator it = m_levelInfos.begin(); it != m_levelInfos.end(); ++it)
    {
        {
            if (!azstricmp(PathUtil::GetFileName(it->GetName()).c_str(), levelName.c_str()))
            {
                return &(*it);
            }
        }
    }

    // Try stripping out the folder to find the raw filename
    AZStd::string sLevelName(levelName);
    size_t lastSlash = sLevelName.find_last_of('\\');
    if (lastSlash == AZStd::string::npos)
    {
        lastSlash = sLevelName.find_last_of('/');
    }
    if (lastSlash != AZStd::string::npos)
    {
        sLevelName = sLevelName.substr(lastSlash + 1, sLevelName.size() - lastSlash - 1);
        return GetLevelInfoInternal(sLevelName.c_str());
    }

    return 0;
}

//------------------------------------------------------------------------
void CLevelSystem::AddListener(ILevelSystemListener* pListener)
{
    AZStd::vector<ILevelSystemListener*>::iterator it = AZStd::find(m_listeners.begin(), m_listeners.end(), pListener);

    if (it == m_listeners.end())
    {
        m_listeners.reserve(12);
        m_listeners.push_back(pListener);
    }
}

//------------------------------------------------------------------------
void CLevelSystem::RemoveListener(ILevelSystemListener* pListener)
{
    AZStd::vector<ILevelSystemListener*>::iterator it = AZStd::find(m_listeners.begin(), m_listeners.end(), pListener);

    if (it != m_listeners.end())
    {
        m_listeners.erase(it);

        if (m_listeners.empty())
        {
            m_listeners.shrink_to_fit();
        }
    }
}

//------------------------------------------------------------------------
bool CLevelSystem::LoadLevel(const char* _levelName)
{
    if (gEnv->IsEditor())
    {
        AZ_TracePrintf("CrySystem::CLevelSystem", "LoadLevel for %s was called in the editor - not actually loading.\n", _levelName);
        return false;
    }

    // If a level is currently loaded, unload it before loading the next one.
    if (IsLevelLoaded())
    {
        UnloadLevel();
    }

    gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_PREPARE, 0, 0);
    PrepareNextLevel(_levelName);

    ILevel* level = LoadLevelInternal(_levelName);
    if (level)
    {
        OnLoadingComplete(_levelName);
    }

    return (level != nullptr);
}
//------------------------------------------------------------------------
ILevel* CLevelSystem::LoadLevelInternal(const char* _levelName)
{
    gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START);
    AZ_ASSET_NAMED_SCOPE("Level: %s", _levelName);

    CryLog ("Level system is loading \"%s\"", _levelName);
    INDENT_LOG_DURING_SCOPE();

    char levelName[256];
    azstrcpy(levelName, AZ_ARRAY_SIZE(levelName), _levelName);

    // Not remove a scope!!!
    {
        CLevelInfo* pLevelInfo = GetLevelInfoInternal(levelName);

        if (!pLevelInfo)
        {
            // alert the listener
            OnLevelNotFound(levelName);

            return 0;
        }

        m_bLevelLoaded = false;

        m_lastLevelName = levelName;

        delete m_pCurrentLevel;
        CLevel* pLevel = new CLevel();
        pLevel->m_levelInfo = *pLevelInfo;
        m_pCurrentLevel = pLevel;

        //////////////////////////////////////////////////////////////////////////
        // Read main level info.
        if (!pLevelInfo->ReadInfo())
        {
            OnLoadingError(levelName, "Failed to read level info (level.pak might be corrupted)!");
            return 0;
        }
        //[AlexMcC|19.04.10]: Update the level's LevelInfo
        pLevel->m_levelInfo = *pLevelInfo;
        //////////////////////////////////////////////////////////////////////////

        gEnv->pConsole->SetScrollMax(600);
        ICVar* con_showonload = gEnv->pConsole->GetCVar("con_showonload");
        if (con_showonload && con_showonload->GetIVal() != 0)
        {
            gEnv->pConsole->ShowConsole(true);
            ICVar* g_enableloadingscreen = gEnv->pConsole->GetCVar("g_enableloadingscreen");
            if (g_enableloadingscreen)
            {
                g_enableloadingscreen->Set(0);
            }
        }

        m_pLoadingLevelInfo = pLevelInfo;
        OnLoadingStart(levelName);


        auto pPak = gEnv->pCryPak;

        AZStd::string levelPath(pLevelInfo->GetPath());

        ICVar* pSpamDelay = gEnv->pConsole->GetCVar("log_SpamDelay");
        float spamDelay = 0.0f;
        if (pSpamDelay)
        {
            spamDelay = pSpamDelay->GetFVal();
            pSpamDelay->Set(0.0f);
        }

        {
            AZStd::string missionXml("mission_");
            missionXml += pLevelInfo->m_defaultGameTypeName;
            missionXml += ".xml";
            AZStd::string xmlFile(pLevelInfo->GetPath());
            xmlFile += "/";
            xmlFile += missionXml;

            if (!gEnv->IsEditor())
            {
                AZStd::string entitiesFilename =
                    AZStd::string::format("%s/%s.entities_xml", pLevelInfo->GetPath(), pLevelInfo->m_defaultGameTypeName.c_str());
                AZStd::vector<char> fileBuffer;
                CCryFile entitiesFile;
                if (entitiesFile.Open(entitiesFilename.c_str(), "rt"))
                {
                    fileBuffer.resize(entitiesFile.GetLength());

                    if (fileBuffer.size() == entitiesFile.ReadRaw(fileBuffer.begin(), fileBuffer.size()))
                    {
                        AZ::IO::ByteContainerStream<AZStd::vector<char>> fileStream(&fileBuffer);
                        EBUS_EVENT(AzFramework::GameEntityContextRequestBus, LoadFromStream, fileStream, false);
                    }
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Movie system must be reset after entities.
        //////////////////////////////////////////////////////////////////////////
        IMovieSystem* movieSys = gEnv->pMovieSystem;
        if (movieSys != NULL)
        {
            // bSeekAllToStart needs to be false here as it's only of interest in the editor
            movieSys->Reset(true, false);
        }

        gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_PRECACHE);

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        gEnv->pConsole->SetScrollMax(600 / 2);

        pPak->GetResourceList(AZ::IO::IArchive::RFOM_NextLevel)->Clear();

        if (pSpamDelay)
        {
            pSpamDelay->Set(spamDelay);
        }

        m_bLevelLoaded = true;
        gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_END);
    }

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_END, 0, 0);

    if (auto cvar = gEnv->pConsole->GetCVar("sv_map"); cvar)
    {
        cvar->Set(levelName);
    }

    gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_PRECACHE_START, 0, 0);

    return m_pCurrentLevel;
}

//------------------------------------------------------------------------
void CLevelSystem::PrepareNextLevel(const char* levelName)
{
    CLevelInfo* pLevelInfo = GetLevelInfoInternal(levelName);
    if (!pLevelInfo)
    {
        // alert the listener
        OnLevelNotFound(levelName);
        return;
    }

    // This work not required in-editor.
    if (!gEnv || !gEnv->IsEditor())
    {
        const AZ::TimeMs timeMs = AZ::GetRealElapsedTimeMs();
        const double timeSec = AZ::TimeMsToSecondsDouble(timeMs);
        m_levelLoadStartTime = CTimeValue(timeSec);

        // Open pak file for a new level.
        pLevelInfo->OpenLevelPak();

        // switched to level heap, so now imm start the loading screen (renderer will be reinitialized in the levelheap)
        gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN, 0, 0);
        gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_PREPARE);
    }

    for (AZStd::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnPrepareNextLevel(pLevelInfo->GetName());
    }
}

//------------------------------------------------------------------------
void CLevelSystem::OnLevelNotFound(const char* levelName)
{
    for (AZStd::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnLevelNotFound(levelName);
    }
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingStart(const char* levelName)
{
    if (gEnv->pCryPak->GetRecordFileOpenList() == AZ::IO::IArchive::RFOM_EngineStartup)
    {
        gEnv->pCryPak->RecordFileOpen(AZ::IO::IArchive::RFOM_Level);
    }

    const AZ::TimeMs timeMs = AZ::GetRealElapsedTimeMs();
    m_fLastTime = AZ::TimeMsToSeconds(timeMs);

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_START, 0, 0);

    for (AZStd::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnLoadingStart(levelName);
    }
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingError(const char* levelName, const char* error)
{
    ILevelInfo* pLevelInfo = m_pLoadingLevelInfo;
    if (!pLevelInfo)
    {
        CRY_ASSERT(false);
        return;
    }

    for (AZStd::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnLoadingError(levelName, error);
    }

    ((CLevelInfo*)pLevelInfo)->CloseLevelPak();
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingComplete(const char* levelName)
{
    const AZ::TimeMs timeMs = AZ::GetRealElapsedTimeMs();
    const double timeSec = AZ::TimeMsToSecondsDouble(timeMs);
    const CTimeValue t(timeSec);
    m_fLastLevelLoadTime = (t - m_levelLoadStartTime).GetSeconds();

    LogLoadingTime();

    m_nLoadedLevelsCount++;

    // Hide console after loading.
    gEnv->pConsole->ShowConsole(false);

    for (AZStd::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnLoadingComplete(levelName);
    }

#if AZ_LOADSCREENCOMPONENT_ENABLED
    EBUS_EVENT(LoadScreenBus, Stop);
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingProgress(const char* levelName, int progressAmount)
{
    for (AZStd::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnLoadingProgress(levelName, progressAmount);
    }
}

//------------------------------------------------------------------------
void CLevelSystem::OnUnloadComplete(const char* levelName)
{
    for (AZStd::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnUnloadComplete(levelName);
    }
}

//////////////////////////////////////////////////////////////////////////
void CLevelSystem::LogLoadingTime()
{
    if (gEnv->IsEditor())
    {
        return;
    }

    if (!GetISystem()->IsDevMode())
    {
        return;
    }

    char vers[128];
    GetISystem()->GetFileVersion().ToString(vers, sizeof(vers));

    const char* sChain = "";
    if (m_nLoadedLevelsCount > 0)
    {
        sChain = " (Chained)";
    }

    gEnv->pLog->Log("Game Level Load Time: [%s] Level %s loaded in %.2f seconds%s", vers, m_lastLevelName.c_str(), m_fLastLevelLoadTime, sChain);
}

//////////////////////////////////////////////////////////////////////////
void CLevelSystem::UnloadLevel()
{
    if (gEnv->IsEditor())
    {
        return;
    }
    if (!m_pLoadingLevelInfo)
    {
        return;
    }

    CryLog("UnloadLevel Start");
    INDENT_LOG_DURING_SCOPE();

    // Flush core buses. We're about to unload Cry modules and need to ensure we don't have module-owned functions left behind.
    AZ::Data::AssetBus::ExecuteQueuedEvents();
    AZ::TickBus::ExecuteQueuedEvents();
    AZ::MainThreadRenderRequestBus::ExecuteQueuedEvents();

    if (gEnv && gEnv->pSystem)
    {
        // clear all error messages to prevent stalling due to runtime file access check during chainloading
        gEnv->pSystem->ClearErrorMessages();
    }

    if (gEnv && gEnv->pCryPak)
    {
        gEnv->pCryPak->DisableRuntimeFileAccess(false);
    }

    const AZ::TimeMs beginTimeMs = AZ::GetRealElapsedTimeMs();

    // Clear level entities and prefab instances.
    EBUS_EVENT(AzFramework::GameEntityContextRequestBus, ResetGameContext);

    if (gEnv->pMovieSystem)
    {
        gEnv->pMovieSystem->Reset(false, false);
        gEnv->pMovieSystem->RemoveAllSequences();
    }

    OnUnloadComplete(m_lastLevelName.c_str());

    // -- kenzo: this will close all pack files for this level
    // (even the ones which were not added through here, if this is not desired,
    // then change code to close only level.pak)
    if (m_pLoadingLevelInfo)
    {
        ((CLevelInfo*)m_pLoadingLevelInfo)->CloseLevelPak();
        m_pLoadingLevelInfo = NULL;
    }

    m_lastLevelName.clear();

    SAFE_RELEASE(m_pCurrentLevel);

    // Force Lua garbage collection (may no longer be needed now the legacy renderer has been removed).
    // Normally the GC step is triggered at the end of this method (by the ESYSTEM_EVENT_LEVEL_POST_UNLOAD event).
    EBUS_EVENT(AZ::ScriptSystemRequestBus, GarbageCollect);

    // Perform level unload procedures for the LyShine UI system
    if (gEnv && gEnv->pLyShine)
    {
        gEnv->pLyShine->OnLevelUnload();
    }

    m_bLevelLoaded = false;

    [[maybe_unused]] const AZ::TimeMs unloadTimeMs = AZ::GetRealElapsedTimeMs() - beginTimeMs;
    CryLog("UnloadLevel End: %.1f sec", AZ::TimeMsToSeconds(unloadTimeMs));

    // Must be sent last.
    // Cleanup all containers
    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_POST_UNLOAD, 0, 0);
    AzFramework::InputChannelRequestBus::Broadcast(&AzFramework::InputChannelRequests::ResetState);
}

} // namespace LegacyLevelSystem
