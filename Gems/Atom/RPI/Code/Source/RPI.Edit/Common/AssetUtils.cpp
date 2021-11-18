/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/IO/Path/Path.h>

namespace AZ
{
    namespace RPI
    {
        namespace AssetUtils
        {
            AZStd::string GetProductPathByAssetId(const AZ::Data::AssetId& assetId)
            {
                AZStd::string productPath;
                if (assetId.IsValid())
                {
                    Data::AssetCatalogRequestBus::BroadcastResult(productPath, &Data::AssetCatalogRequests::GetAssetPathById, assetId);
                    AZ::StringFunc::Path::Normalize(productPath);
                }
                return productPath;
            }

            AZStd::string GetSourcePathByAssetId(const AZ::Data::AssetId& assetId)
            {
                AZStd::string sourcePath;
                if (assetId.IsValid())
                {
                    const AZStd::string& productPath = GetProductPathByAssetId(assetId);
                    if (!productPath.empty())
                    {
                        bool sourceFileFound = false;
                        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(sourceFileFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetFullSourcePathFromRelativeProductPath, productPath, sourcePath);
                        AZ::StringFunc::Path::Normalize(sourcePath);
                    }
                }
                return sourcePath;
            }

            AZStd::string ResolvePathReference(const AZStd::string& originatingSourceFilePath, const AZStd::string& referencedSourceFilePath)
            {
                // The IsAbsolute part prevents "second join parameter is an absolute path" warnings in StringFunc::Path::Join below
                if (referencedSourceFilePath.empty() || AZ::IO::PathView{referencedSourceFilePath}.IsAbsolute())
                {
                    return referencedSourceFilePath;
                }

                AZStd::string normalizedReferencedPath = referencedSourceFilePath;
                AzFramework::StringFunc::Path::Normalize(normalizedReferencedPath);

                AZStd::string originatingSourceFolder = originatingSourceFilePath;
                AzFramework::StringFunc::Path::StripFullName(originatingSourceFolder);

                AZStd::string pathFromOriginatingFolder;
                AzFramework::StringFunc::Path::Join(originatingSourceFolder.c_str(), referencedSourceFilePath.c_str(), pathFromOriginatingFolder);

                bool assetFound = false;
                AZ::Data::AssetInfo sourceInfo;
                AZStd::string watchFolder;

                // Try to find the source file starting at the originatingSourceFilePath, and return the full path
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(assetFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, pathFromOriginatingFolder.c_str(), sourceInfo, watchFolder);
                if (assetFound)
                {
                    AZStd::string fullSourcePath;
                    // Construct fails if either of the watchFolder (root) or the pathFromOriginatingFolder is empty.
                    // For some testing purposes, root can be empty.
                    if (AzFramework::StringFunc::Path::ConstructFull(watchFolder.c_str(), pathFromOriginatingFolder.c_str(), fullSourcePath, true))
                    {
                        return fullSourcePath;
                    }
                    else
                    {
                        return pathFromOriginatingFolder;
                    }
                }

                // Try to find the source file starting at the asset root, and return the full path
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(assetFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, normalizedReferencedPath.c_str(), sourceInfo, watchFolder);
                if (assetFound)
                {
                    AZStd::string fullSourcePath;
                    // Construct fails if either of the watchFolder (root) or the normalizedReferencedPath is empty.
                    // For some testing purposes, root can be empty.
                    if (AzFramework::StringFunc::Path::ConstructFull(watchFolder.c_str(), normalizedReferencedPath.c_str(), fullSourcePath, true))
                    {
                        return fullSourcePath;
                    }
                    else
                    {
                        return normalizedReferencedPath;
                    }
                }

                // If no source file could be found, just return the original reference path. Something else will probably fail and report errors.
                return normalizedReferencedPath;
            }

            AZStd::vector<AZStd::string> GetPossibleDepenencyPaths(const AZStd::string& originatingSourceFilePath, const AZStd::string& referencedSourceFilePath)
            {
                AZStd::vector<AZStd::string> results;

                // Use the referencedSourceFilePath as a relative path starting at originatingSourceFilePath
                AZStd::string combinedPath = originatingSourceFilePath;
                AzFramework::StringFunc::Path::StripFullName(combinedPath);
                AzFramework::StringFunc::Path::Join(combinedPath.c_str(), referencedSourceFilePath.c_str(), combinedPath);
                results.push_back(combinedPath);

                // Use the referencedSourceFilePath as a standard asset path
                results.push_back(referencedSourceFilePath);

                return results;
            }

            Outcome<Data::AssetId> MakeAssetId(const AZStd::string& sourcePath, uint32_t productSubId, TraceLevel reporting)
            {
                bool assetFound = false;
                AZ::Data::AssetInfo sourceInfo;
                AZStd::string watchFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(assetFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, sourcePath.c_str(), sourceInfo, watchFolder);

                if (!assetFound)
                {
                    AssetUtilsInternal::ReportIssue(reporting, AZStd::string::format("Could not find asset [%s]", sourcePath.c_str()).c_str());
                    return AZ::Failure();
                }
                else
                {
                    return AZ::Success(AZ::Data::AssetId(sourceInfo.m_assetId.m_guid, productSubId));
                }
            }

            Outcome<Data::AssetId> MakeAssetId(const AZStd::string& originatingSourcePath, const AZStd::string& referencedSourceFilePath, uint32_t productSubId, TraceLevel reporting)
            {
                AZStd::string resolvedPath = ResolvePathReference(originatingSourcePath, referencedSourceFilePath);
                return MakeAssetId(resolvedPath, productSubId, reporting);
            }
        } // namespace AssetUtils
    } // namespace RPI
} // namespace AZ
