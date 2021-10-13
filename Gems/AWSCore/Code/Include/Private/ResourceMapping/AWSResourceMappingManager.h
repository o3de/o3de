/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        static constexpr const char AWSResourceMappingManagerName[] = "AWSResourceMappingManager";
        static constexpr const char ManagerUnexpectedStatusErrorMessage[] =
            "AWSResourceMappingManager is in unexpected status.";
        static constexpr const char ResourceMappingFileInvalidPathErrorMessage[] =
            "Failed to get resource mapping config file path.";
        static constexpr const char ResourceMappingKeyNotFoundErrorMessage[] =
            "Failed to find resource mapping key: %s";
        static constexpr const char ResourceMappingFileNotLoadedErrorMessage[] =
            "Resource mapping config file is not loaded, please confirm %s is setup correctly.";
        static constexpr const char ResourceMappingFileLoadFailureErrorMessage[] =
            "Resource mapping config file failed to load, please confirm file is present and in correct format.";
        static constexpr const char ResourceMappingRESTApiIdAndStageInconsistentErrorMessage[] =
            "Resource mapping %s and %s have inconsistent region value, return empty service url.";
        static constexpr const char ResourceMappingRESTApiInvalidServiceUrlErrorMessage[] =
            "Unable to format REST Api url with RESTApiId=%s, RESTApiRegion=%s, RESTApiStage=%s, return empty service url.";
        static constexpr const char ResourceMappingFileInvalidJsonFormatErrorMessage[] =
            "Failed to read resource mapping config file: %s";
        static constexpr const char ResourceMappingFileInvalidSchemaErrorMessage[] =
            "Failed to load resource mapping config file json schema.";
        static constexpr const char ResourceMappingFileInvalidContentErrorMessage[] =
            "Failed to parse resource mapping config file: %s";

        enum class Status : AZ::u8
        {
            NotLoaded = 0,
            Ready = 1,
            Error = 2
        };

        AWSResourceMappingManager();
        ~AWSResourceMappingManager() override = default;

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

        Status GetStatus() const;

    private:
        // Get resource attribute error message based on the status
        AZStd::string GetResourceAttributeErrorMessageByStatus(const AZStd::string& resourceKeyName) const;

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

        Status m_status;
        // Resource mapping related data
        AZStd::string m_defaultAccountId;
        AZStd::string m_defaultRegion;
        AZStd::unordered_map<AZStd::string, AWSResourceMappingAttributes> m_resourceMappings;
    };
} // namespace AWSCore
