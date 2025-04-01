/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PathDependencyManager.h"
#include <AzCore/std/string/wildcard.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Jobs/Algorithms.h>
#include <utilities/PlatformConfiguration.h>
#include <utilities/assetUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <utilities/StatsCapture.h>

namespace AssetProcessor
{
    void SanitizeForDatabase(AZStd::string& str)
    {
        AZStd::to_lower(str.begin(), str.end());

        // Not calling normalize because wildcards should be preserved.
        if (AZ::StringFunc::Contains(str, AZ_WRONG_DATABASE_SEPARATOR, true))
        {
            AZStd::replace(str.begin(), str.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);
            AzFramework::StringFunc::Replace(str, AZ_DOUBLE_CORRECT_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR_STRING);
        }
    }

    PathDependencyManager::PathDependencyManager(AZStd::shared_ptr<AssetDatabaseConnection> stateData, PlatformConfiguration* platformConfig)
        : m_stateData(stateData), m_platformConfig(platformConfig)
    {

    }

    void PathDependencyManager::QueueSourceForDependencyResolution(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
    {
        m_queuedForResolve.push_back(sourceEntry);
    }

    void PathDependencyManager::ProcessQueuedDependencyResolves()
    {
        if (m_queuedForResolve.empty())
        {
            return;
        }

        auto queuedForResolve = m_queuedForResolve;
        m_queuedForResolve.clear();

        // Grab every product from the database and map to Source PK -> [products]
        AZStd::unordered_map<AZ::s64, AZStd::vector<AzToolsFramework::AssetDatabase::ProductDatabaseEntry>> productMap;
        m_stateData->QueryCombined([&productMap](const AzToolsFramework::AssetDatabase::CombinedDatabaseEntry& entry)
        {
            productMap[entry.m_sourcePK].push_back(entry);
            return true;
        });

        // Build up a list of all the paths we need to search for: products + 2 variations of the source path
        AZStd::vector<SearchEntry> searches;

        for (const auto& entry : queuedForResolve)
        {
            // Search for each product
            for (const auto& productEntry : productMap[entry.m_sourceID])
            {
                const AZStd::string& productName = productEntry.m_productName;

                // strip path of the <platform>/
                AZStd::string_view result = AssetUtilities::StripAssetPlatformNoCopy(productName);
                searches.emplace_back(result, false, &entry, &productEntry);
            }

            // Search for the source path
            AZStd::string sourceNameWithScanFolder =
                ToScanFolderPrefixedPath(aznumeric_cast<int>(entry.m_scanFolderPK), entry.m_sourceName.c_str());
            AZStd::string sanitizedSourceName = entry.m_sourceName;

            SanitizeForDatabase(sourceNameWithScanFolder);
            SanitizeForDatabase(sanitizedSourceName);

            searches.emplace_back(sourceNameWithScanFolder, true, &entry, nullptr);
            searches.emplace_back(sanitizedSourceName, true, &entry, nullptr);
        }

        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer unresolvedDependencies;
        m_stateData->GetUnresolvedProductDependencies(unresolvedDependencies);

        AZStd::recursive_mutex mapMutex;
        // Map of <Source PK => Map of <Matched SearchEntry => Product Dependency>>
        AZStd::unordered_map<AZ::s64, AZStd::unordered_map<const SearchEntry*, AZStd::unordered_set<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>>> sourceIdToMatchedSearchDependencies;

        // For every search path we created, we're going to see if it matches up against any of the unresolved dependencies
        AZ::parallel_for_each(
            searches.begin(), searches.end(),
            [&sourceIdToMatchedSearchDependencies, &mapMutex, &unresolvedDependencies](const SearchEntry& search)
            {
                AZStd::unordered_set<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry> matches;
                for (const auto& entry: unresolvedDependencies)
                {
                    AZ::IO::PathView searchPath(search.m_path);

                    if(((entry.m_dependencyType == AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::ProductDep_SourceFile && search.m_isSourcePath)
                        || (entry.m_dependencyType == AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::ProductDep_ProductFile && !search.m_isSourcePath))
                        && searchPath.Match(entry.m_unresolvedPath))
                    {
                        matches.insert(entry);
                    }
                }

                if (!matches.empty())
                {
                    AZStd::scoped_lock lock(mapMutex);
                    auto& productDependencyDatabaseEntries = sourceIdToMatchedSearchDependencies[search.m_sourceEntry->m_sourceID][&search];
                    productDependencyDatabaseEntries.insert(matches.begin(), matches.end());
                }
            });

        for (const auto& entry : queuedForResolve)
        {
            RetryDeferredDependencies(entry, sourceIdToMatchedSearchDependencies[entry.m_sourceID], productMap[entry.m_sourceID]);
        }
    }

    void PathDependencyManager::SaveUnresolvedDependenciesToDatabase(AssetBuilderSDK::ProductPathDependencySet& unresolvedDependencies,
        const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry, const AZStd::string& platform)
    {
        using namespace AzToolsFramework::AssetDatabase;

        ProductDependencyDatabaseEntryContainer dependencyContainer;
        for (const auto& unresolvedPathDep : unresolvedDependencies)
        {
            auto dependencyType = unresolvedPathDep.m_dependencyType == AssetBuilderSDK::ProductPathDependencyType::SourceFile ?
                ProductDependencyDatabaseEntry::ProductDep_SourceFile :
                ProductDependencyDatabaseEntry::ProductDep_ProductFile;

            ProductDependencyDatabaseEntry placeholderDependency(
                productEntry.m_productID,
                AZ::Uuid::CreateNull(),
                0,
                AZStd::bitset<64>(),
                platform,
                0,
                // Use a string that will make it easy to route errors back here correctly. An empty string can be a symptom of many
                // other problems. This string says that something went wrong in this function.
                AZStd::string("INVALID_PATH"),
                dependencyType);

            AZStd::string path = AssetUtilities::NormalizeFilePath(unresolvedPathDep.m_dependencyPath.c_str()).toUtf8().constData();
            bool isExactDependency = IsExactDependency(path);

            if (isExactDependency && unresolvedPathDep.m_dependencyType == AssetBuilderSDK::ProductPathDependencyType::SourceFile)
            {
                QString relativePath, scanFolder;

                if (!AzFramework::StringFunc::Path::IsRelative(path.c_str()))
                {
                    if (m_platformConfig->ConvertToRelativePath(QString::fromUtf8(path.c_str()), relativePath, scanFolder))
                    {
                        auto* scanFolderInfo = m_platformConfig->GetScanFolderByPath(scanFolder);

                        path = ToScanFolderPrefixedPath(aznumeric_cast<int>(scanFolderInfo->ScanFolderID()), relativePath.toUtf8().constData());

                    }
                }
            }

            SanitizeForDatabase(path);

            placeholderDependency.m_unresolvedPath = path;
            dependencyContainer.push_back(placeholderDependency);
        }

        if (!m_stateData->UpdateProductDependencies(dependencyContainer))
        {
            AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to save unresolved dependencies to database for product %d (%s)",
                productEntry.m_productID, productEntry.m_productName.c_str());
        }
    }

    void PathDependencyManager::SetDependencyResolvedCallback(const DependencyResolvedCallback& callback)
    {
        m_dependencyResolvedCallback = callback;
    }

    bool PathDependencyManager::IsExactDependency(AZStd::string_view path)
    {
        return path.find('*') == AZStd::string_view::npos;
    }

    void PathDependencyManager::GetMatchedExclusions(
        const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry,
        const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry,
        AZStd::vector<AZStd::pair<DependencyProductIdInfo, bool>>& excludedDependencies,
        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType dependencyType,
        const MapSet& exclusionMaps) const
    {
        bool handleProductDependencies = dependencyType == AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType::ProductDep_ProductFile;
        AZStd::string_view assetName = handleProductDependencies ? productEntry.m_productName : sourceEntry.m_sourceName;

        const DependencyProductMap& excludedPathDependencyIds = handleProductDependencies ? exclusionMaps.m_productPathDependencyIds : exclusionMaps.m_sourcePathDependencyIds;
        const DependencyProductMap& excludedWildcardPathDependencyIds = handleProductDependencies ? exclusionMaps.m_wildcardProductPathDependencyIds : exclusionMaps.m_wildcardSourcePathDependencyIds;

        // strip asset platform from path
        AZStd::string strippedPath = handleProductDependencies ? AssetUtilities::StripAssetPlatform(assetName).toUtf8().constData() : sourceEntry.m_sourceName;
        SanitizeForDatabase(strippedPath);

        auto unresolvedIter = excludedPathDependencyIds.find(ExcludedDependenciesSymbol + strippedPath);
        if (unresolvedIter != excludedPathDependencyIds.end())
        {
            for (const auto& dependencyProductIdInfo : unresolvedIter->second)
            {
                excludedDependencies.emplace_back(dependencyProductIdInfo, true); // true = is exact dependency
            }
        }

        for (const auto& pair : excludedWildcardPathDependencyIds)
        {
            AZStd::string filter = pair.first.substr(1);
            if (wildcard_match(filter, strippedPath))
            {
                for (const auto& dependencyProductIdInfo : pair.second)
                {
                    excludedDependencies.emplace_back(dependencyProductIdInfo, false); // false = wildcard dependency
                }
            }
        }
    }

    PathDependencyManager::DependencyProductMap& PathDependencyManager::SelectMap(MapSet& mapSet, bool wildcard, AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType type)
    {
        const bool isSource = type == AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry::DependencyType::ProductDep_SourceFile;

        if (wildcard)
        {
            if (isSource)
            {
                return mapSet.m_wildcardSourcePathDependencyIds;
            }

            return mapSet.m_wildcardProductPathDependencyIds;
        }

        if (isSource)
        {
            return mapSet.m_sourcePathDependencyIds;
        }

        return mapSet.m_productPathDependencyIds;
    }

    PathDependencyManager::MapSet PathDependencyManager::PopulateExclusionMaps() const
    {
        using namespace AzToolsFramework::AssetDatabase;

        MapSet mapSet;

        m_stateData->QueryProductDependencyExclusions([&mapSet](ProductDependencyDatabaseEntry& unresolvedDep)
        {
            DependencyProductIdInfo idPair;
            idPair.m_productDependencyId = unresolvedDep.m_productDependencyID;
            idPair.m_productId = unresolvedDep.m_productPK;
            idPair.m_platform = unresolvedDep.m_platform;
            AZStd::string path = unresolvedDep.m_unresolvedPath;
            AZStd::to_lower(path.begin(), path.end());
            const bool isExactDependency = IsExactDependency(path);

            auto& map = SelectMap(mapSet, !isExactDependency, unresolvedDep.m_dependencyType);
            map[path].push_back(AZStd::move(idPair));

            return true;
        });

        return mapSet;
    }

    void PathDependencyManager::NotifyResolvedDependencies(const AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencyContainer) const
    {
        if (!m_dependencyResolvedCallback)
        {
            return;
        }

        for (const auto& dependency : dependencyContainer)
        {
            AzToolsFramework::AssetDatabase::ProductDatabaseEntry productEntry;
            if (!m_stateData->GetProductByProductID(dependency.m_productPK, productEntry))
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to get existing product with productId %i from the database", dependency.m_productPK);
            }

            AzToolsFramework::AssetDatabase::SourceDatabaseEntry dependentSource;
            if (!m_stateData->GetSourceByJobID(productEntry.m_jobPK, dependentSource))
            {
                AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to get existing product from job ID of product %i from the database", dependency.m_productPK);
            }

            m_dependencyResolvedCallback(AZ::Data::AssetId(dependentSource.m_sourceGuid, productEntry.m_subID), dependency);
        }
    }

    void PathDependencyManager::SaveResolvedDependencies(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry, const MapSet& exclusionMaps, const AZStd::string& sourceNameWithScanFolder,
        const AZStd::unordered_set<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>& dependencyEntries,
        AZStd::string_view matchedPath, bool isSourceDependency, const AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& matchedProducts,
        AZStd::vector<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>& dependencyContainer) const
    {
        for (const auto& productDependencyDatabaseEntry : dependencyEntries)
        {
            const bool isExactDependency = IsExactDependency(productDependencyDatabaseEntry.m_unresolvedPath);
            AZ::s64 dependencyId = isExactDependency ? productDependencyDatabaseEntry.m_productDependencyID : AzToolsFramework::AssetDatabase::InvalidEntryId;

            if (isSourceDependency && !isExactDependency && matchedPath == sourceNameWithScanFolder)
            {
                // Since we did a search for the source 2 different ways, filter one out
                // Scanfolder-prefixes are only for exact dependencies
                break;
            }

            for (const auto& matchedProduct : matchedProducts)
            {
                // Check if this match is excluded before continuing
                AZStd::vector<AZStd::pair<DependencyProductIdInfo, bool>> exclusions; // bool = is exact dependency
                GetMatchedExclusions(sourceEntry, matchedProduct, exclusions, productDependencyDatabaseEntry.m_dependencyType, exclusionMaps);

                if (!exclusions.empty())
                {
                    bool isExclusionForThisProduct = false;
                    bool isExclusionExact = false;

                    for (const auto& exclusionPair : exclusions)
                    {
                        if (exclusionPair.first.m_productId == productDependencyDatabaseEntry.m_productPK && exclusionPair.first.m_platform == productDependencyDatabaseEntry.m_platform)
                        {
                            isExclusionExact = exclusionPair.second;
                            isExclusionForThisProduct = true;
                            break;
                        }
                    }

                    if (isExclusionForThisProduct)
                    {
                        if (isExactDependency && isExclusionExact)
                        {
                            AZ_Error("PathDependencyManager", false, "Dependency exclusion found for an exact dependency.  It is not valid to both include and exclude a file by the same rule.  File: %s", isSourceDependency ? sourceEntry.m_sourceName.c_str() : matchedProduct.m_productName.c_str());
                        }

                        continue;
                    }
                }

                // We need to make sure this product is for the same platform the dependency is for
                AzToolsFramework::AssetDatabase::JobDatabaseEntry jobEntry;
                if (!m_stateData->GetJobByJobID(matchedProduct.m_jobPK, jobEntry))
                {
                    AZ_Error(AssetProcessor::ConsoleChannel, false, "Failed to get job entry for product %s", matchedProduct.ToString().c_str());
                }

                if (jobEntry.m_platform != productDependencyDatabaseEntry.m_platform)
                {
                    continue;
                }

                // All checks passed, this is a valid dependency we need to save to the db
                AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry entry;

                entry.m_productDependencyID = dependencyId;
                entry.m_productPK = productDependencyDatabaseEntry.m_productPK;
                entry.m_dependencySourceGuid = sourceEntry.m_sourceGuid;
                entry.m_dependencySubID = matchedProduct.m_subID;
                entry.m_platform = productDependencyDatabaseEntry.m_platform;
                entry.m_dependencyFlags = AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::NoLoad);

                dependencyContainer.push_back(AZStd::move(entry));

                // If there's more than 1 product, reset the ID so further products create new db entries
                dependencyId = AzToolsFramework::AssetDatabase::InvalidEntryId;
            }
        }
    }

    void PathDependencyManager::RetryDeferredDependencies(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry,
        const AZStd::unordered_map<const SearchEntry*, AZStd::unordered_set<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>>& matches,
        const AZStd::vector<AzToolsFramework::AssetDatabase::ProductDatabaseEntry>& products)
    {
        MapSet exclusionMaps = PopulateExclusionMaps();

        AZStd::string sourceNameWithScanFolder = ToScanFolderPrefixedPath(aznumeric_cast<int>(sourceEntry.m_scanFolderPK), sourceEntry.m_sourceName.c_str());
        SanitizeForDatabase(sourceNameWithScanFolder);

        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyVector;

        // Go through all the matched dependencies
        for (const auto& pair : matches)
        {
            const SearchEntry* searchEntry = pair.first;
            const bool isSourceDependency = searchEntry->m_isSourcePath;

            AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer matchedProducts;

            // Figure out the list of products to work with, for a source match, use all the products, otherwise just use the matched products
            if (isSourceDependency)
            {
                matchedProducts = products;
            }
            else
            {
                matchedProducts.push_back(*searchEntry->m_productEntry);
            }

            // Go through each dependency we're resolving and create a db entry for each product that resolved it (wildcard/source dependencies will generally create more than 1)
            SaveResolvedDependencies(
                sourceEntry, exclusionMaps, sourceNameWithScanFolder, pair.second, searchEntry->m_path, isSourceDependency, matchedProducts,
                dependencyVector);
        }

        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer existingDependencies;
        if (!m_stateData->GetDirectReverseProductDependenciesBySourceGuidAllPlatforms(sourceEntry.m_sourceGuid, existingDependencies))
        {
            AZ_Error("PathDependencyManager", false, "Failed to query existing product dependencies for source `%s` (%s)",
                sourceEntry.m_sourceName.c_str(),
                sourceEntry.m_sourceGuid.ToString<AZStd::string>().c_str());
        }
        else
        {

            // Remove any existing dependencies from the list of dependencies we're about to save
            dependencyVector.erase(AZStd::remove_if(dependencyVector.begin(), dependencyVector.end(), [&existingDependencies](const auto& entry) -> bool
            {
                return AZStd::find(existingDependencies.begin(), existingDependencies.end(), entry) != existingDependencies.end();
            }), dependencyVector.end());
        }

        // Save everything to the db, this will update matched non-wildcard dependencies and add new records for wildcard matches
        if (!m_stateData->UpdateProductDependencies(dependencyVector))
        {
            AZ_Error("PathDependencyManager", false, "Failed to update product dependencies");
        }
        else
        {
            // Send a notification for each dependency that has been resolved
            NotifyResolvedDependencies(dependencyVector);
        }
    }

    void CleanupPathDependency(AssetBuilderSDK::ProductPathDependency& pathDependency)
    {
        if (pathDependency.m_dependencyType == AssetBuilderSDK::ProductPathDependencyType::SourceFile)
        {
            // Nothing to cleanup if the dependency type was already pointing at source.
            return;
        }
        // Many workflows use source and product extensions for textures interchangeably, assuming that a later system will clean up the path.
        // Multiple systems use the AZ Serialization system to reference assets and collect these asset references. Not all of these systems
        // check if the references are to source or product asset types.
        // Instead requiring each of these systems to handle this (and failing in hard to track down ways later when they don't), check here, and clean things up.
        const AZStd::vector<AZStd::string> sourceImageExtensions = { ".tif", ".tiff", ".bmp", ".gif", ".jpg", ".jpeg", ".tga", ".png" };
        for (const AZStd::string& sourceImageExtension : sourceImageExtensions)
        {
            if (AzFramework::StringFunc::Path::IsExtension(pathDependency.m_dependencyPath.c_str(), sourceImageExtension.c_str()))
            {
                // This was a source format image reported initially as a product file dependency. Fix that to be a source file dependency.
                pathDependency.m_dependencyType = AssetBuilderSDK::ProductPathDependencyType::SourceFile;
                break;
            }
        }
    }

    void PathDependencyManager::ResolveDependencies(AssetBuilderSDK::ProductPathDependencySet& pathDeps, AZStd::vector<AssetBuilderSDK::ProductDependency>& resolvedDeps, const AZStd::string& platform, [[maybe_unused]] const AZStd::string& productName)
    {
        const AZ::Data::ProductDependencyInfo::ProductDependencyFlags productDependencyFlags =
            AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::NoLoad);

        AZStd::vector<AssetBuilderSDK::ProductDependency> excludedDeps;

        // Check the path dependency set and find any conflict (include and exclude the same path dependency)
        AssetBuilderSDK::ProductPathDependencySet conflicts;
        for (const AssetBuilderSDK::ProductPathDependency& pathDep : pathDeps)
        {
            auto conflictItr = find_if(pathDeps.begin(), pathDeps.end(),
                [&pathDep](const AssetBuilderSDK::ProductPathDependency& pathDepForComparison)
            {
                return (pathDep.m_dependencyPath == ExcludedDependenciesSymbol + pathDepForComparison.m_dependencyPath ||
                    pathDepForComparison.m_dependencyPath == ExcludedDependenciesSymbol + pathDep.m_dependencyPath) &&
                    pathDep.m_dependencyType == pathDepForComparison.m_dependencyType;
            });

            if (conflictItr != pathDeps.end())
            {
                conflicts.insert(pathDep);
            }
        }

        auto pathIter = pathDeps.begin();
        while (pathIter != pathDeps.end())
        {
            if (conflicts.find(*pathIter) != conflicts.end())
            {
                // Ignore conflicted path dependencies
                AZ_Error(AssetProcessor::DebugChannel, false,
                    "Cannot resolve path dependency %s for product %s since there's a conflict\n",
                    pathIter->m_dependencyPath.c_str(), productName.c_str());
                ++pathIter;
                continue;
            }

            AssetBuilderSDK::ProductPathDependency cleanedupDependency(*pathIter);
            CleanupPathDependency(cleanedupDependency);
            AZStd::string dependencyPathSearch = cleanedupDependency.m_dependencyPath;

            bool isExcludedDependency = dependencyPathSearch.starts_with(ExcludedDependenciesSymbol);
            dependencyPathSearch = isExcludedDependency ? dependencyPathSearch.substr(1) : dependencyPathSearch;
            // The database uses % for wildcards, both path based searching uses *, so keep a copy of the path with the * wildcard for later
            // use.
            AZStd::string pathWildcardSearchPath(dependencyPathSearch);
            bool isExactDependency = !AzFramework::StringFunc::Replace(dependencyPathSearch, '*', '%');

            if (cleanedupDependency.m_dependencyType == AssetBuilderSDK::ProductPathDependencyType::ProductFile)
            {
                SanitizeForDatabase(dependencyPathSearch);
                SanitizeForDatabase(pathWildcardSearchPath);
                AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer productInfoContainer;
                QString productNameWithPlatform = QString("%1%2%3").arg(platform.c_str(), AZ_CORRECT_DATABASE_SEPARATOR_STRING, dependencyPathSearch.c_str());

                if (AzFramework::StringFunc::Equal(productNameWithPlatform.toUtf8().data(), productName.c_str()))
                {
                    AZ_Warning(AssetProcessor::ConsoleChannel, false,
                        "Invalid dependency: Product Asset ( %s ) has listed itself as one of its own Product Dependencies.",
                        productName.c_str());
                    pathIter = pathDeps.erase(pathIter);
                    continue;
                }

                if (isExactDependency)
                {
                    // Search for products in the cache platform folder
                    // Example: If a path dependency is "test1.asset" in AutomatedTesting on PC, this would search
                    //  "AutomatedTesting/Cache/pc/test1.asset"
                    m_stateData->GetProductsByProductName(productNameWithPlatform, productInfoContainer);

                }
                else
                {
                    m_stateData->GetProductsLikeProductName(productNameWithPlatform, AzToolsFramework::AssetDatabase::AssetDatabaseConnection::LikeType::Raw, productInfoContainer);
                }

                // See if path matches any product files
                if (!productInfoContainer.empty())
                {
                    AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceDatabaseEntry;

                    for (const auto& productDatabaseEntry : productInfoContainer)
                    {
                        if (m_stateData->GetSourceByJobID(productDatabaseEntry.m_jobPK, sourceDatabaseEntry))
                        {
                            // The SQL wildcard search is greedy and doesn't match the path based, glob style wildcard search that is
                            // expected in this case. This also matches the behavior of resolving unmet dependencies later. There are two
                            // cases that wildcard dependencies resolve:
                            //  1. When the product with the wildcard dependency is first created, it resolves those dependencies against
                            //  what's already in the database. That's this case.
                            //  2. When another product is created, all existing wildcard dependencies are compared against that product to
                            //  see if it matches them.
                            // This check here makes sure that the filter for 1 matches 2.
                            if (!isExactDependency)
                            {
                                AZ::IO::PathView searchPath(productDatabaseEntry.m_productName);

                                if (!searchPath.Match(pathWildcardSearchPath))
                                {
                                    continue;
                                }
                            }

                            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencyList = isExcludedDependency ? excludedDeps : resolvedDeps;
                            productDependencyList.emplace_back(AZ::Data::AssetId(sourceDatabaseEntry.m_sourceGuid, productDatabaseEntry.m_subID), productDependencyFlags);
                        }
                        else
                        {
                            AZ_Error(AssetProcessor::ConsoleChannel, false, "Source for JobID %i not found (from product %s)", productDatabaseEntry.m_jobPK, dependencyPathSearch.c_str());
                        }

                        // For exact dependencies we expect that there is only 1 match.  Even if we processed more than 1, the results could be inconsistent since the other assets may not be finished processing yet
                        if (isExactDependency)
                        {
                            break;
                        }
                    }

                    // Wildcard and excluded dependencies never get removed since they can be fulfilled by a future product
                    if (isExactDependency && !isExcludedDependency)
                    {
                        pathIter = pathDeps.erase(pathIter);
                        continue;
                    }
                }
            }
            else
            {
                // For source assets, the casing of the input path must be maintained. Just fix up the path separators.
                AZStd::replace(dependencyPathSearch.begin(), dependencyPathSearch.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);
                AzFramework::StringFunc::Replace(dependencyPathSearch, AZ_DOUBLE_CORRECT_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR_STRING);

                // See if path matches any source files
                AzToolsFramework::AssetDatabase::SourceDatabaseEntryContainer sourceInfoContainer;

                if (isExactDependency)
                {
                    QString databaseName;
                    QString scanFolder;

                    if (ProcessInputPathToDatabasePathAndScanFolder(dependencyPathSearch.c_str(), databaseName, scanFolder))
                    {
                        AzToolsFramework::AssetDatabase::SourceDatabaseEntry source;
                        if(m_stateData->GetSourceBySourceNameScanFolderId(databaseName, m_platformConfig->GetScanFolderByPath(scanFolder)->ScanFolderID(), source))
                        {
                            sourceInfoContainer.push_back(AZStd::move(source));
                        }
                    }
                }
                else
                {
                    m_stateData->GetSourcesLikeSourceName(dependencyPathSearch.c_str(), AzToolsFramework::AssetDatabase::AssetDatabaseConnection::LikeType::Raw, sourceInfoContainer);
                }

                if (!sourceInfoContainer.empty())
                {
                    bool productsAvailable = false;
                    for (const auto& sourceDatabaseEntry : sourceInfoContainer)
                    {
                        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer productInfoContainer;

                        if (m_stateData->GetProductsBySourceID(sourceDatabaseEntry.m_sourceID, productInfoContainer, AZ::Uuid::CreateNull(), "", platform.c_str()))
                        {
                            productsAvailable = true;
                            // Add a dependency on every product of this source file
                            for (const auto& productDatabaseEntry : productInfoContainer)
                            {
                                AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencyList = isExcludedDependency ? excludedDeps : resolvedDeps;
                                productDependencyList.emplace_back(AZ::Data::AssetId(sourceDatabaseEntry.m_sourceGuid, productDatabaseEntry.m_subID), productDependencyFlags);
                            }
                        }

                        // For exact dependencies we expect that there is only 1 match.  Even if we processed more than 1, the results could be inconsistent since the other assets may not be finished processing yet
                        if (isExactDependency)
                        {
                            break;
                        }
                    }

                    if (isExactDependency && productsAvailable && !isExcludedDependency)
                    {
                        pathIter = pathDeps.erase(pathIter);
                        continue;
                    }
                }
            }
            pathIter->m_dependencyPath = cleanedupDependency.m_dependencyPath;
            pathIter->m_dependencyType = cleanedupDependency.m_dependencyType;
            ++pathIter;
        }

        // Remove the excluded dependency from the resolved dependency list and leave them unresolved
        resolvedDeps.erase(AZStd::remove_if(resolvedDeps.begin(), resolvedDeps.end(),
            [&excludedDeps](const AssetBuilderSDK::ProductDependency& resolvedDependency)
        {
            auto excludedDependencyItr = AZStd::find_if(excludedDeps.begin(), excludedDeps.end(),
                [&resolvedDependency](const AssetBuilderSDK::ProductDependency& excludedDependency)
            {
                return resolvedDependency.m_dependencyId == excludedDependency.m_dependencyId &&
                    resolvedDependency.m_flags == excludedDependency.m_flags;
            });

            return excludedDependencyItr != excludedDeps.end();
        }), resolvedDeps.end());
    }

    bool PathDependencyManager::ProcessInputPathToDatabasePathAndScanFolder(const char* dependencyPathSearch, QString& databaseName, QString& scanFolder) const
    {
        if (!AzFramework::StringFunc::Path::IsRelative(dependencyPathSearch))
        {
            // absolute paths just get converted directly
            return m_platformConfig->ConvertToRelativePath(QString::fromUtf8(dependencyPathSearch), databaseName, scanFolder);
        }
        else
        {
            // relative paths get the first matching asset, and then they get the usual call.
            QString absolutePath = m_platformConfig->FindFirstMatchingFile(QString::fromUtf8(dependencyPathSearch));
            if (!absolutePath.isEmpty())
            {
                return m_platformConfig->ConvertToRelativePath(absolutePath, databaseName, scanFolder);
            }
        }

        return false;
    }

    AZStd::string PathDependencyManager::ToScanFolderPrefixedPath(int scanFolderId, const char* relativePath) const
    {
        static constexpr char ScanFolderSeparator = '$';

        return AZStd::string::format("%c%d%c%s", ScanFolderSeparator, scanFolderId, ScanFolderSeparator, relativePath);
    }

} // namespace AssetProcessor
