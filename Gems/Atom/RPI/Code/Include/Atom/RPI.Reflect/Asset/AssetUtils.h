/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/Asset/AssetSystemBus.h>

namespace AZ
{
    namespace RPI
    {
        namespace AssetUtils
        {
            enum class TraceLevel
            {
                None,
                Warning,
                Error,
                Assert,
            };

            // Declarations...

            //! Finds the AssetId for a given product file path.
            Data::AssetId GetAssetIdForProductPath(const char* productPath, TraceLevel reporting = TraceLevel::Warning, Data::AssetType assetType = Data::s_invalidAssetType, bool autoGenerateId = false);

            //! Gets an Asset<AssetDataT> reference for a given product file path. This function does not cause the asset to load.
            //! @return a null asset if the asset could not be found.
            template<typename AssetDataT>
            Data::Asset<AssetDataT> GetAssetByProductPath(const char* productPath, TraceLevel reporting = TraceLevel::Warning);

            //! Loads an asset using a product file path, on the current thread.
            //! @return a null asset if the asset could not be found or loaded.
            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadAssetByProductPath(const char* productPath, TraceLevel reporting = TraceLevel::Warning);

            //! Loads an asset using an AssetId, on the current thread.
            //! @return a null asset if the asset could not be found or loaded.
            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadAssetById(Data::AssetId assetId, TraceLevel reporting = TraceLevel::Warning);
            
            //! Loads a critial asset using a file path (both source and product path should be same), on the current thread.
            //! If the asset wasn't compiled, wait until the asset is compiled.
            //! @return a null asset if the asset could not be compiled or loaded.
            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadCriticalAsset(const AZStd::string& assetFilePath, TraceLevel reporting = TraceLevel::Error);

            template<typename AssetDataT>
            bool LoadBlocking(AZ::Data::Asset<AssetDataT>& asset, TraceLevel reporting = TraceLevel::Warning);

            namespace AssetUtilsInternal
            {
                //! May call AZ_Warning, AZ_Error, or AZ_Assert depending on TraceLevel
                void ReportIssue(TraceLevel traceLevel, const char* message);
            }

            // Definitions...

            template<typename AssetDataT>
            Data::Asset<AssetDataT> GetAssetByProductPath(const char* productPath, TraceLevel reporting)
            {
                Data::AssetId assetId = GetAssetIdForProductPath(productPath, reporting);

                if(!assetId.IsValid())
                {
                    return {};
                }

                Data::Asset<AssetDataT> asset(AZ::Data::AssetLoadBehavior::PreLoad);
                if (!asset.Create(assetId))
                {
                    return {};
                }

                return asset;
            }

            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadAssetByProductPath(const char* productPath, TraceLevel reporting)
            {
                Data::AssetId assetId = GetAssetIdForProductPath(productPath, reporting);
                if (assetId.IsValid())
                {
                    Data::Asset<AssetDataT> asset = AZ::Data::AssetManager::Instance().GetAsset<AssetDataT>(
                        assetId, AZ::Data::AssetLoadBehavior::PreLoad);
                    asset.BlockUntilLoadComplete();

                    if (!asset.IsReady())
                    {
                        AssetUtilsInternal::ReportIssue(reporting, AZStd::string::format("Could not load '%s'", productPath).c_str());
                        return {};
                    }

                    return asset;
                }
                else
                {
                    return {};
                }
            }

            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadAssetById(Data::AssetId assetId, TraceLevel reporting)
            {
                if (!assetId.IsValid())
                {
                    AssetUtilsInternal::ReportIssue(reporting, AZStd::string::format("Could not load '%s'", assetId.ToString<AZStd::string>().c_str()).c_str());
                    return {};
                }

                Data::Asset<AssetDataT> asset = AZ::Data::AssetManager::Instance().GetAsset<AssetDataT>(
                    assetId, AZ::Data::AssetLoadBehavior::PreLoad
                    );
                    asset.BlockUntilLoadComplete();

                if (!asset.IsReady())
                {
                    AssetUtilsInternal::ReportIssue(reporting, AZStd::string::format("Could not load '%s'", assetId.ToString<AZStd::string>().c_str()).c_str());
                    return {};
                }

                return asset;
            }

            template<typename AssetDataT>
            Data::Asset<AssetDataT> LoadCriticalAsset(const AZStd::string& assetFilePath, TraceLevel reporting)
            {
                bool apConnected = false;
                AzFramework::AssetSystemRequestBus::BroadcastResult(
                    apConnected, &AzFramework::AssetSystemRequestBus::Events::ConnectedWithAssetProcessor);
                if (apConnected)
                {
                    AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
                    AzFramework::AssetSystemRequestBus::BroadcastResult(
                        status, &AzFramework::AssetSystemRequestBus::Events::CompileAssetSync, assetFilePath);
                    if (status != AzFramework::AssetSystem::AssetStatus_Compiled)
                    {
                        AssetUtilsInternal::ReportIssue(reporting, AZStd::string::format("Could not compile asset '%s'", assetFilePath.c_str()).c_str());
                        return {};
                    }
                }

                return LoadAssetByProductPath<AssetDataT>(assetFilePath.c_str(), reporting);
            }

            template<typename AssetDataT>
            bool LoadBlocking(AZ::Data::Asset<AssetDataT>& asset, TraceLevel reporting)
            {
                if (asset.IsReady())
                {
                    return true;
                }

                if (!asset.GetId().IsValid())
                {
                    AssetUtilsInternal::ReportIssue(reporting, "Could not load null asset reference");
                    return false;
                }

                Data::Asset<AssetDataT> loadedAsset = AZ::Data::AssetManager::Instance().GetAsset<AssetDataT>(
                    asset.GetId(),
                    true, // queue load
                    nullptr, // filter
                    true // blocking
                    );

                if (!loadedAsset.IsReady())
                {
                    AssetUtilsInternal::ReportIssue(reporting, AZStd::string::format("Could not load '%s'", asset.template ToString<AZStd::string>().c_str()).c_str());
                    return false;
                }

                asset = loadedAsset;

                return true;
            }

            //! This class can be used to do basic asynchronous loading of assets without the need to implement
            //! multiple ebus functions to handle callbacks. It will invoke the provided callback function when the
            //! asset loads or errors. It will stop listening on destruction, so it should be held onto until the
            //! callback fires.
            class AsyncAssetLoader
                : private Data::AssetBus::MultiHandler
            {
            public:
                using AssetCallback = AZStd::function<void(Data::Asset<Data::AssetData>, bool /*success*/)>;

                AsyncAssetLoader() = default;
                ~AsyncAssetLoader();

                template <typename AssetDataT>
                static AsyncAssetLoader Create(const AZStd::string& path, uint32_t subId, AssetCallback callback);

                template <typename AssetDataT>
                static AsyncAssetLoader Create(Data::AssetId assetId, AssetCallback callback);

            private:

                explicit AsyncAssetLoader(AssetCallback callback);

                template <typename AssetDataT>
                void StartLoad(Data::AssetId& assetId);

                // Data::AssetBus::MultiHandler overrides..
                void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
                void OnAssetError(Data::Asset<Data::AssetData> asset) override;

                void HandleCallback(Data::Asset<Data::AssetData> asset, bool isError);

                AssetCallback m_callback;
                Data::Asset<Data::AssetData> m_asset;
            };
        }
    }
}

#include <Atom/RPI.Reflect/Asset/AssetUtils.inl>

