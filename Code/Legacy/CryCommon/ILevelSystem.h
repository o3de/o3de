/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Gathers level information. Loads a level.
#pragma once

#include <IXml.h>
#include <AzCore/Asset/AssetCommon.h>

struct IConsoleCmdArgs;

namespace AZ::IO
{
    struct IArchive;
}

// [LYN-2376] Remove once legacy slice support is removed
struct ILevelInfo
{
    virtual ~ILevelInfo() = default;

    virtual const char* GetName() const = 0;
    virtual const char* GetPath() const = 0;
    virtual const char* GetAssetName() const = 0;
};


/*!
 * Extend this class and call ILevelSystem::AddListener() to receive level system related events.
 */
struct ILevelSystemListener
{
    virtual ~ILevelSystemListener() = default;
    //! Called when loading a level fails due to it not being found.
    virtual void OnLevelNotFound([[maybe_unused]] const char* levelName) {}
    //! Called after ILevelSystem::PrepareNextLevel() completes.
    virtual void OnPrepareNextLevel([[maybe_unused]] const char* levelName) {}
    //! Called after ILevelSystem::OnLoadingStart() completes, before the level actually starts loading.
    virtual void OnLoadingStart([[maybe_unused]] const char* levelName) {}
    //! Called after the level finished
    virtual void OnLoadingComplete([[maybe_unused]] const char* levelName) {}
    //! Called when there's an error loading a level, with the level info and a description of the error.
    virtual void OnLoadingError([[maybe_unused]] const char* levelName, [[maybe_unused]] const char* error) {}
    //! Called whenever the loading status of a level changes. progressAmount goes from 0->100.
    virtual void OnLoadingProgress([[maybe_unused]] const char* levelName, [[maybe_unused]] int progressAmount) {}
    //! Called after a level is unloaded, before the data is freed.
    virtual void OnUnloadComplete([[maybe_unused]] const char* levelName) {}
};

struct ILevelSystem
{
    virtual ~ILevelSystem() = default;

    virtual void Release() = 0;

    virtual void AddListener(ILevelSystemListener* pListener) = 0;
    virtual void RemoveListener(ILevelSystemListener* pListener) = 0;

    virtual bool LoadLevel(const char* levelName) = 0;
    virtual void UnloadLevel() = 0;
    virtual bool IsLevelLoaded() = 0;
    virtual const char* GetCurrentLevelName() const = 0;

    // If the level load failed then we need to have a different shutdown procedure vs when a level is naturally unloaded
    virtual void SetLevelLoadFailed(bool loadFailed) = 0;
    virtual bool GetLevelLoadFailed() = 0;

    virtual AZ::Data::AssetType GetLevelAssetType() const = 0;

    static const char* GetLevelsDirectoryName()
    {
        return LevelsDirectoryName;
    }

    // [LYN-2376] Deprecated methods, to be removed once slices are removed:
    virtual void Rescan(const char* levelsFolder) = 0;
    virtual int GetLevelCount() = 0;
    virtual ILevelInfo* GetLevelInfo(int level) = 0;
    virtual ILevelInfo* GetLevelInfo(const char* levelName) = 0;

protected:

    static constexpr const char* LevelsDirectoryName = "levels";
};
