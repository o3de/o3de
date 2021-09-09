/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Config_wwise.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>

// For AZ_Printf statements...
#define WWISE_CONFIG_WINDOW "WwiseConfig"


namespace Audio::Wwise
{
    static AZStd::string_view s_configuredBanksPath = DefaultBanksPath;

    const AZStd::string_view GetBanksRootPath()
    {
        return s_configuredBanksPath;
    }

    void SetBanksRootPath(const AZStd::string_view path)
    {
        s_configuredBanksPath = path;
    }

    // static
    void ConfigurationSettings::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PlatformMapping>()
                ->Version(2)
                ->Field("assetPlatform", &PlatformMapping::m_assetPlatform)
                ->Field("altAssetPlatform", &PlatformMapping::m_altAssetPlatform)
                ->Field("enginePlatform", &PlatformMapping::m_enginePlatform)
                ->Field("wwisePlatform", &PlatformMapping::m_wwisePlatform)
                ->Field("bankSubPath", &PlatformMapping::m_bankSubPath)
                ;

            serializeContext->Class<ConfigurationSettings>()
                ->Version(1)
                ->Field("platformMaps", &ConfigurationSettings::m_platformMappings)
                ;
        }
    }

    bool ConfigurationSettings::Load(const AZStd::string& filePath)
    {
        AZ::IO::Path fileIoPath(filePath);
        auto outcome = AZ::JsonSerializationUtils::ReadJsonFile(fileIoPath.Native());
        if (!outcome)
        {
            AZ_Printf(WWISE_CONFIG_WINDOW, "ERROR: %s\n", outcome.GetError().c_str());
            return false;
        }

        m_platformMappings.clear();

        AZ::JsonDeserializerSettings deserializeSettings;
        AZ::ComponentApplicationBus::BroadcastResult(deserializeSettings.m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        auto result = AZ::JsonSerialization::Load(*this, outcome.GetValue(), deserializeSettings);
        if (result.GetProcessing() != AZ::JsonSerializationResult::Processing::Completed)
        {
            AZ_Printf(WWISE_CONFIG_WINDOW, "ERROR: Deserializing Json file '%s'\n", filePath.c_str());
            return false;
        }

        AZ_Printf(WWISE_CONFIG_WINDOW, "Loaded '%s' successfully.\n", filePath.c_str());
        return true;
    }

    bool ConfigurationSettings::Save(const AZStd::string& filePath)
    {
        AZ::JsonSerializerSettings serializeSettings;
        AZ::ComponentApplicationBus::BroadcastResult(serializeSettings.m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        rapidjson::Document jsonDoc;
        auto result = AZ::JsonSerialization::Store(jsonDoc, jsonDoc.GetAllocator(), *this, serializeSettings);
        if (result.GetProcessing() != AZ::JsonSerializationResult::Processing::Completed)
        {
            AZ_Printf(WWISE_CONFIG_WINDOW, "ERROR: Serializing Json file '%s'\n", filePath.c_str());
            return false;
        }

        auto outcome = AZ::JsonSerializationUtils::WriteJsonFile(jsonDoc, filePath);
        if (!outcome)
        {
            AZ_Printf(WWISE_CONFIG_WINDOW, "ERROR: %s\n", outcome.GetError().c_str());
            return false;
        }

        AZ_Printf(WWISE_CONFIG_WINDOW, "Saved '%s' successfully.\n", filePath.c_str());
        return true;
    }

} // namespace Audio::Wwise
