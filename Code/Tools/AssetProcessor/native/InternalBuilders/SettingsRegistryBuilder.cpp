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
    void SettingsRegistryBuilder::SettingsExporter::WriteName(AZStd::string_view name)
    {
        if (m_includeName)
        {
            m_writer.Key(name.data(), aznumeric_caster(name.length()));
        }
    }

    SettingsRegistryBuilder::SettingsExporter::SettingsExporter(
        rapidjson::StringBuffer& buffer, const AZStd::vector<AZStd::string>& excludes)
        : m_writer(rapidjson::Writer<rapidjson::StringBuffer>(buffer))
        , m_excludes(excludes)
    {
    }

    AZ::SettingsRegistryInterface::VisitResponse SettingsRegistryBuilder::SettingsExporter::Traverse(
        AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::VisitAction action,
        AZ::SettingsRegistryInterface::Type type)
    {
        for (const AZStd::string& exclude : m_excludes)
        {
            if (exclude == path)
            {
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            }
        }

        if (action == AZ::SettingsRegistryInterface::VisitAction::Begin)
        {
            AZ_Assert(type == AZ::SettingsRegistryInterface::Type::Object || type == AZ::SettingsRegistryInterface::Type::Array,
                "Unexpected type visited: %i.", type);
            WriteName(valueName);
            if (type == AZ::SettingsRegistryInterface::Type::Object)
            {
                m_result = m_result && m_writer.StartObject();
                m_includeNameStack.push(true);
                m_includeName = true;
            }
            else
            {
                m_result = m_result && m_writer.StartArray();
                m_includeNameStack.push(false);
                m_includeName = false;
            }
        }
        else if (action == AZ::SettingsRegistryInterface::VisitAction::End)
        {
            if (type == AZ::SettingsRegistryInterface::Type::Object)
            {
                m_result = m_result && m_writer.EndObject();
            }
            else
            {
                m_result = m_result && m_writer.EndArray();
            }
            AZ_Assert(!m_includeNameStack.empty(), "Attempting to close a json array or object that wasn't started.");
            m_includeNameStack.pop();
            m_includeName = !m_includeNameStack.empty() ? m_includeNameStack.top() : true;
        }
        else if (type == AZ::SettingsRegistryInterface::Type::Null)
        {
            WriteName(valueName);
            m_result = m_result && m_writer.Null();
        }

        return m_result ?
            AZ::SettingsRegistryInterface::VisitResponse::Continue :
            AZ::SettingsRegistryInterface::VisitResponse::Done;
    }

    void SettingsRegistryBuilder::SettingsExporter::Visit(
        AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, bool value)
    {
        WriteName(valueName);
        m_result = m_result && m_writer.Bool(value);
    }

    void SettingsRegistryBuilder::SettingsExporter::Visit(
        AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::s64 value)
    {
        WriteName(valueName);
        m_result = m_result && m_writer.Int64(value);
    }

    void SettingsRegistryBuilder::SettingsExporter::Visit(
        AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZ::u64 value)
    {
        WriteName(valueName);
        m_result = m_result && m_writer.Uint64(value);
    }

    void SettingsRegistryBuilder::SettingsExporter::Visit(
        AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, double value)
    {
        WriteName(valueName);
        m_result = m_result && m_writer.Double(value);
    }

    void SettingsRegistryBuilder::SettingsExporter::Visit(
        AZStd::string_view, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value)
    {
        WriteName(valueName);
        m_result = m_result && m_writer.String(value.data(), aznumeric_caster(value.length()));
    }

    bool SettingsRegistryBuilder::SettingsExporter::Finalize()
    {
        if (!m_includeNameStack.empty())
        {
            AZ_Assert(false, "m_includeNameStack is expected to be empty. This means that there was an object or array what wasn't closed.");
            return false;
        }
        return m_result;
    }

    void SettingsRegistryBuilder::SettingsExporter::Reset(rapidjson::StringBuffer& buffer)
    {
        m_writer.Reset(buffer);
        m_includeName = false;
        m_result = true;
    }



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
        // Exclude the AssetProcessor settings from the game regsitry
        excludes.emplace_back(AssetProcessor::AssetProcessorSettingsKey); 
        
        AZStd::vector<char> scratchBuffer;
        scratchBuffer.reserve(512 * 1024); // Reserve 512kb to avoid repeatedly resizing the buffer;
        AZStd::fixed_vector<AZStd::string_view, AzFramework::MaxPlatformCodeNames> platformCodes;
        AzFramework::PlatformHelper::AppendPlatformCodeNames(platformCodes, request.m_platformInfo.m_identifier);
        AZ_Assert(platformCodes.size() <= 1, "A one-to-one mapping of asset type platform identifier"
            " to platform codename is required in the SettingsRegistryBuilder."
            " The bootstrap.game is now only produced per build configuration and doesn't take into account"
            " different platforms names");

        const AZStd::string& assetPlatformIdentifier = request.m_jobDescription.GetPlatformIdentifier();
        // Determines the suffix that will be used for the launcher based on processing server vs non-server assets
        const char* launcherType = assetPlatformIdentifier != AzFramework::PlatformHelper::GetPlatformName(AzFramework::PlatformId::SERVER)
            ? "_GameLauncher" : "_ServerLauncher";
        
        AZ::SettingsRegistryInterface::Specializations specializations[] =
        {
            { AZStd::string_view{"release"}, AZStd::string_view{"game"} },
            { AZStd::string_view{"profile"}, AZStd::string_view{"game"} },
            { AZStd::string_view{"debug"}, AZStd::string_view{"game"} }
        };

        // Add the project specific specializations
        auto projectName = AZ::Utils::GetProjectName();
        if (!projectName.empty())
        {
            for (AZ::SettingsRegistryInterface::Specializations& specialization : specializations)
            {
                specialization.Append(projectName);
                // The Game Launcher normally has a build target name of <ProjectName>Launcher
                // Add that as a specialization to pick up the gem dependencies files that are specialized
                // on a the Game Launcher target if the asset platform isn't "server"
                specialization.Append(projectName + launcherType);
            }
        }

        AZStd::string outputPath;
        AzFramework::StringFunc::Path::Join(request.m_tempDirPath.c_str(), "bootstrap.game.", outputPath);
        size_t extensionOffset = outputPath.length();

        rapidjson::StringBuffer outputBuffer;
        outputBuffer.Reserve(512 * 1024); // Reserve 512kb to avoid repeatedly resizing the buffer;
        SettingsExporter exporter(outputBuffer, excludes);

        if (!platformCodes.empty())
        {
            AZStd::string_view platform = platformCodes.front();
            constexpr AZ::u32 productSubID = 0;
            for (size_t i = 0; i < AZStd::size(specializations); ++i)
            {
                const AZ::SettingsRegistryInterface::Specializations& specialization = specializations[i];
                if (m_isShuttingDown)
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
                    return;
                }

                using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
                // Placeholder Key used by the local Settings Registry for storing all Gems SourcePaths
                // array entries.
                constexpr auto PlaceholderGemKey = FixedValueString(AZ::SettingsRegistryMergeUtils::OrganizationRootKey)
                    + "/Gems/__SettingsRegistryBuilderPlaceholder";

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

                    // The purpose of this section is to copy the Gem's SourcePaths from the Global Settings Registry
                    // the local SettingsRegistry. The reason this is needed is so that the call to
                    // `MergeSettingsToRegistry_GemRegistries` below is able to locate each gem's "<gem-root>/Registry" folder
                    // that will be merged into the bootstrap.game.<configuration>.setreg file
                    // This is used by the GameLauncher applications to read from a single merged .setreg file
                    // containing the settings needed to run a game/simulation without have access to the source code base registry
                    AZStd::vector<AzFramework::GemInfo> gemInfos;
                    size_t pathIndex{};
                    if (AzFramework::GetGemsInfo(gemInfos, *settingsRegistry))
                    {
                        AZStd::vector<AZ::IO::PathView> sourcePaths;
                        for (const AzFramework::GemInfo& gemInfo : gemInfos)
                        {
                            for (const AZ::IO::Path& absoluteSourcePath : gemInfo.m_absoluteSourcePaths)
                            {
                                if (auto foundIt = AZStd::find(sourcePaths.begin(), sourcePaths.end(), absoluteSourcePath);
                                    foundIt == sourcePaths.end())
                                {
                                    sourcePaths.emplace_back(absoluteSourcePath);
                                }
                            }
                        }

                        for (const AZ::IO::PathView& sourcePath : sourcePaths)
                        {
                            // Use JSON Pointer to append elements to the SourcePaths array
                            registry.Set(FixedValueString::format("%s/SourcePaths/%zu", PlaceholderGemKey.c_str(), pathIndex++),
                                sourcePath.Native());
                        }
                    }
                }

                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_EngineRegistry(registry, platform, specialization, &scratchBuffer);
                // This function iterates over each path for each the "/Amazon/Gems/<gem-name>/SourcePaths" key and attempts
                // to merge the "Registry" directory in each path.
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_GemRegistries(registry, platform, specialization, &scratchBuffer);
                AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectRegistry(registry, platform, specialization, &scratchBuffer);

                // The Placeholder Key is removed now that each gem's "<gem-root>/Registry" directory have been merged to
                // the local Settings Registry instance via `MergeSettingsToRegistry_GemRegistries`
                registry.Remove(PlaceholderGemKey);

                // Merge the Project User and User home settings registry only in non-release builds
                constexpr bool executeRegDumpCommands = false;
                AZ::CommandLine* commandLine{};
                AZ::ComponentApplicationBus::Broadcast([&commandLine](AZ::ComponentApplicationRequests* appRequests)
                {
                    commandLine = appRequests->GetAzCommandLine();
                });

                if (!specialization.Contains("release"))
                {
                    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_O3deUserRegistry(registry, platform, specialization, &scratchBuffer);
                    if (commandLine)
                    {
                        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, *commandLine, executeRegDumpCommands);
                    }
                    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ProjectUserRegistry(registry, platform, specialization, &scratchBuffer);
                }

                if (commandLine)
                {
                    AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(registry, *commandLine, executeRegDumpCommands);
                }


                if (registry.Visit(exporter, ""))
                {
                    if (!exporter.Finalize())
                    {
                        return;
                    }

                    outputPath += specialization.GetSpecialization(0); // Append configuration
                    outputPath += ".setreg";

                    AZ::IO::SystemFile file;
                    if (!file.Open(outputPath.c_str(),
                        AZ::IO::SystemFile::OpenMode::SF_OPEN_CREATE | AZ::IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
                    {
                        AZ_Error("Settings Registry Builder", false, R"(Failed to open file "%s" for writing.)", outputPath.c_str());
                        return;
                    }
                    if (file.Write(outputBuffer.GetString(), outputBuffer.GetSize()) != outputBuffer.GetSize())
                    {
                        AZ_Error("Settings Registry Builder", false, R"(Failed to write settings registry to file "%s".)", outputPath.c_str());
                        return;
                    }
                    file.Close();

                    response.m_outputProducts.emplace_back(outputPath, m_assetType, productSubID + aznumeric_cast<AZ::u32>(i));
                    response.m_outputProducts.back().m_dependenciesHandled = true;

                    outputPath.erase(extensionOffset);
                }

                outputBuffer.Clear();
                exporter.Reset(outputBuffer);
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
