/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Asset/AssetUtils.h>
#include <QString>

namespace AzToolsFramework::AssetUtils::Internal
{
    constexpr const char* AssetProcessorSettingsKey{ "/Amazon/AssetProcessor/Settings" };

    constexpr const char* AssetConfigPlatformDir = "AssetProcessorConfig";
    constexpr const char* RestrictedPlatformDir = "restricted";

    AZStd::vector<AZ::IO::Path> FindWildcardMatches(AZStd::string_view sourceFolder, AZStd::string_view relativeName)
    {
        if (relativeName.empty())
        {
            return {};
        }

        AZ::IO::Path sourceWildcard{ sourceFolder };


        AZStd::vector<AZ::IO::Path> returnList;

        // Walks the sourceFolder and subdirectories to search for the relativeName as a wild card using a breathe-first search
        AZStd::queue<AZ::IO::Path> searchFolders;
        searchFolders.push(AZStd::move(sourceWildcard));
        while (!searchFolders.empty())
        {
            const AZ::IO::Path& searchFolder = searchFolders.front();
            AZ::IO::SystemFile::FindFileCB findFiles = [&returnList, &relativeName, &searchFolder, &searchFolders](AZStd::string_view fileView, bool isFile) -> bool
            {
                if (fileView == "." || fileView == "..")
                {
                    return true;
                }

                if (isFile)
                {
                    if (AZStd::wildcard_match(relativeName, fileView))
                    {
                        returnList.emplace_back(searchFolder / fileView);
                    }
                }
                else
                {
                    // Use the current search directory to append the fileView directory wild card
                    searchFolders.push(searchFolder / fileView);
                }

                return true;
            };


            AZ::IO::SystemFile::FindFiles((searchFolder / "*").c_str(), findFiles);
            searchFolders.pop();
        }

        return returnList;
    }

    void AddGemConfigFiles(const AZStd::vector<AzFramework::GemInfo>& gemInfoList, AZStd::vector<AZ::IO::Path>& configFiles)
    {
        const char* AssetProcessorGemConfigIni = "AssetProcessorGemConfig.ini";
        const char* AssetProcessorGemConfigSetreg = "AssetProcessorGemConfig.setreg";
        for (const AzFramework::GemInfo& gemElement : gemInfoList)
        {
            for (const AZ::IO::Path& gemAbsoluteSourcePath : gemElement.m_absoluteSourcePaths)
            {
                configFiles.push_back(gemAbsoluteSourcePath / AssetProcessorGemConfigIni);
                configFiles.push_back(gemAbsoluteSourcePath / AssetProcessorGemConfigSetreg);
            }
        }
    }
}

namespace AzToolsFramework::AssetUtils
{
    // Visitor for reading the "/Amazon/AssetProcessor/Settings/Platforms" entries from the Settings Registry
    struct EnabledPlatformsVisitor
        : AZ::SettingsRegistryInterface::Visitor
    {
        using AZ::SettingsRegistryInterface::Visitor::Visit;
        void Visit(AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value) override;

        AZStd::vector<AZStd::string> m_enabledPlatforms;
    };

    void EnabledPlatformsVisitor::Visit([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value)
    {
        if (value == "enabled")
        {
            m_enabledPlatforms.emplace_back(valueName);
        }
        else if (value == "disabled")
        {
            auto platformEntrySearch = [&valueName](AZStd::string_view platformEntry)
            {
                return valueName == platformEntry;
            };
            auto removeIt = AZStd::remove_if(m_enabledPlatforms.begin(), m_enabledPlatforms.end(), platformEntrySearch);
            m_enabledPlatforms.erase(removeIt, m_enabledPlatforms.end());
        }
    }

    void ReadEnabledPlatformsFromSettingsRegistry(AZ::SettingsRegistryInterface& settingsRegistry,
        AZStd::vector<AZStd::string>& enabledPlatforms)
    {
        // note that the current host platform is enabled by default.
        enabledPlatforms.push_back(AzToolsFramework::AssetSystem::GetHostAssetPlatform());

        // in the setreg the platform can be missing (commented out)
        // in which case it is disabled implicitly by not being there
        // or it can be 'disabled' which means that it is explicitly disabled.
        // or it can be 'enabled' which means that it is explicitly enabled.
        EnabledPlatformsVisitor visitor;
        settingsRegistry.Visit(visitor, AZ::SettingsRegistryInterface::FixedValueString(Internal::AssetProcessorSettingsKey) + "/Platforms");
        enabledPlatforms.insert(enabledPlatforms.end(), AZStd::make_move_iterator(visitor.m_enabledPlatforms.begin()),
            AZStd::make_move_iterator(visitor.m_enabledPlatforms.end()));
    }

    AZStd::vector<AZStd::string> GetEnabledPlatforms(AZ::SettingsRegistryInterface& settingsRegistry,
        const AZStd::vector<AZ::IO::Path>& configFiles)
    {
        AZStd::vector<AZStd::string> enabledPlatforms;

        for (const auto& configFile : configFiles)
        {
            if (AZ::IO::SystemFile::Exists(configFile.c_str()))
            {
                // If the config file is a settings registry file use the SettingsRegistryInterface MergeSettingsFile function
                // otherwise use the SettingsRegistryMergeUtils MergeSettingsToRegistry_ConfigFile function to merge an INI-style
                // file to the settings registry
                if (configFile.Extension() == ".setreg")
                {
                    settingsRegistry.MergeSettingsFile(configFile.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch);
                }
                else
                {
                    AZ::SettingsRegistryMergeUtils::ConfigParserSettings configParserSettings;
                    configParserSettings.m_registryRootPointerPath = Internal::AssetProcessorSettingsKey;
                    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ConfigFile(settingsRegistry, configFile.Native(), configParserSettings);
                }
            }
        }
        ReadEnabledPlatformsFromSettingsRegistry(settingsRegistry, enabledPlatforms);
        return enabledPlatforms;
    }

    constexpr const char* AssetProcessorPlatformConfigFileName = "AssetProcessorPlatformConfig.ini";
    constexpr const char* AssetProcessorPlatformConfigSetreg = "AssetProcessorPlatformConfig.setreg";

    bool AddPlatformConfigFilePaths(AZStd::string_view engineRoot, AZStd::vector<AZ::IO::Path>& configFilePaths)
    {
        auto restrictedRoot = AZ::IO::Path{ engineRoot } / Internal::RestrictedPlatformDir;

        // first collect public platform configs
        AZStd::vector<AZ::IO::Path> platformDirs{ AZ::IO::Path{ engineRoot } / Internal::AssetConfigPlatformDir };

        // then collect restricted platform configs
        // Append the AssetConfigPlatformDir value to each directory
        AZ::IO::SystemFile::FindFileCB findRestrictedAssetConfigs = [&restrictedRoot, &platformDirs](AZStd::string_view fileView, bool isFile) -> bool
        {
            if (fileView != "." && fileView != "..")
            {
                if (!isFile)
                {
                    platformDirs.push_back(restrictedRoot / fileView / Internal::AssetConfigPlatformDir);
                }
            }
            return true;
        };
        AZ::IO::SystemFile::FindFiles((restrictedRoot / "*").c_str(), findRestrictedAssetConfigs);

        // Iterator over all platform directories for platform config files
        AZStd::vector<AZ::IO::Path> allPlatformConfigs;
        for (const auto& platformDir : platformDirs)
        {
            for (const char* configPath : { AssetProcessorPlatformConfigFileName, AssetProcessorPlatformConfigSetreg })
            {
                AZStd::vector<AZ::IO::Path> platformConfigs = Internal::FindWildcardMatches(platformDir.Native(), configPath);
                allPlatformConfigs.insert(allPlatformConfigs.end(), AZStd::make_move_iterator(platformConfigs.begin()), AZStd::make_move_iterator(platformConfigs.end()));
            }
        }

        const bool platformConfigFilePathsAdded = !allPlatformConfigs.empty();
        configFilePaths.insert(configFilePaths.end(), AZStd::make_move_iterator(allPlatformConfigs.begin()), AZStd::make_move_iterator(allPlatformConfigs.end()));
        return platformConfigFilePathsAdded;
    }

    AZStd::vector<AZ::IO::Path> GetConfigFiles(AZStd::string_view engineRoot, AZStd::string_view assetRoot, AZStd::string_view projectPath,
        bool addPlatformConfigs, bool addGemsConfigs, AZ::SettingsRegistryInterface* settingsRegistry)
    {
        constexpr const char* AssetProcessorGamePlatformConfigFileName = "AssetProcessorGamePlatformConfig.ini";
        constexpr const char* AssetProcessorGamePlatformConfigSetreg = "AssetProcessorGamePlatformConfig.setreg";
        AZStd::vector<AZ::IO::Path> configFiles;

        // Add the AssetProcessorPlatformConfig setreg file at the engine root
        configFiles.push_back(AZ::IO::Path(engineRoot) / AssetProcessorPlatformConfigSetreg);

        if (addPlatformConfigs)
        {
            AddPlatformConfigFilePaths(engineRoot, configFiles);
        }

        if (addGemsConfigs)
        {
            AZStd::vector<AzFramework::GemInfo> gemInfoList;
            if (!AzFramework::GetGemsInfo(gemInfoList, *settingsRegistry))
            {
                AZ_Error("AzToolsFramework::AssetUtils", false, "Failed to read gems from project folder(%.*s).\n", AZ_STRING_ARG(projectPath));
                return {};
            }

            Internal::AddGemConfigFiles(gemInfoList, configFiles);
        }

        AZ::IO::Path assetRootDir(assetRoot);
        assetRootDir /= projectPath;

        AZ::IO::Path projectConfigFile = assetRootDir / AssetProcessorGamePlatformConfigFileName;
        configFiles.push_back(projectConfigFile);

        // Add a file entry for the Project AssetProcessor setreg file
        projectConfigFile = assetRootDir / AssetProcessorGamePlatformConfigSetreg;
        configFiles.push_back(projectConfigFile);

        return configFiles;
    }

    bool UpdateFilePathToCorrectCase(AZStd::string_view rootPath, AZStd::string& relPathFromRoot)
    {
        AZ::StringFunc::Path::Normalize(relPathFromRoot);
        AZStd::vector<AZStd::string> tokens;
        AZ::StringFunc::Tokenize(relPathFromRoot.c_str(), tokens, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

        AZStd::string validatedPath;
        if (rootPath.empty())
        {
            AzFramework::ApplicationRequests::Bus::BroadcastResult(validatedPath, &AzFramework::ApplicationRequests::GetEngineRoot);
        }
        else
        {
            validatedPath = rootPath;
        }

        bool success = true;

        for (int idx = 0; idx < tokens.size(); idx++)
        {
            AZStd::string element = tokens[idx];
            bool foundAMatch = false;
            AZ::IO::FileIOBase::GetInstance()->FindFiles(validatedPath.c_str(), "*", [&](const char* file)
            {
                if (idx != tokens.size() - 1 && !AZ::IO::FileIOBase::GetInstance()->IsDirectory(file))
                {
                    // only the last token is supposed to be a filename, we can skip filenames before that
                    return true;
                }

                AZStd::string absFilePath(file);
                AZ::StringFunc::Path::Normalize(absFilePath);
                auto found = absFilePath.rfind(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
                size_t startingPos = found + 1;
                if (found != AZStd::string::npos && absFilePath.size() > startingPos)
                {
                    AZStd::string componentName = AZStd::string(absFilePath.begin() + startingPos, absFilePath.end());
                    if (AZ::StringFunc::Equal(componentName.c_str(), tokens[idx].c_str()))
                    {
                        tokens[idx] = componentName;
                        foundAMatch = true;
                        return false;
                    }
                }

                return true;
            });

            if (!foundAMatch)
            {
                success = false;
                break;
            }

            AZStd::string absoluteFilePath;
            AZ::StringFunc::Path::ConstructFull(validatedPath.c_str(), element.c_str(), absoluteFilePath);

            validatedPath = absoluteFilePath; // go one step deeper.
        }

        if (success)
        {
            relPathFromRoot.clear();
            AZ::StringFunc::Join(relPathFromRoot, tokens.begin(), tokens.end(), AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        }

        return success;
    }
} //namespace AzToolsFramework::AssetUtils
