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

// Description : Gathers level information. Loads a level.


#ifndef CRYINCLUDE_CRYACTION_ILEVELSYSTEM_H
#define CRYINCLUDE_CRYACTION_ILEVELSYSTEM_H
#pragma once

#include <CrySizer.h>
#include <IXml.h>

struct ILevelRotationFile;
struct IConsoleCmdArgs;

namespace AZ::IO
{
    struct IArchive;
}

struct ILevelRotation
{
    virtual ~ILevelRotation() {}
    typedef uint32  TExtInfoId;

    struct IExtendedInfo
    {
    };


    typedef uint8 TRandomisationFlags;

    enum EPlaylistRandomisationFlags
    {
        ePRF_None = 0,
        ePRF_Shuffle = 1 << 0,
        ePRF_MaintainPairs = 1 << 1,
    };

    virtual bool Load([[maybe_unused]] ILevelRotationFile* file) { return false; }
    virtual bool LoadFromXmlRootNode(const XmlNodeRef rootNode, [[maybe_unused]] const char* altRootTag) { return false; }

    virtual void Reset() {}
    virtual int  AddLevel([[maybe_unused]] const char* level) { return 0; }
    virtual void AddGameMode([[maybe_unused]] int level, [[maybe_unused]] const char* gameMode) {}

    virtual int  AddLevel([[maybe_unused]] const char* level, [[maybe_unused]] const char* gameMode) { return 0; }


    //call to set the playlist ready for a new session
    virtual void Initialise([[maybe_unused]] int nSeed) {}

    virtual bool First() { return false; }
    virtual bool Advance() { return false; }
    virtual bool AdvanceAndLoopIfNeeded() { return false; }

    virtual const char* GetNextLevel() const { return nullptr; }
    virtual const char* GetNextGameRules() const { return nullptr; }
    virtual int GetLength() const { return 0; }
    virtual int GetTotalGameModeEntries() const { return 0; }
    virtual int GetNext() const { return 0; }

    virtual const char* GetLevel([[maybe_unused]] uint32 idx, [[maybe_unused]] bool accessShuffled = true) const { return nullptr; }
    virtual int GetNGameRulesForEntry([[maybe_unused]] uint32 idx, [[maybe_unused]] bool accessShuffled = true) const { return 0; }
    virtual const char* GetGameRules([[maybe_unused]] uint32 idx, [[maybe_unused]] uint32 iMode, [[maybe_unused]] bool accessShuffled = true) const { return nullptr; }

    virtual const char* GetNextGameRulesForEntry([[maybe_unused]] int idx) const { return nullptr; }

    virtual const int NumAdvancesTaken() const { return 0; }
    virtual void ResetAdvancement() {}

    virtual bool IsRandom() const { return false; }

    virtual ILevelRotation::TRandomisationFlags GetRandomisationFlags() const { return ePRF_None; }
    virtual void SetRandomisationFlags([[maybe_unused]] TRandomisationFlags flags) {}

    virtual void ChangeLevel([[maybe_unused]] IConsoleCmdArgs* pArgs = NULL) {}

    virtual bool NextPairMatch() const { return false; }
};


struct ILevelInfo
{
    virtual ~ILevelInfo() {}
    typedef std::vector<string> TStringVec;

    struct TGameTypeInfo
    {
        string  name;
        string  xmlFile;
        int         cgfCount;
        void GetMemoryUsage(ICrySizer* pSizer) const
        {
            pSizer->AddObject(name);
            pSizer->AddObject(xmlFile);
        }
    };

    struct SMinimapInfo
    {
        SMinimapInfo()
            : fStartX(0)
            , fStartY(0)
            , fEndX(1)
            , fEndY(1)
            , fDimX(1)
            , fDimY(1)
            , iWidth(1024)
            , iHeight(1024) {}

        string sMinimapName;
        int iWidth;
        int iHeight;
        float fStartX;
        float fStartY;
        float fEndX;
        float fEndY;
        float fDimX;
        float fDimY;
    };

    virtual const char* GetName() const = 0;
    virtual const bool IsOfType(const char* sType) const = 0;
    virtual const char* GetPath() const = 0;
    virtual const char* GetPaks() const = 0;
    virtual const char* GetDisplayName() const = 0;
    virtual const char* GetPreviewImagePath() const = 0;
    virtual const char* GetBackgroundImagePath() const = 0;
    virtual const char* GetMinimapImagePath() const = 0;
    //virtual const ILevelInfo::TStringVec& GetMusicLibs() const = 0; // Gets reintroduced when level specific music data loading is implemented.
    virtual const bool MetadataLoaded() const = 0;
    virtual bool GetIsModLevel() const = 0;
    virtual const uint32 GetScanTag() const = 0;
    virtual const uint32 GetLevelTag() const = 0;

    virtual int GetGameTypeCount() const = 0;
    virtual const ILevelInfo::TGameTypeInfo* GetGameType(int gameType) const = 0;
    virtual bool SupportsGameType(const char* gameTypeName) const = 0;
    virtual const ILevelInfo::TGameTypeInfo* GetDefaultGameType() const = 0;
    virtual ILevelInfo::TStringVec GetGameRules() const = 0;
    virtual bool HasGameRules() const = 0;

    virtual const ILevelInfo::SMinimapInfo& GetMinimapInfo() const = 0;

    virtual const char* GetDefaultGameRules() const = 0;
};


struct ILevel
{
    virtual ~ILevel() {}
    virtual void Release() = 0;
    virtual ILevelInfo* GetLevelInfo() = 0;
};

/*!
 * Extend this class and call ILevelSystem::AddListener() to receive level system related events.
 */
struct ILevelSystemListener
{
    virtual ~ILevelSystemListener() {}
    //! Called when loading a level fails due to it not being found.
    virtual void OnLevelNotFound([[maybe_unused]] const char* levelName) {}
    //! Called after ILevelSystem::PrepareNextLevel() completes.
    virtual void OnPrepareNextLevel([[maybe_unused]] ILevelInfo* pLevel) {}
    //! Called after ILevelSystem::OnLoadingStart() completes, before the level actually starts loading.
    virtual void OnLoadingStart([[maybe_unused]] ILevelInfo* pLevel) {}
    //! Called after the level finished
    virtual void OnLoadingComplete([[maybe_unused]] ILevel* pLevel) {}
    //! Called when there's an error loading a level, with the level info and a description of the error.
    virtual void OnLoadingError([[maybe_unused]] ILevelInfo* pLevel, [[maybe_unused]] const char* error) {}
    //! Called whenever the loading status of a level changes. progressAmount goes from 0->100.
    virtual void OnLoadingProgress([[maybe_unused]] ILevelInfo* pLevel, [[maybe_unused]] int progressAmount) {}
    //! Called after a level is unloaded, before the data is freed.
    virtual void OnUnloadComplete([[maybe_unused]] ILevel* pLevel) {}

    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const { }
};

struct ILevelSystem
    : public ILevelSystemListener
{
    enum
    {
        TAG_MAIN = 'MAIN',
        TAG_UNKNOWN = 'ZZZZ'
    };

    static constexpr char LevelsDirectoryName[] = "levels";
    static constexpr char LevelPakName[] = "level.pak";

    virtual void Release() = 0;
    virtual void Rescan(const char* levelsFolder, const uint32 tag) = 0;
    virtual void ScanFolder(const char* subfolder, bool modFolder, const uint32 tag) = 0;
    virtual void PopulateLevels(string searchPattern, string& folder, AZ::IO::IArchive* pPak, bool& modFolder, const uint32& tag, bool fromFileSystemOnly)  = 0;
    virtual void LoadRotation() {}
    virtual int GetLevelCount() = 0;
    virtual DynArray<string>* GetLevelTypeList() = 0;
    virtual ILevelInfo* GetLevelInfo(int level) = 0;
    virtual ILevelInfo* GetLevelInfo(const char* levelName) = 0;

    virtual void AddListener(ILevelSystemListener* pListener) = 0;
    virtual void RemoveListener(ILevelSystemListener* pListener) = 0;

    virtual ILevel* GetCurrentLevel() const = 0;
    virtual ILevel* LoadLevel(const char* levelName) = 0;
    virtual void UnLoadLevel() = 0;
    virtual ILevel* SetEditorLoadedLevel(const char* levelName, bool bReadLevelInfoMetaData = false) = 0;
    virtual bool IsLevelLoaded() = 0;
    virtual void PrepareNextLevel(const char* levelName) = 0;

    virtual ILevelRotation* GetLevelRotation() { return nullptr; }

    virtual ILevelRotation* FindLevelRotationForExtInfoId([[maybe_unused]] const ILevelRotation::TExtInfoId findId) { return nullptr; }

    virtual bool AddExtendedLevelRotationFromXmlRootNode(const XmlNodeRef rootNode, [[maybe_unused]] const char* altRootTag, [[maybe_unused]] const ILevelRotation::TExtInfoId extInfoId) { return false; }
    virtual void ClearExtendedLevelRotations() {}
    virtual ILevelRotation* CreateNewRotation([[maybe_unused]] const ILevelRotation::TExtInfoId id) { return nullptr; }

    // Retrieve`s last level level loading time.
    virtual float GetLastLevelLoadTime() = 0;

    // If the level load failed then we need to have a different shutdown procedure vs when a level is naturally unloaded
    virtual void SetLevelLoadFailed(bool loadFailed) = 0;
    virtual bool GetLevelLoadFailed() = 0;
};

#endif // CRYINCLUDE_CRYACTION_ILEVELSYSTEM_H
