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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <native/AssetManager/assetProcessorManager.h>

class QString;

namespace AssetProcessor
{
    class PlatformConfiguration;
    class AssetDatabaseConnection;

    const char ExcludedDependenciesSymbol = ':';

    /// Handles resolving and saving product path dependencies
    class PathDependencyManager
    {
    public:
        // The two Ids needed for a ProductDependency entry, and platform. Used for saving ProductDependencies that are pending resolution
        struct DependencyProductIdInfo
        {
            AZ::s64 m_productId{};
            AZ::s64 m_productDependencyId{};
            AZStd::string m_platform;
        };
        using DependencyProductMap = AZStd::unordered_map<AZStd::string, AZStd::vector<DependencyProductIdInfo>>;

        PathDependencyManager(AZStd::shared_ptr<AssetDatabaseConnection> stateData, PlatformConfiguration* platformConfig);

        /// This function is responsible for looking up existing, unresolved dependencies that the current asset satisfies.
        /// These can be dependencies on either the source asset or one of the product assets
        void RetryDeferredDependencies(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry);

        /// This function is responsible for taking the path dependencies output by the current asset and trying to resolve them to AssetIds
        /// This does not look for dependencies that the current asset satisfies.
        void ResolveDependencies(AssetBuilderSDK::ProductPathDependencySet& pathDeps, AZStd::vector<AssetBuilderSDK::ProductDependency>& resolvedDeps, const AZStd::string& platform, const AZStd::string& productName);

        /// Saves a product's unresolved dependencies to the database
        void SaveUnresolvedDependenciesToDatabase(AssetBuilderSDK::ProductPathDependencySet& unresolvedDependencies, const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry, const AZStd::string& platform);

        using DependencyResolvedCallback = AZStd::function<void(const AZ::Data::AssetId&, const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry&)>;

        void SetDependencyResolvedCallback(const DependencyResolvedCallback& callback);

    private:

        struct MapSet
        {
            DependencyProductMap m_sourcePathDependencyIds;
            DependencyProductMap m_productPathDependencyIds;
            DependencyProductMap m_wildcardSourcePathDependencyIds;
            DependencyProductMap m_wildcardProductPathDependencyIds;
        };

        MapSet PopulateExclusionMaps() const;
        void NotifyResolvedDependencies(const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencyContainer) const;
        void SaveResolvedDependencies(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry, const MapSet& exclusionMaps, const AZStd::string& sourceNameWithScanFolder, const AZStd::vector<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>& dependencyEntries, AZStd::string_view matchedPath, bool isSourceDependency, const AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& matchedProducts, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencyContainer) const;
        static DependencyProductMap& SelectMap(MapSet& mapSet, bool wildcard, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType type);

        /// Returns false if a path contains wildcards, true otherwise
        static bool IsExactDependency(AZStd::string_view path);

        /// Removes /platform/project/ from the start of a product path
        static AZStd::string StripPlatformAndProject(AZStd::string_view relativeProductPath);

        /// Prefixes the scanFolderId to the relativePath
        AZStd::string ToScanFolderPrefixedPath(int scanFolderId, const char* relativePath) const;

        /// Takes a path and breaks it into a database-prefixed relative path and scanFolder path
        /// This function can accept an absolute source path, an un-prefixed relative path, and a prefixed relative path
        /// The file returned will be the first one matched based on scanfolder priority
        bool ProcessInputPathToDatabasePathAndScanFolder(const char* dependencyPathSearch, QString& databaseName, QString& scanFolder) const;

        /// Gets any matched dependency exclusions
        /// @param sourceEntry source database entry corresponds to the newly finished product
        /// @param productEntry product database entry corresponds to the newly finished product
        /// @param excludedDependencies dependencies that should be ignored even if their file paths match any existing wildcard pattern
        /// @param dependencyType type of the dependencies we are handling
        /// @param exclusionMaps MapSet containing all the path dependency exclusions
        void GetMatchedExclusions(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry, const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry,
            AZStd::vector<AZStd::pair<DependencyProductIdInfo, bool>>& excludedDependencies, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType dependencyType,
            const MapSet& exclusionMaps) const;

        AZStd::shared_ptr<AssetDatabaseConnection> m_stateData;
        PlatformConfiguration* m_platformConfig{};
        DependencyResolvedCallback m_dependencyResolvedCallback{};
    };
} // namespace AssetProcessor
