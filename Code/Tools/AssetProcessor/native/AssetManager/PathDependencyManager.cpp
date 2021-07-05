/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PathDependencyManager.h"
#include <AzCore/std/string/wildcard.h>
#include <AzCore/Asset/AssetCommon.h>
#include <utilities/PlatformConfiguration.h>
#include <utilities/assetUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AssetProcessor
{
    void SanitizeForDatabase(AZStd::string& str)
    {
        // Not calling normalize because wildcards should be preserved.
        AZStd::to_lower(str.begin(), str.end());
        AZStd::replace(str.begin(), str.end(), AZ_WRONG_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR);
        AzFramework::StringFunc::Replace(str, AZ_DOUBLE_CORRECT_DATABASE_SEPARATOR, AZ_CORRECT_DATABASE_SEPARATOR_STRING);
    }

    PathDependencyManager::PathDependencyManager(AZStd::shared_ptr<AssetDatabaseConnection> stateData, PlatformConfiguration* platformConfig)
        : m_stateData(stateData), m_platformConfig(platformConfig)
    {

    }

    void PathDependencyManager::SaveUnresolvedDependenciesToDatabase(AssetBuilderSDK::ProductPathDependencySet& unresolvedDependencies, const AzToolsFramework::AssetDatabase::ProductDatabaseEntry& productEntry, const AZStd::string& platform)
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
        const AZStd::vector<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>& dependencyEntries,
        AZStd::string_view matchedPath, bool isSourceDependency, const AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer& matchedProducts,
        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer& dependencyContainer) const
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
                dependencyContainer.push_back();
                auto& entry = dependencyContainer.back();

                entry.m_productDependencyID = dependencyId;
                entry.m_productPK = productDependencyDatabaseEntry.m_productPK;
                entry.m_dependencySourceGuid = sourceEntry.m_sourceGuid;
                entry.m_dependencySubID = matchedProduct.m_subID;
                entry.m_platform = productDependencyDatabaseEntry.m_platform;

                // If there's more than 1 product, reset the ID so further products create new db entries
                dependencyId = AzToolsFramework::AssetDatabase::InvalidEntryId;
            }
        }
    }

    void PathDependencyManager::RetryDeferredDependencies(const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& sourceEntry)
    {
        MapSet exclusionMaps = PopulateExclusionMaps();

        // Gather a list of all the products this source file produced
        AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer products;
        if (!m_stateData->GetProductsBySourceName(sourceEntry.m_sourceName.c_str(), products))
        {
            AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Source %s did not have any products.  Skipping dependency processing.\n", sourceEntry.m_sourceName.c_str());
            return;
        }

        AZStd::unordered_map<AZStd::string, AZStd::vector<AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry>> map;

        // Build up a list of all the paths we need to search for: products + 2 variations of the source path
        AZStd::vector<AZStd::string> searchPaths;

        for (const auto& productEntry : products)
        {
            const AZStd::string& productName = productEntry.m_productName;

            // strip path of the <platform>/
            AZStd::string strippedPath = AssetUtilities::StripAssetPlatform(productName).toUtf8().constData();
            SanitizeForDatabase(strippedPath);

            searchPaths.push_back(strippedPath);
        }

        AZStd::string sourceNameWithScanFolder = ToScanFolderPrefixedPath(aznumeric_cast<int>(sourceEntry.m_scanFolderPK), sourceEntry.m_sourceName.c_str());
        AZStd::string sanitizedSourceName = sourceEntry.m_sourceName;

        SanitizeForDatabase(sourceNameWithScanFolder);
        SanitizeForDatabase(sanitizedSourceName);

        searchPaths.push_back(sourceNameWithScanFolder);
        searchPaths.push_back(sanitizedSourceName);

        m_stateData->QueryProductDependenciesUnresolvedAdvanced(searchPaths, [&map](AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntry& entry, const AZStd::string& matchedPath)
        {
            map[matchedPath].push_back(AZStd::move(entry));
            return true;
        });

        AzToolsFramework::AssetDatabase::ProductDependencyDatabaseEntryContainer dependencyContainer;

        // Go through all the matched dependencies
        for (const auto& pair : map)
        {
            AZStd::string_view matchedPath = pair.first;
            const bool isSourceDependency = matchedPath == sanitizedSourceName || matchedPath == sourceNameWithScanFolder;

            AzToolsFramework::AssetDatabase::ProductDatabaseEntryContainer matchedProducts;

            // Figure out the list of products to work with, for a source match, use all the products, otherwise just use the matched products
            if (isSourceDependency)
            {
                matchedProducts = products;
            }
            else
            {
                for (const auto& productEntry : products)
                {
                    const AZStd::string& productName = productEntry.m_productName;

                    // strip path of the leading asset platform /<platform>
                    AZStd::string strippedPath = AssetUtilities::StripAssetPlatform(productName).toUtf8().constData();
                    SanitizeForDatabase(strippedPath);

                    if (strippedPath == matchedPath)
                    {
                        matchedProducts.push_back(productEntry);
                    }
                }
            }

            // Go through each dependency we're resolving and create a db entry for each product that resolved it (wildcard/source dependencies will generally create more than 1)
            SaveResolvedDependencies(sourceEntry, exclusionMaps, sourceNameWithScanFolder, pair.second, matchedPath, isSourceDependency, matchedProducts, dependencyContainer);
        }

        // Save everything to the db
        if (!m_stateData->UpdateProductDependencies(dependencyContainer))
        {
            AZ_Error("PathDependencyManager", false, "Failed to update product dependencies");
        }
        else
        {
            // Send a notification for each dependency that has been resolved
            NotifyResolvedDependencies(dependencyContainer);
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
            bool isExactDependency = !AzFramework::StringFunc::Replace(dependencyPathSearch, '*', '%');

            if (cleanedupDependency.m_dependencyType == AssetBuilderSDK::ProductPathDependencyType::ProductFile)
            {
                SanitizeForDatabase(dependencyPathSearch);
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
                        m_stateData->GetSourcesBySourceNameScanFolderId(databaseName, m_platformConfig->GetScanFolderByPath(scanFolder)->ScanFolderID(), sourceInfoContainer);
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
