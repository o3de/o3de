/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <Configuration/AWSCoreConfiguration.h>

namespace AWSCore
{
    AWSCoreConfiguration::AWSCoreConfiguration()
        : m_sourceProjectFolder("")
        , m_profileName(AWSCoreDefaultProfileName)
        , m_resourceMappingConfigFileName("")
    {
    }

    void AWSCoreConfiguration::ActivateConfig()
    {
        AWSCoreInternalRequestBus::Handler::BusConnect();
    }

    void AWSCoreConfiguration::DeactivateConfig()
    {
        AWSCoreInternalRequestBus::Handler::BusDisconnect();
    }

    AZStd::string AWSCoreConfiguration::GetProfileName() const
    {
        return m_profileName;
    }

    AZStd::string AWSCoreConfiguration::GetResourceMappingConfigFilePath() const
    {
        if (m_sourceProjectFolder.empty())
        {
            AZ_Warning(AWSCoreConfigurationName, false, ProjectSourceFolderNotFoundErrorMessage);
            return "";
        }
        if (m_resourceMappingConfigFileName.empty())
        {
            AZ_Warning(AWSCoreConfigurationName, false, ResourceMappingFileNameNotFoundErrorMessage);
            return "";
        }
        AZStd::string configFilePath = AZStd::string::format("%s/%s/%s",
            m_sourceProjectFolder.c_str(), AWSCoreResourceMappingConfigFolderName, m_resourceMappingConfigFileName.c_str());
        AzFramework::StringFunc::Path::Normalize(configFilePath);
        return configFilePath;
    }

    AZStd::string AWSCoreConfiguration::GetResourceMappingConfigFolderPath() const
    {
        if (m_sourceProjectFolder.empty())
        {
            AZ_Warning(AWSCoreConfigurationName, false, ProjectSourceFolderNotFoundErrorMessage);
            return "";
        }
        AZStd::string configFolderPath = AZStd::string::format(
            "%s/%s", m_sourceProjectFolder.c_str(), AWSCoreResourceMappingConfigFolderName);
        AzFramework::StringFunc::Path::Normalize(configFolderPath);
        return configFolderPath;
    }

    void AWSCoreConfiguration::InitConfig()
    {
        InitSourceProjectFolderPath();
        InitSettingsRegistry();
    }

    void AWSCoreConfiguration::InitSettingsRegistry()
    {
        if (m_sourceProjectFolder.empty())
        {
            AZ_Warning(AWSCoreConfigurationName, false, ProjectSourceFolderNotFoundErrorMessage);
            return;
        }

        AZStd::string settingsRegistryPath = AZStd::string::format("%s/%s/%s",
            m_sourceProjectFolder.c_str(), AZ::SettingsRegistryInterface::RegistryFolder, AWSCoreConfiguration::AWSCoreConfigurationFileName);
        AzFramework::StringFunc::Path::Normalize(settingsRegistryPath);

        if (!m_settingsRegistry.MergeSettingsFile(settingsRegistryPath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, ""))
        {
            AZ_Warning(AWSCoreConfigurationName, false, SettingsRegistryLoadFailureErrorMessage);
            return;
        }

        ParseSettingsRegistryValues();
    }

    void AWSCoreConfiguration::InitSourceProjectFolderPath()
    {
        auto sourceProjectFolder = AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@");
        if (!sourceProjectFolder)
        {
            AZ_Error(AWSCoreConfigurationName, false, ProjectSourceFolderNotFoundErrorMessage);
        }
        else
        {
            m_sourceProjectFolder = sourceProjectFolder;
        }
    }

    void AWSCoreConfiguration::ParseSettingsRegistryValues()
    {
        m_resourceMappingConfigFileName.clear();
        auto resourceMappingConfigFileNamePath = AZStd::string::format("%s%s",
            AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSCoreResourceMappingConfigFileNameKey);
        if (!m_settingsRegistry.Get(m_resourceMappingConfigFileName, resourceMappingConfigFileNamePath))
        {
            AZ_Warning(AWSCoreConfigurationName, false, ResourceMappingFileNameNotFoundErrorMessage);
        }

        m_profileName.clear();
        auto profileNamePath = AZStd::string::format(
            "%s%s", AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSCoreProfileNameKey);
        if (!m_settingsRegistry.Get(m_profileName, profileNamePath))
        {
            AZ_Warning(AWSCoreConfigurationName, false, ProfileNameNotFoundErrorMessage);
            m_profileName = AWSCoreDefaultProfileName;
        }
    }

    void AWSCoreConfiguration::ResetSettingsRegistryData()
    {
        auto profileNamePath = AZStd::string::format("%s%s",
            AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSCoreProfileNameKey);
        m_settingsRegistry.Remove(profileNamePath);
        m_profileName = AWSCoreDefaultProfileName;

        auto resourceMappingConfigFileNamePath = AZStd::string::format("%s%s",
            AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSCoreResourceMappingConfigFileNameKey);
        m_settingsRegistry.Remove(resourceMappingConfigFileNamePath);
        m_resourceMappingConfigFileName.clear();
    }

    void AWSCoreConfiguration::ReloadConfiguration()
    {
        ResetSettingsRegistryData();
        InitSettingsRegistry();
    }
} // namespace AWSCore
