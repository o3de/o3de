/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/std/string/string.h>

#include <AWSCoreInternalBus.h>

namespace AWSCore
{
    //! Responsible for handling configuration for AWSCore gem
    //! TODO: Expose internal field through edit behavior if required
    class AWSCoreConfiguration
        : AWSCoreInternalRequestBus::Handler
    {
    public:
        static constexpr const char AWSCoreConfigurationName[] = "AWSCoreConfiguration";
        static constexpr const char AWSCoreConfigurationFileName[] = "awscoreconfiguration.setreg";

        static constexpr const char AWSCoreResourceMappingConfigFolderName[] = "Config";
        static constexpr const char AWSCoreResourceMappingConfigFileNameKey[] = "/AWSCore/ResourceMappingConfigFileName";

        static constexpr const char AWSCoreDefaultProfileName[] = "default";
        static constexpr const char AWSCoreProfileNameKey[] = "/AWSCore/ProfileName";

        static constexpr const char ProjectSourceFolderNotFoundErrorMessage[] =
            "Failed to get project source folder path.";
        static constexpr const char ProfileNameNotFoundErrorMessage[] =
            "Failed to get profile name, return default value instead.";
        static constexpr const char ResourceMappingFileNameNotFoundErrorMessage[] =
            "Failed to get resource mapping config file name, return empty value instead.";
        static constexpr const char SettingsRegistryLoadFailureErrorMessage[] =
            "Failed to load AWSCore settings registry file.";


        AWSCoreConfiguration();
        ~AWSCoreConfiguration() = default;

        void ActivateConfig();
        void DeactivateConfig();
        void InitConfig();

        // AWSCoreInternalRequestBus interface implementation
        AZStd::string GetResourceMappingConfigFilePath() const override;
        AZStd::string GetResourceMappingConfigFolderPath() const override;
        AZStd::string GetProfileName() const override;
        void ReloadConfiguration() override;

    private:
        // Initialize settings registry reference by loading for project .setreg file
        void InitSettingsRegistry();

        // Initialize source project folder path
        void InitSourceProjectFolderPath();

        // Parse values from project .setreg file
        void ParseSettingsRegistryValues();

        // Reset settings registry data
        void ResetSettingsRegistryData();

        AZStd::string m_sourceProjectFolder;
        AZ::SettingsRegistryImpl m_settingsRegistry;
        AZStd::string m_profileName;
        AZStd::string m_resourceMappingConfigFileName;
    };
} // namespace AWSCore
