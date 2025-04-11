/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

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
                    bool sourceFileFound = false;
                    AZ::Data::AssetInfo assetInfo;
                    AZStd::string watchFolder;

                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                        sourceFileFound,
                        &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourceUUID,
                        assetId.m_guid,
                        assetInfo,
                        watchFolder);

                    if (sourceFileFound)
                    {
                        AzFramework::StringFunc::Path::ConstructFull(
                            watchFolder.c_str(), assetInfo.m_relativePath.c_str(), sourcePath, true);
                    }
                }
                return sourcePath;
            }

            AZStd::string ResolvePathReference(
                const AZStd::string& originatingSourceFilePath, const AZStd::string& referencedSourceFilePath)
            {
                // Convert incoming paths containing aliases into absolute paths
                AZ::IO::FixedMaxPath originatingPath;
                AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(originatingPath, AZ::IO::PathView{ originatingSourceFilePath });
                originatingPath = originatingPath.LexicallyNormal();

                AZ::IO::FixedMaxPath referencedPath;
                AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(referencedPath, AZ::IO::PathView{ referencedSourceFilePath });
                referencedPath = referencedPath.LexicallyNormal();

                // If the referenced path is empty or absolute then the path does not need to be resolved and can be returned immediately
                if (referencedPath.empty() || referencedPath.IsAbsolute())
                {
                    return referencedPath.String();
                }

                // Compose a path from the originating source file folder to the referenced source file
                AZ::IO::FixedMaxPath combinedPath = originatingPath.ParentPath();
                combinedPath /= referencedPath;
                combinedPath = combinedPath.LexicallyNormal();

                // Try to find the source file starting at the originatingSourceFilePath, and return the full path
                bool pathFound = false;
                AZ::Data::AssetInfo sourceInfo;
                AZStd::string rootFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    pathFound,
                    &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
                    combinedPath.c_str(),
                    sourceInfo,
                    rootFolder);
                if (pathFound)
                {
                    // Construct fails if either of the rootFolder or the referencedPath is empty. For some testing purposes, root can be
                    // empty.
                    AZStd::string fullSourcePath;
                    if (AzFramework::StringFunc::Path::ConstructFull(rootFolder.c_str(), combinedPath.c_str(), fullSourcePath, true))
                    {
                        return fullSourcePath;
                    }
                    return combinedPath.String();
                }

                // Try to find the source file starting at the asset root, and return the full path
                pathFound = false;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    pathFound,
                    &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
                    referencedPath.c_str(),
                    sourceInfo,
                    rootFolder);
                if (pathFound)
                {
                    // Construct fails if either of the rootFolder or the referencedPath is empty. For some testing purposes, root can be
                    // empty.
                    AZStd::string fullSourcePath;
                    if (AzFramework::StringFunc::Path::ConstructFull(rootFolder.c_str(), referencedPath.c_str(), fullSourcePath, true))
                    {
                        return fullSourcePath;
                    }
                    return referencedPath.String();
                }

                // If no source file was found, return the original reference path. Something else will probably fail and report errors.
                return referencedPath.String();
            }

            Outcome<Data::AssetId> MakeAssetId(const AZStd::string& sourcePath, uint32_t productSubId, TraceLevel reporting)
            {
                AZ::IO::FixedMaxPath sourcePathNoAlias;
                AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(sourcePathNoAlias, AZ::IO::PathView{ sourcePath });
                sourcePathNoAlias = sourcePathNoAlias.LexicallyNormal();

                bool assetFound = false;
                AZ::Data::AssetInfo sourceInfo;
                AZStd::string watchFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    assetFound,
                    &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
                    sourcePathNoAlias.c_str(),
                    sourceInfo,
                    watchFolder);

                if (!assetFound)
                {
                    AssetUtilsInternal::ReportIssue(
                        reporting, AZStd::string::format("Could not find asset for source file [%s]", sourcePath.c_str()).c_str());
                    return AZ::Failure();
                }

                return AZ::Success(AZ::Data::AssetId(sourceInfo.m_assetId.m_guid, productSubId));
            }

            Outcome<Data::AssetId> MakeAssetId(
                const AZStd::string& originatingSourcePath,
                const AZStd::string& referencedSourceFilePath,
                uint32_t productSubId,
                TraceLevel reporting)
            {
                const AZStd::string resolvedPath = ResolvePathReference(originatingSourcePath, referencedSourceFilePath);
                return MakeAssetId(resolvedPath, productSubId, reporting);
            }

            AZStd::string SanitizeFileName(AZStd::string filename)
            {
                filename = AZStd::regex_replace(filename, AZStd::regex{R"([^a-zA-Z0-9_\-\.]+)"}, "_"); // Replace unsupported characters
                filename = AZStd::regex_replace(filename, AZStd::regex{"\\.\\.+"}, "_"); // Don't allow multiple dots, that could mess up extensions
                filename = AZStd::regex_replace(filename, AZStd::regex{"__+"}, "_"); // Prevent multiple underscores being introduced by the above rules
                filename = AZStd::regex_replace(filename, AZStd::regex{"\\.+$"}, ""); // Don't allow dots at the end, that could mess up extensions
                
                // These rules should be compatible with those in FileDialog::GetSaveFileName, though the replacement rules here may be a bit more strict than the FileDialog validation.
                AZ_Assert(AzQtComponents::FileDialog::IsValidFileName(filename.c_str()), "The rules of AssetUtils::SanitizeFileName() must be compatible with AzQtComponents::FileDialog.");

                return filename;
            }

        } // namespace AssetUtils
    } // namespace RPI
} // namespace AZ
