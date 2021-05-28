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
        m_profileName.clear();

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
