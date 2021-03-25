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
        static constexpr const char AWSCORE_CONFIGURATION_FILENAME[] = "awscoreconfiguration.setreg";

        static constexpr const char AWSCORE_RESOURCE_MAPPING_CONFIG_FOLDERNAME[] = "Config";
        static constexpr const char AWSCORE_RESOURCE_MAPPING_CONFIG_FILENAME_KEY[] = "/AWSCore/ResourceMappingConfigFileName";

        static constexpr const char AWSCORE_DEFAULT_PROFILE_NAME[] = "default";
        static constexpr const char AWSCORE_PROFILENAME_KEY[] = "/AWSCore/ProfileName";

        AWSCoreConfiguration();
        ~AWSCoreConfiguration() = default;

        void ActivateConfig();
        void DeactivateConfig();
        void InitConfig();

        // AWSCoreInternalRequestBus interface implementation
        AZStd::string GetResourceMappingConfigFilePath() const override;
        AZStd::string GetProfileName() const override;
        void ReloadConfiguration() override;

    private:
        // Initialize settings registry reference by loading for project .setreg file
        void InitSettingsRegistry();

        // Initialize source project folder path
        void InitSourceProjectFolderPath();

        // Parse values from project .setreg file
        void ParseSettingsRegistryValues();

        AZStd::string m_sourceProjectFolder;
        AZ::SettingsRegistryImpl m_settingsRegistry;
        AZStd::string m_profileName;
        AZStd::string m_resourceMappingConfigFileName;
    };
} // namespace AWSCore
