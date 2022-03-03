/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/schema.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AWSCoreInternalBus.h>
#include <Configuration/AWSCoreConfiguration.h>
#include <ResourceMapping/AWSResourceMappingConstants.h>
#include <ResourceMapping/AWSResourceMappingManager.h>
#include <ResourceMapping/AWSResourceMappingUtils.h>

namespace AWSCore
{
    AWSResourceMappingManager::AWSResourceMappingManager()
        : m_status(Status::NotLoaded)
        , m_defaultAccountId("")
        , m_defaultRegion("")
    {
    }

    void AWSResourceMappingManager::ActivateManager()
    {
        ReloadConfigFile();
        AWSResourceMappingRequestBus::Handler::BusConnect();
    }

    void AWSResourceMappingManager::DeactivateManager()
    {
        AWSResourceMappingRequestBus::Handler::BusDisconnect();
        ResetResourceMappingsData();
    }

    AZStd::string AWSResourceMappingManager::GetResourceAttributeErrorMessageByStatus(const AZStd::string& resourceKeyName) const
    {
        switch (m_status)
        {
        case Status::NotLoaded:
            return AZStd::string::format(ResourceMappingFileNotLoadedErrorMessage, AWSCoreConfiguration::AWSCoreConfigurationFileName);
        case Status::Ready:
            return AZStd::string::format(ResourceMappingKeyNotFoundErrorMessage, resourceKeyName.c_str());
        case Status::Error:
            return ResourceMappingFileLoadFailureErrorMessage;
        default:
            return ManagerUnexpectedStatusErrorMessage;
        }
    }

    AZStd::string AWSResourceMappingManager::GetDefaultAccountId() const
    {
        if (m_defaultAccountId.empty())
        {
            AZ_Warning(AWSResourceMappingManagerName, false,
                GetResourceAttributeErrorMessageByStatus(ResourceMappingAccountIdKeyName).c_str());
        }
        return m_defaultAccountId;
    }

    AZStd::string AWSResourceMappingManager::GetDefaultRegion() const
    {
        if (m_defaultRegion.empty())
        {
            AZ_Warning(AWSResourceMappingManagerName, false,
                GetResourceAttributeErrorMessageByStatus(ResourceMappingRegionKeyName).c_str());
        }
        return m_defaultRegion;
    }

    AZStd::string AWSResourceMappingManager::GetResourceAccountId(const AZStd::string& resourceKeyName) const
    {
        return GetResourceAttribute([this](auto& attributes) {
            if (!attributes.resourceAccountId.empty())
            {
                return attributes.resourceAccountId;
            }
            return m_defaultAccountId;
        }, resourceKeyName);
    }

    AZStd::string AWSResourceMappingManager::GetResourceNameId(const AZStd::string& resourceKeyName) const
    {
        return GetResourceAttribute([](auto& attributes) {
            return attributes.resourceNameId;
        }, resourceKeyName);
    }

    AZStd::string AWSResourceMappingManager::GetResourceRegion(const AZStd::string& resourceKeyName) const
    {
        return GetResourceAttribute([this](auto& attributes) {
            if (!attributes.resourceRegion.empty())
            {
                return attributes.resourceRegion;
            }
            return m_defaultRegion;
        }, resourceKeyName);
    }

    AZStd::string AWSResourceMappingManager::GetResourceType(const AZStd::string& resourceKeyName) const
    {
        return GetResourceAttribute([](auto& attributes) {
            return attributes.resourceType;
        }, resourceKeyName);
    }

    AZStd::string AWSResourceMappingManager::GetServiceUrlByServiceName(const AZStd::string& serviceName) const
    {
        return GetServiceUrlByRESTApiIdAndStage(
            AZStd::string::format("%s%s", serviceName.c_str(), AWSFeatureGemRESTApiIdKeyNameSuffix),
            AZStd::string::format("%s%s", serviceName.c_str(), AWSFeatureGemRESTApiStageKeyNameSuffix));
    }

    AZStd::string AWSResourceMappingManager::GetServiceUrlByRESTApiIdAndStage(
        const AZStd::string& restApiIdKeyName, const AZStd::string& restApiStageKeyName) const
    {
        AZStd::string serviceRESTApiId = GetResourceNameId(restApiIdKeyName);
        AZStd::string serviceRESTApiStage = GetResourceNameId(restApiStageKeyName);

        AZStd::string serviceRegion = GetResourceRegion(restApiIdKeyName);
        if (serviceRegion != GetResourceRegion(restApiStageKeyName))
        {
            AZ_Warning(AWSResourceMappingManagerName, false, ResourceMappingRESTApiIdAndStageInconsistentErrorMessage,
                restApiIdKeyName.c_str(), restApiStageKeyName.c_str());
            return "";
        }

        AZStd::string serviceRESTApiUrl = AWSResourceMappingUtils::FormatRESTApiUrl(serviceRESTApiId, serviceRegion, serviceRESTApiStage);
        AZ_Warning(AWSResourceMappingManagerName, !serviceRESTApiUrl.empty(), ResourceMappingRESTApiInvalidServiceUrlErrorMessage,
            serviceRESTApiId.c_str(), serviceRegion.c_str(), serviceRESTApiStage.c_str());
        return serviceRESTApiUrl;
    }

    AZStd::string AWSResourceMappingManager::GetResourceAttribute(
        AZStd::function<AZStd::string(const AWSResourceMappingAttributes&)> getAttributeFunction, const AZStd::string& resourceKeyName) const
    {
        auto iter = m_resourceMappings.find(resourceKeyName);
        if (iter != m_resourceMappings.end())
        {
            return getAttributeFunction(iter->second);
        }

        AZ_Warning(AWSResourceMappingManagerName, false, GetResourceAttributeErrorMessageByStatus(resourceKeyName).c_str());
        return "";
    }

    AWSResourceMappingManager::Status AWSResourceMappingManager::GetStatus() const
    {
        return m_status;
    }

    void AWSResourceMappingManager::ParseJsonDocument(const rapidjson::Document& jsonDocument)
    {
        m_defaultAccountId = jsonDocument.FindMember(ResourceMappingAccountIdKeyName)->value.GetString();
        m_defaultRegion = jsonDocument.FindMember(ResourceMappingRegionKeyName)->value.GetString();

        auto resourceMappings = jsonDocument.FindMember(ResourceMappingResourcesKeyName)->value.GetObject();
        for (auto mappingIter = resourceMappings.MemberBegin(); mappingIter != resourceMappings.MemberEnd(); ++mappingIter)
        {
            auto mappingValue = mappingIter->value.GetObject();
            if (mappingValue.MemberCount() != 0)
            {
                AWSResourceMappingAttributes attributes = ParseJsonObjectIntoResourceMappingAttributes(mappingValue);
                auto mappingKey = mappingIter->name.GetString();
                m_resourceMappings.emplace(AZStd::make_pair(mappingKey, attributes));
            }
        }
    }

    AWSResourceMappingManager::AWSResourceMappingAttributes AWSResourceMappingManager::ParseJsonObjectIntoResourceMappingAttributes(
        const JsonObject& jsonObject)
    {
        AWSResourceMappingAttributes attributes;
        if (jsonObject.HasMember(ResourceMappingAccountIdKeyName))
        {
            attributes.resourceAccountId = jsonObject.FindMember(ResourceMappingAccountIdKeyName)->value.GetString();
        }
        attributes.resourceNameId = jsonObject.FindMember(ResourceMappingNameIdKeyName)->value.GetString();
        if (jsonObject.HasMember(ResourceMappingRegionKeyName))
        {
            attributes.resourceRegion = jsonObject.FindMember(ResourceMappingRegionKeyName)->value.GetString();
        }
        attributes.resourceType = jsonObject.FindMember(ResourceMappingTypeKeyName)->value.GetString();
        return attributes;
    }

    void AWSResourceMappingManager::ReloadConfigFile(bool reloadConfigFileName)
    {
        ResetResourceMappingsData();

        if (reloadConfigFileName)
        {
            AWSCoreInternalRequestBus::Broadcast(&AWSCoreInternalRequests::ReloadConfiguration);
        }

        AZStd::string configJsonPath = "";
        AWSCoreInternalRequestBus::BroadcastResult(configJsonPath, &AWSCoreInternalRequests::GetResourceMappingConfigFilePath);
        if (configJsonPath.empty())
        {
            AZ_Warning(AWSResourceMappingManagerName, false, ResourceMappingFileInvalidPathErrorMessage);
            return;
        }

        AzFramework::StringFunc::Path::Normalize(configJsonPath);
        auto readJsonOutcome = AZ::JsonSerializationUtils::ReadJsonFile(configJsonPath);
        if (readJsonOutcome.IsSuccess())
        {
            auto jsonDocument = readJsonOutcome.TakeValue();
            if (!ValidateJsonDocumentAgainstSchema(jsonDocument))
            {
                // Failed to satisfy the validation against json schema
                m_status = Status::Error;
                return;
            }
            ParseJsonDocument(jsonDocument);
        }
        else
        {
            m_status = Status::Error;
            AZ_Warning(AWSResourceMappingManagerName, false,
                ResourceMappingFileInvalidJsonFormatErrorMessage, readJsonOutcome.GetError().c_str());
            return;
        }

        // Resource mapping config file gets loaded successfully
        m_status = Status::Ready;
    }

    void AWSResourceMappingManager::ResetResourceMappingsData()
    {
        m_status = Status::NotLoaded;
        m_defaultAccountId = "";
        m_defaultRegion = "";
        m_resourceMappings.clear();
    }

    bool AWSResourceMappingManager::ValidateJsonDocumentAgainstSchema(const rapidjson::Document& jsonDocument)
    {
        AZ::IO::Path executablePath = AZ::IO::PathView(AZ::Utils::GetExecutableDirectory());
        AZ::IO::Path jsonSchemaPath = (executablePath / ResourceMapppingJsonSchemaFilePath).LexicallyNormal();
        AZ::Outcome<rapidjson::Document, AZStd::string> readJsonOutcome = AZ::JsonSerializationUtils::ReadJsonFile(jsonSchemaPath.c_str());
        if (!readJsonOutcome.IsSuccess() || readJsonOutcome.TakeValue().ObjectEmpty())
        {
            AZ_Error(AWSResourceMappingManagerName, false, ResourceMappingFileInvalidSchemaErrorMessage);
            return false;
        }

        auto jsonSchema = rapidjson::SchemaDocument(readJsonOutcome.TakeValue());
        rapidjson::SchemaValidator validator(jsonSchema);

        if (!jsonDocument.Accept(validator))
        {
            rapidjson::StringBuffer error;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(error);
            validator.GetError().Accept(writer);
            AZ_Warning(AWSResourceMappingManagerName, false, ResourceMappingFileInvalidContentErrorMessage, error.GetString());

            return false;
        }
        return true;
    }
} // namespace AWSCore
