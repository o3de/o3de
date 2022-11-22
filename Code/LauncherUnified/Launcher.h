/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h> // for AZ_COMMAND_LINE_LEN
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Memory/Memory.h>
#include <CryCommon/platform.h>

struct IOutputPrintSink;

#define COMMAND_LINE_ARG_COUNT_LIMIT (AZ_COMMAND_LINE_LEN+1) / 2        // Assume that the limit to how many arguments we can maintain is the max buffer size divided by 2
                                                                        // to account for an argument and a spec in between each argument (with the worse case scenario being

namespace O3DELauncher
{
    struct PlatformMainInfo
    {
        typedef bool (*ResourceLimitUpdater)();
        typedef void (*OnPostApplicationStart)();

        PlatformMainInfo() = default;

        //! Copy the command line into a buffer as is or reconstruct a
        //! quoted version of the command line from the arg c/v.  The
        //! internal buffer is fixed to \ref AZ_COMMAND_LINE_LEN meaning
        //! this call can fail if the command line exceeds that length
        bool CopyCommandLine(int argc, char** argv);
        bool AddArgument(const char* arg);

        char m_commandLine[AZ_COMMAND_LINE_LEN] = { 0 };
        size_t m_commandLineLen = 0;

        //! Keep static sized arrays to manage and provide the main arguments (argc, argv)
        char    m_commandLineArgBuffer[AZ_COMMAND_LINE_LEN] = { 0 };
        size_t  m_nextCommandLineArgInsertPoint = 0;
        int     m_argC = 0;
        char*   m_argV[COMMAND_LINE_ARG_COUNT_LIMIT] = { nullptr };

        ResourceLimitUpdater m_updateResourceLimits = nullptr; //!< callback for updating system resources, if necessary
        OnPostApplicationStart m_onPostAppStart = nullptr;  //!< callback notifying the platform specific entry point that AzGameFramework::GameApplication::Start has been called
        AZ::IAllocator* m_allocator = nullptr; //!< Used to allocate the temporary bootstrap memory, as well as the main \ref SystemAllocator heap. If null, OSAllocator will be used

        const char* m_appResourcesPath = "."; //!< Path to the device specific assets, default is equivalent to blank path in ParseEngineConfig
        const char* m_appWriteStoragePath = nullptr; //!< Path to writeable storage if different than assets path, used to override userPath and logPath

        const char* m_additionalVfsResolution = nullptr; //!< additional things to check if VFS is not working for the desired platform

        void* m_window = nullptr; //!< maps to \ref SSystemInitParams::hWnd
        void* m_instance = nullptr; //!< maps to \ref SSystemInitParams::hInstance
        IOutputPrintSink* m_printSink = nullptr; //!< maps to \ref SSystemInitParams::pPrintSync
    };

    enum class ReturnCode : unsigned char
    {
        Success = 0,

        ErrExePath,             //!< Failed to get the executable path
        ErrCommandLine,         //!< Failed to copy the command line
        ErrValidation,          //!< Failed to validate secret
        ErrResourceLimit,       //!< Failed to increase unix resource limits
        ErrAppDescriptor,       //!< Failed to locate the application descriptor file
        ErrCrySystemLib,        //!< Failed to load required CrySystem library
        ErrCrySystemInterface,  //!< Failed to create the CrySystem interface
        ErrCryEnvironment,      //!< Failed to initialize the CryEngine environment
        ErrAssetProccessor,     //!< Failed to connect to the asset processor
        ErrUnitTestFailure,     //!< In Unit Test mode, one or more of the unit tests failed.
        ErrUnitTestNotSupported,//!< In Unit Test mode is not supported in its current configuration
    };

    const char* GetReturnCodeString(ReturnCode code);

    //! The main entry point for all O3DE launchers
    ReturnCode Run(const PlatformMainInfo& mainInfo = PlatformMainInfo());

    //////////////////////////////////////////////////////////////////////////
    // The following functions are defined by launcher project
    //////////////////////////////////////////////////////////////////////////

    //! This function returns the name of the project
    AZStd::string_view GetProjectName();

    //! This function returns the path of the project as known by the build system
    AZStd::string_view GetProjectPath();

    //! This function returns the build system target name
    AZStd::string_view GetBuildTargetName();

    //////////////////////////////////////////////////////////////////////////
    // The following functions are defined per launcher type (e.g. Game/Server)
    //////////////////////////////////////////////////////////////////////////

    //! Indicates if it should wait for a connection to the AssetProcessor (will attempt to open it if true)
    bool WaitForAssetProcessorConnect();

    //! Indicates if it is a dedicated server
    bool IsDedicatedServer();

    //! Gets the name of the log file to use
    const char* GetLogFilename();
}

