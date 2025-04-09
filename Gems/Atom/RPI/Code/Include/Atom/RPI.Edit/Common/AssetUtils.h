/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/JSON/document.h>

#include <Atom/RPI.Edit/Configuration.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace RPI
    {
        namespace AssetUtils
        {
            // Declarations...

            // Note that these functions default to TraceLevel::Error to preserve legacy behavior of these APIs. It would be nice to make the default match
            // RPI.Reflect/Asset/AssetUtils.h which is TraceLevel::Warning, but we are close to a release so it isn't worth the risk at this time.

            ATOM_RPI_EDIT_API Outcome<Data::AssetId> MakeAssetId(const AZStd::string& sourcePath, uint32_t productSubId, TraceLevel reporting = TraceLevel::Error);

            ATOM_RPI_EDIT_API Outcome<Data::AssetId> MakeAssetId(const AZStd::string& originatingSourcePath, const AZStd::string& referencedSourceFilePath, uint32_t productSubId, TraceLevel reporting = TraceLevel::Error);

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(
                const AZStd::string& sourcePath,
                uint32_t productSubId = 0,
                TraceLevel reporting = TraceLevel::Error,
                const AZ::Data::AssetLoadParameters& assetLoadParameters = AZ::Data::AssetLoadParameters{});

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(
                const AZStd::string& originatingSourcePath,
                const AZStd::string& referencedSourceFilePath,
                uint32_t productSubId = 0,
                TraceLevel reporting = TraceLevel::Error,
                const AZ::Data::AssetLoadParameters& assetLoadParameters = AZ::Data::AssetLoadParameters{});

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(
                const AZ::Data::AssetId& assetId,
                const char* sourcePathForDebug,
                TraceLevel reporting = TraceLevel::Error,
                const AZ::Data::AssetLoadParameters& assetLoadParameters = AZ::Data::AssetLoadParameters{});

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(
                const AZ::Data::AssetId& assetId,
                TraceLevel reporting = TraceLevel::Error,
                const AZ::Data::AssetLoadParameters& assetLoadParameters = AZ::Data::AssetLoadParameters{});

            //! Attempts to resolve the full path to a product asset given its ID
            ATOM_RPI_EDIT_API AZStd::string GetProductPathByAssetId(const AZ::Data::AssetId& assetId);

            //! Attemts to resolve the full path to a source asset given its ID
            ATOM_RPI_EDIT_API AZStd::string GetSourcePathByAssetId(const AZ::Data::AssetId& assetId);

            //! Tries to resolve a relative file reference, given the path of a referencing file.
            //! @param originatingSourceFilePath  Path to the parent file that references referencedSourceFilePath. May be absolute or relative to asset-root.
            //! @param referencedSourceFilePath   Path that the parent file references. May be relative to the parent file location or relative to asset-root.
            //! @return A full path for referencedSourceFilePath, if a full path was found. If a full path could not be constructed, returns referencedSourceFilePath unmodified.
            ATOM_RPI_EDIT_API AZStd::string ResolvePathReference(const AZStd::string& originatingSourceFilePath, const AZStd::string& referencedSourceFilePath);

            //! Takes an arbitrary string and replaces some characters to make it a valid filename. The result will be compatible with AzQtComponents::FileDialog.
            //! Ex. SanitizeFileName("Left=>Right.txt") == "Left_Right.txt"
            //! Ex. SanitizeFileName("Material::Red#1") == "Material_Red_1"
            ATOM_RPI_EDIT_API AZStd::string SanitizeFileName(AZStd::string filename);

            // Definitions...

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(
                const AZStd::string& sourcePath,
                uint32_t productSubId,
                TraceLevel reporting,
                const AZ::Data::AssetLoadParameters& assetLoadParameters)
            {
                auto assetId = MakeAssetId(sourcePath, productSubId, reporting);
                if (assetId.IsSuccess())
                {
                    return LoadAsset<AssetDataT>(assetId.GetValue(), sourcePath.c_str(), reporting, assetLoadParameters);
                }
                else
                {
                    return AZ::Failure();
                }
            }

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(
                const AZStd::string& originatingSourcePath,
                const AZStd::string& referencedSourceFilePath,
                uint32_t productSubId,
                TraceLevel reporting,
                const AZ::Data::AssetLoadParameters& assetLoadParameters)
            {
                AZStd::string resolvedPath = ResolvePathReference(originatingSourcePath, referencedSourceFilePath);
                return LoadAsset<AssetDataT>(resolvedPath, productSubId, reporting, assetLoadParameters);
            }

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(
                const AZ::Data::AssetId& assetId,
                TraceLevel reporting,
                const AZ::Data::AssetLoadParameters& assetLoadParameters)
            {
                return LoadAsset<AssetDataT>(assetId, nullptr, reporting, assetLoadParameters);
            }

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(
                const AZ::Data::AssetId& assetId,
                [[maybe_unused]] const char* sourcePathForDebug,
                TraceLevel reporting,
                const AZ::Data::AssetLoadParameters& assetLoadParameters)
            {
                if (nullptr == AZ::IO::FileIOBase::GetInstance()->GetAlias("@products@"))
                {
                    // The absence of "@products@" is not necessarily the reason LoadAsset() can't be used in CreateJobs(), but it
                    // is a symptom of calling LoadAsset() from CreateJobs() which is not supported.
                    AZ_Assert(false, "It appears AssetUtils::LoadAsset() is being called in CreateJobs(). It can only be used in ProcessJob().");
                    return AZ::Failure();
                }

                Data::Asset<AssetDataT> asset = AZ::Data::AssetManager::Instance().GetAsset<AssetDataT>(assetId, AZ::Data::AssetLoadBehavior::PreLoad, assetLoadParameters);
                asset.BlockUntilLoadComplete();

                if (asset.IsReady())
                {
                    return AZ::Success(asset);
                }
                else
                {
                    AssetUtilsInternal::ReportIssue(reporting, AZStd::string::format("Could not load %s [Source='%s' Cache='%s' AssetID=%s] ",
                        AzTypeInfo<AssetDataT>::Name(),
                        sourcePathForDebug ? sourcePathForDebug : "<unknown>",
                        asset.GetHint().empty() ? "<unknown>" : asset.GetHint().c_str(),
                        assetId.ToString<AZStd::string>().c_str()).c_str());

                    return AZ::Failure();
                }
            }
        } // namespace AssetUtils
    } // namespace RPI
} // namespace AZ

