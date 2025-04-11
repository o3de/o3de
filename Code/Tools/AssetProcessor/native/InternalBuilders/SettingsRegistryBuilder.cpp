/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <limits>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <native/InternalBuilders/SettingsRegistryBuilder.h>
#include <native/utilities/PlatformConfiguration.h>

namespace AssetProcessor
{
    SettingsRegistryBuilder::SettingsRegistryBuilder()
        : m_builderId("{1BB18B28-2953-4922-A80B-E7375FCD7FC1}")
        , m_assetType("{FEBB3C7B-9C8B-46C3-8AAF-3D132D811087}")
    {
        AssetBuilderSDK::AssetBuilderCommandBus::Handler::BusConnect(m_builderId);
    }

    bool SettingsRegistryBuilder::Initialize()
    {
        AssetBuilderSDK::AssetBuilderDesc   builderDesc;
        builderDesc.m_name = "Settings Registry Builder";
        builderDesc.m_patterns.emplace_back("*/engine.json", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        builderDesc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::Internal;
        builderDesc.m_busId = m_builderId;
        builderDesc.m_createJobFunction = AZStd::bind(&SettingsRegistryBuilder::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDesc.m_processJobFunction = AZStd::bind(&SettingsRegistryBuilder::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDesc.m_version = 3;

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDesc);

        return true;
    }

    void SettingsRegistryBuilder::Uninitialize() {}

    void SettingsRegistryBuilder::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void SettingsRegistryBuilder::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor job;
            job.m_jobKey = "Settings Registry";
            // The settings are the very first thing the game reads so needs to available before anything else.
            job.m_priority = std::numeric_limits<decltype(job.m_priority)>::max();
            job.m_critical = true;
            job.SetPlatformIdentifier(info.m_identifier.c_str());
            response.m_createJobOutputs.push_back(AZStd::move(job));
        }

        AZ::IO::Path settingsRegistryWildcard = AZStd::string_view(AZ::Utils::GetEnginePath());
        settingsRegistryWildcard /= AZ::SettingsRegistryInterface::RegistryFolder;
        settingsRegistryWildcard /= "*.setreg";
        response.m_sourceFileDependencyList.emplace_back(AZStd::move(settingsRegistryWildcard.Native()), AZ::Uuid::CreateNull(),
            AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Wildcards);

        auto projectPath = AZ::IO::Path(AZStd::string_view(AZ::Utils::GetProjectPath()));
        response.m_sourceFileDependencyList.emplace_back(
            AZStd::move((projectPath / AZ::SettingsRegistryInterface::RegistryFolder / "*.setreg").Native()),
            AZ::Uuid::CreateNull(),
            AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Wildcards);
        response.m_sourceFileDependencyList.emplace_back(
            AZStd::move((projectPath / AZ::SettingsRegistryInterface::DevUserRegistryFolder / "*.setreg").Native()),
            AZ::Uuid::CreateNull(),
            AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Wildcards);

        if (auto settingsRegistry = AZ::Interface<AZ::SettingsRegistryInterface>::Get(); settingsRegistry != nullptr)
        {
            AZStd::vector<AzFramework::GemInfo> gemInfos;
            if (AzFramework::GetGemsInfo(gemInfos, *settingsRegistry))
            {
                // Gather unique list of Settings Registry wildcard directories
                AZStd::vector<AZ::IO::Path> gemSettingsRegistryWildcards;
                for (const AzFramework::GemInfo& gemInfo : gemInfos)
                {
                    for (const AZ::IO::Path& absoluteSourcePath : gemInfo.m_absoluteSourcePaths)
                    {
                        auto gemSettingsRegistryWildcard = absoluteSourcePath / AZ::SettingsRegistryInterface::RegistryFolder / "*.setreg";
                        if (auto foundIt = AZStd::find(gemSettingsRegistryWildcards.begin(), gemSettingsRegistryWildcards.end(), gemSettingsRegistryWildcard);
                            foundIt == gemSettingsRegistryWildcards.end())
                        {
                            gemSettingsRegistryWildcards.emplace_back(gemSettingsRegistryWildcard);
                        }
                    }
                }

                // Add to the Source File Dependency list
                for (AZ::IO::Path& gemSettingsRegistryWildcard : gemSettingsRegistryWildcards)
                {
                    response.m_sourceFileDependencyList.emplace_back(
                        AZStd::move(gemSettingsRegistryWildcard.Native()), AZ::Uuid::CreateNull(),
                        AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Wildcards);
                }
            }
        }
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void SettingsRegistryBuilder::ProcessJob(
        const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;

        AZStd::vector<AZStd::string> excludes = ReadExcludesFromRegistry();
        // Exclude the AssetProcessor settings from the game registry
        excludes.emplace_back(AssetProcessor::AssetProcessorSettingsKey);

        AZStd::vector<char> scratchBuffer;
        scratchBuffer.reserve(512 * 1024); // Reserve 512kb to avoid repeatedly resizing the buffer;
        AZStd::fixed_vector<AZStd::string_view, AzFramework::MaxPlatformCodeNames> platformCodes;
        AzFramework::PlatformHelper::AppendPlatformCodeNames(platformCodes, request.m_platformInfo.m_identifier);
        AZ_Assert(platformCodes.size() <= 1, "A one-to-one mapping of asset type platform identifier"
            " to platform codename is required in the SettingsRegistryBuilder."
            " The bootstrap.<launcher-type>.<config>.setreg is now only produced per launcher type + build configuration and doesn't take into account"
            " different platforms names");

        const AZStd::string& assetPlatformIdentifier = request.m_jobDescription.GetPlatformIdentifier();
        // Determines the suffix that will be used for the launcher based on processing server vs non-server assets
        const char* launcherType = assetPlatformIdentifier != AzFramework::PlatformHelper::GetPlatformName(AzFramework::PlatformId::SERVER)
            ? "_GameLauncher" : "_ServerLauncher";

        AZ::SettingsRegistryInterface::FilenameTags specializations[] =
        {
            { AZStd::string_view{"client"}, AZStd::string_view{"release"} },
            { AZStd::string_view{"client"} , AZStd::string_view{"profile"} },
            { AZStd::string_view{"client"}, AZStd::string_view{"debug"} },
            { AZStd::string_view{"server"}, AZStd::string_view{"release"} },
            { AZStd::string_view{"server"} , AZStd::string_view{"profile"} },
            { AZStd::string_view{"server"}, AZStd::string_view{"debug"} },
            { AZStd::string_view{"unified"}, AZStd::string_view{"release"} },
            { AZStd::string_view{"unified"} , AZStd::string_view{"profile"} },
            { AZStd::string_view{"unified"}, AZStd::string_view{"debug"} }
        };

        constexpr size_t LauncherTypeIndex = 0;
        constexpr size_t BuildConfigIndex = 1;

        // Append the specialization filename tag of "launcher" get all the `<filename>.*.launcher.*.setreg` files
        // to be merged into the aggregate Settings Registry
        constexpr AZStd::string_view LauncherFilenameTag = "launcher";
        for (AZ::SettingsRegistryInterface::FilenameTags& specialization : specializations)
        {
            specialization.Append(LauncherFilenameTag);
            // Also add the "game" tag for backwards compatibility with any existing
            // `<filename>.*.game.*.setreg` files
            specialization.Append("game");
        }

        // Add the project specific specializations
        auto projectName = AZ::Utils::GetProjectName();
        if (!projectName.empty())
        {
            for (AZ::SettingsRegistryInterface::FilenameTags& specialization : specializations)
            {
                specialization.Append(projectName);
                // The Game Launcher normally has a build target name of <ProjectName>Launcher
                // Add that as a specialization to pick up the gem dependencies files that are specialized
                // on a the Game Launcher target if the asset platform isn't "server"
                specialization.Append(projectName + launcherType);
            }
        }

        AZ::IO::Path outputPath = AZ::IO::Path(request.m_tempDirPath) / "bootstrap.";
        size_t extensionOffset = outputPath.Native().size();

        if (!platformCodes.empty())
        {
            // Setting up the Dumper settings for exporting the settings registry to a file
            AZStd::string outputBuffer;
            outputBuffer.reserve(512 * 1024); // Reserve 512kb to avoid repeatedly resizing the buffer;
            AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
            dumperSettings.m_includeFilter = [&excludes](AZStd::string_view jsonKeyPath)
            {
                auto ExcludeField = [&jsonKeyPath](AZStd::string_view excludePath)
                {
                    return AZ::SettingsRegistryMergeUtils::IsPathDescendantOrEqual(excludePath, jsonKeyPath);
                };
                // Include a path only if it is not equal or a suffix of any paths of the exclude vector
                return AZStd::ranges::find_if(excludes, ExcludeField) == AZStd::ranges::end(excludes);
            };

            AZStd::string_view platform = platformCodes.front();
            for (size_t i = 0; i < AZStd::size(specializations); ++i)
            {
                const AZ::SettingsRegistryInterface::FilenameTags& specialization = specializations[i];
                if (m_isShuttingDown)
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                    return;
                }

                using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

                AZ::SettingsRegistryImpl registry;

                // Seed the local settings registry using the AssetProcessor Settings Registry
                if (auto settingsRegistry = AZ::Interface<AZ::SettingsRegistryInterface>::Get(); settingsRegistry != nullptr)
                {
                    AZStd::array settingsToCopy{
                        AZStd::string::format("%s/project_path", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey),
                        AZStd::string{AZ::SettingsRegistryMergeUtils::FilePathKey_BinaryFolder},
                        AZStd::string{AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder},
                        AZStd::string{AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath},
                        AZStd::string{AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder},
                        AZStd::string{AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder},
                    };

                    for (const auto& settingsKey : settingsToCopy)
                    {
                        FixedValueString settingsValue;
                        [[maybe_unused]] bool settingsCopied = settingsRegistry->Get(settingsValue, settingsKey)
                            && registry.Set(settingsKey, settingsValue);
                        AZ_Warning("Settings Registry Builder", settingsCopied, "Unable to copy setting %s from AssetProcessor settings registry"
                            " to local settings registry", settingsKey.c_str());
                    }

                    // The purpose of this section is to copy the active gems entry and manifest gems entries
                    // to a local SettingsRegistry.
                    // The reason this is needed is so that the call to
                    // `MergeSettingsToRegistry_GemRegistries` below is able to locate each gems root directory
                    // that will be merged into the bootstrap.<launcher-type>.<configuration>.setreg file
                    // This is used by the GameLauncher applications to read from a single merged .setreg file
                    // containing the settings needed to run a game/simulation without have access to the source code base registry
                    auto CopySettingsToLocalRegistry = [&registry, settingsRegistry, copiedSettings = AZStd::string()]
                    (AZStd::string_view copyFieldKey) mutable
                    {
                        // Copy Settings at the specified field key recursively to the local settings registry
                        copiedSettings.clear();
                        AZ::IO::ByteContainerStream copiedSettingsStream(&copiedSettings);
                        AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
                        AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*settingsRegistry, copyFieldKey,
                            copiedSettingsStream, dumperSettings);
                        registry.MergeSettings(copiedSettings, AZ::SettingsRegistryInterface::Format::JsonMergePatch, copyFieldKey);
                    };

                    CopySettingsToLocalRegistry(AZ::SettingsRegistryMergeUtils::ActiveGemsRootKey);
                    CopySettingsToLocalRegistry(AZ::SettingsRegistryMergeUtils::ManifestGemsRootKey);
                }

                AZ::SettingsRegistryInterface::MergeSettingsResult mergeResult;
                mergeResult.Combine(AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_EngineRegistry(registry, platform, specialization, &scratchBuffer));
                mergeResult.Combine(AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_GemRegistries(registry, platform, specialization, &scratchBuffer));
                mergeResult.Combine(AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectRegistry(registry, platform, specialization, &scratchBuffer));

                // Output any Settings Registry Merge result messages using the info log level if not empty
                if (auto& operationMessages = mergeResult.GetMessages();
                    !operationMessages.empty())
                {
                    [[maybe_unused]] AZStd::string_view launcherString = specialization.GetSpecialization(LauncherTypeIndex);
                    [[maybe_unused]] AZStd::string_view buildConfiguration = specialization.GetSpecialization(BuildConfigIndex);
                    AZ_Info("Settings Registry Builder", R"(Launcher Type: "%.*s", Build configuration: "%.*s")" "\n"
                        "Merging the Engine, Gem, Project Registry directories resulted in the following messages:\n%s\n",
                        AZ_STRING_ARG(launcherString), AZ_STRING_ARG(buildConfiguration),
                         operationMessages.c_str());
                }

                // The Gem Root Key and Manifest Gems Root is removed now that each gems "<gem-root>/Registry" directory
                // have been merged to the local Settings Registry
                registry.Remove(AZ::SettingsRegistryMergeUtils::ActiveGemsRootKey);
                registry.Remove(AZ::SettingsRegistryMergeUtils::ManifestGemsRootKey);

                AZ::CommandLine* commandLine{};
                AZ::ComponentApplicationBus::Broadcast([&commandLine](AZ::ComponentApplicationRequests* appRequests)
                {
                    commandLine = appRequests->GetAzCommandLine();
                });

                if (commandLine)
                {
                    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, *commandLine, {});
                }

                if (AZ::IO::ByteContainerStream outputStream(&outputBuffer);
                    AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(registry, "", outputStream, dumperSettings))
                {
                    AZStd::string_view specializationString(specialization.GetSpecialization(LauncherTypeIndex));
                    outputPath.Native() += specializationString; // Append launcher type (client, server, or unified)
                    specializationString = specialization.GetSpecialization(BuildConfigIndex);
                    outputPath.Native() += '.';
                    outputPath.Native() += specializationString; // Append configuration
                    outputPath.Native() += ".setreg";

                    AZ::IO::SystemFile file;
                    if (!file.Open(outputPath.c_str(),
                        AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE | AZ::IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
                    {
                        AZ_Error("Settings Registry Builder", false, R"(Failed to open file "%s" for writing.)", outputPath.c_str());
                        return;
                    }
                    if (file.Write(outputBuffer.data(), outputBuffer.size()) != outputBuffer.size())
                    {
                        AZ_Error("Settings Registry Builder", false, R"(Failed to write settings registry to file "%s".)", outputPath.c_str());
                        return;
                    }
                    file.Close();

                    // Hash only the launcher type and build config specializations tags
                    size_t hashedSpecialization{};
                    // Get the launcher type specialization tag
                    AZStd::hash_combine(hashedSpecialization, specialization.GetSpecialization(LauncherTypeIndex));
                    // Get the build config specialization tag
                    AZStd::hash_combine(hashedSpecialization, specialization.GetSpecialization(BuildConfigIndex));
                    AZ_Assert(hashedSpecialization != 0, "Product ID generation failed for specialization %.*s."
                        " This can result in a product ID collision with other builders for this asset.",
                        AZ_STRING_ARG(specializationString));

                    auto setregSubId = static_cast<AZ::u32>(hashedSpecialization);
                    response.m_outputProducts.emplace_back(outputPath.Native(), m_assetType, setregSubId);
                    response.m_outputProducts.back().m_dependenciesHandled = true;

                    outputPath.Native().erase(extensionOffset);
                }

                // Clear the output buffer, to make sure previous loop iterations settings are not being appended
                outputBuffer.clear();
            }
        }

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    AZStd::vector<AZStd::string> SettingsRegistryBuilder::ReadExcludesFromRegistry() const
    {
        AZStd::vector<AZStd::string> excludes;
        auto builderRegistry = AZ::SettingsRegistry::Get();
        AZStd::string path = "/Amazon/AssetBuilder/SettingsRegistry/Excludes/";
        size_t offset = path.length();
        size_t counter = 0;
        do
        {
            path += AZStd::to_string(counter);

            AZStd::string exclude;
            if (builderRegistry->Get(exclude, path))
            {
                excludes.push_back(AZStd::move(exclude));
            }
            else
            {
                return excludes;
            }

            counter++;
            path.erase(offset);
        } while (true);
    }
} // namespace AssetProcessor
