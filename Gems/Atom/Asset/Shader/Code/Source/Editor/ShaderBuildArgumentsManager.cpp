/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ShaderBuildArgumentsManager.h"

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/IO/FileIO.h>

#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RHI.Edit/ShaderBuildOptions.h>

namespace AZ
{
    namespace ShaderBuilder
    {
        static AZStd::string ResolvePathAliases(AZStd::string_view inPath)
        {
            AZ::IO::FixedMaxPath resolvedPath;
            
            auto fileIO = AZ::IO::FileIOBase::GetInstance();
            const bool success = fileIO->ResolvePath(resolvedPath, inPath);
            
            if (success)
            {
                return AZStd::string(resolvedPath.c_str());
            }

            return { inPath };
        }

        void ShaderBuildArgumentsManager::Init()
        {
            AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> removeBuildArgumentsMap;
            AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> addBuildArgumentsMap;
            auto configFiles = DiscoverConfigurationFiles();
            for (auto const& [scopeName, jsonFilePath] : configFiles)
            {
                AZ::RHI::ShaderBuildOptions shaderBuildOptions;
                if (!AZ::RPI::JsonUtils::LoadObjectFromFile(jsonFilePath.c_str(), shaderBuildOptions))
                {
                    AZ_Error(
                        LogName, false, "Failed to load shader build options file=<%s> for scope=<%s>", jsonFilePath.c_str(),
                        scopeName.c_str());
                    continue;
                }
                [[maybe_unused]]const auto addedDefinitionCount = shaderBuildOptions.m_addBuildArguments.AppendDefinitions(shaderBuildOptions.m_definitions);
                AZ_Assert(addedDefinitionCount >= 0, "Failed to add definitions");

                removeBuildArgumentsMap.emplace(scopeName, AZStd::move(shaderBuildOptions.m_removeBuildArguments));
                addBuildArgumentsMap.emplace(scopeName, AZStd::move(shaderBuildOptions.m_addBuildArguments));
            }
            Init(AZStd::move(removeBuildArgumentsMap), AZStd::move(addBuildArgumentsMap));
        }

        void ShaderBuildArgumentsManager::Init(AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> && removeBuildArgumentsMap
                                             , AZStd::unordered_map<AZStd::string, AZ::RHI::ShaderBuildArguments> && addBuildArgumentsMap)
        {
            m_removeBuildArgumentsMap.clear();
            m_removeBuildArgumentsMap.swap(removeBuildArgumentsMap);

            m_addBuildArgumentsMap.clear();
            m_addBuildArgumentsMap.swap(addBuildArgumentsMap);

            m_argumentsStack = {};
            m_argumentsNameStack = {};
            m_argumentsStack.push(m_addBuildArgumentsMap[""]);
            m_argumentsNameStack.push("");
        }

        const AZ::RHI::ShaderBuildArguments& ShaderBuildArgumentsManager::PushArgumentsInternal(const AZStd::string& name, const AZ::RHI::ShaderBuildArguments& arguments)
        {
            m_argumentsNameStack.push(name);
            m_argumentsStack.push(arguments);
            return m_argumentsStack.top();
        }

        const AZ::RHI::ShaderBuildArguments& ShaderBuildArgumentsManager::PushArgumentScope(const AZStd::string& name)
        {
            AZ_Assert(!name.empty(), "This function requires non empty names");
            const auto& currentTopName = m_argumentsNameStack.top();
            auto newTopName = currentTopName.empty() ? name : AZStd::string::format("%s.%s", currentTopName.c_str(), name.c_str());

            auto it = m_addBuildArgumentsMap.find(newTopName);
            if (it == m_addBuildArgumentsMap.end())
            {
                // It is normal not to have arguments for a specific key. Because this class works as a stack we'll just push a copy
                // of whatever is currently at the top of the stack.
                return PushArgumentsInternal(newTopName, GetCurrentArguments());
            }

            // Init() guarantees that if there's an Add set of arguments, there is also a Remove set of arguments. BUT either of both, the Add and the Remove Sets can be empty,
            // what matters is that they are valid instances of ShaderBuildArguments class.
            auto removeIt = m_removeBuildArgumentsMap.find(newTopName);
            AZ_Assert(removeIt != m_removeBuildArgumentsMap.end(), "There must be an instance of arguments to remove for %s", currentTopName.c_str());

            auto newArguments = GetCurrentArguments() - removeIt->second;
            return PushArgumentsInternal(newTopName, newArguments + it->second);
        }

        const AZ::RHI::ShaderBuildArguments& ShaderBuildArgumentsManager::PushArgumentScope(const AZ::RHI::ShaderBuildArguments& removeArguments,
            const AZ::RHI::ShaderBuildArguments& addArguments, const AZStd::vector<AZStd::string>& definitions)
        {
            const AZStd::string anyName("?");
            const auto& currentTopName = m_argumentsNameStack.top();
            auto newTopName = currentTopName.empty() ? anyName : AZStd::string::format("%s.%s", currentTopName.c_str(), anyName.c_str());

            auto newArguments = GetCurrentArguments() - removeArguments;
            [[maybe_unused]] const auto addedDefinitionCount = newArguments.AppendDefinitions(definitions);
            AZ_Assert(addedDefinitionCount >= 0, "Failed to add definitions");

            return PushArgumentsInternal(newTopName, newArguments + addArguments);
        }


        const AZ::RHI::ShaderBuildArguments& ShaderBuildArgumentsManager::GetCurrentArguments() const
        {
            return m_argumentsStack.top();
        }


        void ShaderBuildArgumentsManager::PopArgumentScope()
        {
            // We always keep the global scope.
            if (m_argumentsStack.size() > 1)
            {
                m_argumentsStack.pop();
                m_argumentsNameStack.pop();
            }
        }

        void DiscoverConfigurationFilesInDirectoryRecursively(const AZ::IO::FixedMaxPath& dirPath, const AZStd::string& keyName,
            AZStd::unordered_map<AZStd::string, AZ::IO::FixedMaxPath>& discoveredFiles)
        {
            auto findFileCB = [&](const char * fileName, bool isFile) -> bool
            {
                if (fileName[0] == '.')
                {
                    return true;
                }
                AZ::IO::FixedMaxPath fullPath = dirPath / fileName;
                if (isFile)
                {
                    discoveredFiles[keyName] = fullPath;
                }
                else
                {
                    AZStd::string subKeyName = AZStd::string::format("%s%s%s", keyName.c_str(), keyName.empty() ? "" : ".", fileName);
                    DiscoverConfigurationFilesInDirectoryRecursively(fullPath, subKeyName, discoveredFiles);
                }
                return true;
            };

            AZ::IO::FixedMaxPath filter(dirPath);
            filter /= "*";
            AZ::IO::SystemFile::FindFiles(filter.c_str(), findFileCB);
        }

        AZStd::unordered_map<AZStd::string, AZ::IO::FixedMaxPath> ShaderBuildArgumentsManager::DiscoverConfigurationFilesInDirectory(const AZ::IO::FixedMaxPath& dirPath)
        {
            AZStd::unordered_map<AZStd::string, AZ::IO::FixedMaxPath> configurationFiles;

            auto jsonPath = dirPath / ShaderBuildOptionsJson;
            if (AZ::IO::SystemFile::Exists(jsonPath.c_str()))
            {
                // The global scope has no name.
                configurationFiles[""] = jsonPath;
            }

            AZ::IO::FixedMaxPath platformsDirPath(dirPath);
            platformsDirPath /= PlatformsDir;

            DiscoverConfigurationFilesInDirectoryRecursively(platformsDirPath, "", configurationFiles);

            return configurationFiles;
        }


        AZ::IO::FixedMaxPath ShaderBuildArgumentsManager::GetDefaultConfigDirectoryPath()
        {
            AZStd::string defaultConfigDirectory(DefaultConfigPathDirectory);
            defaultConfigDirectory = ResolvePathAliases(defaultConfigDirectory);
            // The default directory, which contains factory settings, must always exist.
            AZ_Assert(AZ::IO::SystemFile::Exists(defaultConfigDirectory.c_str()), "The default directory with shader build arguments must exist: %s", defaultConfigDirectory.c_str());
            return { defaultConfigDirectory };
        }

        AZ::IO::FixedMaxPath ShaderBuildArgumentsManager::GetUserConfigDirectoryPath()
        {
            AZStd::string userConfig;
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            if (settingsRegistry)
            {
                settingsRegistry->Get(userConfig, ConfigPathRegistryKey);
            }
            if (userConfig.empty())
            {
                return {};
            }
            userConfig = ResolvePathAliases(userConfig);
            return { userConfig };
        }

        AZStd::unordered_map<AZStd::string, AZ::IO::FixedMaxPath> ShaderBuildArgumentsManager::DiscoverConfigurationFiles()
        {
            const auto defaultConfigDirectoryPath = GetDefaultConfigDirectoryPath();
            auto configFiles = DiscoverConfigurationFilesInDirectory(defaultConfigDirectoryPath);
            auto userConfigPath = GetUserConfigDirectoryPath();
            if (userConfigPath.empty() || (defaultConfigDirectoryPath == userConfigPath))
            {
                // The user chose not to customize the command line arguments.
                // Let's return the Atom's default.
                return configFiles;
            }

            auto userConfigFiles = DiscoverConfigurationFilesInDirectory(userConfigPath);

            // Replace only the file paths that are customized by the user.
            for (auto const& [key, val] : configFiles)
            {
                const auto itor = userConfigFiles.find(key);
                if (itor == userConfigFiles.end())
                {
                    continue;
                }
                configFiles[key] = userConfigFiles[key];
            }
            return configFiles;
        }
    } // namespace ShaderBuilder
} // AZ
