/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Utils.h"
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/base.h>
#include <AzCore/std/functional.h>
#include <algorithm>
#include <cstring>

namespace AZ::Test
{
    bool ContainsParameter(int argc, char** argv, const std::string& param)
    {
        return GetParameterIndex(argc, argv, param) >= 0;
    }

    void CopyParameters(int argc, char** target, char** source)
    {
        for (int i = 0; i < argc; i++)
        {
            const size_t dstSize = std::strlen(source[i]) + 1;
            target[i] = new char[dstSize];
            azstrcpy(target[i], dstSize, source[i]);
        }
    }

    int GetParameterIndex(int argc, char** argv, const std::string& param)
    {
        for (int i = 0; i < argc; i++)
        {
            if (param == argv[i])
            {
                return i;
            }
        }
        return -1;
    }

    std::vector<std::string> GetParameterList(int& argc, char** argv, const std::string& param, bool removeOnReturn)
    {
        std::vector<std::string> parameters;
        int paramIndex = GetParameterIndex(argc, argv, param);
        if (paramIndex > 0)
        {
            int index = paramIndex + 1;
            while (index < argc && !StartsWith(argv[index], "-"))
            {
                parameters.push_back(std::string(argv[index]));
                index++;
            }
        }
        if (removeOnReturn)
        {
            RemoveParameters(argc, argv, paramIndex, paramIndex + (int)parameters.size());
        }
        return parameters;
    }

    std::string GetParameterValue(int& argc, char** argv, const std::string& param, bool removeOnReturn)
    {
        std::string value("");
        int index = GetParameterIndex(argc, argv, param);
        // Make sure we have a valid parameter index and value after the parameter index
        if (index > 0 && index < (argc - 1))
        {
            value = argv[index + 1];
            if (removeOnReturn)
            {
                RemoveParameters(argc, argv, index, index + 1);
            }
        }
        return value;
    }

    void RemoveParameters(int& argc, char** argv, int startIndex, int endIndex)
    {
        // protect against invalid order of parameters
        if (startIndex > endIndex)
        {
            return;
        }

        // constraint to valid range
        endIndex = std::min(endIndex, argc - 1);
        startIndex = std::max(startIndex, 0);

        int numRemoved = 0;
        int i = startIndex;
        int j = endIndex + 1;

        // copy all existing paramters
        while (j < argc)
        {
            argv[i++] = argv[j++];
        }

        // null out all the remaining parameters and count how many
        // were removed simultaneously
        while (i < argc)
        {
            argv[i++] = nullptr;
            ++numRemoved;
        }

        argc -= numRemoved;
    }

    char** SplitCommandLine(int& size, char* const cmdLine)
    {
        std::vector<char*> tokens;
        [[maybe_unused]] char* next_token = nullptr;
        char* tok = azstrtok(cmdLine, 0, " ", &next_token);
        while (tok != nullptr)
        {
            tokens.push_back(tok);
            tok = azstrtok(nullptr, 0, " ", &next_token);
        }
        size = (int)tokens.size();
        char** token_array = new char*[size];
        for (size_t i = 0; i < size; i++)
        {
            const size_t dstSize = std::strlen(tokens[i]) + 1;
            token_array[i] = new char[dstSize];
            azstrcpy(token_array[i], dstSize, tokens[i]);
        }
        return token_array;
    }

    bool EndsWith(const std::string& s, const std::string& ending)
    {
        if (ending.length() > s.length())
        {
            return false;
        }
        return std::equal(ending.rbegin(), ending.rend(), s.rbegin());
    }

    bool StartsWith(const std::string& s, const std::string& beginning)
    {
        if (beginning.length() > s.length())
        {
            return false;
        }
        return std::equal(beginning.begin(), beginning.end(), s.begin());
    }

    AZStd::string GetCurrentExecutablePath()
    {
        AZStd::string executablePath{ AZStd::string_view(AZ::Utils::GetExecutableDirectory()) };
        return executablePath;
    }

    AZStd::string GetEngineRootPath()
    {
        if (auto registry = AZ::SettingsRegistry::Get(); registry != nullptr)
        {
            return AZ::SettingsRegistryMergeUtils::FindEngineRoot(*registry).String();
        }
        else
        {
            AZ::SettingsRegistryImpl localRegistry;
            return AZ::SettingsRegistryMergeUtils::FindEngineRoot(localRegistry).String();
        }
    }

    void AddActiveGem(AZStd::string_view gemName, AZ::SettingsRegistryInterface& registry, AZ::IO::FileIOBase* fileIo)
    {
        // Merge an empty object to the ActiveGems
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        const auto activeGemEntry =
            FixedValueString::format("%s/%.*s", AZ::SettingsRegistryMergeUtils::ActiveGemsRootKey, AZ_STRING_ARG(gemName));
        registry.MergeSettings("{}", AZ::SettingsRegistryInterface::Format::JsonMergePatch, activeGemEntry);

        if (fileIo != nullptr)
        {
            auto gemPathEntry =
                FixedValueString::format("%s/%.*s/Path", AZ::SettingsRegistryMergeUtils::ManifestGemsRootKey, AZ_STRING_ARG(gemName));
            if (AZ::IO::FixedMaxPath gemRootPath; registry.Get(gemRootPath.Native(), gemPathEntry))
            {
                const auto gemAlias{ FixedValueString::format("@gemroot:%.*s@", AZ_STRING_ARG(gemName)) };
                const auto gemAliasPath = FixedValueString(gemRootPath);
                fileIo->SetAlias(gemAlias.c_str(), gemAliasPath.c_str());
            }
            else
            {
                AZ_Error(
                    "AzTest",
                    false,
                    "Cannot set @gemroot@ alias for gem \"%.*s\"."
                    " It does not have a gem root folder path within"
                    " the \"%s\" section of the SettingsRegistry. Make the sure gem is registered in an"
                    " o3de manifest (o3de_manifest.json, engine.json, project.json, gem.json)"
                    " and that the manifest files have been merged to the SettingsRegistry."
                    " Invoke `AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ManifestGemsPaths`"
                    " to explicitly merge the gem root folder paths from all manifest to the registry",
                    AZ_STRING_ARG(gemName),
                    AZ::SettingsRegistryMergeUtils::ManifestGemsRootKey);
            }
        }
    }

    AZ::IO::Path ScopedAutoTempDirectory::Resolve(const char* path) const
    {
        return AZ::IO::Path(m_tempDirectory) / path;
    }

    const char* ScopedAutoTempDirectory::GetDirectory() const
    {
        return m_tempDirectory;
    }

    AZ::IO::Path ScopedAutoTempDirectory::GetDirectoryAsPath() const
    {
        return m_tempDirectory;
    }

    AZ::IO::FixedMaxPath ScopedAutoTempDirectory::GetDirectoryAsFixedMaxPath() const
    {
        return m_tempDirectory;
    }

    // Method to delete a folder recursively
    static void DeleteFolderRecursive(const AZ::IO::PathView& path)
    {
        AZ::IO::FixedMaxPath nullTerminatedPath{ path };
        auto callback = [&nullTerminatedPath](AZStd::string_view filename, bool isFile) -> bool
        {
            if (isFile)
            {
                auto filePath = nullTerminatedPath / filename;
                AZ::IO::SystemFile::Delete(filePath.c_str());
            }
            else
            {
                if (filename != "." && filename != "..")
                {
                    auto folderPath = nullTerminatedPath / filename;
                    DeleteFolderRecursive(folderPath);
                }
            }
            return true;
        };
        auto searchPath = nullTerminatedPath / "*";
        AZ::IO::SystemFile::FindFiles(searchPath.c_str(), callback);
        AZ::IO::SystemFile::DeleteDir(nullTerminatedPath.c_str());
    }

    ScopedAutoTempDirectory::~ScopedAutoTempDirectory()
    {
        if (m_tempDirectory[0] != '\0')
        {
            // Delete the directory and its contents if a temp directory was created
            AZ::IO::PathView pathView(m_tempDirectory);
            DeleteFolderRecursive(pathView);
        }
    }

    AZStd::optional<AZ::IO::FixedMaxPath> CreateTestFile(const ScopedAutoTempDirectory& tempDirectory, AZ::IO::PathView relativePath, AZStd::string_view content)
    {
        return CreateTestFile(tempDirectory, relativePath, AZStd::span(reinterpret_cast<const AZStd::byte*>(content.data()), content.size()));
    }

    AZStd::optional<AZ::IO::FixedMaxPath> CreateTestFile(
        const ScopedAutoTempDirectory& tempDirectory, AZ::IO::PathView relativePath, AZStd::span<const AZStd::byte> content)
    {
        auto path = tempDirectory.GetDirectoryAsFixedMaxPath() / relativePath;

        constexpr auto openMode =
            AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;

        AZ::IO::SystemFile file;
        if (!file.Open(path.c_str(), openMode))
        {
            return AZStd::nullopt;
        }

        if (file.Write(content.data(), content.size()) != content.size())
        {
            // If the contents of the file could not be written return nullopt and delete the file
            AZ_Assert(false, "Unable to write content to test file: %s", path.c_str());
            file.Close();
            AZ::IO::SystemFile::Delete(file.Name());
        }

        return AZ::IO::FixedMaxPath(file.Name());
    }
} // namespace AZ::Test
