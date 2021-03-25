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

#include <AzCore/JSON/document.h>
#include <AzCore/std/containers/unordered_map.h>

#include <ResourceMapping/AWSResourceMappingBus.h>

namespace AWSCore
{
    //! AWSResourceMappingManager is the manager holding AWS resource mapping data.
    //! The manager provides mapping key based AWS resource attributes lookups,
    //! and API to reload AWS resource mapping data synchronously.
    //! The manager doesn't support to modify or update AWS resource mapping data.
    class AWSResourceMappingManager
        : AWSResourceMappingRequestBus::Handler
    {
        //! AWSResourceMappingAttributes is a private data structure for holding
        //! AWS resource mapping attributes.
        //! Which includes AccountId, NameId, Region and Type
        struct AWSResourceMappingAttributes
        {
            AWSResourceMappingAttributes()
                : resourceAccountId("")
                , resourceNameId("")
                , resourceRegion("")
                , resourceType("")
            {}

            AZStd::string resourceAccountId;
            AZStd::string resourceNameId;
            AZStd::string resourceRegion;
            AZStd::string resourceType;
        };

    public:
        AWSResourceMappingManager();
        ~AWSResourceMappingManager() = default;

        void ActivateManager();
        void DeactivateManager();

        // AWSResourceMappingRequestBus interface implementation
        AZStd::string GetDefaultAccountId() const override;
        AZStd::string GetDefaultRegion() const override;
        AZStd::string GetResourceAccountId(const AZStd::string& resourceKeyName) const override;
        AZStd::string GetResourceNameId(const AZStd::string& resourceKeyName) const override;
        AZStd::string GetResourceRegion(const AZStd::string& resourceKeyName) const override;
        AZStd::string GetResourceType(const AZStd::string& resourceKeyName) const override;
        AZStd::string GetServiceUrlByServiceName(const AZStd::string& serviceName) const override;
        AZStd::string GetServiceUrlByRESTApiIdAndStage(
            const AZStd::string& restApiIdKeyName, const AZStd::string& restApiStageKeyName) const override;
        void ReloadConfigFile(bool reloadConfigFileName = false) override;

    private:
        // Get resource attribute from resource mappings
        AZStd::string GetResourceAttribute(
            AZStd::function<AZStd::string(const AWSResourceMappingAttributes&)> getAttributeFunction,
            const AZStd::string& resourceKeyName) const;

        // Parse JSON document into manager internal data
        void ParseJsonDocument(const rapidjson::Document& jsonDocument);

        // Parse JSON object into manager internal data structure
        typedef rapidjson::GenericObject<true, rapidjson::Value> JsonObject;
        AWSResourceMappingAttributes ParseJsonObjectIntoResourceMappingAttributes(
            const JsonObject& jsonObject);

        // Reset manager internal data
        void ResetResourceMappingsData();

        // Validate JSON document against schema
        bool ValidateJsonDocumentAgainstSchema(const rapidjson::Document& jsonDocument);

        // Resource mapping related data
        AZStd::string m_defaultAccountId;
        AZStd::string m_defaultRegion;
        AZStd::unordered_map<AZStd::string, AWSResourceMappingAttributes> m_resourceMappings;
    };
} // namespace AWSCore
