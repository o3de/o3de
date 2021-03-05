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


#ifndef CRYINCLUDE_CRYCOMMON_IRESOURCEMANAGER_H
#define CRYINCLUDE_CRYCOMMON_IRESOURCEMANAGER_H
#pragma once

namespace AZ::IO
{
    struct IResourceList;
}

struct SLayerPakStats
{
    struct SEntry
    {
        string name;
        size_t nSize;
        string status;
        bool bStreaming;
    };
    typedef std::vector<SEntry> TEntries;
    TEntries m_entries;

    size_t m_MaxSize;
    size_t m_UsedSize;
};

//////////////////////////////////////////////////////////////////////////
// IResource manager interface
//////////////////////////////////////////////////////////////////////////
struct IResourceManager
{
    // <interfuscator:shuffle>
    virtual ~IResourceManager(){}
    // Called by level system to set the level folder
    virtual void PrepareLevel(const char* sLevelFolder, const char* sLevelName) = 0;
    // Called by level system after the level has been unloaded.
    virtual void UnloadLevel() = 0;
    // Call to get current level resource list.
    virtual AZ::IO::IResourceList* GetLevelResourceList() = 0;
    // Load pak file from level cache to memory.
    // sBindRoot is a path in virtual file system, where new pak will be mapper to (ex. LevelCache/mtl)
    virtual bool LoadLevelCachePak(const char* sPakName, const char* sBindRoot, bool bOnlyDuringLevelLoading = true) = 0;
    // Unloads level cache pak file from memory.
    virtual void UnloadLevelCachePak(const char* sPakName) = 0;

    //Loads the pak file for mode switching into memory e.g. Single player mode to Multiplayer mode
    virtual bool LoadModeSwitchPak(const char* sPakName, const bool multiplayer) = 0;
    //Unloads the mode switching pak file
    virtual void UnloadModeSwitchPak(const char* sPakName, const char* sResourceListName, const bool multiplayer) = 0;

    // Load general pak file to memory.
    virtual bool LoadPakToMemAsync(const char* pPath, bool bLevelLoadOnly) = 0;
    // Unload all aync paks
    virtual void UnloadAllAsyncPaks() = 0;
    // Load pak file from active layer to memory.
    virtual bool LoadLayerPak(const char* sLayerName) = 0;
    // Unloads layer pak file from memory if no more references.
    virtual void UnloadLayerPak(const char* sLayerName) = 0;
    // Retrieve stats on the layer pak
    virtual void GetLayerPakStats(SLayerPakStats& stats, bool bCollectAllStats) const = 0;

    // Return time it took to load and precache the level.
    virtual CTimeValue GetLastLevelLoadTime() const = 0;

    virtual void GetMemoryStatistics(ICrySizer* pSizer) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IRESOURCEMANAGER_H
