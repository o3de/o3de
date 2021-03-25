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

#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/schema.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AWSCoreInternalBus.h>
#include <ResourceMapping/AWSResourceMappingConstants.h>
#include <ResourceMapping/AWSResourceMappingManager.h>
#include <ResourceMapping/AWSResourceMappingUtils.h>

namespace AWSCore
{
    AWSResourceMappingManager::AWSResourceMappingManager()
        : m_defaultAccountId("")
        , m_defaultRegion("")
        , m_resourceMappings()
    {
    }

    void AWSResourceMappingManager::ActivateManager()
    {
        ReloadConfigFile(true);
        AWSResourceMappingRequestBus::Handler::BusConnect();
    }

    void AWSResourceMappingManager::DeactivateManager()
    {
        AWSResourceMappingRequestBus::Handler::BusDisconnect();
        ResetResourceMappingsData();
    }

    AZStd::string AWSResourceMappingManager::GetDefaultAccountId() const
    {
        if (m_defaultAccountId.empty())
        {
            AZ_Warning("AWSResourceMappingManager", false, "Account Id should not be empty, please make sure config file is valid.");
        }
        return m_defaultAccountId;
    }

    AZStd::string AWSResourceMappingManager::GetDefaultRegion() const
    {
        if (m_defaultRegion.empty())
        {
            AZ_Warning("AWSResourceMappingManager", false, "Region should not be empty, please make sure config file is valid.");
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
            AZStd::string::format("%s%s", serviceName.c_str(), AWS_FEATURE_GEM_RESTAPI_ID_KEYNAME_SUFFIX),
            AZStd::string::format("%s%s", serviceName.c_str(), AWS_FEATURE_GEM_RESTAPI_STAGE_KEYNAME_SUFFIX));
    }

    AZStd::string AWSResourceMappingManager::GetServiceUrlByRESTApiIdAndStage(
        const AZStd::string& restApiIdKeyName, const AZStd::string& restApiStageKeyName) const
    {
        AZStd::string serviceRESTApiId = GetResourceNameId(restApiIdKeyName);
        AZStd::string serviceRESTApiStage = GetResourceNameId(restApiStageKeyName);

        AZStd::string serviceRegion = GetResourceRegion(restApiIdKeyName);
        if (serviceRegion != GetResourceRegion(restApiStageKeyName))
        {
            AZ_Warning(
                "AWSResourceMappingManager", false, "%s and %s have inconsistent region value, return empty service url.",
                restApiIdKeyName.c_str(), restApiStageKeyName.c_str());
            return "";
        }

        AZStd::string serviceRESTApiUrl = AWSResourceMappingUtils::FormatRESTApiUrl(serviceRESTApiId, serviceRegion, serviceRESTApiStage);
        AZ_Warning(
            "AWSResourceMappingManager", !serviceRESTApiUrl.empty(),
            "Unable to format REST Api url with RESTApiId=%s, RESTApiRegion=%s, RESTApiStage=%s, return empty service url.",
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

        AZ_Warning("AWSResourceMappingManager", false, "Failed to find resource mapping key: %s.", resourceKeyName.c_str());
        return "";
    }

    void AWSResourceMappingManager::ParseJsonDocument(const rapidjson::Document& jsonDocument)
    {
        m_defaultAccountId = jsonDocument.FindMember(RESOURCE_MAPPING_ACCOUNTID_KEYNAME)->value.GetString();
        m_defaultRegion = jsonDocument.FindMember(RESOURCE_MAPPING_REGION_KEYNAME)->value.GetString();

        auto resourceMappings = jsonDocument.FindMember(RESOURCE_MAPPING_RESOURCES_KEYNAME)->value.GetObject();
        for (auto mappingIter = resourceMappings.MemberBegin(); mappingIter != resourceMappings.MemberEnd(); mappingIter++)
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
        if (jsonObject.HasMember(RESOURCE_MAPPING_ACCOUNTID_KEYNAME))
        {
            attributes.resourceAccountId = jsonObject.FindMember(RESOURCE_MAPPING_ACCOUNTID_KEYNAME)->value.GetString();
        }
        attributes.resourceNameId = jsonObject.FindMember(RESOURCE_MAPPING_NAMEID_KEYNAME)->value.GetString();
        if (jsonObject.HasMember(RESOURCE_MAPPING_REGION_KEYNAME))
        {
            attributes.resourceRegion = jsonObject.FindMember(RESOURCE_MAPPING_REGION_KEYNAME)->value.GetString();
        }
        attributes.resourceType = jsonObject.FindMember(RESOURCE_MAPPING_TYPE_KEYNAME)->value.GetString();
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
            AZ_Warning("AWSResourceMappingManager", false, "Failed to get resource mapping config file path.");
            return;
        }

        AzFramework::StringFunc::Path::Normalize(configJsonPath);
        AZ::IO::Path configJsonFileIOPath(configJsonPath);
        auto readJsonOutcome = AzFramework::FileFunc::ReadJsonFile(configJsonFileIOPath);
        if (readJsonOutcome.IsSuccess())
        {
            auto jsonDocument = readJsonOutcome.TakeValue();
            if (!ValidateJsonDocumentAgainstSchema(jsonDocument))
            {
                // Failed to satisfy the validation against json schema
                return;
            }
            ParseJsonDocument(jsonDocument);
        }
        else
        {
            AZ_Warning(
                "AWSResourceMappingManager", false, "Failed to get read resource mapping config file: %s\n Error: %s",
                configJsonPath.c_str(), readJsonOutcome.GetError().c_str());
        }
    }

    void AWSResourceMappingManager::ResetResourceMappingsData()
    {
        m_defaultAccountId = "";
        m_defaultRegion = "";
        m_resourceMappings.clear();
    }

    bool AWSResourceMappingManager::ValidateJsonDocumentAgainstSchema(const rapidjson::Document& jsonDocument)
    {
        rapidjson::Document jsonSchemaDocument;
        if (jsonSchemaDocument.Parse(RESOURCE_MAPPING_JSON_SCHEMA).HasParseError())
        {
            AZ_Error("AWSResourceMappingManager", false, "Invalid resource mapping json schema.");
            return false;
        }

        auto jsonSchema = rapidjson::SchemaDocument(jsonSchemaDocument);
        rapidjson::SchemaValidator validator(jsonSchema);

        if (!jsonDocument.Accept(validator))
        {
            rapidjson::StringBuffer error;
            validator.GetInvalidSchemaPointer().StringifyUriFragment(error);
            AZ_Warning("AWSResourceMappingManager", false, "Failed to load config file, invalid schema: %s.", error.GetString());
            AZ_Warning("AWSResourceMappingManager", false, "Failed to load config file, invalid keyword: %s.", validator.GetInvalidSchemaKeyword());
            error.Clear();
            validator.GetInvalidDocumentPointer().StringifyUriFragment(error);
            AZ_Warning("AWSResourceMappingManager", false, "Failed to load config file, invalid document: %s.", error.GetString());
            return false;
        }
        return true;
    }
} // namespace AWSCore
