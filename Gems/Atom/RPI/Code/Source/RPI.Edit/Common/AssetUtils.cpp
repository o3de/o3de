/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzCore/IO/IOUtils.h>

namespace AZ
{
    namespace RPI
    {
        namespace AssetUtils
        {
            uint32_t CalcSrgProductSubId(const Name& srgName)
            {
                //[GFX TODO] Make SrgLayoutBuilder share this code after the gems reorg is complete
                return static_cast<uint32_t>(AZStd::hash<AZStd::string>()(srgName.GetStringView()) & 0xFFFFFFFF);
            }

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
                // We potentially add the parent dependency as both a direct path and a relative path rather than use AssetUtils::ResolvePathReference
                // because there is no guarantee that the Asset Processor has seen the parent file yet (which ResolvePathReference requires).
                // In that case, we have to add both possible locations because we don't know where it will show up.

                AZStd::vector<AZStd::string> results;

                // The first dependency we add is using the referencedSourceFilePath as a relative path. This gives relative paths priority over asset-root paths.
                AZStd::string combinedPath = originatingSourceFilePath;
                AzFramework::StringFunc::Path::StripFullName(combinedPath);
                AzFramework::StringFunc::Path::Join(combinedPath.c_str(), referencedSourceFilePath.c_str(), combinedPath);
                results.push_back(combinedPath);

                // If the parent file exists at the relative path, then there is no need to report a dependency on the asset-root path.
                bool assetFound = false;
                AZ::Data::AssetInfo sourceInfo;
                AZStd::string watchFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(assetFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, combinedPath.c_str(), sourceInfo, watchFolder);

                if (!assetFound)
                {
                    // The parent file wasn't found at the relative path, so we need a dependency on the asset-root path in case the file
                    // exists there. Note, we still keep the relative path dependency above because we don't know whether it's missing because
                    // it doesn't exist, or just because the AP hasn't found it yet.
                    results.push_back(referencedSourceFilePath);
                }

                return results;
            }

            Outcome<Data::AssetId> MakeAssetId(const AZStd::string& sourcePath, uint32_t productSubId)
            {
                bool assetFound = false;
                AZ::Data::AssetInfo sourceInfo;
                AZStd::string watchFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(assetFound, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, sourcePath.c_str(), sourceInfo, watchFolder);

                if (!assetFound)
                {
                    AZ_Error("AssetUtils", false, "Could not find asset [%s]", sourcePath.c_str());
                    return AZ::Failure();
                }
                else
                {
                    return AZ::Success(AZ::Data::AssetId(sourceInfo.m_assetId.m_guid, productSubId));
                }
            }

            Outcome<Data::AssetId> MakeAssetId(const AZStd::string& originatingSourcePath, const AZStd::string& referencedSourceFilePath, uint32_t productSubId)
            {
                AZStd::string resolvedPath = ResolvePathReference(originatingSourcePath, referencedSourceFilePath);
                return MakeAssetId(resolvedPath, productSubId);
            }
        } // namespace AssetUtils
    } // namespace RPI
} // namespace AZ
