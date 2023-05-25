/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DefaultClientIdProvider.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

namespace AWSMetrics
{
    constexpr const char* EngineVersionJsonKeyFileFormat1 = "O3DEVersion";
    constexpr const char* EngineVersionJsonKeyFileFormat2 = "display_version";

    AZStd::unique_ptr<IdentityProvider> IdentityProvider::CreateIdentityProvider()
    {
        return AZStd::make_unique<DefaultClientIdProvider>(GetEngineVersion());
    }

    AZStd::string IdentityProvider::GetEngineVersion()
    {
        constexpr auto rootKey = AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::EngineSettingsRootKey);
        constexpr auto engineVersionKeyFileFormat1 = rootKey + "/" + EngineVersionJsonKeyFileFormat1;
        constexpr auto engineVersionKeyFileFormat2 = rootKey + "/" + EngineVersionJsonKeyFileFormat2;
        AZStd::string engineVersion;

        if(auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            // prefer latest file format
            if(settingsRegistry->Get(engineVersion, engineVersionKeyFileFormat2))
            {
                return engineVersion;
            }
            else if(settingsRegistry->Get(engineVersion, engineVersionKeyFileFormat1))
            {
                return engineVersion;
            }
        }

        auto engineSettingsPath = AZ::IO::FixedMaxPath{ AZ::Utils::GetEnginePath() } / "engine.json";
        if (AZ::IO::SystemFile::Exists(engineSettingsPath.c_str()))
        {
            AZ::SettingsRegistryImpl settingsRegistry;
            if (settingsRegistry.MergeSettingsFile(
                    engineSettingsPath.Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, AZ::SettingsRegistryMergeUtils::EngineSettingsRootKey))
            {
                if(!settingsRegistry.Get(engineVersion, engineVersionKeyFileFormat2))
                {
                    settingsRegistry.Get(engineVersion, engineVersionKeyFileFormat1);
                }
            }
        }
        return engineVersion;
    }
}
