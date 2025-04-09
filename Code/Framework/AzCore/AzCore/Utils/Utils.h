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

        //! Retrieves the full path of the directory containing the executable
        AZ::IO::FixedMaxPathString GetExecutableDirectory();

        //! Retrieves the full path to the engine from the settings registry
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetEnginePath(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the full path to the project from the settings registry
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetProjectPath(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the full path to the project user path from the settings registry
        //! This path defaults to <project-path>/user, but can be overridden via the --project-user-path option
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetProjectUserPath(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the full path to the project log path from the settings registry
        //! This path defaults to <project-user-path>/logs, but can be overridden via the --project-log-path option
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetProjectLogPath(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the full path to the project path for the current asset platform from the settings registry
        //! This path is based on <project-cache-path>/<asset-platform>,
        //! where on Windows <asset-platform> = "pc", Linux  <asset-platform> = linux, etc...
        //! The list of OS -> asset platforms is available in the `AZ::PlatformDefaults::OSPlatformToDefaultAssetPlatform` function
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetProjectProductPathForPlatform(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the project name from the settings registry
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::SettingsRegistryInterface::FixedValueString GetProjectName(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the project display name from the settings registry
        //! First attempts to lookup the "display_name" field read from the project.json settings section of the
        //! Settings Registry ! and if that is not available, the "project_name" field is read instead.
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::SettingsRegistryInterface::FixedValueString GetProjectDisplayName(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Lookups the full path for a gem, given it's name from the settings registry
        //! @param gemName path of the gem whose paths will be queried from within the settings registry
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetGemPath(AZStd::string_view gemName, AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the full directory to the Home directory, i.e. "<userhome>" or overrideHomeDirectory
        //! @param settingsRegistry pointer to the SettingsRegistry to lookup the overrideHomeDirectory
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetHomeDirectory(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the full directory to the O3DE manifest directory, i.e. "<userhome>/.o3de"
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetO3deManifestDirectory(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the full path where the manifest file lives, i.e. "<userhome>/.o3de/o3de_manifest.json"
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetO3deManifestPath(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the full directory to the O3DE logs directory, i.e. "<userhome>/.o3de/Logs"
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetO3deLogsDirectory(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves a full directory which can be used to write files into during development
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetDevWriteStoragePath(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the application root path for a non-host platform
        //! On host platforms this returns a nullopt
        AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultAppRootPath();

        //! Retrieves the default development write storage path to use on the current platform, may be considered
        //! temporary or cache storage
        AZStd::optional<AZ::IO::FixedMaxPathString> GetDefaultDevWriteStoragePath();

        //! Attempts the supplied path to an absolute path.
        //! Returns nullopt if path cannot be converted to an absolute path
        AZStd::optional<AZ::IO::FixedMaxPathString> ConvertToAbsolutePath(AZStd::string_view path);
        bool ConvertToAbsolutePath(AZ::IO::FixedMaxPath& outputPath, AZStd::string_view path);
        bool ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 absolutePathMaxSize);

        //! Save a string to a file. Otherwise returns a failure with error message.
        AZ::Outcome<void, AZStd::string> WriteFile(AZStd::string_view content, AZStd::string_view filePath);
        AZ::Outcome<void, AZStd::string> WriteFile(AZStd::span<AZStd::byte const> content, AZStd::string_view filePath);

        //! Read a file into a string. Returns a failure with error message if the content could not be loaded or if
        //! the file size is larger than the max file size provided.
        template<typename Container = AZStd::string>
        AZ::Outcome<Container, AZStd::string> ReadFile(
            AZStd::string_view filePath, size_t maxFileSize = AZStd::numeric_limits<size_t>::max());


        //! Retrieves the full path where the 3rd Party path is configured to based on the value of 'default_third_party_folder'
        //! in the o3de manifest file (o3de_manifest.json)
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup of the manifest file to base the lookup from
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        //! Returns the outcome of the request, the configured 3rd Party path if successful, the error message if not.
        AZ::Outcome<AZStd::string, AZStd::string> Get3rdPartyDirectory(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);

        //! Retrieves the full directory to the O3DE Python virtual environment (venv) folder. 
        //! @param settingsRegistry pointer to the SettingsRegistry to use for lookup
        //! If nullptr, the AZ::Interface instance of the SettingsRegistry is used
        AZ::IO::FixedMaxPathString GetO3dePythonVenvRoot(AZ::SettingsRegistryInterface* settingsRegistry = nullptr);


        //! error code value returned when GetEnv fails
        enum class GetEnvErrorCode
        {
            EnvNotSet = 1,
            BufferTooSmall,
            NotImplemented
        };

        //! Return result from GetEnv when an environment variable value
        //! fails to be returned
        //! The `m_requiredSize` value is set to the needed size of the buffer
        //! when the GetEnvErrorCode is set to `BufferTooSmall`
        struct GetEnvErrorResult
        {
            //! Contains the error code of the GetEnv operation
            GetEnvErrorCode m_errorCode{};
            //! Set only when the error code is `BufferTooSmall`
            size_t m_requiredSize{};
        };
        using GetEnvOutcome = AZ::Outcome<AZStd::string_view, GetEnvErrorResult>;

        //! Queries the value of an environment variable
        //! @param envvalueBuffer Span of buffer to populate with environment variable value if found
        //! @param envname The environment variable name
        //! @return Return an outcome with a string_view of the environment varaible if successful,
        //! otherwise if the environment variable is not set or the span size cannot fit the environment
        //! variable value, a GetEnvErrorResult will be returned
        GetEnvOutcome GetEnv(AZStd::span<char> envvalueBuffer, const char* envname);

        //! Queries if environment variable is set
        //! @param envname The environment variable name
        //! @return Return true if successful, otherwise false
        bool IsEnvSet(const char* envname);

        //! Create or modify environment variable.
        //! @param envname The environment variable name
        //! @param envvalue The environment variable value
        //! @param overwrite If name does exist in the environment, then its value is changed to value if overwrite is nonzero;
        //! if overwrite is zero, then the value of name is not changed
        //! @returns Return true if successful, otherwise false
        bool SetEnv(const char* envname, const char* envvalue, bool overwrite);

        //! Remove environment variable.
        //! @param envname The environment variable name
        //! @returns Return true if successful, otherwise false
        bool UnsetEnv(const char* envname);
    }
}
