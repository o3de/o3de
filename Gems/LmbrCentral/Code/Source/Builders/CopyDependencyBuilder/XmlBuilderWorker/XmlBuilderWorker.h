/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <AzCore/XML/rapidxml.h>
#include <AzFramework/Asset/XmlSchemaAsset.h>
#include <AzCore/Dependency/Version.h>

#include <Builders/CopyDependencyBuilder/CopyDependencyBuilderWorker.h>

namespace CopyDependencyBuilder
{
    const char SchemaNamePattern[] = "*.xmlschema";
    const char VersionContraintRegexStr[] = "(?:(~>|~=|[>=<]{1,2}) *([0-9]+(?:\\.[0-9]+)*))";
    const char VersionRegexStr[] = "([0-9]+)(?:\\.(.*)){0,1}";
    const size_t MaxVersionPartsCount = 4;

    class XmlBuilderWorker
        : public CopyDependencyBuilderWorker
    {
    public:
        AZ_RTTI(XmlBuilderWorker, "{7FC5D0F1-25E3-4CD2-8FB9-81CB29D940E3}");

        XmlBuilderWorker();

        void RegisterBuilderWorker() override;
        void UnregisterBuilderWorker() override;

        AZ::Outcome<AZStd::vector<AssetBuilderSDK::SourceFileDependency>, AZStd::string> GetSourceDependencies(
            const AssetBuilderSDK::CreateJobsRequest& request) const override;

        bool ParseProductDependencies(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies) override;

        void AddSchemaFileDirectory(const AZStd::string& schemaFileLocation);
        void SetPrintDebug(bool setDebug) { m_printDebug = setDebug; }
    private:
        // Traverse the entire source file to create a list of all the XML nodes and mappings from node names to the corresponding nodes
        void TraverseSourceFile(
            const AZ::rapidxml::xml_node<char>* currentNode,
            AZStd::unordered_map<AZStd::string, AZStd::vector<const AZ::rapidxml::xml_node<char>*>>& xmlNodeMappings,
            AZStd::vector<const AZ::rapidxml::xml_node<char>*>& xmlNodeList) const;

        enum class SchemaMatchResult
        {
            MatchFound,
            NoMatchFound,
            Error
        };

        SchemaMatchResult MatchExistingSchema(
            const AZStd::string& sourceFilePath,
            AZStd::vector<AZStd::string>& sourceDependencyPaths,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies,
            const AZStd::string& watchFolderPath) const;

        SchemaMatchResult MatchLastUsedSchema(
            const AZStd::string& sourceFilePath,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies,
            const AZStd::string& watchFolderPath) const;

        SchemaMatchResult ParseXmlFile(
            const AZStd::string& schemaFilePath,
            const AZStd::string& sourceFilePath,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies,
            const AZStd::string& watchFolderPath) const;

        AZ::Outcome <void, bool> SearchForMatchingRule(
            const AZStd::string& sourceFilePath, 
            const AZStd::string& schemaFilePath,
            const AZ::Version<MaxVersionPartsCount>& version,
            const AZStd::vector<AzFramework::MatchingRule>& matchingRules,
            const AZStd::string& watchFolderPath) const;

        bool SearchForDependencySearchRule(
            AZ::rapidxml::xml_node<char>* xmlFileRootNode, 
            const AZStd::string& schemaFilePath,
            const AZ::Version<MaxVersionPartsCount>& version,
            const AZStd::vector<AzFramework::DependencySearchRule>& matchingRules,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies,
            const AZStd::string& sourceAssetFolder,
            const AZStd::string& watchFolderPath) const;

        AZStd::list<AZStd::string> m_schemaFileDirectories;
        bool m_printDebug{ false };
    };
}
