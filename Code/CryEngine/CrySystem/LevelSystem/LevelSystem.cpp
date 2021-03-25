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

#include "CrySystem_precompiled.h"
#include "LevelSystem.h"
#include <IAudioSystem.h>
#include "IMovieSystem.h"
#include "IMaterialEffects.h"
#include <IResourceManager.h>
#include <ILocalizationManager.h>
#include "IDeferredCollisionEvent.h"
#include "CryPath.h"
#include <Pak/CryPakUtils.h>

#include <LoadScreenBus.h>

#include <AzCore/Debug/AssetTracking.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Input/Buses/Requests/InputChannelRequestBus.h>

#include "MainThreadRenderRequestBus.h"
#include <LyShine/ILyShine.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzCore/Script/ScriptSystemBus.h>

#ifdef WIN32
#include <CryWindows.h>
#endif

namespace LegacyLevelSystem
{

int CLevelSystem::s_loadCount = 0;
const char* ArchiveExtension = ".pak";
const char TerrarinTexturePakName[] = "terraintexture.pak";

//------------------------------------------------------------------------
bool CLevelInfo::SupportsGameType(const char* gameTypeName) const
{
    //read level meta data
    for (int i = 0; i < m_gamerules.size(); ++i)
    {
        if (!azstricmp(m_gamerules[i].c_str(), gameTypeName))
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------
const char* CLevelInfo::GetDisplayName() const
{
    return m_levelDisplayName.c_str();
}

//////////////////////////////////////////////////////////////////////////
bool CLevelInfo::ReadInfo()
{
    string levelPath = m_levelPath;
    string xmlFile = levelPath + string("/LevelInfo.xml");
    XmlNodeRef rootNode = GetISystem()->LoadXmlFromFile(xmlFile.c_str());

    if (rootNode)
    {
        m_heightmapSize = atoi(rootNode->getAttr("HeightmapSize"));

        string dataFile = levelPath + string("/LevelDataAction.xml");
        XmlNodeRef dataNode = GetISystem()->LoadXmlFromFile(dataFile.c_str());
        if (!dataNode)
        {
            dataFile = levelPath + string("/LevelData.xml");
            dataNode = GetISystem()->LoadXmlFromFile(dataFile.c_str());
        }

        if (dataNode)
        {
            XmlNodeRef gameTypesNode = dataNode->findChild("Missions");

            if ((gameTypesNode != 0) && (gameTypesNode->getChildCount() > 0))
            {
                m_gameTypes.clear();
                //m_musicLibs.clear();

                for (int i = 0; i < gameTypesNode->getChildCount(); i++)
                {
                    XmlNodeRef gameTypeNode = gameTypesNode->getChild(i);

                    if (gameTypeNode->isTag("Mission"))
                    {
                        const char* gameTypeName = gameTypeNode->getAttr("Name");

                        if (gameTypeName)
                        {
                            ILevelInfo::TGameTypeInfo info;

                            info.cgfCount = 0;
                            gameTypeNode->getAttr("CGFCount", info.cgfCount);
                            info.name = gameTypeNode->getAttr("Name");
                            info.xmlFile = gameTypeNode->getAttr("File");
                            m_gameTypes.push_back(info);
                        }
                    }
                }

                // Gets reintroduced when level specific music data loading is implemented.
                /*XmlNodeRef musicLibraryNode = dataNode->findChild("MusicLibrary");

                if ((musicLibraryNode!=0) && (musicLibraryNode->getChildCount() > 0))
                {
                    for (int i = 0; i < musicLibraryNode->getChildCount(); i++)
                    {
                        XmlNodeRef musicLibrary = musicLibraryNode->getChild(i);

                        if (musicLibrary->isTag("Library"))
                        {
                            const char *musicLibraryName = musicLibrary->getAttr("File");

                            if (musicLibraryName)
                            {
                                m_musicLibs.push_back(string("music/") + musicLibraryName);
                            }
                        }
                    }
                }*/
            }
        }
    }
    return rootNode != 0;
}

//////////////////////////////////////////////////////////////////////////
void CLevelInfo::ReadMetaData()
{
    string fullPath(GetPath());
    int slashPos = fullPath.find_last_of("\\/");
    string mapName = fullPath.substr(slashPos + 1, fullPath.length() - slashPos);
    fullPath.append("/");
    fullPath.append(mapName);
    fullPath.append(".xml");

    m_levelDisplayName = string("@ui_") + mapName;

    if (!gEnv->pCryPak->IsFileExist(fullPath.c_str()))
    {
        return;
    }

    XmlNodeRef mapInfo = GetISystem()->LoadXmlFromFile(fullPath.c_str());
    //retrieve the coordinates of the map
    bool foundMinimapInfo = false;
    if (mapInfo)
    {
        for (int n = 0; n < mapInfo->getChildCount(); ++n)
        {
            XmlNodeRef rulesNode = mapInfo->getChild(n);
            const char* name = rulesNode->getTag();
            if (!azstricmp(name, "Gamerules"))
            {
                for (int a = 0; a < rulesNode->getNumAttributes(); ++a)
                {
                    const char* key, * value;
                    rulesNode->getAttributeByIndex(a, &key, &value);
                    m_gamerules.push_back(value);
                }
            }
            else if (!azstricmp(name, "Display"))
            {
                XmlString v;
                if (rulesNode->getAttr("Name", v))
                {
                    m_levelDisplayName = v.c_str();
                }
            }
            else if (!azstricmp(name, "PreviewImage"))
            {
                const char* pFilename = NULL;
                if (rulesNode->getAttr("Filename", &pFilename))
                {
                    m_previewImagePath = pFilename;
                }
            }
            else if (!azstricmp(name, "BackgroundImage"))
            {
                const char* pFilename = NULL;
                if (rulesNode->getAttr("Filename", &pFilename))
                {
                    m_backgroundImagePath = pFilename;
                }
            }
            else if (!azstricmp(name, "Minimap"))
            {
                foundMinimapInfo = true;

                const char* minimap_dds = "";
                foundMinimapInfo &= rulesNode->getAttr("Filename", &minimap_dds);
                m_minimapImagePath = minimap_dds;
                m_minimapInfo.sMinimapName = GetPath();
                m_minimapInfo.sMinimapName.append("/");
                m_minimapInfo.sMinimapName.append(minimap_dds);

                foundMinimapInfo &= rulesNode->getAttr("startX", m_minimapInfo.fStartX);
                foundMinimapInfo &= rulesNode->getAttr("startY", m_minimapInfo.fStartY);
                foundMinimapInfo &= rulesNode->getAttr("endX", m_minimapInfo.fEndX);
                foundMinimapInfo &= rulesNode->getAttr("endY", m_minimapInfo.fEndY);
                foundMinimapInfo &= rulesNode->getAttr("width", m_minimapInfo.iWidth);
                foundMinimapInfo &= rulesNode->getAttr("height", m_minimapInfo.iHeight);
                m_minimapInfo.fDimX = m_minimapInfo.fEndX - m_minimapInfo.fStartX;
                m_minimapInfo.fDimY = m_minimapInfo.fEndY - m_minimapInfo.fStartY;
                m_minimapInfo.fDimX = m_minimapInfo.fDimX > 0 ? m_minimapInfo.fDimX : 1;
                m_minimapInfo.fDimY = m_minimapInfo.fDimY > 0 ? m_minimapInfo.fDimY : 1;
            }
            else if (!azstricmp(name, "Tag"))
            {
                m_levelTag = ILevelSystem::TAG_UNKNOWN;
                SwapEndian(m_levelTag, eBigEndian);
                const char* pTag = NULL;
                if (rulesNode->getAttr("Value", &pTag))
                {
                    m_levelTag = 0;
                    memcpy(&m_levelTag, pTag, std::min(sizeof(m_levelTag), strlen(pTag)));
                }
            }
            else if (!azstricmp(name, "LevelType"))
            {
                const char* levelType;
                if (rulesNode->getAttr("value", &levelType))
                {
                    m_levelTypeList.push_back(levelType);
                }
            }
        }
        m_bMetaDataRead = true;
    }
    if (!foundMinimapInfo)
    {
        gEnv->pLog->LogWarning("Map %s: Missing or invalid minimap info!", mapName.c_str());
    }
}

//------------------------------------------------------------------------
const ILevelInfo::TGameTypeInfo* CLevelInfo::GetDefaultGameType() const
{
    if (!m_gameTypes.empty())
    {
        return &m_gameTypes[0];
    }

    return 0;
};

/// Used by console auto completion.
struct SLevelNameAutoComplete
    : public IConsoleArgumentAutoComplete
{
    std::vector<string> levels;
    virtual int GetCount() const { return levels.size(); };
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
            gEnv->pSystem->GetILevelSystem()->UnLoadLevel();
            gEnv->pSystem->GetILevelSystem()->LoadLevel(args->GetArg(1));
        }
    }
}

//------------------------------------------------------------------------
static void UnloadMap([[maybe_unused]] IConsoleCmdArgs* args)
{
    if (gEnv->pSystem && gEnv->pSystem->GetILevelSystem() && !gEnv->IsEditor())
    {
        gEnv->pSystem->GetILevelSystem()->UnLoadLevel();
        if (gEnv->p3DEngine)
        {
            gEnv->p3DEngine->LoadEmptyLevel();
        }
    }
}

//------------------------------------------------------------------------
CLevelSystem::CLevelSystem(ISystem* pSystem, const char* levelsFolder)
    : m_pSystem(pSystem)
    , m_pCurrentLevel(0)
    , m_pLoadingLevelInfo(0)
{
    LOADING_TIME_PROFILE_SECTION;
    CRY_ASSERT(pSystem);

    //Load user defined level types
    if (XmlNodeRef levelTypeNode = m_pSystem->LoadXmlFromFile("Libs/Levels/leveltypes.xml"))
    {
        for (unsigned int i = 0; i < levelTypeNode->getChildCount(); ++i)
        {
            XmlNodeRef child = levelTypeNode->getChild(i);
            const char* levelType;

            if (child->getAttr("value", &levelType))
            {
                m_levelTypeList.push_back(string(levelType));
            }
        }
    }

    //if (!gEnv->IsEditor())
    Rescan(levelsFolder, ILevelSystem::TAG_MAIN);

    m_fLastLevelLoadTime = 0;
    m_fFilteredProgress = 0;
    m_fLastTime = 0;
    m_bLevelLoaded = false;
    m_bRecordingFileOpens = false;

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
    auto pPak = gEnv->pCryPak;

    if (AZ::IO::IArchive::LevelPackOpenEvent* levelPakOpenEvent = pPak->GetLevelPackOpenEvent())
    {
        m_levelPackOpenHandler = AZ::IO::IArchive::LevelPackOpenEvent::Handler([this](const AZStd::vector<AZStd::string>& levelDirs)
        {
            for (AZStd::string dir : levelDirs)
            {
                AZ::StringFunc::Path::StripComponent(dir, true);
                string searchPattern = string(dir.c_str(), dir.size()) + AZ_FILESYSTEM_SEPARATOR_WILDCARD;
                bool modFolder = false;
                string rootFolder(dir.c_str(), dir.size());
                PopulateLevels(searchPattern, rootFolder, gEnv->pCryPak, modFolder, ILevelSystem::TAG_MAIN, false);
            }
        });
        m_levelPackOpenHandler.Connect(*levelPakOpenEvent);
    }

    if (AZ::IO::IArchive::LevelPackCloseEvent* levelPakCloseEvent = pPak->GetLevelPackCloseEvent())
    {
        m_levelPackCloseHandler = AZ::IO::IArchive::LevelPackCloseEvent::Handler([this](AZStd::string_view)
        {
            Rescan(ILevelSystem::LevelsDirectoryName, ILevelSystem::TAG_MAIN);
        });
        m_levelPackCloseHandler.Connect(*levelPakCloseEvent);
    }
}

//------------------------------------------------------------------------
CLevelSystem::~CLevelSystem()
{
}

//------------------------------------------------------------------------
void CLevelSystem::Rescan(const char* levelsFolder, const uint32 tag)
{
    if (levelsFolder)
    {
        if (const ICmdLineArg* pModArg = m_pSystem->GetICmdLine()->FindArg(eCLAT_Pre, "MOD"))
        {
            if (m_pSystem->IsMODValid(pModArg->GetValue()))
            {
                m_levelsFolder.Format("Mods/%s/%s", pModArg->GetValue(), levelsFolder);
                m_levelInfos.clear();
                ScanFolder(0, true, tag);
            }
        }

        m_levelsFolder = levelsFolder;
    }

    CRY_ASSERT(!m_levelsFolder.empty());
    m_levelInfos.clear();
    m_levelInfos.reserve(64);
    ScanFolder(0, false, tag);

    g_LevelNameAutoComplete->levels.clear();
    for (int i = 0; i < (int)m_levelInfos.size(); i++)
    {
        g_LevelNameAutoComplete->levels.push_back(PathUtil::GetFileName(m_levelInfos[i].GetName()));
    }
}

//-----------------------------------------------------------------------
void CLevelSystem::ScanFolder(const char* subfolder, bool modFolder, const uint32 tag)
{
    string folder;
    if (subfolder && subfolder[0])
    {
        folder = subfolder;
    }

    string search(m_levelsFolder);
    if (!folder.empty())
    {
        if (AZ::StringFunc::StartsWith(folder.c_str(), m_levelsFolder.c_str()))
        {
            search = folder;
        }
        else
        {
            search += string("/") + folder;
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

    bool allowFileSystem = true;
    AZ::IO::ArchiveFileIterator handle = pPak->FindFirst(search.c_str(), 0, allowFileSystem);

    if (handle)
    {
        do
        {
            AZStd::string extension;
            AZStd::string levelName;
            AZ::StringFunc::Path::Split(handle.m_filename.data(), nullptr, nullptr, &levelName, &extension);
            if (extension == ArchiveExtension)
            {
                if (AZ::StringFunc::Equal(handle.m_filename.data(), ILevelSystem::LevelPakName) || AZ::StringFunc::Equal(handle.m_filename.data(), TerrarinTexturePakName))
                {
                    // level folder contain pak files like 'level.pak' and 'terraintexture.pak'
                    // which we only want to load during level loading.
                    continue;
                }
                AZStd::string levelContainerPakPath;
                AZ::StringFunc::Path::Join("@assets@", m_levelsFolder.c_str(), levelContainerPakPath);
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
        AZStd::fixed_string<AZ::IO::IArchive::MaxPath> fullLevelPakPath;
        gEnv->pCryPak->OpenPack(iter->c_str(), (unsigned)0, nullptr, &fullLevelPakPath, false);
    }

    // Levels in bundles now take priority over levels outside of bundles.
    PopulateLevels(search, folder, pPak, modFolder, tag, false);
    // Load levels outside of the bundles to maintain backward compatibility.
    PopulateLevels(search, folder, pPak, modFolder, tag, true);
      
}

void CLevelSystem::PopulateLevels(string searchPattern, string& folder, AZ::IO::IArchive* pPak, bool& modFolder, const uint32& tag, bool fromFileSystemOnly)
{
    // allow this find first to actually touch the file system
    // (causes small overhead but with minimal amount of levels this should only be around 150ms on actual DVD Emu)
    AZ::IO::ArchiveFileIterator handle = pPak->FindFirst(searchPattern.c_str(), 0, fromFileSystemOnly);

    if (handle)
    {
        do
        {
            if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) != AZ::IO::FileDesc::Attribute::Subdirectory || handle.m_filename == "." || handle.m_filename == "..")
            {
                continue;
            }

            string levelFolder;
            if (fromFileSystemOnly)
            {
                levelFolder = (folder.empty() ? "" : (folder + "/")) + string(handle.m_filename.data(), handle.m_filename.size());
            }
            else
            {
                AZStd::string levelName = AZ::IO::PathView(handle.m_filename).Filename().Native();
                levelFolder = (folder.empty() ? "" : (folder + "/")) + string(levelName.c_str());
            }

            string levelPath;
            if (AZ::StringFunc::StartsWith(levelFolder.c_str(), m_levelsFolder.c_str()))
            {
                levelPath = levelFolder;
            }
            else
            {
                levelPath = m_levelsFolder + "/" + levelFolder;
            }
            string paks = levelPath + string("/*.pak");

            const string levelPakName = levelPath + "/level.pak";
            const string levelInfoName = levelPath + "/levelinfo.xml";

            if (!pPak->IsFileExist(levelPakName.c_str(), fromFileSystemOnly ? AZ::IO::IArchive::eFileLocation_OnDisk : AZ::IO::IArchive::eFileLocation_InPak) && !pPak->IsFileExist(levelInfoName.c_str(), fromFileSystemOnly ? AZ::IO::IArchive::eFileLocation_OnDisk : AZ::IO::IArchive::eFileLocation_InPak))
            {
                ScanFolder(levelFolder.c_str(), modFolder, tag);
                continue;
            }

            CLevelInfo levelInfo;
            levelInfo.m_levelPath = levelPath;
            levelInfo.m_levelPaks = paks;
            levelInfo.m_levelName = levelFolder;
            levelInfo.m_levelName = UnifyName(levelInfo.m_levelName);
            levelInfo.m_isModLevel = modFolder;
            levelInfo.m_scanTag = tag;
            levelInfo.m_levelTag = ILevelSystem::TAG_UNKNOWN;
            levelInfo.m_isPak = !fromFileSystemOnly;

            SwapEndian(levelInfo.m_scanTag, eBigEndian);
            SwapEndian(levelInfo.m_levelTag, eBigEndian);

            CLevelInfo* pExistingInfo = GetLevelInfoInternal(levelInfo.m_levelName);
            if (pExistingInfo && pExistingInfo->MetadataLoaded() == false)
            {
                //Reload metadata if it failed to load
                pExistingInfo->ReadMetaData();
            }

            // Don't add the level if it is already in the list
            if (pExistingInfo == NULL)
            {
                levelInfo.ReadMetaData();

                m_levelInfos.push_back(levelInfo);
            }
            else
            {
                // Levels in bundles take priority over levels outside bundles.
                if (!pExistingInfo->m_isPak && levelInfo.m_isPak)
                {
                    *pExistingInfo = levelInfo;
                }
                else
                {
                    // Update the scan tag
                    pExistingInfo->m_scanTag = tag;
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
CLevelInfo* CLevelSystem::GetLevelInfoInternal(const char* levelName)
{
    // If level not found by full name try comparing with only filename
    for (std::vector<CLevelInfo>::iterator it = m_levelInfos.begin(); it != m_levelInfos.end(); ++it)
    {
        if (!azstricmp(it->GetName(), levelName))
        {
            return &(*it);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    for (std::vector<CLevelInfo>::iterator it = m_levelInfos.begin(); it != m_levelInfos.end(); ++it)
    {
        if (!azstricmp(PathUtil::GetFileName(it->GetName()), levelName))
        {
            return &(*it);
        }
    }

    // Try stripping out the folder to find the raw filename
    string sLevelName(levelName);
    size_t lastSlash = sLevelName.find_last_of('\\');
    if (lastSlash == string::npos)
    {
        lastSlash = sLevelName.find_last_of('/');
    }
    if (lastSlash != string::npos)
    {
        sLevelName = sLevelName.substr(lastSlash + 1, sLevelName.size() - lastSlash - 1);
        return GetLevelInfoInternal(sLevelName.c_str());
    }

    return 0;
}

//------------------------------------------------------------------------
void CLevelSystem::AddListener(ILevelSystemListener* pListener)
{
    std::vector<ILevelSystemListener*>::iterator it = std::find(m_listeners.begin(), m_listeners.end(), pListener);

    if (it == m_listeners.end())
    {
        m_listeners.reserve(12);
        m_listeners.push_back(pListener);
    }
}

//------------------------------------------------------------------------
void CLevelSystem::RemoveListener(ILevelSystemListener* pListener)
{
    std::vector<ILevelSystemListener*>::iterator it = std::find(m_listeners.begin(), m_listeners.end(), pListener);

    if (it != m_listeners.end())
    {
        m_listeners.erase(it);

        if (m_listeners.empty())
        {
            stl::free_container(m_listeners);
        }
    }
}

//------------------------------------------------------------------------
ILevel* CLevelSystem::LoadLevel(const char* _levelName)
{
    if (gEnv->IsEditor())
    {
        AZ_TracePrintf("CrySystem::CLevelSystem", "LoadLevel for %s was called in the editor - not actually loading.\n", _levelName);
        return nullptr;
    }

    gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_PREPARE, 0, 0);
    PrepareNextLevel(_levelName);

    ILevel* level = LoadLevelInternal(_levelName);
    if (level)
    {
        OnLoadingComplete(level);
    }

    return level;
}
//------------------------------------------------------------------------
ILevel* CLevelSystem::LoadLevelInternal(const char* _levelName)
{
    gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START);
    AZ_ASSET_NAMED_SCOPE("Level: %s", _levelName);

    CryLog ("Level system is loading \"%s\"", _levelName);
    INDENT_LOG_DURING_SCOPE();

    char levelName[256];
    cry_strcpy(levelName, _levelName);

    // Not remove a scope!!!
    {
        LOADING_TIME_PROFILE_SECTION;

        //m_levelLoadStartTime = gEnv->pTimer->GetAsyncTime();

        CLevelInfo* pLevelInfo = GetLevelInfoInternal(levelName);

        if (!pLevelInfo)
        {
            // alert the listener
            OnLevelNotFound(levelName);

            return 0;
        }

        m_bLevelLoaded = false;

        const bool bLoadingSameLevel = m_lastLevelName.compareNoCase(levelName) == 0;
        m_lastLevelName = levelName;

        delete m_pCurrentLevel;
        CLevel* pLevel = new CLevel();
        pLevel->m_levelInfo = *pLevelInfo;
        m_pCurrentLevel = pLevel;

        //////////////////////////////////////////////////////////////////////////
        // Read main level info.
        if (!pLevelInfo->ReadInfo())
        {
            OnLoadingError(pLevelInfo, "Failed to read level info (level.pak might be corrupted)!");
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

        // Reset the camera to (1,1,1) (not (0,0,0) which is the invalid/uninitialised state,
        // to avoid the hack in the renderer to not show anything if the camera is at the origin).
        CCamera defaultCam;
        defaultCam.SetPosition(Vec3(1.0f));
        m_pSystem->SetViewCamera(defaultCam);

        m_pLoadingLevelInfo = pLevelInfo;
        OnLoadingStart(pLevelInfo);


        auto pPak = gEnv->pCryPak;

        string levelPath = pLevelInfo->GetPath();

        /*
        ICVar *pFileCache = gEnv->pConsole->GetCVar("sys_FileCache");       CRY_ASSERT(pFileCache);

        if(pFileCache->GetIVal())
        {
        if(pPak->OpenPack("",pLevelInfo->GetPath()+string("/FileCache.dat")))
        gEnv->pLog->Log("FileCache.dat loaded");
        else
        gEnv->pLog->Log("FileCache.dat not loaded");
        }
        */

        m_pSystem->SetThreadState(ESubsys_Physics, false);

        ICVar* pSpamDelay = gEnv->pConsole->GetCVar("log_SpamDelay");
        float spamDelay = 0.0f;
        if (pSpamDelay)
        {
            spamDelay = pSpamDelay->GetFVal();
            pSpamDelay->Set(0.0f);
        }

        if (gEnv->p3DEngine)
        {
            bool is3DEngineLoaded = gEnv->IsEditor() ? gEnv->p3DEngine->InitLevelForEditor(pLevelInfo->GetPath(), pLevelInfo->GetDefaultGameType()->name)
                : gEnv->p3DEngine->LoadLevel(pLevelInfo->GetPath(), pLevelInfo->GetDefaultGameType()->name);
            if (!is3DEngineLoaded)
            {
                OnLoadingError(pLevelInfo, "3DEngine failed to handle loading the level");

                return 0;
            }
        }

        // Parse level specific config data.
        string const sLevelNameOnly = PathUtil::GetFileName(levelName);

        if (!sLevelNameOnly.empty() && sLevelNameOnly.compareNoCase("Untitled") != 0)
        {
            const char* controlsPath = nullptr;
            Audio::AudioSystemRequestBus::BroadcastResult(controlsPath, &Audio::AudioSystemRequestBus::Events::GetControlsPath);
            if (controlsPath)
            {
                AZStd::string sAudioLevelPath(controlsPath);
                sAudioLevelPath.append("levels/");
                sAudioLevelPath += sLevelNameOnly;

                Audio::SAudioManagerRequestData<Audio::eAMRT_PARSE_CONTROLS_DATA> oAMData(sAudioLevelPath.c_str(), Audio::eADS_LEVEL_SPECIFIC);
                Audio::SAudioRequest oAudioRequestData;
                oAudioRequestData.nFlags = (Audio::eARF_PRIORITY_HIGH | Audio::eARF_EXECUTE_BLOCKING); // Needs to be blocking so data is available for next preloading request!
                oAudioRequestData.pData = &oAMData;
                Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

                Audio::SAudioManagerRequestData<Audio::eAMRT_PARSE_PRELOADS_DATA> oAMData2(sAudioLevelPath.c_str(), Audio::eADS_LEVEL_SPECIFIC);
                oAudioRequestData.pData = &oAMData2;
                Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

                Audio::TAudioPreloadRequestID nPreloadRequestID = INVALID_AUDIO_PRELOAD_REQUEST_ID;

                Audio::AudioSystemRequestBus::BroadcastResult(nPreloadRequestID, &Audio::AudioSystemRequestBus::Events::GetAudioPreloadRequestID, sLevelNameOnly.c_str());
                if (nPreloadRequestID != INVALID_AUDIO_PRELOAD_REQUEST_ID)
                {
                    Audio::SAudioManagerRequestData<Audio::eAMRT_PRELOAD_SINGLE_REQUEST> requestData(nPreloadRequestID, true);
                    oAudioRequestData.pData = &requestData;
                    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);
                }
            }
        }

        string missionXml = pLevelInfo->GetDefaultGameType()->xmlFile;
        string xmlFile = string(pLevelInfo->GetPath()) + "/" + missionXml;

        if (!gEnv->IsEditor())
        {
            AZStd::string entitiesFilename = AZStd::string::format("%s/%s.entities_xml", pLevelInfo->GetPath(), pLevelInfo->GetDefaultGameType()->name.c_str());
            AZStd::vector<char> fileBuffer;
            CCryFile entitiesFile;
            if (entitiesFile.Open(entitiesFilename.c_str(), "rt"))
            {
                fileBuffer.resize(entitiesFile.GetLength());

                if (fileBuffer.size() == entitiesFile.ReadRaw(fileBuffer.begin(), fileBuffer.size()))
                {
                    AZ::IO::ByteContainerStream<AZStd::vector<char> > fileStream(&fileBuffer);
                    EBUS_EVENT(AzFramework::GameEntityContextRequestBus, LoadFromStream, fileStream, false);
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
        // Notify 3D engine that loading finished
        //////////////////////////////////////////////////////////////////////////
        if (gEnv->p3DEngine)
        {
            gEnv->p3DEngine->PostLoadLevel();
        }

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

    m_pSystem->SetThreadState(ESubsys_Physics, true);

    return m_pCurrentLevel;
}

//------------------------------------------------------------------------
ILevel* CLevelSystem::SetEditorLoadedLevel(const char* levelName, bool bReadLevelInfoMetaData)
{
    CLevelInfo* pLevelInfo = GetLevelInfoInternal(levelName);

    if (!pLevelInfo)
    {
        gEnv->pLog->LogError("Failed to get level info for level %s!", levelName);
        return 0;
    }

    if (bReadLevelInfoMetaData)
    {
        pLevelInfo->ReadMetaData();
    }

    m_lastLevelName = levelName;

    SAFE_DELETE(m_pCurrentLevel);
    CLevel* pLevel = new CLevel();
    pLevel->m_levelInfo = *pLevelInfo;
    m_pCurrentLevel = pLevel;
    m_bLevelLoaded = true;

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
        m_levelLoadStartTime = gEnv->pTimer->GetAsyncTime();

        // Open pak file for a new level.
        pLevelInfo->OpenLevelPak();

        // switched to level heap, so now imm start the loading screen (renderer will be reinitialized in the levelheap)
        gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN, (UINT_PTR)pLevelInfo, 0);
        gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_PREPARE);

        // Inform resource manager about loading of the new level.
        GetISystem()->GetIResourceManager()->PrepareLevel(pLevelInfo->GetPath(), pLevelInfo->GetName());
    }

    for (std::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnPrepareNextLevel(pLevelInfo);
    }
}

//------------------------------------------------------------------------
void CLevelSystem::OnLevelNotFound(const char* levelName)
{
    for (std::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnLevelNotFound(levelName);
    }
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingStart(ILevelInfo* pLevelInfo)
{
    if (gEnv->pCryPak->GetRecordFileOpenList() == AZ::IO::IArchive::RFOM_EngineStartup)
    {
        gEnv->pCryPak->RecordFileOpen(AZ::IO::IArchive::RFOM_Level);
    }

    m_fFilteredProgress = 0.f;
    m_fLastTime = gEnv->pTimer->GetAsyncCurTime();

    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_START, 0, 0);

    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    for (std::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnLoadingStart(pLevelInfo);
    }
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingError(ILevelInfo* pLevelInfo, const char* error)
{
    if (!pLevelInfo)
    {
        pLevelInfo = m_pLoadingLevelInfo;
    }
    if (!pLevelInfo)
    {
        CRY_ASSERT(false);
        return;
    }

    if (gEnv->pRenderer)
    {
        gEnv->pRenderer->SetTexturePrecaching(false);
    }

    for (std::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnLoadingError(pLevelInfo, error);
    }

    ((CLevelInfo*)pLevelInfo)->CloseLevelPak();
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingComplete(ILevel* pLevelInfo)
{
    if (m_bRecordingFileOpens)
    {
        // Stop recoding file opens.
        gEnv->pCryPak->RecordFileOpen(AZ::IO::IArchive::RFOM_Disabled);
        // Save recorded list.
        SaveOpenedFilesList();
    }

    CTimeValue t = gEnv->pTimer->GetAsyncTime();
    m_fLastLevelLoadTime = (t - m_levelLoadStartTime).GetSeconds();

    LogLoadingTime();

    m_nLoadedLevelsCount++;

    // Hide console after loading.
    gEnv->pConsole->ShowConsole(false);

    for (std::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnLoadingComplete(pLevelInfo);
    }

#if AZ_LOADSCREENCOMPONENT_ENABLED
    EBUS_EVENT(LoadScreenBus, Stop);
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
}

//------------------------------------------------------------------------
void CLevelSystem::OnLoadingProgress(ILevelInfo* pLevel, int progressAmount)
{
    for (std::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnLoadingProgress(pLevel, progressAmount);
    }
}

//------------------------------------------------------------------------
void CLevelSystem::OnUnloadComplete(ILevel* pLevel)
{
    for (std::vector<ILevelSystemListener*>::const_iterator it = m_listeners.begin(); it != m_listeners.end(); ++it)
    {
        (*it)->OnUnloadComplete(pLevel);
    }
}

//------------------------------------------------------------------------
string& CLevelSystem::UnifyName(string& name)
{
    //name.MakeLower();
    name.replace('\\', '/');

    return name;
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

    string text;
    text.Format("Game Level Load Time: [%s] Level %s loaded in %.2f seconds%s", vers, m_lastLevelName.c_str(), m_fLastLevelLoadTime, sChain);
    gEnv->pLog->Log(text.c_str());
}

void CLevelSystem::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(m_levelInfos);
    pSizer->AddObject(m_levelsFolder);
    pSizer->AddObject(m_listeners);
}

void CLevelInfo::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(m_levelName);
    pSizer->AddObject(m_levelPath);
    pSizer->AddObject(m_levelPaks);
    //pSizer->AddObject(m_musicLibs);
    pSizer->AddObject(m_gamerules);
    pSizer->AddObject(m_gameTypes);
}


//////////////////////////////////////////////////////////////////////////
bool CLevelInfo::OpenLevelPak()
{
    LOADING_TIME_PROFILE_SECTION;

    string levelpak = m_levelPath + string("/level.pak");
    AZStd::fixed_string<AZ::IO::IArchive::MaxPath> fullLevelPakPath;
    bool bOk = gEnv->pCryPak->OpenPack(levelpak.c_str(), m_isPak ? AZ::IO::IArchive::FLAGS_LEVEL_PAK_INSIDE_PAK : (unsigned)0, NULL, &fullLevelPakPath, false);
    m_levelPakFullPath.assign(fullLevelPakPath.c_str());
    return bOk;
}

//////////////////////////////////////////////////////////////////////////
void CLevelInfo::CloseLevelPak()
{
    LOADING_TIME_PROFILE_SECTION;
    if (!m_levelPakFullPath.empty())
    {
        gEnv->pCryPak->ClosePack(m_levelPakFullPath.c_str(), AZ::IO::IArchive::FLAGS_PATH_REAL);
        stl::free_container(m_levelPakFullPath);
    }
}

const bool CLevelInfo::IsOfType(const char* sType) const
{
    for (unsigned int i = 0; i < m_levelTypeList.size(); ++i)
    {
        if (strcmp(m_levelTypeList[i], sType) == 0)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CLevelSystem::SaveOpenedFilesList()
{
    if (!m_pLoadingLevelInfo)
    {
        return;
    }

    // Write resource list to file.
    string filename = PathUtil::Make(m_pLoadingLevelInfo->GetPath(), "resourcelist.txt");
    AZ::IO::HandleType fileHandle = fxopen(filename.c_str(), "wt", true);
    if (fileHandle != AZ::IO::InvalidHandle)
    {
        auto pResList = gEnv->pCryPak->GetResourceList(AZ::IO::IArchive::RFOM_Level);
        for (const char* fname = pResList->GetFirst(); fname; fname = pResList->GetNext())
        {
            AZ::IO::Print(fileHandle, "%s\n", fname);
        }
        gEnv->pFileIO->Close(fileHandle);
    }
}

//////////////////////////////////////////////////////////////////////////
void CLevelSystem::UnLoadLevel()
{
    if (gEnv->IsEditor())
    {
        return;
    }
    if (!m_pLoadingLevelInfo)
    {
        return;
    }

    CryLog("UnLoadLevel Start");
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

    CTimeValue tBegin = gEnv->pTimer->GetAsyncTime();

    I3DEngine* p3DEngine = gEnv->p3DEngine;
    if (p3DEngine)
    {
        IDeferredPhysicsEventManager* pPhysEventManager = p3DEngine->GetDeferredPhysicsEventManager();
        if (pPhysEventManager)
        {
            // clear deferred physics queues before renderer, since we could have jobs running
            // which access a rendermesh
            pPhysEventManager->ClearDeferredEvents();
        }
    }

    //AM: Flush render thread (Flush is not exposed - using EndFrame())
    //We are about to delete resources that could be in use
    if (gEnv->pRenderer)
    {
        gEnv->pRenderer->EndFrame();


        bool isLoadScreenPlaying = false;
#if AZ_LOADSCREENCOMPONENT_ENABLED
        LoadScreenBus::BroadcastResult(isLoadScreenPlaying, &LoadScreenBus::Events::IsPlaying);        
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED

        // force a black screen as last render command. 
        //if load screen is playing do not call this draw as it may lead to a crash due to UI loading code getting
        //pumped while loading the shaders for this draw.
        if (!isLoadScreenPlaying)
        {
            gEnv->pRenderer->BeginFrame();
            gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
            gEnv->pRenderer->Draw2dImage(0, 0, 800, 600, -1, 0.0f, 0.0f, 1.0f, 1.0f, 0.f, 0.0f, 0.0f, 0.0f, 1.0, 0.f);
            gEnv->pRenderer->EndFrame();
        }

        //flush any outstanding texture requests
        gEnv->pRenderer->FlushPendingTextureTasks();
    }

    // Clear level entities and prefab instances.
    EBUS_EVENT(AzFramework::GameEntityContextRequestBus, ResetGameContext);

    if (gEnv->pMovieSystem)
    {
        gEnv->pMovieSystem->Reset(false, false);
        gEnv->pMovieSystem->RemoveAllSequences();
    }

    // Unload level specific audio binary data.
    Audio::SAudioManagerRequestData<Audio::eAMRT_UNLOAD_AFCM_DATA_BY_SCOPE> oAMData(Audio::eADS_LEVEL_SPECIFIC);
    Audio::SAudioRequest oAudioRequestData;
    oAudioRequestData.nFlags = (Audio::eARF_PRIORITY_HIGH | Audio::eARF_EXECUTE_BLOCKING);
    oAudioRequestData.pData = &oAMData;
    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

    // Now unload level specific audio config data.
    Audio::SAudioManagerRequestData<Audio::eAMRT_CLEAR_CONTROLS_DATA> oAMData2(Audio::eADS_LEVEL_SPECIFIC);
    oAudioRequestData.pData = &oAMData2;
    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

    Audio::SAudioManagerRequestData<Audio::eAMRT_CLEAR_PRELOADS_DATA> oAMData3(Audio::eADS_LEVEL_SPECIFIC);
    oAudioRequestData.pData = &oAMData3;
    Audio::AudioSystemRequestBus::Broadcast(&Audio::AudioSystemRequestBus::Events::PushRequestBlocking, oAudioRequestData);

    // Reset the camera to (0,0,0) which is the invalid/uninitialised state
    CCamera defaultCam;
    m_pSystem->SetViewCamera(defaultCam);

    OnUnloadComplete(m_pCurrentLevel);

    // -- kenzo: this will close all pack files for this level
    // (even the ones which were not added through here, if this is not desired,
    // then change code to close only level.pak)
    if (m_pLoadingLevelInfo)
    {
        ((CLevelInfo*)m_pLoadingLevelInfo)->CloseLevelPak();
        m_pLoadingLevelInfo = NULL;
    }

    stl::free_container(m_lastLevelName);

    GetISystem()->GetIResourceManager()->UnloadLevel();

    SAFE_RELEASE(m_pCurrentLevel);
    
    /*
        Force Lua garbage collection before p3DEngine->UnloadLevel() and pRenderer->FreeResources(flags) are called.
        p3DEngine->UnloadLevel() will destroy particle emitters even if they're still referenced by Lua objects that are yet to be collected.
        (as per comment in 3dEngineLoad.cpp (line 501) - "Force to clean all particles that are left, even if still referenced.").
        Then, during the next GC cycle, Lua finally cleans up, the particle emitter smart pointers will be pointing to invalid memory).
        Normally the GC step is triggered at the end of this method (by the ESYSTEM_EVENT_LEVEL_POST_UNLOAD event), which is too late
        (after the render resources have been purged).
        This extra GC step takes a few ms more level unload time, which is a small price for fixing nasty crashes.
        If, however, we wanted to claim it back, we could potentially get rid of the GC step that is triggered by ESYSTEM_EVENT_LEVEL_POST_UNLOAD to break even.
    */

    EBUS_EVENT(AZ::ScriptSystemRequestBus, GarbageCollect);

    // Delete engine resources
    if (p3DEngine)
    {
        p3DEngine->UnloadLevel();

    }
    // Force to clean render resources left after deleting all objects and materials.
    IRenderer* pRenderer = gEnv->pRenderer;
    if (pRenderer)
    {
        pRenderer->FlushRTCommands(true, true, true);

        CryComment("Deleting Render meshes, render resources and flush texture streaming");
        // This may also release some of the materials.
        int flags = FRR_DELETED_MESHES | FRR_FLUSH_TEXTURESTREAMING | FRR_OBJECTS | FRR_RENDERELEMENTS | FRR_RP_BUFFERS | FRR_POST_EFFECTS;

        // Always keep the system resources around in the editor.
        // If a level load fails for any reason, then do not unload the system resources, otherwise we will not have any system resources to continue rendering the console and debug output text.
        if (!gEnv->IsEditor() && !GetLevelLoadFailed())
        {
            flags |= FRR_SYSTEM_RESOURCES;
        }

        pRenderer->FreeResources(flags);
        CryComment("done");
    }

    // Perform level unload procedures for the LyShine UI system
    if (gEnv && gEnv->pLyShine)
    {
        gEnv->pLyShine->OnLevelUnload();
    }

    m_bLevelLoaded = false;

    CTimeValue tUnloadTime = gEnv->pTimer->GetAsyncTime() - tBegin;
    CryLog("UnLoadLevel End: %.1f sec", tUnloadTime.GetSeconds());

    // Must be sent last.
    // Cleanup all containers
    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_POST_UNLOAD, 0, 0);
    AzFramework::InputChannelRequestBus::Broadcast(&AzFramework::InputChannelRequests::ResetState);
}

DynArray<string>* CLevelSystem::GetLevelTypeList()
{
    return &m_levelTypeList;
}

} // namespace LegacyLevelSystem
