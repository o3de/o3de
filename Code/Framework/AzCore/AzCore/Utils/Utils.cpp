/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/FileReader.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AZ::Utils::ConsoleCommands
{
    // Register Console Command that can query the OS environment variables
    constexpr const char* GetEnvCommandName = "os_GetEnv";
    void GetEnv(const AZ::ConsoleCommandContainer& commandArgs)
    {
        if (commandArgs.empty())
        {
            AZ_Error(
                "os_GetEnv",
                false,
                "No environment variable name has been supplied.");
            return;
        }

        for (AZStd::string_view commandArg : commandArgs)
        {
            auto QueryOSEnvironmentVariable = [commandArg](char* buffer, size_t size)
            {
                // 1024 is an arbitrary power of 2 size that we already have
                // a template instantiation of(so therefore the symbol size of the library will not increase).
                // It should be more than enough to store any reasonable environment variable name that the caller
                // would want to query
                constexpr size_t EnvKeyMaxSize = 1024;
                AZStd::fixed_string<EnvKeyMaxSize> envKey{ commandArg };
                auto getEnvOutcome = AZ::Utils::GetEnv(AZStd::span(buffer, size), envKey.c_str());
                return getEnvOutcome ? getEnvOutcome.GetValue().size() : 0;
            };

            // There is actually no limit to environment variable sizes on Unix platforms, but there is a limit
            // of 8191 on Windows: See doc page at
            // https://learn.microsoft.com/en-US/troubleshoot/windows-client/shell-experience/command-line-string-limitation#examples
            // However 8192 is being used as the limit as that should cover 100% of Windows use cases
            // and 99.99 % of the use cases of querying an environment varaible on Unix platforms
            // Also the string made of combining the environment varaible name with an <equal-sign> and the environment variable value
            // will be truncated to fit within 8192 characters
            constexpr size_t EnvKeyPlusValueMaxSize = 8192;
            AZStd::fixed_string<EnvKeyPlusValueMaxSize> envValue;
            envValue.resize_and_overwrite(envValue.capacity(), QueryOSEnvironmentVariable);
            auto formattedEnvOutput = AZStd::fixed_string<EnvKeyPlusValueMaxSize>::format("%.*s=%s\n", AZ_STRING_ARG(commandArg), envValue.c_str());
            AZ::Debug::Trace::Instance().Output(GetEnvCommandName, formattedEnvOutput.c_str());
        }
    }
    AZ_CONSOLEFREEFUNC("os_GetEnv", GetEnv, AZ::ConsoleFunctorFlags::DontReplicate,
        "Queries the OS for an environment variable with the specified name(s).\n"
        "Each argument is the name of the environment variable to query.\n"
        "Multiple arguments can be specified and each of their values are output on separate lines.\n"
        "Usage: os_GetEnv PATH\n"
        "Usage: os_GetEnv LD_LIBRARY_PATH HOME\n");
}

namespace AZ::Utils
{
    ExecutablePathResult GetExecutableDirectory(char* exeStorageBuffer, size_t exeStorageSize)
    {
        AZ::Utils::GetExecutablePathReturnType result = GetExecutablePath(exeStorageBuffer, exeStorageSize);
        if (result.m_pathStored != AZ::Utils::ExecutablePathResult::Success)
        {
            *exeStorageBuffer = '\0';
            return result.m_pathStored;
        }

        // If it contains the filename, zero out the last path separator character...
        if (result.m_pathIncludesFilename)
        {
            AZ::IO::PathView exePathView(exeStorageBuffer);
            AZStd::string_view exeDirectory = exePathView.ParentPath().Native();
            exeStorageBuffer[exeDirectory.size()] = '\0';
        }

        return result.m_pathStored;
    }

    AZ::IO::FixedMaxPathString GetExecutableDirectory()
    {
        AZ::IO::FixedMaxPathString executableDirectory;
        if(GetExecutableDirectory(executableDirectory.data(), executableDirectory.capacity())
            == ExecutablePathResult::Success)
        {
            // Updated the size field within the fixed string by using char_traits to calculate the string length
            executableDirectory.resize_no_construct(AZStd::char_traits<char>::length(executableDirectory.data()));
        }

        return executableDirectory;
    }

    AZStd::optional<AZ::IO::FixedMaxPathString> ConvertToAbsolutePath(AZStd::string_view path)
    {
        AZ::IO::FixedMaxPathString absolutePath;
        AZ::IO::FixedMaxPathString srcPath{ path };
        if (ConvertToAbsolutePath(srcPath.c_str(), absolutePath.data(), absolutePath.capacity()))
        {
            // Fix the size value of the fixed string by calculating the c-string length using char traits
            absolutePath.resize_no_construct(AZStd::char_traits<char>::length(absolutePath.data()));
            return absolutePath;
        }

        return AZStd::nullopt;
    }

    bool ConvertToAbsolutePath(AZ::IO::FixedMaxPath& outputPath, AZStd::string_view path)
    {
        AZ::IO::FixedMaxPathString srcPath{ path };
        if (ConvertToAbsolutePath(srcPath.c_str(), outputPath.Native().data(), outputPath.Native().capacity()))
        {
            // Fix the size value of the fixed string by calculating the c-string length using char traits
            outputPath.Native().resize_no_construct(AZStd::char_traits<char>::length(outputPath.Native().data()));
            return true;
        }

        return false;
    }

    AZ::IO::FixedMaxPathString GetO3deManifestDirectory(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            AZ::SettingsRegistryInterface::FixedValueString settingsValue;
            if (settingsRegistry->Get(settingsValue, AZ::SettingsRegistryMergeUtils::FilePathKey_O3deManifestRootFolder))
            {
                return AZ::IO::FixedMaxPathString{ settingsValue };
            }
        }

        // If the O3DEManifest key isn't set in the settings registry
        // fallback to use the user's home directory with the .o3de folder appended to it
        AZ::IO::FixedMaxPath path = GetHomeDirectory(settingsRegistry);
        path /= ".o3de";
        return path.Native();
    }

    AZ::IO::FixedMaxPathString GetO3deManifestPath(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        AZ::IO::FixedMaxPath o3deManifestPath = GetO3deManifestDirectory(settingsRegistry);
        if (!o3deManifestPath.empty())
        {
            o3deManifestPath /= "o3de_manifest.json";
        }

        return o3deManifestPath.Native();
    }

    AZ::IO::FixedMaxPathString GetO3deLogsDirectory(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        AZ::IO::FixedMaxPath path = GetO3deManifestDirectory(settingsRegistry);
        path /= "Logs";
        return path.Native();
    }

    AZ::IO::FixedMaxPathString GetEnginePath(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            if (AZ::IO::FixedMaxPathString settingsValue;
                settingsRegistry->Get(settingsValue, AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
            {
                return settingsValue;
            }
        }
        return {};
    }

    AZ::IO::FixedMaxPathString GetProjectPath(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            if (AZ::IO::FixedMaxPathString settingsValue;
                settingsRegistry->Get(settingsValue, AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath))
            {
                return settingsValue;
            }
        }
        return {};
    }

    AZ::SettingsRegistryInterface::FixedValueString GetProjectName(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            FixedValueString projectNameKey{ AZ::SettingsRegistryMergeUtils::ProjectSettingsRootKey };
            projectNameKey += "/project_name";

            if (FixedValueString settingsValue;
                settingsRegistry->Get(settingsValue, projectNameKey))
            {
                return settingsValue;
            }
        }
        return {};
    }

    AZ::SettingsRegistryInterface::FixedValueString GetProjectDisplayName(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            FixedValueString projectNameKey{ AZ::SettingsRegistryMergeUtils::ProjectSettingsRootKey };
            projectNameKey += "/display_name";

            if (FixedValueString settingsValue;
                settingsRegistry->Get(settingsValue, projectNameKey))
            {
                return settingsValue;
            }
        }
        // fallback to querying the "project_name", if the "display_name" could not be read
        return GetProjectName(settingsRegistry);
    }

    AZ::IO::FixedMaxPathString GetGemPath(AZStd::string_view gemName, AZ::SettingsRegistryInterface* settingsRegistry)
    {
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            const auto manifestGemJsonPath = FixedValueString::format("%s/%.*s/Path",
                AZ::SettingsRegistryMergeUtils::ManifestGemsRootKey, AZ_STRING_ARG(gemName));

            if (AZ::IO::FixedMaxPathString settingsValue;
                settingsRegistry->Get(settingsValue, manifestGemJsonPath))
            {
                return settingsValue;
            }
        }
        return {};
    }

    AZ::IO::FixedMaxPathString GetProjectUserPath(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            if (AZ::IO::FixedMaxPathString settingsValue;
                settingsRegistry->Get(settingsValue, AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath))
            {
                return settingsValue;
            }
        }
        return {};
    }

    AZ::IO::FixedMaxPathString GetProjectLogPath(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            if (AZ::IO::FixedMaxPathString settingsValue;
                settingsRegistry->Get(settingsValue, AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectLogPath))
            {
                return settingsValue;
            }
        }
        return {};
    }

    AZ::IO::FixedMaxPathString GetDevWriteStoragePath(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            if (AZ::IO::FixedMaxPathString settingsValue;
                settingsRegistry->Get(settingsValue, AZ::SettingsRegistryMergeUtils::FilePathKey_DevWriteStorage))
            {
                return settingsValue;
            }
        }
        return {};
    }

    AZ::IO::FixedMaxPathString GetProjectProductPathForPlatform(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            if (AZ::IO::FixedMaxPathString settingsValue;
                settingsRegistry->Get(settingsValue, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder))
            {
                return settingsValue;
            }
        }
        return {};
    }

    AZ::Outcome<void, AZStd::string> WriteFile(AZStd::string_view content, AZStd::string_view filePath)
    {
        return WriteFile(AZStd::span(reinterpret_cast<const AZStd::byte*>(content.data()), content.size()), filePath);
    }

    AZ::Outcome<void, AZStd::string> WriteFile(AZStd::span<AZStd::byte const> content, AZStd::string_view filePath)
    {
        AZ::IO::FixedMaxPath filePathFixed = filePath; // Because FileIOStream requires a null-terminated string
        AZ::IO::FileIOStream stream(filePathFixed.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);

        bool success = false;

        if (stream.IsOpen())
        {
            AZ::IO::SizeType bytesWritten = stream.Write(content.size(), content.data());

            if (bytesWritten == content.size())
            {
                success = true;
            }
        }

        if (success)
        {
            return AZ::Success();
        }
        else
        {
            return AZ::Failure(AZStd::string::format("Could not write to file '%s'", filePathFixed.c_str()));
        }
    }

    template<typename Container>
    AZ::Outcome<Container, AZStd::string> ReadFile(AZStd::string_view filePath, size_t maxFileSize)
    {
        IO::FileReader file;
        if (!file.Open(AZ::IO::FileIOBase::GetInstance(), filePath.data()))
        {
            return AZ::Failure(AZStd::string::format("Failed to open '%.*s'.", AZ_STRING_ARG(filePath)));
        }

        AZ::IO::SizeType length = file.Length();

        if (length > maxFileSize)
        {
            return AZ::Failure(AZStd::string{ "Data is too large." });
        }
        else if (length == 0)
        {
            return AZ::Failure(AZStd::string::format("Failed to load '%.*s'. File is empty.", AZ_STRING_ARG(filePath)));
        }

        Container fileContent;
        fileContent.resize_no_construct(length);
        AZ::IO::SizeType bytesRead = file.Read(length, fileContent.data());
        file.Close();

        // Resize again just in case bytesRead is less than length for some reason
        fileContent.resize_no_construct(bytesRead);
        return AZ::Success(AZStd::move(fileContent));
    }

    AZ::Outcome<AZStd::string, AZStd::string> Get3rdPartyDirectory(AZ::SettingsRegistryInterface* settingsRegistry /*= nullptr*/)
    {
        // Locate the manifest file
        auto manifestPath = GetO3deManifestPath(settingsRegistry);

        auto readJsonDocOutput = AZ::JsonSerializationUtils::ReadJsonFile(manifestPath);
        if (!readJsonDocOutput.IsSuccess())
        {
            return AZ::Failure(readJsonDocOutput.GetError());
        }

        rapidjson::Document configurationFile = readJsonDocOutput.TakeValue();

        auto thirdPartyFolder = configurationFile["default_third_party_folder"].GetString();
        return AZ::Success(thirdPartyFolder);
    }

    AZ::IO::FixedMaxPathString GetO3dePythonVenvRoot(AZ::SettingsRegistryInterface* settingsRegistry /*= nullptr*/)
    {
        // Locate the manifest directory
        auto manifestDirectory = AZ::IO::FixedMaxPath(GetO3deManifestDirectory(settingsRegistry)) / "Python" / "venv";
        return manifestDirectory.Native();
    }


    template AZ::Outcome<AZStd::string, AZStd::string> ReadFile(AZStd::string_view filePath, size_t maxFileSize);
    template AZ::Outcome<AZStd::vector<int8_t>, AZStd::string> ReadFile(AZStd::string_view filePath, size_t maxFileSize);
    template AZ::Outcome<AZStd::vector<uint8_t>, AZStd::string> ReadFile(AZStd::string_view filePath, size_t maxFileSize);
    template AZ::Outcome<AZStd::vector<AZStd::byte>, AZStd::string> ReadFile(AZStd::string_view filePath, size_t maxFileSize);
}
