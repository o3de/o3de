/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformDef.h>
#include <AzCore/base.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZ
{
    namespace Utils
    {
        //! Terminates the application without going through the shutdown procedure.
        //! This is used when due to abnormal circumstances the application can no
        //! longer continue. On most platforms and in most configurations this will
        //! lead to a crash report, though this is not guaranteed.
        void RequestAbnormalTermination();

        //! Shows a platform native error message. This message box is available even before
        //! the engine is fully initialized. This function will do nothing on platforms
        //! that can't meet this requirement. Do not not use this function for common
        //! message boxes as it's designed to only be used in situations where the engine
        //! is booting or shutting down. NativeMessageBox will not return until the user
        //! has closed the message box.
        void NativeErrorMessageBox(const char* title, const char* message);

        //! Enum used for the GetExecutablePath return type which indicates
        //! whether the function returned with a success value or a specific error
        enum class ExecutablePathResult : int8_t
        {
            Success,
            BufferSizeNotLargeEnough,
            GeneralError
        };

        //! Structure used to encapsulate the return value of GetExecutablePath
        //! Two pieces of information is returned.
        //! 1. Whether the executable path was able to be stored in the buffer.
        //! 2. If the executable path that was returned includes the executable filename
        struct GetExecutablePathReturnType
        {
            ExecutablePathResult m_pathStored{ ExecutablePathResult::Success };
            bool m_pathIncludesFilename{};
        };

        //! Retrieves the path to the application executable
        //! @param exeStorageBuffer output buffer which is used to store the executable path within
        //! @param exeStorageSize size of the exeStorageBuffer
        //! @returns a struct that indicates if the executable path was able to be stored within the executableBuffer
        //! as well as if the executable path contains the executable filename or the executable directory
        GetExecutablePathReturnType GetExecutablePath(char* exeStorageBuffer, size_t exeStorageSize);

        //! Retrieves the directory of the application executable
        //! @param exeStorageBuffer output buffer which is used to store the executable path within
        //! @param exeStorageSize size of the exeStorageBuffer
        //! @returns a result object that indicates if the executable directory was able to be stored within the buffer
        ExecutablePathResult GetExecutableDirectory(char* exeStorageBuffer, size_t exeStorageSize);

        //! Retrieves the full path of the directroy containing the executable
        AZ::IO::FixedMaxPathString GetExecutableDirectory();

        //! Retrieves the full path to the engine from settings registry
        AZ::IO::FixedMaxPathString GetEnginePath();

        //! Retrieves the full path to the project from settings registry
        AZ::IO::FixedMaxPathString GetProjectPath();

        //! Retrieves the project name from the settings registry
        AZ::SettingsRegistryInterface::FixedValueString GetProjectName();

        //! Retrieves the full directory to the Home directory, i.e. "<userhome> or overrideHomeDirectory"
        AZ::IO::FixedMaxPathString GetHomeDirectory();

        //! Retrieves the full directory to the O3DE manifest directory, i.e. "<userhome>/.o3de"
        AZ::IO::FixedMaxPathString GetO3deManifestDirectory();

        //! Retrieves the full path where the manifest file lives, i.e. "<userhome>/.o3de/o3de_manifest.json"
        AZ::IO::FixedMaxPathString GetEngineManifestPath();

        //! Retrieves the full directory to the O3DE logs directory, i.e. "<userhome>/.o3de/Logs"
        AZ::IO::FixedMaxPathString GetO3deLogsDirectory();

        //! Retrieves the App root path to use on the current platform
        //! If the optional is not engaged the AppRootPath should be calculated based
        //! on the location of the bootstrap.cfg file
        AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultAppRootPath();

        //! Retrieves the development write storage path to use on the current platform, may be considered
        //! temporary or cache storage
        AZStd::optional<AZ::IO::FixedMaxPathString> GetDevWriteStoragePath();

        // Attempts the supplied path to an absolute path.
        //! Returns nullopt if path cannot be converted to an absolute path
        AZStd::optional<AZ::IO::FixedMaxPathString> ConvertToAbsolutePath(AZStd::string_view path);
        bool ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 absolutePathMaxSize);

        //! Save a string to a file. Otherwise returns a failure with error message.
        AZ::Outcome<void, AZStd::string> WriteFile(AZStd::string_view content, AZStd::string_view filePath);

        //! Read a file into a string. Returns a failure with error message if the content could not be loaded or if
        //! the file size is larger than the max file size provided.
        template<typename Container = AZStd::string>
        AZ::Outcome<Container, AZStd::string> ReadFile(
            AZStd::string_view filePath, size_t maxFileSize = AZStd::numeric_limits<size_t>::max());
    }
}
