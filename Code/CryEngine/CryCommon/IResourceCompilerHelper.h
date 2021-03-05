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
#ifndef CRYINCLUDE_CRYCOMMON_IRESOURCECOMPILERHELPER_H
#define CRYINCLUDE_CRYCOMMON_IRESOURCECOMPILERHELPER_H

#pragma once

// DO NOT USE AZSTD

#include <string>

// IResourceCompilerHelper exists to define an interface that allows
// remote or local compilation of resources through the "resource Compiler" executable
// in most tools it will be implemented as a local execution.  However, in the engine
// it will be substituted for a remote RC invocation through the Asset Processor if
// that system is enabled (via con var and define)

// DO NOT USE CRYSTRING or CRY ALLOCATORS HERE.  This is used in maya plugins, that kind of thing.
// the following path utils are special versions of these functions which take pains
// to not use crystring.
// the functions in this interface must be cross platform.
namespace RCPathUtil
{
    // given a full path, return the extension (it will be a pointer into the existing string)
    const char* GetExt(const char* filepath);

    // given a full path, return the file only (it will be a pointer into the existing string)
    const char* GetFile(const char* filepath);

    // given a filepath, get only the path.
    std::string GetPath(const char* filepath);
    std::string ReplaceExtension(const char* filepath, const char* ext);
    bool IsRelativePath(const char* p);
}

class IResourceCompilerListener;

enum ERcExitCode
{
    eRcExitCode_Success = 0,   // must be 0
    eRcExitCode_Error = 1,
    eRcExitCode_FatalError = 100,
    eRcExitCode_Crash = 101,
    eRcExitCode_UserFixing = 200,
    eRcExitCode_Pending = 666,
};

/// A pure virtual interface to the RC Helper system
/// the RC helper system allows you to make requests to a remote process in order to process
/// an asset for you.
class IResourceCompilerHelper
{
public:
    virtual ~IResourceCompilerHelper() {}

    // defines the result of a call via this API to the RC system
    enum ERcCallResult
    {
        eRcCallResult_success, // everything is OK
        eRcCallResult_notFound, // the RC executable is not found
        eRcCallResult_error, // the RC executable returned an error
        eRcCallResult_crash, // the RC executable did not finish
    };

    //
    // Arguments:
    //   szFileName null terminated ABSOLUTE file path or 0 can be used to test for rc.exe existence
    //   relative path needs to be relative to rc_plugins directory
    //   szAdditionalSettings - 0 or e.g. "/refresh" or "/refresh /xyz=56"
    //
    // this is a SYNCHRONOUS, BLOCKING call and will return once the process is complete
    virtual  ERcCallResult CallResourceCompiler(
        const char* szFileName = 0,
        const char* szAdditionalSettings = 0,
        IResourceCompilerListener* listener = 0,
        bool bMayShowWindow = true,
        bool bSilent = false,
        bool bNoUserDialog = false,
        const wchar_t* szWorkingDirectory = 0,
        const wchar_t* szRootPath = 0) = 0;

    // InvokeResourceCompiler - a utility that calls the above CallResourceCompiler function
    // but generates appropriate settings so you don't have to specify each option.
    // This is a BLOCKING call
    // the srcFile can be relative to the project root or an absolute path
    // the dstFilePath MUST be relative to the same folder as the Src File path
    // this will output dstFilePath in the same folder as srcFile.
    virtual ERcCallResult InvokeResourceCompiler(const char* szSrcFilePath, const char* szDstFilePath, const bool bUserDialog);

    // --------------------- utility functions ---------------------------------

    // given a RC.EXE process exit code like 101, convert it to the above ERcCallResult
    ERcCallResult ConvertResourceCompilerExitCodeToResultCode(int exitCode);

    // given a ERcCallResult, convert it to a simple english string for debugging.
    static const char* GetCallResultDescription(ERcCallResult result);

    // given a filename such as "blah.tif" convert it to the appropriate output name "blah.dds" for example
    static void GetOutputFilename(const char* szFilePath, char* buffer, size_t bufferSizeInBytes);

    //////////////////////////////////////////////////////////////////////////

    enum SourceImageTypes
    {
        SOURCE_IMAGE_TYPE_TIF,
        SOURCE_IMAGE_TYPE_BMP,
        SOURCE_IMAGE_TYPE_GIF,
        SOURCE_IMAGE_TYPE_JPG,
        SOURCE_IMAGE_TYPE_JPEG,
        SOURCE_IMAGE_TYPE_JPE,
        SOURCE_IMAGE_TYPE_TGA,
        SOURCE_IMAGE_TYPE_PNG,
        NUM_SOURCE_IMAGE_TYPE
    };

    enum EngineImageTypes
    {
        ENGINE_IMAGE_TYPE_DDS,
        NUM_ENGINE_IMAGE_TYPE
    };

private:
    static const char* SourceImageFormatExts[NUM_SOURCE_IMAGE_TYPE];
    static const char* SourceImageFormatExtsWithDot[NUM_SOURCE_IMAGE_TYPE];
    static const char* EngineImageFormatExts[NUM_ENGINE_IMAGE_TYPE];
    static const char* EngineImageFormatExtsWithDot[NUM_ENGINE_IMAGE_TYPE];

public:
    static unsigned int GetNumSourceImageFormats();
    static const char* GetSourceImageFormat(unsigned int index, bool bWithDot);

    static unsigned int GetNumEngineImageFormats();
    static const char* GetEngineImageFormat(unsigned int index, bool bWithDot);

    static bool IsSourceImageFormatSupported(const char* szExtension);
    static bool IsGameImageFormatSupported(const char* szExtension);
};

////////////////////////////////////////////////////////////////////////////
// Listener for synchronous resource-compilation.
// Connects the listener to the output of the RC process.
class IResourceCompilerListener
{
public:
    // FbxImportDialog relies on this enum being in the order from most verbose to least verbose
    enum MessageSeverity
    {
        MessageSeverity_Debug = 0,
        MessageSeverity_Info,
        MessageSeverity_Warning,
        MessageSeverity_Error
    };

    virtual void OnRCMessage(MessageSeverity /*severity*/, const char* /*text*/) {}
    virtual ~IResourceCompilerListener() {}
};

#endif // CRYINCLUDE_CRYCOMMON_IRESOURCECOMPILERHELPER_H
