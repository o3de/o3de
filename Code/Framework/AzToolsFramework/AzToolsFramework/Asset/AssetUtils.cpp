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
#include <AzCore/Utils/Utils.h>
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
        void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, AZStd::string_view value) override
        {
            if (value == "enabled")
            {
                m_enabledPlatforms.emplace_back(visitArgs.m_fieldName);
            }
            else if (value == "disabled")
            {
                auto platformEntrySearch = [&visitArgs](AZStd::string_view platformEntry)
                {
                    return visitArgs.m_fieldName == platformEntry;
                };
                auto removeIt = AZStd::remove_if(m_enabledPlatforms.begin(), m_enabledPlatforms.end(), platformEntrySearch);
                m_enabledPlatforms.erase(removeIt, m_enabledPlatforms.end());
            }
        }

        AZStd::vector<AZStd::string> m_enabledPlatforms;
    };

    void ReadEnabledPlatformsFromSettingsRegistry(AZ::SettingsRegistryInterface& settingsRegistry,
        AZStd::vector<AZStd::string>& enabledPlatforms)
    {
        // note that the current host platform is enabled by default.
        // in the setreg the platform can be missing (commented out)
        // in which case it is disabled implicitly by not being there
        // or it can be 'disabled' which means that it is explicitly disabled.
        // or it can be 'enabled' which means that it is explicitly enabled.
        EnabledPlatformsVisitor visitor;
        visitor.m_enabledPlatforms.push_back(AzToolsFramework::AssetSystem::GetHostAssetPlatform());
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

    AZStd::vector<AZ::IO::Path> GetConfigFiles(AZStd::string_view engineRoot, AZStd::string_view projectPath,
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

        AZ::IO::Path projectRoot(projectPath);

        AZ::IO::Path projectConfigFile = projectRoot / AssetProcessorGamePlatformConfigFileName;
        configFiles.push_back(projectConfigFile);

        // Add a file entry for the Project AssetProcessor setreg file
        projectConfigFile = projectRoot / AssetProcessorGamePlatformConfigSetreg;
        configFiles.push_back(projectConfigFile);

        return configFiles;
    }

    bool UpdateFilePathToCorrectCase(AZStd::string_view rootPath, AZStd::string& relPathFromRoot, bool checkEntirePath /* = true */)
    {
        // The reason this is so expensive is that it could also be the case that the DIRECTORY path is the wrong case.
        // For example, the actual path might be /SomeFolder/textures/whatever/texture.dat
        // but the real file on disk is actually /SomeFolder/Textures/Whatever/texture.dat 
        // Note the case difference of the directories.  If you were to ask the operating system just to list all of
        // the files in the input folder (/SomeFolder/textures/whatever/*) it would not find any since the directory 
        // does not itself exist.  Not only that, but many operating system calls on case-insensitive file systems
        // actually use the input given in their return - that is, if you ask a WINAPI call to construct an absolute path
        // to a file and give it the wrong case as input, the output absolute path will also be the wrong case.
        // Thus, to actually correct a path it has to start at the bottom and whenever it encounters
        // a path segment, it has to do a read of the actual directory information to see which files exist in that
        // directory, and whether a file or directory exists with the correct name but with different case.

        // if checkEntirePath is false, we only check the file name, and nothing else.  This is for the case where
        // the path is known to be good except for the file name, such as when the relPathFromRoot comes from just
        // taking an existing, known-good file and replacing its extension only.

        AZ::StringFunc::Path::Normalize(relPathFromRoot);
        AZStd::vector<AZStd::string> tokens;
        AZ::StringFunc::Tokenize(relPathFromRoot.c_str(), tokens, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

        AZ::IO::FixedMaxPath validatedPath;
        if (rootPath.empty())
        {
            validatedPath = AZ::Utils::GetEnginePath();
        }
        else
        {
            validatedPath = rootPath;
        }

        bool success = true;

        for (int idx = 0; idx < tokens.size(); idx++)
        {
            if (!checkEntirePath)
            {
                if (idx != tokens.size() - 1)
                {
                    // only the last token is potentially incorrect, we can skip filenames before that
                    validatedPath /= tokens[idx]; // go one step deeper.
                    continue;
                }
            }
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

            validatedPath /= element; // go one step deeper.
        }

        if (success)
        {
            relPathFromRoot.clear();
            AZ::StringFunc::Join(relPathFromRoot, tokens.begin(), tokens.end(), AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
        }

        return success;
    }
} //namespace AzToolsFramework::AssetUtils
