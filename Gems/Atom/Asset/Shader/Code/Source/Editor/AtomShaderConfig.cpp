/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomShaderConfig.h"

#include <AzCore/IO/SystemFile.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Platform/PlatformDefaults.h>

#include <AzCore/Serialization/Json/JsonUtils.h>


namespace AZ
{
    namespace ShaderBuilder
    {
        namespace AtomShaderConfig
        {
            [[maybe_unused]] static constexpr char AtomShaderConfigName[] = "AtomShaderConfig";

            bool MutateToFirstAbsoluteFolderThatExists(AZStd::string& relativeFolder, AZStd::vector<AZStd::string>& watchFolders)
            {
                if (!AzFramework::StringFunc::Path::IsRelative(relativeFolder.c_str()))
                {
                    return true;
                }
                for (AZStd::string folder : watchFolders)
                {
                    AzFramework::StringFunc::Path::Normalize(relativeFolder); // this is an external input, so defense is indicated.
                    AzFramework::StringFunc::Path::Normalize(folder);
                    AZStd::string absoluteOutput;
                    AzFramework::StringFunc::Path::Join(folder.c_str(),
                        relativeFolder.c_str(),
                        absoluteOutput,
                        true   /* join overlapping */,
                        false  /* case insensitive */);
                    if (AZ::IO::SystemFile::Exists(absoluteOutput.c_str()))
                    {
                        relativeFolder = std::move(absoluteOutput);
                        return true;
                    }
                }
                return false;
            }

            AZStd::string GetAssetConfigPath(const char* platformFolder)
            {
                bool success = false;
                AZStd::vector<AZStd::string> scanFoldersVector;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success,
                    &AzToolsFramework::AssetSystemRequestBus::Events::GetScanFolders,
                    scanFoldersVector);
                AZ_Warning(AtomShaderConfigName, success, "Could not acquire a list of scan folders from the database.");
                if (success)
                {
                    // platform specific shader build user configuration
                    static constexpr char ConfigFileName[] = "AtomShaderCapabilities.json";
                    static constexpr char ConfigPalFolder[] = "Config/Platform";
                    AZStd::string assetRoot = ConfigPalFolder;
                    if (MutateToFirstAbsoluteFolderThatExists(assetRoot, scanFoldersVector))
                    {
                        AZStd::string configFile;
                        AzFramework::StringFunc::Path::Join(assetRoot.c_str(), platformFolder, configFile);
                        AzFramework::StringFunc::Path::Join(configFile.c_str(), ConfigFileName, configFile);
                        return configFile;
                    }
                }
                return {};
            }

            CapabilitiesConfigFile GetMinDescriptorSetsFromConfigFile(const char* platformFolder)
            {
                CapabilitiesConfigFile limits;
                AZStd::string configFilePath = GetAssetConfigPath(platformFolder);
                if (IO::SystemFile::Exists(configFilePath.c_str()))
                {
                    Outcome loadResult = JsonSerializationUtils::LoadObjectFromFile(limits, configFilePath);
                    AZ_Warning(AtomShaderConfigName, loadResult.IsSuccess(), "Failed to load capabilities settings from file [%s]", configFilePath.c_str());
                }
                return limits;
            }

            // @config argument comes from a platform abstracted folder
            AZStd::string FormatSupplementaryArgumentsFromConfigAtomShader(const CapabilitiesConfigFile& config)
            {
                AZStd::string commandLineArguments;
                // the map config.m_counts is the deserialized JSON data from the config file about hardware capabilities.
                // the keys are unsanitized, therefore they could be anything in the wild.
                // proceed if any element of the map's values has is a non -1:
                if (AZStd::any_of(AZStd::begin(config.m_descriptorCounts), AZStd::end(config.m_descriptorCounts), [](auto pair) { return pair.second != -1; }))
                {
                    commandLineArguments = " --min-descriptors=";
                    // for each key that indeed corresponds to an element of the DescriptorSpace enumerators, let's fetch it
                    // and convert it to a string:
                    AZStd::array<AZStd::string, AzEnumTraits<DescriptorSpace>::Count> valuesAsString = { {"-1","-1","-1","-1","-1"} };
                    for (const auto& enumElement : DescriptorSpaceMembers)
                    {
                        auto iterator = config.m_descriptorCounts.find(enumElement.m_string);
                        if (iterator != config.m_descriptorCounts.end())
                        {
                            valuesAsString[enumElement.m_value] = AZStd::to_string(iterator->second);
                        }
                    }
                    AzFramework::StringFunc::Join(commandLineArguments,
                        AZStd::begin(valuesAsString),
                        AZStd::end(valuesAsString),
                        ",");
                }

                if (config.m_maxSpaces != -1)
                {
                    commandLineArguments += AZStd::string::format(" --max-spaces=%d", config.m_maxSpaces);
                }
                return commandLineArguments;
            }

            void AddParametersFromConfigFile(AZStd::string& parameters, const AssetBuilderSDK::PlatformInfo& platform)
            {
                auto platformId = static_cast<AzFramework::PlatformId>(
                    AzFramework::PlatformHelper::GetPlatformIndexFromName(platform.m_identifier.c_str()));
                // The platforms available don't match the PAL folders. But the tags enrich it just enough to be able to rehabilitate android:
                const char* palFolder = platform.HasTag("android") ? "Android"
                    : AzFramework::PlatformIdToPalFolder(platformId);

                CapabilitiesConfigFile minDescriptors = GetMinDescriptorSetsFromConfigFile(palFolder);
                parameters += FormatSupplementaryArgumentsFromConfigAtomShader(minDescriptors);
            }

        } // namespace AtomShaderConfig
    } // namespace ShaderBuilder
} //namespace AZ
