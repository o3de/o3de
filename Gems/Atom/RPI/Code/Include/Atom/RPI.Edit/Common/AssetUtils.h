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

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ
{
    namespace RPI
    {
        namespace AssetUtils
        {
            // Declarations...

            Outcome<Data::AssetId> MakeAssetId(const AZStd::string& sourcePath, uint32_t productSubId);

            Outcome<Data::AssetId> MakeAssetId(const AZStd::string& originatingSourcePath, const AZStd::string& referencedSourceFilePath, uint32_t productSubId);

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(const AZStd::string& sourcePath, uint32_t productSubId = 0);

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(const AZStd::string& originatingSourcePath, const AZStd::string& referencedSourceFilePath, uint32_t productSubId = 0);

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(const AZ::Data::AssetId& assetId, const char* sourcePathForDebug);

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(const AZ::Data::AssetId& assetId);

            //! Attempts to resolve the full path to a product asset given its ID
            AZStd::string GetProductPathByAssetId(const AZ::Data::AssetId& assetId);

            //! Attemts to resolve the full path to a source asset given its ID
            AZStd::string GetSourcePathByAssetId(const AZ::Data::AssetId& assetId);

            //! Tries to resolve a relative file reference, given the path of a referencing file.
            //! @param originatingSourceFilePath  Path to the parent file that references referencedSourceFilePath. May be absolute or relative to asset-root.
            //! @param referencedSourceFilePath   Path that the parent file references. May be relative to the parent file location or relative to asset-root.
            //! @return A full path for referencedSourceFilePath, if a full path was found. If a full path could not be constructed, returns referencedSourceFilePath unmodified.
            AZStd::string ResolvePathReference(const AZStd::string& originatingSourceFilePath, const AZStd::string& referencedSourceFilePath);

            //! Returns the list of paths where a source asset file could possibly appear.
            //! This is intended for use by AssetBuilders when reporting dependencies, to support relative paths between source files.
            //! When a source data file references another file using a relative path, the path might be relative to the originating
            //! file or it might be a standard source asset path (i.e. relative to the logical asset-root). This function will help reporting
            //! dependencies on all possible locations where that file may appear at some point in the future.
            //! For example a file MyGem/Assets/Foo/a.json might reference another file as "Bar/b.json". In this case, calling
            //! GetPossibleDepenencyPaths("Foo/a.json", "Bar/b.json") might return {"Foo/Bar/b.json", and "Bar/b.json"} because
            //! it's possible that b.json could be found in either MyGem/Assets/Foo/Bar/a.json or in MyGem/Assets/Bar/a.json.
            //! @param originatingSourceFilePath  Path to a file that references referencedSourceFilePath. May be absolute or relative to asset-root.
            //! @param referencedSourceFilePath   The referenced path as it appears in the originating file. May be relative to the originating file location or relative to asset-root.
            //! @return the list of possible paths, ordered from highest priority to lowest priority
            AZStd::vector<AZStd::string> GetPossibleDepenencyPaths(const AZStd::string& originatingSourceFilePath, const AZStd::string& referencedSourceFilePath);

            // Definitions...

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(const AZStd::string& sourcePath, uint32_t productSubId)
            {
                auto assetId = MakeAssetId(sourcePath, productSubId);
                if (assetId.IsSuccess())
                {
                    return LoadAsset<AssetDataT>(assetId.GetValue(), sourcePath.c_str());
                }
                else
                {
                    return AZ::Failure();
                }
            }

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(const AZStd::string& originatingSourcePath, const AZStd::string& referencedSourceFilePath, uint32_t productSubId)
            {
                AZStd::string resolvedPath = ResolvePathReference(originatingSourcePath, referencedSourceFilePath);
                return LoadAsset<AssetDataT>(resolvedPath, productSubId);
            }

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(const AZ::Data::AssetId& assetId)
            {
                return LoadAsset<AssetDataT>(assetId, nullptr);
            }

            template<typename AssetDataT>
            Outcome<AZ::Data::Asset<AssetDataT>> LoadAsset(const AZ::Data::AssetId& assetId, [[maybe_unused]] const char* sourcePathForDebug)
            {
                if (nullptr == AZ::IO::FileIOBase::GetInstance()->GetAlias("@products@"))
                {
                    // The absence of "@products@" is not necessarily the reason LoadAsset() can't be used in CreateJobs(), but it
                    // is a symptom of calling LoadAsset() from CreateJobs() which is not supported.
                    AZ_Assert(false, "It appears AssetUtils::LoadAsset() is being called in CreateJobs(). It can only be used in ProcessJob().");
                    return AZ::Failure();
                }

                Data::Asset<AssetDataT> asset = AZ::Data::AssetManager::Instance().GetAsset<AssetDataT>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
                asset.BlockUntilLoadComplete();

                if (asset.IsReady())
                {
                    return AZ::Success(asset);
                }
                else
                {
                    AZ_Error("AssetUtils", false, "Could not load %s [Source='%s' Cache='%s' AssetID=%s] ",
                        AzTypeInfo<AssetDataT>::Name(),
                        sourcePathForDebug ? sourcePathForDebug : "<unknown>",
                        asset.GetHint().empty() ? "<unknown>" : asset.GetHint().c_str(),
                        assetId.ToString<AZStd::string>().c_str());

                    return AZ::Failure();
                }
            }
        } // namespace AssetUtils
    } // namespace RPI
} // namespace AZ

