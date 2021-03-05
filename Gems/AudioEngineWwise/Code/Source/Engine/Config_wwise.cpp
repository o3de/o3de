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

#include <Config_wwise.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/FileFunc/FileFunc.h>

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
        auto outcome = AzFramework::FileFunc::ReadJsonFile(fileIoPath);
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

        auto outcome = AzFramework::FileFunc::WriteJsonFile(jsonDoc, filePath);
        if (!outcome)
        {
            AZ_Printf(WWISE_CONFIG_WINDOW, "ERROR: %s\n", outcome.GetError().c_str());
            return false;
        }

        AZ_Printf(WWISE_CONFIG_WINDOW, "Saved '%s' successfully.\n", filePath.c_str());
        return true;
    }

} // namespace Audio::Wwise
