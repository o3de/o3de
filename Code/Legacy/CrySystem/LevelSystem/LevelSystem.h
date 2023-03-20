/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "ILevelSystem.h"
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Archive/IArchive.h>
#include <CryCommon/TimeValue.h>

// [LYN-2376] Remove the entire file once legacy slice support is removed

namespace LegacyLevelSystem
{

class CLevelInfo
    : public ILevelInfo
{
    friend class CLevelSystem;
public:
    CLevelInfo() = default;

    // ILevelInfo
    const char* GetName() const override { return m_levelName.c_str(); }
    const char* GetPath() const override { return m_levelPath.c_str(); }
    const char* GetAssetName() const override { return m_levelAssetName.c_str(); }
    // ~ILevelInfo


private:
    bool ReadInfo();

    bool OpenLevelPak();
    void CloseLevelPak();

    AZStd::string m_defaultGameTypeName;
    AZStd::string m_levelName;
    AZStd::string m_levelPath;
    AZStd::string m_levelAssetName;

    AZStd::string m_levelPakFullPath;

    bool          m_isPak = false;
};

struct ILevel
{
    virtual ~ILevel() = default;
    virtual void Release() = 0;
    virtual ILevelInfo* GetLevelInfo() = 0;
};

class CLevel
    : public ILevel
{
    friend class CLevelSystem;
public:
    CLevel() {}
    virtual ~CLevel() = default;

    void Release() override { delete this; }

    ILevelInfo* GetLevelInfo() override { return &m_levelInfo; }

private:
    CLevelInfo m_levelInfo;
};

class CLevelSystem
    : public ILevelSystem
    , AzFramework::LevelSystemLifecycleInterface::Registrar
{
public:
    CLevelSystem(ISystem* pSystem, const char* levelsFolder);
    virtual ~CLevelSystem();

    void Release() override { delete this; };

    // ILevelSystem
    void Rescan(const char* levelsFolder) override;
    int GetLevelCount() override;
    ILevelInfo* GetLevelInfo(int level) override;
    ILevelInfo* GetLevelInfo(const char* levelName) override;

    void AddListener(ILevelSystemListener* pListener) override;
    void RemoveListener(ILevelSystemListener* pListener) override;

    bool LoadLevel(const char* levelName) override;
    void UnloadLevel() override;

    //! AzFramework::LevelSystemLifecycleInterface overrides.
    //! @{
    bool IsLevelLoaded() const override { return m_bLevelLoaded; }

    const char* GetCurrentLevelName() const override
    {
        if (m_pCurrentLevel && m_pCurrentLevel->GetLevelInfo())
        {
            return m_pCurrentLevel->GetLevelInfo()->GetName();
        }
        return "";
    }
    //! @}

    // If the level load failed then we need to have a different shutdown procedure vs when a level is naturally unloaded
    void SetLevelLoadFailed(bool loadFailed) override { m_levelLoadFailed = loadFailed; }
    bool GetLevelLoadFailed() override { return m_levelLoadFailed; }

    // Unsupported by legacy level system.
    AZ::Data::AssetType GetLevelAssetType() const override { return {}; }

    // ~ILevelSystem

private:

    float GetLastLevelLoadTime() { return m_fLastLevelLoadTime; }

    void ScanFolder(const char* subfolder, bool modFolder);
    void PopulateLevels(
        AZStd::string searchPattern, const AZStd::string& folder, AZ::IO::IArchive* pPak, bool& modFolder, bool fromFileSystemOnly);
    void PrepareNextLevel(const char* levelName);
    ILevel* LoadLevelInternal(const char* _levelName);

    // Methods to notify ILevelSystemListener
    void OnLevelNotFound(const char* levelName);
    void OnLoadingStart(const char* levelName);
    void OnLoadingComplete(const char* levelName);
    void OnLoadingError(const char* levelName, const char* error);
    void OnLoadingProgress(const char* levelName, int progressAmount);
    void OnUnloadComplete(const char* levelName);

    void LogLoadingTime();
    bool LoadLevelInfo(CLevelInfo& levelInfo);

    // internal get functions for the level infos ... they preserve the type and don't
    // directly cast to the interface
    CLevelInfo* GetLevelInfoInternal(int level);
    CLevelInfo* GetLevelInfoInternal(const AZStd::string& levelName);

    ISystem* m_pSystem;
    AZStd::vector<CLevelInfo> m_levelInfos;
    AZStd::string m_levelsFolder;
    ILevel* m_pCurrentLevel;
    ILevelInfo* m_pLoadingLevelInfo;

    AZStd::string m_lastLevelName;
    float                   m_fLastLevelLoadTime;
    float                   m_fLastTime;

    bool                    m_bLevelLoaded;
    bool                    m_levelLoadFailed = false;

    int                     m_nLoadedLevelsCount;

    CTimeValue              m_levelLoadStartTime;

    AZStd::vector<ILevelSystemListener*> m_listeners;

    AZ::IO::IArchive::LevelPackOpenEvent::Handler m_levelPackOpenHandler;
    AZ::IO::IArchive::LevelPackCloseEvent::Handler m_levelPackCloseHandler;

    static constexpr const char* LevelPakName = "level.pak";
};

} // namespace LegacyLevelSystem
