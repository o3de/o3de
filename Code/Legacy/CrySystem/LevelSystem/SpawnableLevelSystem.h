/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ILevelSystem.h"
#include <AzCore/Console/IConsole.h>
#include <AzFramework/Archive/IArchive.h>
#include <AzFramework/Spawnable/RootSpawnableInterface.h>
#include <CryCommon/TimeValue.h>

namespace LegacyLevelSystem
{

class SpawnableLevelSystem
        : public ILevelSystem
        , public AzFramework::RootSpawnableNotificationBus::Handler
    {
    public:
        explicit SpawnableLevelSystem(ISystem* pSystem);
        ~SpawnableLevelSystem() override;

        // ILevelSystem
        void Release() override;

        void AddListener(ILevelSystemListener* pListener) override;
        void RemoveListener(ILevelSystemListener* pListener) override;

        bool LoadLevel(const char* levelName) override;
        void UnloadLevel() override;
        bool IsLevelLoaded() override;
        const char* GetCurrentLevelName() const override;

        // If the level load failed then we need to have a different shutdown procedure vs when a level is naturally unloaded
        void SetLevelLoadFailed(bool loadFailed) override;
        bool GetLevelLoadFailed() override;
        AZ::Data::AssetType GetLevelAssetType() const override;

        // The following methods are deprecated from ILevelSystem and will be removed once slice support is removed.

        // [LYN-2376] Remove once legacy slice support is removed
        void Rescan([[maybe_unused]] const char* levelsFolder) override;
        int GetLevelCount() override;
        ILevelInfo* GetLevelInfo([[maybe_unused]] int level) override;
        ILevelInfo* GetLevelInfo([[maybe_unused]] const char* levelName) override;

    private:
        void OnRootSpawnableAssigned(AZ::Data::Asset<AzFramework::Spawnable> rootSpawnable, uint32_t generation) override;
        void OnRootSpawnableReleased(uint32_t generation) override;

        void PrepareNextLevel(const char* levelName);
        bool LoadLevelInternal(const char* levelName);

        // Methods to notify ILevelSystemListener
        void OnPrepareNextLevel(const char* levelName);
        void OnLevelNotFound(const char* levelName);
        void OnLoadingStart(const char* levelName);
        void OnLoadingComplete(const char* levelName);
        void OnLoadingError(const char* levelName, const char* error);
        void OnLoadingProgress(const char* levelName, int progressAmount);
        void OnUnloadComplete(const char* levelName);

        void LogLoadingTime();

        AZStd::string m_lastLevelName;
        float m_fLastLevelLoadTime{0.0f};
        float m_fLastTime{0.0f};

        bool m_bLevelLoaded{false};
        bool m_levelLoadFailed{false};

        int m_nLoadedLevelsCount{0};

        CTimeValue m_levelLoadStartTime;

        AZStd::vector<ILevelSystemListener*> m_listeners;

        // Information about the currently-loaded root spawnable, used for tracking loads and unloads.
        uint64_t m_rootSpawnableGeneration{0};
        AZ::Data::AssetId m_rootSpawnableId{};
    };

} // namespace LegacyLevelSystem
