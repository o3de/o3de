/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/utilities/AssetServerHandler.h>
#include <native/resourcecompiler/rcjob.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzToolsFramework/Archive/ArchiveAPI.h>
#include <AzCore/JSON/pointer.h>
#include <QDir>

namespace AssetProcessor
{
    void CleanupFilename(QString& string)
    {
        AZStd::string buffer(string.toUtf8().toStdString().c_str());
        AZStd::string forbiddenChars("\\/:?\"<>|");
        AZStd::replace_if(
            buffer.begin(),
            buffer.end(),
            [forbiddenChars](char c) { return AZStd::string::npos != forbiddenChars.find(c); },
            ' ');
        string.clear();
        string.append(buffer.c_str());
    }

    AssetServerMode CheckServerMode()
    {
        AssetServerMode enableCacheServerMode = AssetServerMode::Inactive;

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            bool enableAssetCacheServerMode = false;
            AZ::SettingsRegistryInterface::FixedValueString key(AssetProcessor::AssetProcessorServerKey);
            key += "/";
            if (settingsRegistry->Get(enableAssetCacheServerMode, key + "enableCacheServer"))
            {
                enableCacheServerMode = enableAssetCacheServerMode ? AssetServerMode::Server : AssetServerMode::Client;
                AZ_Warning(AssetProcessor::DebugChannel, false, "The 'enableCacheServer' key is deprecated. Please swith to 'assetCacheServerMode'");
            }

            AZStd::string assetCacheServerModeValue;
            if (settingsRegistry->Get(assetCacheServerModeValue, key + AssetCacheServerModeKey))
            {
                AZStd::to_lower(assetCacheServerModeValue.begin(), assetCacheServerModeValue.end());

                if(assetCacheServerModeValue == "server")
                {
                    return AssetServerMode::Server;
                }
                else if (assetCacheServerModeValue == "client")
                {
                    return AssetServerMode::Client;
                }
                else if (assetCacheServerModeValue != "inactive")
                {
                    AZ_Warning(AssetProcessor::DebugChannel, false, "Unknown mode for 'assetCacheServerMode' (%s)", assetCacheServerModeValue.c_str());
                }
            }
        }

        return enableCacheServerMode;
    }

    AZStd::string CheckServerAddress()
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry)
        {
            AZStd::string address;
            if (settingsRegistry->Get(address,
                AZ::SettingsRegistryInterface::FixedValueString(AssetProcessor::AssetProcessorServerKey)
                + "/"
                + CacheServerAddressKey))
            {
                AZ_TracePrintf(AssetProcessor::DebugChannel, "Server Address: %s\n", address.c_str());
                return AZStd::move(address);
            }
        }
        return {};
    }

    QString AssetServerHandler::ComputeArchiveFilePath(const AssetProcessor::BuilderParams& builderParams)
    {
        QFileInfo fileInfo(builderParams.m_processJobRequest.m_sourceFile.c_str());
        QString assetServerAddress = QDir::toNativeSeparators(QString{m_serverAddress.c_str()});
        if (!assetServerAddress.isEmpty())
        {
            QString archiveFileName = builderParams.GetServerKey() + ".zip";
            CleanupFilename(archiveFileName);
            QDir archiveFolder = QDir(assetServerAddress).filePath(fileInfo.path());
            QString archiveFilePath = archiveFolder.filePath(archiveFileName);

            // create directories if does not exists
            if (!archiveFolder.exists())
            {
                archiveFolder.mkdir(".");
            }
            return archiveFilePath;
        }
        else
        {
            AZStd::string filePath;
            AssetServerInfoBus::BroadcastResult(filePath, &AssetServerInfoBus::Events::ComputeArchiveFilePath, builderParams);
            if (!filePath.empty())
            {
                QString archiveFilePath(filePath.c_str());
                return archiveFilePath;
            }
        }

        return QString();
    }

    const char* AssetServerHandler::GetAssetServerModeText(AssetServerMode mode)
    {
        switch (mode)
        {
            case AssetServerMode::Inactive: return "inactive";
            case AssetServerMode::Server: return "server";
            case AssetServerMode::Client: return "client";
            default:
                break;
        }
        return "unknown";
    }

    AssetServerHandler::AssetServerHandler()
    {
        SetRemoteCachingMode(CheckServerMode());
        SetServerAddress(CheckServerAddress());
        AssetServerBus::Handler::BusConnect();
    }

    AssetServerHandler::~AssetServerHandler()
    {
        SetRemoteCachingMode(AssetServerMode::Inactive);
        AssetServerBus::Handler::BusDisconnect();
    }

    bool AssetServerHandler::IsServerAddressValid()
    {
        QString address{m_serverAddress.c_str()};
        bool isValid = !address.isEmpty() && QDir(address).exists();
        return isValid;
    }

    void AssetServerHandler::HandleRemoteConfiguration()
    {
        if (m_assetCachingMode == AssetServerMode::Inactive || !IsServerAddressValid())
        {
            return;
        }
        AZ::IO::Path settingsFilePath{ m_serverAddress };
        settingsFilePath /= "settings.json";

        auto* recognizerConfiguration = AZ::Interface<AssetProcessor::RecognizerConfiguration>::Get();
        if (!recognizerConfiguration)
        {
            return;
        }

        if (m_assetCachingMode == AssetServerMode::Server)
        {
            AZStd::string jsonBuffer;
            const auto& assetCacheRecognizerContainer = recognizerConfiguration->GetAssetCacheRecognizerContainer();
            AssetProcessor::PlatformConfiguration::ConvertToJson(assetCacheRecognizerContainer, jsonBuffer);
            if (jsonBuffer.empty())
            {
                // no configuration to save
                return;
            }

            // save the configuration
            rapidjson::Document recognizerDoc;
            recognizerDoc.Parse(jsonBuffer.c_str());
            AZ::JsonSerializationUtils::WriteJsonFile(recognizerDoc, settingsFilePath.LexicallyNormal().c_str());
        }
        else if (m_assetCachingMode == AssetServerMode::Client)
        {
            // load the configuration
            if (!AZ::IO::SystemFile::Exists(settingsFilePath.c_str()))
            {
                // no log since it is okay to not have a settings file
                return;
            }

            auto result = AZ::JsonSerializationUtils::ReadJsonFile(settingsFilePath.LexicallyNormal().c_str());
            if (!result.IsSuccess())
            {
                AZ_Warning(AssetProcessor::DebugChannel, false, "ACS settings file failed with (%s)", result.GetError().c_str());
                return;
            }

            rapidjson::StringBuffer stringBuffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);
            if (result.GetValue().Accept(writer) == false)
            {
                AZ_Warning(AssetProcessor::DebugChannel, false, "ACS failed to load settings file (%s)", settingsFilePath.c_str());
                return;
            }

            RecognizerContainer recognizerContainer;
            if (!AssetProcessor::PlatformConfiguration::ConvertFromJson(stringBuffer.GetString(), recognizerContainer))
            {
                AZ_Warning(AssetProcessor::DebugChannel, false, "ACS failed to convert settings file (%s)", settingsFilePath.c_str());
                return;
            }

            recognizerConfiguration->AddAssetCacheRecognizerContainer(recognizerContainer);
        }
    }

    AssetServerMode AssetServerHandler::GetRemoteCachingMode() const
    {
        return m_assetCachingMode;
    }

    void AssetServerHandler::SetRemoteCachingMode(AssetServerMode mode)
    {
        m_assetCachingMode = mode;
        AssetServerNotificationBus::Broadcast(&AssetServerNotificationBus::Events::OnRemoteCachingModeChanged, mode);
    }

    const AZStd::string& AssetServerHandler::GetServerAddress() const
    {
        return m_serverAddress;
    }

    bool AssetServerHandler::SetServerAddress(const AZStd::string& address)
    {
        AZStd::string previousServerAddress = m_serverAddress;
        m_serverAddress = address;
        if (!IsServerAddressValid())
        {
            m_serverAddress = previousServerAddress;
            AZ_Error(AssetProcessor::DebugChannel,
                m_assetCachingMode == AssetServerMode::Inactive,
                "Server address (%.*s) is invalid! Reverting back to (%.*s)",
                AZ_STRING_ARG(address),
                AZ_STRING_ARG(previousServerAddress));
            return false;
        }
        return true;
    }

    bool AssetServerHandler::RetrieveJobResult(const AssetProcessor::BuilderParams& builderParams)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(builderParams.m_rcJob->GetJobEntry().m_jobRunKey);
        AssetUtilities::QuitListener listener;
        listener.BusConnect();

        QString archiveAbsFilePath = ComputeArchiveFilePath(builderParams);
        if (archiveAbsFilePath.isEmpty())
        {
            AZ_Error(AssetProcessor::DebugChannel, false, "Extracting archive operation failed. Archive Absolute Path is empty.");
            return false;
        }

        if (!QFile::exists(archiveAbsFilePath))
        {
            // file does not exist on the server
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Extracting archive operation canceled. Archive does not exist on server. \n");
            return false;
        }

        if (listener.WasQuitRequested() || jobCancelListener.IsCancelled())
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Extracting archive operation canceled. \n");
            return false;
        }
        AZ_TracePrintf(AssetProcessor::DebugChannel, "Extracting archive for job (%s, %s, %s) with fingerprint (%u).\n",
            builderParams.m_rcJob->GetJobEntry().m_sourceAssetReference.AbsolutePath().c_str(), builderParams.m_rcJob->GetJobKey().toUtf8().data(),
            builderParams.m_rcJob->GetPlatformInfo().m_identifier.c_str(), builderParams.m_rcJob->GetOriginalFingerprint());
        std::future<bool> extractResult;
        AzToolsFramework::ArchiveCommandsBus::BroadcastResult(extractResult,
            &AzToolsFramework::ArchiveCommandsBus::Events::ExtractArchive,
            archiveAbsFilePath.toUtf8().data(), builderParams.GetTempJobDirectory());
        bool success = extractResult.valid() ? extractResult.get() : false;
        AZ_Error(AssetProcessor::DebugChannel, success, "Extracting archive operation failed.\n");
        return success;
    }

    bool AssetServerHandler::StoreJobResult(const AssetProcessor::BuilderParams& builderParams, AZStd::vector<AZStd::string>& sourceFileList)
    {
        AssetBuilderSDK::JobCancelListener jobCancelListener(builderParams.m_rcJob->GetJobEntry().m_jobRunKey);
        AssetUtilities::QuitListener listener;
        listener.BusConnect();
        QString archiveAbsFilePath = ComputeArchiveFilePath(builderParams);

        if (archiveAbsFilePath.isEmpty())
        {
            AZ_Error(AssetProcessor::DebugChannel, false, "Creating archive operation failed. Archive Absolute Path is empty. \n");
            return false;
        }

        if (QFile::exists(archiveAbsFilePath))
        {
            // file already exists on the server
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Creating archive operation canceled. An archive of this asset already exists on server. \n");
            return true;
        }

        if (listener.WasQuitRequested() || jobCancelListener.IsCancelled())
        {
            AZ_TracePrintf(AssetProcessor::DebugChannel, "Creating archive operation canceled. \n");
            return false;
        }

        // make sub-folders if needed
        QFileInfo fileInfo(archiveAbsFilePath);
        if (!fileInfo.absoluteDir().exists())
        {
            if (!fileInfo.absoluteDir().mkpath("."))
            {
                AZ_Error(AssetProcessor::DebugChannel, false, "Could not make archive folder %s !",
                    fileInfo.absoluteDir().absolutePath().toUtf8().data());
                return false;
            }
        }

        AZ_TracePrintf(AssetProcessor::DebugChannel, "Creating archive for job (%s, %s, %s) with fingerprint (%u).\n",
            builderParams.m_rcJob->GetJobEntry().m_sourceAssetReference.AbsolutePath().c_str(), builderParams.m_rcJob->GetJobKey().toUtf8().data(),
            builderParams.m_rcJob->GetPlatformInfo().m_identifier.c_str(), builderParams.m_rcJob->GetOriginalFingerprint());

        std::future<bool> createResult;
        AzToolsFramework::ArchiveCommandsBus::BroadcastResult(createResult,
            &AzToolsFramework::ArchiveCommandsBus::Events::CreateArchive,
            archiveAbsFilePath.toUtf8().data(), builderParams.GetTempJobDirectory());
        bool success = createResult.valid() ? createResult.get() : false;
        AZ_Error(AssetProcessor::DebugChannel, success, "Creating archive operation failed. \n");

        if (success && sourceFileList.size())
        {
            // Check if any of our output products for this job was a source file which would not be in the temp folder
            // If so add it to the archive
            AddSourceFilesToArchive(builderParams, archiveAbsFilePath, sourceFileList);
        }
        return success;
    }

    bool AssetServerHandler::AddSourceFilesToArchive(const AssetProcessor::BuilderParams& builderParams, const QString& archivePath, AZStd::vector<AZStd::string>& sourceFileList)
    {
        bool allSuccess{ true };
        for (const auto& thisProduct : sourceFileList)
        {
            QFileInfo sourceFile{ builderParams.m_rcJob->GetJobEntry().GetAbsoluteSourcePath() };
            QDir sourceDir{ sourceFile.absoluteDir() };

            if (!QFileInfo(sourceDir.absoluteFilePath(thisProduct.c_str())).exists())
            {
                AZ_Warning(AssetProcessor::DebugChannel, false, "Failed to add %s to %s - source does not exist in expected location (sourceDir %s )", thisProduct.c_str(), archivePath.toUtf8().data(), sourceDir.path().toUtf8().data());
                allSuccess = false;
                continue;
            }
            std::future<bool> addResult;
            AzToolsFramework::ArchiveCommandsBus::BroadcastResult(addResult,
                &AzToolsFramework::ArchiveCommandsBus::Events::AddFileToArchive,
                archivePath.toUtf8().data(), sourceDir.path().toUtf8().data(), thisProduct.c_str());
            bool success = addResult.valid() ? addResult.get() : false;
            if (!success)
            {
                AZ_Warning(AssetProcessor::DebugChannel, false, "Failed to add %s to %s", thisProduct.c_str(), archivePath.toUtf8().data());
                allSuccess = false;
            }
        }
        return allSuccess;
    }
}// AssetProcessor
