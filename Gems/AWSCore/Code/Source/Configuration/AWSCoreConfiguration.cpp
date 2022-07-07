/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
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
        ParseSettingsRegistryValues();
    }

    void AWSCoreConfiguration::InitSourceProjectFolderPath()
    {
        auto sourceProjectFolder = AZ::IO::FileIOBase::GetInstance()->GetAlias("@projectroot@");
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
        AZ::SettingsRegistryInterface*  settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            AZ_Warning(AWSCoreConfigurationName, false, GlobalSettingsRegistryLoadFailureErrorMessage);
            return;
        }

        m_resourceMappingConfigFileName.clear();
        auto resourceMappingConfigFileNamePath = AZStd::string::format("%s%s",
            AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSCoreResourceMappingConfigFileNameKey);
        if (!settingsRegistry->Get(m_resourceMappingConfigFileName, resourceMappingConfigFileNamePath))
        {
            AZ_Warning(AWSCoreConfigurationName, false, ResourceMappingFileNameNotFoundErrorMessage);
        }

        m_profileName.clear();
        auto profileNamePath = AZStd::string::format(
            "%s%s", AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSCoreProfileNameKey);
        if (!settingsRegistry->Get(m_profileName, profileNamePath))
        {
            AZ_Warning(AWSCoreConfigurationName, false, ProfileNameNotFoundErrorMessage);
            m_profileName = AWSCoreDefaultProfileName;
        }
    }

    void AWSCoreConfiguration::ResetSettingsRegistryData()
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        if (!settingsRegistry)
        {
            AZ_Warning(AWSCoreConfigurationName, false, GlobalSettingsRegistryLoadFailureErrorMessage);
            return;
        }

        auto profileNamePath = AZStd::string::format("%s%s",
            AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSCoreProfileNameKey);
        settingsRegistry->Remove(profileNamePath);
        m_profileName = AWSCoreDefaultProfileName;

        auto resourceMappingConfigFileNamePath = AZStd::string::format("%s%s",
            AZ::SettingsRegistryMergeUtils::OrganizationRootKey, AWSCoreResourceMappingConfigFileNameKey);
        settingsRegistry->Remove(resourceMappingConfigFileNamePath);
        m_resourceMappingConfigFileName.clear();

        // Reload the AWSCore setting registry file from disk.
        if (m_sourceProjectFolder.empty())
        {
            AZ_Warning(AWSCoreConfigurationName, false, SettingsRegistryFileLoadFailureErrorMessage);
            return;
        }

        auto settingsRegistryPath = AZ::IO::FixedMaxPath(AZStd::string_view{ m_sourceProjectFolder }) /
            AZ::SettingsRegistryInterface::RegistryFolder /
            AWSCoreConfiguration::AWSCoreConfigurationFileName;
        if (!settingsRegistry->MergeSettingsFile(settingsRegistryPath.c_str(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, ""))
        {
            AZ_Warning(AWSCoreConfigurationName, false, SettingsRegistryFileLoadFailureErrorMessage);
            return;
        }
    }

    void AWSCoreConfiguration::ReloadConfiguration()
    {
        ResetSettingsRegistryData();
        ParseSettingsRegistryValues();
    }
} // namespace AWSCore
