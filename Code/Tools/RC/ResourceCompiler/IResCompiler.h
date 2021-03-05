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

// Description : IResourceCompiler interface.

#pragma once

#include <platform.h>
#include <AzCore/std/string/string.h>

struct ConvertContext;
struct CryChunkedFile;
class ICfgFile;
struct IAssetWriter;
struct ICompiler;
class IConfig;
struct IConvertor;
struct IPakSystem;
struct IRCLog;
class MultiplatformConfig;
struct SFileVersion;
class XmlNodeRef;

namespace AZ
{
    namespace Internal
    {
        class EnvironmentInterface;
    } // Internal
} // AZ

struct PlatformInfo
{
    enum
    {
        kMaxNameLength = 15
    };
    enum
    {
        kMaxPlatformNames = 3
    };
    int index;  // unique, starts from 0, increased by one for next platform, persistent for a session
    bool bBigEndian;
    int pointerSize;
    char platformNames[kMaxPlatformNames][kMaxNameLength + 1];  // every name is guaranteed to be zero-terminated, [0] is guaranteed to be non-empty

    void Clear()
    {
        index = -1;
        pointerSize = 0;
        for (int i = 0; i < kMaxPlatformNames; ++i)
        {
            platformNames[i][0] = 0;
        }
    }

    bool HasName(const char* const pName) const
    {
        for (int i = 0; i < kMaxPlatformNames && platformNames[i][0] != 0; ++i)
        {
            if (azstricmp(pName, &platformNames[i][0]) == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool SetName(int idx, const char* const pName)
    {
        if (idx < 0 || idx >= kMaxPlatformNames ||
            !pName || !pName[0])
        {
            return false;
        }
        const size_t len = strlen(pName);
        if (len + 1 > sizeof(platformNames[0]))
        {
            return false;
        }
        memcpy(platformNames[idx], pName, len + 1);
        return true;
    }

    const char* GetMainName() const
    {
        return &platformNames[0][0];
    }

    const AZStd::string GetCommaSeparatedNames() const
    {
        AZStd::string names;
        for (int i = 0; i < PlatformInfo::kMaxPlatformNames && platformNames[i][0] != 0; ++i)
        {
            if (!names.empty())
            {
                names += ",";
            }
            names += &platformNames[i][0];
        }
        return names;
    }
};


/** Main interface of resource compiler.
*/
struct IResourceCompiler
{
    class IExitObserver
    {
    public:
        virtual ~IExitObserver()
        {
        }

        virtual void OnExit() = 0;
    };

    //! Register new convertor.
    virtual void RegisterConvertor(const char* name, IConvertor* conv) = 0;

    // Get the interface for opening files - handles files stored in ZIP archives.
    virtual IPakSystem* GetPakSystem() = 0;

    virtual const ICfgFile* GetIniFile() const = 0;

    virtual int GetPlatformCount() const = 0;
    virtual const PlatformInfo* GetPlatformInfo(int index) const = 0;
    // Returns index of the platform (or -1 if platform not found)
    virtual int FindPlatform(const char* name) const = 0;

    // One input file can generate multiple output files.
    virtual void AddInputOutputFilePair(const char* inputFilename, const char* outputFilename) = 0;

    // Mark file for removal in clean stage.
    virtual void MarkOutputFileForRemoval(const char* sOutputFilename) = 0;

    // Add pointer to an observer object which will be notified in case of 'unexpected' exit()
    virtual void AddExitObserver(IExitObserver* p) = 0;

    // Remove an observer object which was added previously by AddExitObserver() call
    virtual void RemoveExitObserver(IExitObserver* p) = 0;

    virtual IRCLog* GetIRCLog() = 0;
    virtual int GetVerbosityLevel() const = 0;

    virtual const SFileVersion& GetFileVersion() const = 0;

    virtual const void GetGenericInfo(char* buffer, size_t bufferSize, const char* rowSeparator) const = 0;

    // Arguments:
    //   key - must not be 0
    //   helptxt - must not be 0
    virtual void RegisterKey(const char* key, const char* helptxt) = 0;

    // returns the path of the resource compiler executable's directory (ending with backslash)
    virtual const char* GetExePath() const = 0;

    // returns the path of a directory for temporary files (ending with backslash)
    virtual const char* GetTmpPath() const = 0;

    // returns directory which was current at the moment of RC call (ending with backslash)
    virtual const char* GetInitialCurrentDir() const = 0;

    // returns an xmlnode for the given xml file or null node if the file could not be parsed
    virtual XmlNodeRef LoadXml(const char* filename) = 0;

    // returns an xmlnode with the given tag name
    virtual XmlNodeRef CreateXml(const char* tag) = 0;

    virtual bool CompileSingleFileBySingleProcess(const char* filename) = 0;

    // Register Asset Writer interface
    virtual void SetAssetWriter(IAssetWriter* pAssetWriter) = 0;

    // Get Asset Writer interface
    virtual IAssetWriter* GetAssetWriter() const = 0;

    // Get the app root that the resource 
    virtual const char* GetAppRoot() const = 0;

};


extern "C"
{
// this is the plugin function that's exported by plugins
// Registers all convertors residing in this DLL.
// Must be called RegusterConvertors
typedef void(__stdcall * FnRegisterConvertors)(IResourceCompiler* pRC);

// this is the optional initialization function that may be exported by plugins
// It accepts the AZ shared system environment and should attach to it
typedef void (__stdcall * FnInitializeModule)(AZ::Internal::EnvironmentInterface* sharedEnvironment);

// (optional) called before the DLL is unloaded to perform cleanup if you need to.
// must be called BeforeUnloadDLL
typedef void(__stdcall * FnBeforeUnloadDLL)();
}
