/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CrySystem_precompiled.h"
#include "SpawnableLevelSystem.h"
#include "IMovieSystem.h"

#include <LoadScreenBus.h>

#include <AzCore/Debug/AssetTracking.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/Input/Buses/Requests/InputChannelRequestBus.h>

#include "MainThreadRenderRequestBus.h"
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Time/ITime.h>

namespace LegacyLevelSystem
{
    constexpr AZStd::string_view DeferredLoadLevelKey = "/O3DE/Runtime/SpawnableLevelSystem/DeferredLoadLevel";
    //------------------------------------------------------------------------
    static void LoadLevel(const AZ::ConsoleCommandContainer& arguments)
    {
        AZ_Error("SpawnableLevelSystem", !arguments.empty(), "LoadLevel requires a level file name to be provided.");
        AZ_Error("SpawnableLevelSystem", arguments.size() == 1, "LoadLevel requires a single level file name to be provided.");

        if (!arguments.empty() && gEnv && gEnv->pSystem && gEnv->pSystem->GetILevelSystem() && !gEnv->IsEditor())
        {
            gEnv->pSystem->GetILevelSystem()->LoadLevel(arguments[0].data());
        }
        else if (!arguments.empty())
        {
            // The SpawnableLevelSystem isn't available yet.
            // Defer the level load until later by storing it in the SettingsRegistry
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
            {
                settingsRegistry->Set(DeferredLoadLevelKey, arguments.front());
            }
        }
    }

    //------------------------------------------------------------------------
    static void UnloadLevel([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        AZ_Warning("SpawnableLevelSystem", !arguments.empty(), "UnloadLevel doesn't use any arguments.");

        if (gEnv && gEnv->pSystem && gEnv->pSystem->GetILevelSystem() && !gEnv->IsEditor())
        {
            gEnv->pSystem->GetILevelSystem()->UnloadLevel();
        }
    }

    AZ_CONSOLEFREEFUNC(LoadLevel, AZ::ConsoleFunctorFlags::Null, "Unloads the current level and loads a new one with the given asset name");
    AZ_CONSOLEFREEFUNC(UnloadLevel, AZ::ConsoleFunctorFlags::Null, "Unloads the current level");

    //------------------------------------------------------------------------
    SpawnableLevelSystem::SpawnableLevelSystem([[maybe_unused]] ISystem* pSystem)
    {
        CRY_ASSERT(pSystem);

        m_fLastLevelLoadTime = 0;
        m_fLastTime = 0;
        m_bLevelLoaded = false;

        m_levelLoadStartTime.SetValue(0);
        m_nLoadedLevelsCount = 0;

        AZ_Assert(gEnv && gEnv->pCryPak, "gEnv and CryPak must be initialized for loading levels.");
        if (!gEnv || !gEnv->pCryPak)
        {
            return;
        }

        AzFramework::RootSpawnableNotificationBus::Handler::BusConnect();

        // If there were LoadLevel command invocations before the creation of the level system
        // then those invocations were queued.
        // load the last level in the queue, since only one level can be loaded at a time
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            if (AZ::SettingsRegistryInterface::FixedValueString deferredLevelName;
                settingsRegistry->Get(deferredLevelName, DeferredLoadLevelKey) && !deferredLevelName.empty())
            {
                // since this is the constructor any derived classes vtables aren't setup yet
                // call this class LoadLevel function
                AZ_TracePrintf("SpawnableLevelSystem", "The Level System is now available."
                    " Loading level %s which could not be loaded earlier\n", deferredLevelName.c_str());
                SpawnableLevelSystem::LoadLevel(deferredLevelName.c_str());
                // Delete the key with the deferred level name
                settingsRegistry->Remove(DeferredLoadLevelKey);
            }
        }
    }

    //------------------------------------------------------------------------
    SpawnableLevelSystem::~SpawnableLevelSystem()
    {
        AzFramework::RootSpawnableNotificationBus::Handler::BusDisconnect();
    }

    void SpawnableLevelSystem::Release()
    {
        delete this;
    }

    bool SpawnableLevelSystem::IsLevelLoaded()
    {
        return m_bLevelLoaded;
    }

    const char* SpawnableLevelSystem::GetCurrentLevelName() const
    {
        return m_bLevelLoaded ? m_lastLevelName.c_str() : "";
    }

    // If the level load failed then we need to have a different shutdown procedure vs when a level is naturally unloaded
    void SpawnableLevelSystem::SetLevelLoadFailed(bool loadFailed)
    {
        m_levelLoadFailed = loadFailed;
    }

    bool SpawnableLevelSystem::GetLevelLoadFailed()
    {
        return m_levelLoadFailed;
    }

    AZ::Data::AssetType SpawnableLevelSystem::GetLevelAssetType() const
    {
        return azrtti_typeid<AzFramework::Spawnable>();
    }

    // The following methods are deprecated from ILevelSystem and will be removed once slice support is removed.

    // [LYN-2376] Remove once legacy slice support is removed
    void SpawnableLevelSystem::Rescan([[maybe_unused]] const char* levelsFolder)
    {
        AZ_Assert(false, "Rescan - No longer supported.");
    }

    // [LYN-2376] Remove once legacy slice support is removed
    int SpawnableLevelSystem::GetLevelCount()
    {
        AZ_Assert(false, "GetLevelCount - No longer supported.");
        return 0;
    }

    // [LYN-2376] Remove once legacy slice support is removed
    ILevelInfo* SpawnableLevelSystem::GetLevelInfo([[maybe_unused]] int level)
    {
        AZ_Assert(false, "GetLevelInfo - No longer supported.");
        return nullptr;
    }

    // [LYN-2376] Remove once legacy slice support is removed
    ILevelInfo* SpawnableLevelSystem::GetLevelInfo([[maybe_unused]] const char* levelName)
    {
        AZ_Assert(false, "GetLevelInfo - No longer supported.");
        return nullptr;
    }

    //------------------------------------------------------------------------
    void SpawnableLevelSystem::AddListener(ILevelSystemListener* pListener)
    {
        AZStd::vector<ILevelSystemListener*>::iterator it = AZStd::find(m_listeners.begin(), m_listeners.end(), pListener);

        if (it == m_listeners.end())
        {
            m_listeners.push_back(pListener);
        }
    }

    //------------------------------------------------------------------------
    void SpawnableLevelSystem::RemoveListener(ILevelSystemListener* pListener)
    {
        AZStd::vector<ILevelSystemListener*>::iterator it = AZStd::find(m_listeners.begin(), m_listeners.end(), pListener);

        if (it != m_listeners.end())
        {
            m_listeners.erase(it);
        }
    }

    //------------------------------------------------------------------------
    bool SpawnableLevelSystem::LoadLevel(const char* levelName)
    {
        if (gEnv->IsEditor())
        {
            AZ_TracePrintf("CrySystem::CLevelSystem", "LoadLevel for %s was called in the editor - not actually loading.\n", levelName);
            return false;
        }

        // Make sure a spawnable level exists that matches levelname
        AZStd::string validLevelName;
        AZ::Data::AssetId rootSpawnableAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            rootSpawnableAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, levelName, nullptr, false);

        if (rootSpawnableAssetId.IsValid())
        {
            validLevelName = levelName;
        }
        else
        {
            // It's common for users to only provide the level name, but not the full asset path
            // Example: "MyLevel" instead of "Levels/MyLevel/MyLevel.spawnable"
            if (!AZ::IO::PathView(levelName).HasExtension())
            {
                // Search inside the "Levels" folder for a level spawnable matching levelname
                const AZStd::string possibleLevelAssetPath = AZStd::string::format("Levels/%s/%s.spawnable", levelName, levelName);

                AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                    rootSpawnableAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, possibleLevelAssetPath.c_str(),
                    nullptr, false);

                if (rootSpawnableAssetId.IsValid())
                {
                    validLevelName = possibleLevelAssetPath;
                }
            }
        }

        if (validLevelName.empty())
        {
            OnLevelNotFound(levelName);
            return false;
        }

        // If a level is currently loaded, unload it before loading the next one.
        if (IsLevelLoaded())
        {
            UnloadLevel();
        }

        gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_PREPARE, 0, 0);
        PrepareNextLevel(validLevelName.c_str());

        bool result = LoadLevelInternal(validLevelName.c_str());
        if (result)
        {
            OnLoadingComplete(validLevelName.c_str());
        }

        return result;
    }

    //------------------------------------------------------------------------
    bool SpawnableLevelSystem::LoadLevelInternal(const char* levelName)
    {
        gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START);
        AZ_ASSET_NAMED_SCOPE("Level: %s", levelName);

        INDENT_LOG_DURING_SCOPE();

        AZ::Data::AssetId rootSpawnableAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            rootSpawnableAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, levelName, nullptr, false);
        if (!rootSpawnableAssetId.IsValid())
        {
            OnLoadingError(levelName, "AssetCatalog has no entry for the requested level.");

            return false;
        }

        // This scope is specifically used for marking a loading time profile section
        {
            m_bLevelLoaded = false;
            m_lastLevelName = levelName;
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

            // This is a workaround until the replacement for GameEntityContext is done
            AzFramework::GameEntityContextEventBus::Broadcast(&AzFramework::GameEntityContextEventBus::Events::OnPreGameEntitiesStarted);

            OnLoadingStart(levelName);

            auto pPak = gEnv->pCryPak;

            ICVar* pSpamDelay = gEnv->pConsole->GetCVar("log_SpamDelay");
            float spamDelay = 0.0f;
            if (pSpamDelay)
            {
                spamDelay = pSpamDelay->GetFVal();
                pSpamDelay->Set(0.0f);
            }

            AZ::Data::Asset<AzFramework::Spawnable> rootSpawnable(
                rootSpawnableAssetId, azrtti_typeid<AzFramework::Spawnable>(), levelName);

            m_rootSpawnableId = rootSpawnableAssetId;
            m_rootSpawnableGeneration = AzFramework::RootSpawnableInterface::Get()->AssignRootSpawnable(rootSpawnable);

            // This is a workaround until the replacement for GameEntityContext is done
            AzFramework::GameEntityContextEventBus::Broadcast(&AzFramework::GameEntityContextEventBus::Events::OnGameEntitiesStarted);

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

        return true;
    }

    //------------------------------------------------------------------------
    void SpawnableLevelSystem::PrepareNextLevel(const char* levelName)
    {
        AZ::Data::AssetId rootSpawnableAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            rootSpawnableAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, levelName, nullptr, false);
        if (!rootSpawnableAssetId.IsValid())
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

            // switched to level heap, so now imm start the loading screen (renderer will be reinitialized in the levelheap)
            gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN, 0, 0);
            gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_START_PREPARE);
        }

        OnPrepareNextLevel(levelName);
    }

    void SpawnableLevelSystem::OnPrepareNextLevel(const char* levelName)
    {
        AZ_TracePrintf("LevelSystem", "Level system is preparing to load '%s'\n", levelName);

        for (auto& listener : m_listeners)
        {
            listener->OnPrepareNextLevel(levelName);
        }
    }

    //------------------------------------------------------------------------
    void SpawnableLevelSystem::OnLevelNotFound(const char* levelName)
    {
        AZ_Error("LevelSystem", false, "Requested level not found: '%s'\n", levelName);

        for (auto& listener : m_listeners)
        {
            listener->OnLevelNotFound(levelName);
        }
    }

    //------------------------------------------------------------------------
    void SpawnableLevelSystem::OnLoadingStart(const char* levelName)
    {
        AZ_TracePrintf("LevelSystem", "Level system is loading '%s'\n", levelName);

        if (gEnv->pCryPak->GetRecordFileOpenList() == AZ::IO::IArchive::RFOM_EngineStartup)
        {
            gEnv->pCryPak->RecordFileOpen(AZ::IO::IArchive::RFOM_Level);
        }

        const AZ::TimeMs timeMs = AZ::GetRealElapsedTimeMs();
        m_fLastTime = AZ::TimeMsToSeconds(timeMs);

        GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_LOAD_START, 0, 0);

        for (auto& listener : m_listeners)
        {
            listener->OnLoadingStart(levelName);
        }
    }

    //------------------------------------------------------------------------
    void SpawnableLevelSystem::OnLoadingError(const char* levelName, const char* error)
    {
        AZ_Error("LevelSystem", false, "Error loading level '%s': %s\n", levelName, error);

        for (auto& listener : m_listeners)
        {
            listener->OnLoadingError(levelName, error);
        }
    }

    //------------------------------------------------------------------------
    void SpawnableLevelSystem::OnLoadingComplete(const char* levelName)
    {
        const AZ::TimeMs timeMs = AZ::GetRealElapsedTimeMs();
        const double timeSec = AZ::TimeMsToSecondsDouble(timeMs);
        const CTimeValue t(timeSec);
        m_fLastLevelLoadTime = (t - m_levelLoadStartTime).GetSeconds();

        LogLoadingTime();

        m_nLoadedLevelsCount++;

        // Hide console after loading.
        gEnv->pConsole->ShowConsole(false);

        for (auto& listener : m_listeners)
        {
            listener->OnLoadingComplete(levelName);
        }

    #if AZ_LOADSCREENCOMPONENT_ENABLED
        EBUS_EVENT(LoadScreenBus, Stop);
    #endif // if AZ_LOADSCREENCOMPONENT_ENABLED

        AZ_TracePrintf("LevelSystem", "Level load complete: '%s'\n", levelName);
    }

    //------------------------------------------------------------------------
    void SpawnableLevelSystem::OnLoadingProgress(const char* levelName, int progressAmount)
    {
        for (auto& listener : m_listeners)
        {
            listener->OnLoadingProgress(levelName, progressAmount);
        }
    }

    //------------------------------------------------------------------------
    void SpawnableLevelSystem::OnUnloadComplete(const char* levelName)
    {
        for (auto& listener : m_listeners)
        {
            listener->OnUnloadComplete(levelName);
        }

        AZ_TracePrintf("LevelSystem", "Level unload complete: '%s'\n", levelName);
    }

    //////////////////////////////////////////////////////////////////////////
    void SpawnableLevelSystem::LogLoadingTime()
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
    void SpawnableLevelSystem::UnloadLevel()
    {
        if (gEnv->IsEditor())
        {
            return;
        }

        if (m_lastLevelName.empty())
        {
            return;
        }

        AZ_TracePrintf("LevelSystem", "UnloadLevel Start\n");
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

        AzFramework::RootSpawnableInterface::Get()->ReleaseRootSpawnable();

        m_lastLevelName.clear();

        // Force Lua garbage collection (may no longer be needed now the legacy renderer has been removed).
        // Normally the GC step is triggered at the end of this method (by the ESYSTEM_EVENT_LEVEL_POST_UNLOAD event).
        EBUS_EVENT(AZ::ScriptSystemRequestBus, GarbageCollect);

        m_bLevelLoaded = false;

        [[maybe_unused]] const AZ::TimeMs unloadTimeMs = AZ::GetRealElapsedTimeMs() - beginTimeMs;
        AZ_TracePrintf("LevelSystem", "UnloadLevel End: %.1f sec\n", AZ::TimeMsToSeconds(unloadTimeMs));

        // Must be sent last.
        // Cleanup all containers
        GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_POST_UNLOAD, 0, 0);
        AzFramework::InputChannelRequestBus::Broadcast(&AzFramework::InputChannelRequests::ResetState);

        AzFramework::GameEntityContextEventBus::Broadcast(&AzFramework::GameEntityContextEventBus::Events::OnGameEntitiesReset);
    }

    void SpawnableLevelSystem::OnRootSpawnableAssigned(
        [[maybe_unused]] AZ::Data::Asset<AzFramework::Spawnable> rootSpawnable, [[maybe_unused]] uint32_t generation)
    {
    }

    void SpawnableLevelSystem::OnRootSpawnableReleased([[maybe_unused]] uint32_t generation)
    {
    }

} // namespace LegacyLevelSystem
