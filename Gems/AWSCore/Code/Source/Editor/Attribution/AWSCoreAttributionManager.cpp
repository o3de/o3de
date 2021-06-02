/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <Editor/Attribution/AWSCoreAttributionMetric.h>
#include <Editor/Attribution/AWSCoreAttributionManager.h>
#include <Editor/Attribution/AWSAttributionServiceApi.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/PlatformId/PlatformId.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <ResourceMapping/AWSResourceMappingUtils.h>



namespace AWSCore
{
    constexpr char EditorPreferencesFileName[] = "editorpreferences.setreg";
    constexpr char AWSAttributionEnabledKey[] = "/Amazon/Preferences/AWS/AWSAttributionEnabled";
    constexpr char AWSAttributionDelaySecondsKey[] = "/Amazon/Preferences/AWS/AWSAttributionDelaySeconds";
    constexpr char AWSAttributionLastTimeStampKey[] = "/Amazon/Preferences/AWS/AWSAttributionLastTimeStamp";
    constexpr char AWSAttributionApiId[] = "xbzx78kvbk";
    constexpr char AWSAttributionChinaApiId[] = "";
    constexpr char AWSAttributionApiStage[] = "prod";

    void AWSAttributionManager::Init()
    {
    }

    void AWSAttributionManager::MetricCheck()
    {
        if (ShouldGenerateMetric())
        {
            // 1. Gather metadata and assemble metric
            AttributionMetric metric;
            UpdateMetric(metric);            
            // 2. Identify region and chose attribution endpoint
            
            // 3. Post metric
            SubmitMetric(metric);
        }
    }

    bool AWSAttributionManager::ShouldGenerateMetric() const
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "File IO is not initialized.");

        auto registry = AZ::SettingsRegistry::Get();
        AZ_Assert(registry, "Settings registry is not initialized.");

        // Resolve path to editorpreferences.setreg
        AZStd::string editorPreferencesFilePath = AZStd::string::format("@user@/%s/%s", AZ::SettingsRegistryInterface::RegistryFolder, EditorPreferencesFileName);
        AZStd::array<char, AZ::IO::MaxPathLength> resolvedPath {};
        if (!fileIO->ResolvePath(editorPreferencesFilePath.c_str(), resolvedPath.data(), resolvedPath.size()))
        {
            AZ_Warning("AWSAttributionManager", false, "Error resolving path", editorPreferencesFilePath.c_str());
            return false;
        }

        if (!registry->MergeSettingsFile(resolvedPath.data(), AZ::SettingsRegistryInterface::Format::JsonMergePatch))
        {
            AZ_Warning("AWSAttributionManager", false, "Error merging settings registry for path: %s", resolvedPath.data());
            return false;
        }

        bool awsAttributionEnabled = false;
        if (!registry->Get(awsAttributionEnabled, AWSAttributionEnabledKey))
        {
            AZ_Warning("AWSAttributionManager", false, "%s key not found in %s. Defaulting AWSAttributionEnabled to true", AWSAttributionEnabledKey, resolvedPath.data());
            // If not found default to sending the metric.
            awsAttributionEnabled = true;
        }

        if (!awsAttributionEnabled)
        {
            return false;
        }

        // If delayInSeconds is not found, set default to a day
        AZ::u64 delayInSeconds = 0;
        if (!registry->Get(delayInSeconds, AWSAttributionDelaySecondsKey))
        {
            AZ_Warning("AWSAttributionManager", false, "AWSAttribution delay key not found. Defaulting to delay to day");
            delayInSeconds = 86400;
        }

        AZ::u64 lastSendTimeStampSeconds = 0;
        if (!registry->Get(lastSendTimeStampSeconds, AWSAttributionLastTimeStampKey))
        {
            // If last time stamp not found, assume this is the first attempt at sending.
            return true;
        }

        AZStd::chrono::seconds lastSendTimeStamp = AZStd::chrono::seconds(lastSendTimeStampSeconds);
        AZStd::chrono::seconds secondsSinceLastSend =
            AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::system_clock::now().time_since_epoch()) - lastSendTimeStamp;
        if (secondsSinceLastSend.count() >= delayInSeconds)
        {
            return true;
        }

        return false;
    }

    void AWSAttributionManager::SaveSettingsRegistryFile()
    {
        AZ::Job* job = AZ::CreateJobFunction(
            [this]()
            {
                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                AZ_Assert(fileIO, "File IO is not initialized.");

                auto registry = AZ::SettingsRegistry::Get();
                AZ_Assert(registry, "Settings registry is not initialized.");

                // Resolve path to editorpreferences.setreg
                AZStd::string editorPreferencesFilePath = AZStd::string::format("@user@/%s/%s", AZ::SettingsRegistryInterface::RegistryFolder, EditorPreferencesFileName);
                AZStd::array<char, AZ::IO::MaxPathLength> resolvedPath {};
                fileIO->ResolvePath(editorPreferencesFilePath.c_str(), resolvedPath.data(), resolvedPath.size());

                AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
                dumperSettings.m_prettifyOutput = true;
                dumperSettings.m_jsonPointerPrefix = "/Amazon/Preferences";

                AZStd::string stringBuffer;
                AZ::IO::ByteContainerStream stringStream(&stringBuffer);
                if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(
                        *registry, "/Amazon/Preferences", stringStream, dumperSettings))
                {
                    AZ_Warning(
                        "AWSAttributionManager", false, R"(Unable to save changes to the Editor Preferences registry file at "%s"\n)",
                        resolvedPath.data());
                    return;
                }

                bool saved {};
                constexpr auto configurationMode =
                    AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
                if (AZ::IO::SystemFile outputFile; outputFile.Open(resolvedPath.data(), configurationMode))
                {
                    saved = outputFile.Write(stringBuffer.data(), stringBuffer.size()) == stringBuffer.size();
                }

                AZ_Warning(
                    "AWSAttributionManager", saved, R"(Unable to save Editor Preferences registry file to path "%s"\n)",
                    editorPreferencesFilePath.c_str());
            },
            true);
        job->Start();
        
    }

    void AWSAttributionManager::UpdateLastSend()
    {
        auto registry = AZ::SettingsRegistry::Get();
        AZ_Assert(registry, "Settings registry is not initialized.");
       
        if (!registry->Set(AWSAttributionLastTimeStampKey,
            AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::system_clock::now().time_since_epoch()).count()))
        {
            AZ_Error("AWSAttributionManager", true, "Failed to set AWSAttributionLastTimeStamp");
            return;
        }

        AZ_Warning("AWSAttributionManager", false, "UpdateLastSend");
        SaveSettingsRegistryFile();
    }

    AZStd::string AWSAttributionManager::GetEngineVersion() const
    {
        // Read from engine.json
        return "";
    }

    AZStd::string AWSAttributionManager::GetPlatform() const
    {
        return AZ::GetPlatformName(AZ::g_currentPlatform);
    }

    void AWSAttributionManager::GetActiveAWSGems(AZStd::vector<AZStd::string>& gems)
    {
        // Read from project's enabled_gems.cmake?
        AZ_UNUSED(gems);
    }

    void AWSAttributionManager::UpdateMetric(AttributionMetric& metric)
    {
        AZStd::string engineVersion = this->GetEngineVersion();
        metric.SetO3DEVersion(engineVersion);

        AZStd::string platform = this->GetPlatform();
        metric.SetPlatform(platform, "");
        // etc.
    }

    void AWSAttributionManager::SubmitMetric(AttributionMetric& metric)
    {
        auto config = ServiceAPI::AWSAttributionRequestJob::GetDefaultConfig();
        config->region = Aws::Region::US_WEST_2;

        // Get default config for the process to check the region.
        // Assumption to determine China region is the default profile is set to China region.
        auto profile_name = Aws::Auth::GetConfigProfileName();
        Aws::Client::ClientConfiguration clientConfig(profile_name.c_str());
        AZStd::string apiId = AWSAttributionApiId;

        if (clientConfig.region == Aws::Region::CN_NORTH_1 || clientConfig.region == Aws::Region::CN_NORTHWEST_1)
        {
            config->region = Aws::Region::CN_NORTH_1;
            apiId = AWSAttributionChinaApiId;
        }

        config->endpointOverride = AWSResourceMappingUtils::FormatRESTApiUrl(apiId, config->region.value().c_str(), AWSAttributionApiStage).c_str();
        ServiceAPI::AWSAttributionRequestJob* requestJob = ServiceAPI::AWSAttributionRequestJob::Create(
            [this](ServiceAPI::AWSAttributionRequestJob* successJob)
            {
                AZ_UNUSED(successJob);
                AZ_Printf("AWSAttributionManager", "AWSAttributionManager submitted metric succesfully");
                UpdateLastSend();

            },
            [this](ServiceAPI::AWSAttributionRequestJob* failedJob)
            {
                AZ_Warning("AWSAttributionManager", false, "AWSAttributionManager failed to submit metric.\nError Message: %s", failedJob->error.message.c_str());
            }, config);

        requestJob->parameters.metric = metric;
        requestJob->Start();
    }

} // namespace AWSCore
