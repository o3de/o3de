/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Attribution/AWSCoreAttributionMetric.h>
#include <Editor/Attribution/AWSCoreAttributionManager.h>
#include <Editor/Attribution/AWSCoreAttributionConsentDialog.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/PlatformId/PlatformId.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <ResourceMapping/AWSResourceMappingUtils.h>
#include <Credential/AWSCredentialBus.h>

#include <QSysInfo>
#include <QMessageBox>
#include <QCheckBox>


namespace AWSCore
{
    constexpr const char* EngineVersionJsonKey = "O3DEVersion";

    constexpr char EditorAWSPreferencesFileName[] = "editor_aws_preferences.setreg";
    constexpr char AWSAttributionSettingsPrefixKey[] = "/Amazon/AWS/Preferences";
    constexpr char AWSAttributionEnabledKey[] = "/Amazon/AWS/Preferences/AWSAttributionEnabled";
    constexpr char AWSAttributionDelaySecondsKey[] = "/Amazon/AWS/Preferences/AWSAttributionDelaySeconds";
    constexpr char AWSAttributionLastTimeStampKey[] = "/Amazon/AWS/Preferences/AWSAttributionLastTimeStamp";
    constexpr char AWSAttributionConsentShownKey[] = "/Amazon/AWS/Preferences/AWSAttributionConsentShown";
    constexpr char AWSAttributionEndpoint[] = "https://o3deattribution.us-east-1.amazonaws.com";
    constexpr char AWSAttributionChinaEndpoint[] = "";
    const int AWSAttributionDefaultDelayInDays = 7;

    AWSAttributionManager::AWSAttributionManager()
    {
        m_settingsRegistry = AZ::SettingsRegistry::Get();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    AWSAttributionManager::~AWSAttributionManager()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        m_settingsRegistry = nullptr;
    }

    void AWSAttributionManager::Init()
    {
        bool consentShown;
        // If override is used skip merging the settings file
        if (m_settingsRegistry->Get(consentShown, AWSAttributionConsentShownKey))
        {
            return;
        }

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "File IO is not initialized.");

        // Resolve path to editor_aws_preferences.setreg
        const AZStd::string editorAWSPreferencesFilePath =
            AZStd::string::format("@user@/%s/%s", AZ::SettingsRegistryInterface::RegistryFolder, EditorAWSPreferencesFileName);
        AZ::IO::FixedMaxPath resolvedPathAWSPreference;
        if (!fileIO->ResolvePath(resolvedPathAWSPreference, AZ::IO::PathView(editorAWSPreferencesFilePath)))
        {
            AZ_Warning("AWSAttributionManager", false, "Error resolving path %s", resolvedPathAWSPreference.c_str());
            return;
        }

        if (fileIO->Exists(resolvedPathAWSPreference.c_str()))
        {
            m_settingsRegistry->MergeSettingsFile(
                resolvedPathAWSPreference.String(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "");
        }
    }

    void AWSAttributionManager::MetricCheck()
    {
        if (!CheckAWSCredentialsConfigured())
        {
            return;
        }

        if (!CheckConsentShown())
        {
            ShowConsentDialog();
        }

        if (ShouldGenerateMetric())
        {
            // Gather metadata and assemble metric
            AttributionMetric metric;
            UpdateMetric(metric);

            // Post metric
            SubmitMetric(metric);
        }
    }

    bool AWSAttributionManager::ShouldGenerateMetric() const
    {
        bool awsAttributionEnabled = false;
        if (!m_settingsRegistry->Get(awsAttributionEnabled, AWSAttributionEnabledKey))
        {
            AZ_Warning("AWSAttributionManager", false, "Key %s should be set by consent window", AWSAttributionEnabledKey);
            return false;
        }

        if (!awsAttributionEnabled)
        {
            return false;
        }

        // If delayInSeconds is not found, set default to a day
        AZ::u64 delayInSeconds = 0;
        if (!m_settingsRegistry->Get(delayInSeconds, AWSAttributionDelaySecondsKey))
        {
            delayInSeconds = static_cast<AZ::u64>(AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::days(AWSAttributionDefaultDelayInDays)).count());
            m_settingsRegistry->Set(AWSAttributionDelaySecondsKey, delayInSeconds);
        }

        AZ::u64 lastSendTimeStampSeconds = 0;
        if (!m_settingsRegistry->Get(lastSendTimeStampSeconds, AWSAttributionLastTimeStampKey))
        {
            // If last time stamp not found, assume this is the first attempt at sending.
            return true;
        }

        const AZStd::chrono::seconds lastSendTimeStamp = AZStd::chrono::seconds(lastSendTimeStampSeconds);
        const AZStd::chrono::seconds secondsSinceLastSend =
            AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::system_clock::now().time_since_epoch()) - lastSendTimeStamp;
        if (static_cast<AZ::u64>(secondsSinceLastSend.count()) >= delayInSeconds)
        {
            return true;
        }

        return false;
    }

    bool AWSAttributionManager::CheckAWSCredentialsConfigured()
    {
        AWSCore::AWSCredentialResult credentialResult;
        AWSCore::AWSCredentialRequestBus::BroadcastResult(credentialResult, &AWSCore::AWSCredentialRequests::GetCredentialsProvider);
        if (credentialResult.result)
        {
            std::shared_ptr<Aws::Auth::AWSCredentialsProvider> provider = credentialResult.result;
            const auto creds = provider->GetAWSCredentials();
            if (!creds.IsEmpty())
            {
                return true;
            }
        }
        return false;
    }

    void AWSAttributionManager::ShowConsentDialog()
    {
        AWSCoreAttributionConsentDialog* msgBox = aznew AWSCoreAttributionConsentDialog();
        int ret = msgBox->exec();
        m_settingsRegistry->Set(AWSAttributionConsentShownKey, true);
        switch (ret)
        {
        case QMessageBox::Save:
            m_settingsRegistry->Set(AWSAttributionEnabledKey, msgBox->checkBox()->checkState() == Qt::Checked);
            break;
        case QMessageBox::Cancel:
        default:
            m_settingsRegistry->Set(AWSAttributionEnabledKey, false);
            break;
        }

        SaveSettingsRegistryFile();
        delete msgBox;
    }

    // Waiting on Editor QT main window to be initialized before showing consent window.
    // This will have the Editor loading screen in the background when showing consent dialog.
    void AWSAttributionManager::NotifyMainWindowInitialized(QMainWindow* mainWindow)
    {
        AZ_UNUSED(mainWindow);
        MetricCheck();
    }

    void AWSAttributionManager::SaveSettingsRegistryFile()
    {
        AZ::Job* job = AZ::CreateJobFunction(
            [this]()
            {
                AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                AZ_Assert(fileIO, "File IO is not initialized.");

                // Resolve path to editor_aws_preferences.setreg
                const AZStd::string editorPreferencesFilePath = AZStd::string::format("@user@/%s/%s", AZ::SettingsRegistryInterface::RegistryFolder, EditorAWSPreferencesFileName);
                AZ::IO::FixedMaxPath resolvedPathAWSPreference;
                if (!fileIO->ResolvePath(resolvedPathAWSPreference, AZ::IO::PathView(editorPreferencesFilePath)))
                {
                    AZ_Warning("AWSAttributionManager", false, "Error resolving path %s", editorPreferencesFilePath.c_str());
                    return;
                }

                AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
                dumperSettings.m_prettifyOutput = true;
                dumperSettings.m_jsonPointerPrefix = AWSAttributionSettingsPrefixKey;

                AZStd::string stringBuffer;
                AZ::IO::ByteContainerStream stringStream(&stringBuffer);
                if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(
                        *m_settingsRegistry, AWSAttributionSettingsPrefixKey, stringStream, dumperSettings))
                {
                    AZ_Warning(
                        "AWSAttributionManager", false, R"(Unable to save changes to the Editor AWS Preferences registry file at "%s"\n)",
                        resolvedPathAWSPreference.c_str());
                    return;
                }

                [[maybe_unused]] bool saved = false;
                constexpr auto configurationMode =
                    AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
                if (AZ::IO::SystemFile outputFile; outputFile.Open(resolvedPathAWSPreference.c_str(), configurationMode))
                {
                    saved = outputFile.Write(stringBuffer.data(), stringBuffer.size()) == stringBuffer.size();
                }

                AZ_Warning(
                    "AWSAttributionManager", saved, R"(Unable to save Editor AWS Preferences registry file to path "%s"\n)",
                    editorPreferencesFilePath.c_str());
            },
            true);
        job->Start();

    }

    void AWSAttributionManager::UpdateLastSend()
    {
        if (!m_settingsRegistry->Set(AWSAttributionLastTimeStampKey,
            static_cast<AZ::s64>(AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::system_clock::now().time_since_epoch()).count())))
        {
            AZ_Warning("AWSAttributionManager", true, "Failed to set AWSAttributionLastTimeStamp");
            return;
        }
        SaveSettingsRegistryFile();
    }

    void AWSAttributionManager::SetApiEndpointAndRegion(AWSCore::ServiceAPI::AWSAttributionRequestJob::Config* config)
    {
        // Get default config for the process to check the region.
        // Assumption to determine China region is the default profile is set to China region.
        auto profile_name = Aws::Auth::GetConfigProfileName();
        Aws::Client::ClientConfiguration clientConfig(profile_name.c_str());

        if (clientConfig.region == Aws::Region::CN_NORTH_1 || clientConfig.region == Aws::Region::CN_NORTHWEST_1)
        {
            config->region = Aws::Region::CN_NORTH_1;
            config->endpointOverride = AWSAttributionChinaEndpoint;
        }
        else
        {
            config->region = Aws::Region::US_EAST_1;
            config->endpointOverride = AWSAttributionEndpoint;
        }
    }

    bool AWSAttributionManager::CheckConsentShown()
    {
        bool consentShown = false;
        m_settingsRegistry->Get(consentShown, AWSAttributionConsentShownKey);
        return consentShown;
    }

    AZStd::string AWSAttributionManager::GetEngineVersion() const
    {
        AZStd::string engineVersion;
        auto engineSettingsPath = AZ::IO::FixedMaxPath{ AZ::Utils::GetEnginePath() } / "engine.json";
        if (AZ::IO::SystemFile::Exists(engineSettingsPath.c_str()))
        {
            AZ::SettingsRegistryImpl settingsRegistry;
            if (settingsRegistry.MergeSettingsFile(
                    engineSettingsPath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, AZ::SettingsRegistryMergeUtils::EngineSettingsRootKey))
            {
                settingsRegistry.Get(engineVersion, AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::EngineSettingsRootKey) + "/" + EngineVersionJsonKey);
            }
        }
        return engineVersion;
    }

    AZStd::string AWSAttributionManager::MapPlatform(AZ::PlatformID platform)
    {
        // Only map platforms running the editor to Attributions enums
        // PC, Linux, Mac are supported values for now
        switch (platform)
        {
        case AZ::PlatformID::PLATFORM_WINDOWS_64:
            return "PC";
        case AZ::PlatformID::PLATFORM_LINUX_64:
            return "Linux";
        case AZ::PlatformID::PLATFORM_APPLE_MAC:
            return "Mac";
        default:
            return "Other";
        }
    }

    AZStd::string AWSAttributionManager::GetPlatform() const
    {
        return MapPlatform(AZ::g_currentPlatform);
    }

    void AWSAttributionManager::GetActiveAWSGems(AZStd::vector<AZStd::string>& gems)
    {
        AZ::ModuleManagerRequestBus::Broadcast(
            &AZ::ModuleManagerRequestBus::Events::EnumerateModules,
            [&gems](const AZ::ModuleData& moduleData)
            {
                AZ::Entity* moduleEntity = moduleData.GetEntity();
                auto moduleEntityName = moduleEntity->GetName();
                if (moduleEntityName.contains("AWS"))
                    gems.push_back(moduleEntityName.substr(0, moduleEntityName.find_last_of(".")));
                return true;
            });
    }

    void AWSAttributionManager::UpdateMetric(AttributionMetric& metric)
    {
        AZStd::string engineVersion = this->GetEngineVersion();
        metric.SetO3DEVersion(engineVersion);

        AZStd::string platform = this->GetPlatform();
        QString productName = QSysInfo::prettyProductName();
        metric.SetPlatform(platform, productName.toStdString().c_str());

        AZStd::vector<AZStd::string> gemNames;
        GetActiveAWSGems(gemNames);
        for (AZStd::string& gemName : gemNames)
        {
            metric.AddActiveGem(gemName);
        }
    }

    void AWSAttributionManager::SubmitMetric(AttributionMetric& metric)
    {
        AWSCore::ServiceAPI::AWSAttributionRequestJob::Config* config = ServiceAPI::AWSAttributionRequestJob::GetDefaultConfig();
        // Identify region and chose attribution endpoint
        SetApiEndpointAndRegion(config);

        ServiceAPI::AWSAttributionRequestJob* requestJob = ServiceAPI::AWSAttributionRequestJob::Create(
            [this]([[maybe_unused]] ServiceAPI::AWSAttributionRequestJob* successJob)
            {
                UpdateLastSend();
                AZ_Printf("AWSAttributionManager", "AWSAttribution metric submit success");

            },
            []([[maybe_unused]] ServiceAPI::AWSAttributionRequestJob* failJob)
            {
                AZ_Error("AWSAttributionManager", false, "Metrics send error: %s", failJob->error.message.c_str());
            },
            config);

        requestJob->parameters.metric = metric;
        requestJob->Start();
    }

} // namespace AWSCore
