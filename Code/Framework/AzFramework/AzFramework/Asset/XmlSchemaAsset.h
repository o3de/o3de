/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AzFramework
{
    class VersionSearchRule
    {
    public:
        AZ_TYPE_INFO(VersionSearchRule, "{0AC0D453-7F3E-4BA2-9D28-B39052361B0F}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string GetRootNodeAttributeName() const;

    private:
        AZStd::string m_rootNodeAttributeName;
    };

    class MatchingRule
    {
    public:
        AZ_TYPE_INFO(MatchingRule, "{0052598D-594B-44C7-8B0F-F268FBBA6E4F}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string GetFilePathPattern() const;
        AZStd::string GetExcludedFilePathPattern() const;
        AZStd::vector<AZStd::string> GetVersionConstraints() const;
        bool Valid() const;

    private:
        AZStd::string m_filePathPattern;
        AZStd::string m_excludedFilePathPattern;
        AZStd::vector<AZStd::string> m_versionConstraints;
    };

    class XmlSchemaAttribute
    {
    public:
        AZ_TYPE_INFO(XmlSchemaAttribute, "{EE322552-0565-4D54-B022-9A9F134BF447}");

        enum class AttributeType : AZ::u32
        {
            RelativePath,
            Asset
        };

        enum class AttributePathDependencyType : AZ::u32
        {
            SourceFile,
            ProductFile
        };

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string GetName() const;
        AZStd::string GetExpectedExtension() const;
        AZStd::string GetMatchPattern() const;
        AZStd::string GetFindPattern() const;
        AZStd::string GetReplacePattern() const;
        AttributeType GetType() const;
        AttributePathDependencyType GetPathDependencyType() const;
        bool IsRelativeToSourceAssetFolder() const;
        bool IsOptional() const;
        bool CacheRelativePath() const;

    private:
        AZ::Crc32 GetVisibilityProperty() const;

        AZStd::string m_name;
        AZStd::string m_expectedExtension;
        AZStd::string m_matchPattern;
        AZStd::string m_findPattern;
        AZStd::string m_replacePattern;
        AttributeType m_type;
        AttributePathDependencyType m_pathDependencyType;
        bool m_relativeToSourceAssetFolder;
        bool m_optional;
        bool m_cacheRelativePath;
    };

    class XmlSchemaElement
    {
    public:
        AZ_TYPE_INFO(XmlSchemaElement, "{DB03558D-9533-4426-B50F-3DB16F7AA686}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::string GetName() const;
        AZStd::vector<XmlSchemaElement> GetChildElements() const;
        AZStd::vector<XmlSchemaAttribute> GetAttributes() const;
        bool IsOptional() const;
        bool Valid() const;
    private:
        AZStd::vector<XmlSchemaElement> m_childElements;
        AZStd::vector<XmlSchemaAttribute> m_attributes;
        AZStd::string m_name;
        bool m_optional;
    };

    class SearchRuleDefinition
    {
    public:
        AZ_TYPE_INFO(SearchRuleDefinition, "{DA29525E-3032-4919-97B2-3FECCDFF06A6}");

        static void Reflect(AZ::ReflectContext* context);

        XmlSchemaElement GetSearchRuleStructure() const;
        bool IsRelativeToXmlRoot() const;

    private:
        XmlSchemaElement m_searchRuleStructure;
        bool m_relativeToXmlRoot;
    };

    class DependencySearchRule
    {
    public:
        AZ_TYPE_INFO(DependencySearchRule, "{5A6EB7DE-A2EC-47F3-A2AB-91F0560C2E66}");

        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<SearchRuleDefinition> GetSearchRules() const;
        AZStd::vector<AZStd::string> GetVersionConstraints() const;
        bool Valid() const;

    private:
        AZStd::vector<SearchRuleDefinition> m_searchRuleDefinitions;
        AZStd::vector<AZStd::string> m_versionConstraints;
    };

    class XmlSchemaAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(XmlSchemaAsset, "{2DF35909-AF12-40A8-BED2-A033478D864D}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(XmlSchemaAsset, AZ::SystemAllocator);

        static const char* GetDisplayName() { return "XML Schema"; }
        static const char* GetGroup() { return "XmlSchema"; }
        static const char* GetFileFilter() { return "xmlschema"; }

        static void Reflect(AZ::ReflectContext* context);

        VersionSearchRule GetVersionSearchRule() const;
        AZStd::vector<MatchingRule> GetMatchingRules() const;
        AZStd::vector<DependencySearchRule> GetDependencySearchRules() const;
        bool UseAZSerialization() const { return m_useAZSerialization; }

    private:
        VersionSearchRule m_versionSearchRule;
        AZStd::vector<MatchingRule> m_matchingRules;
        AZStd::vector<DependencySearchRule> m_dependencySearchRules;
        bool m_useAZSerialization = false; // The XML file is in AZ serialization format, dependencies can be extracted with that.
    };
}
