/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ
{
    namespace RPI
    {
        //! The StreamingImageAsset's handler with customized loading steps in LoadAssetData override.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API StreamingImageAssetHandler final
            : public AssetHandler<StreamingImageAsset>
            , private Data::AssetBus::MultiHandler
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            using Base = AssetHandler<StreamingImageAsset>;
        public:
            AZ_CLASS_ALLOCATOR(StreamingImageAssetHandler, AZ::SystemAllocator)
            virtual ~StreamingImageAssetHandler();
            
            Data::AssetHandler::LoadResult LoadAssetData(
                const Data::Asset<Data::AssetData>& asset,
                AZStd::shared_ptr<Data::AssetDataStream> stream,
                const Data::AssetFilterCB& assetLoadFilterCB) override;

            void InitAsset(const Data::Asset<Data::AssetData>& asset, bool loadStageSucceeded, bool isReload) override;

            // Return a default fallback image if an asset is missing
            Data::AssetId AssetMissingInCatalog(const Data::Asset<Data::AssetData>& /*asset*/) override;

        private:
            // Data::AssetBus::MultiHandler overrides...
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloadError(Data::Asset<Data::AssetData> asset) override;

            // Update pending image asset info when an ImageMipChainAsset was loaded or failed to load
            void HandleMipChainAssetLoad(Data::Asset<Data::AssetData> imageMipChainAsset, bool isLoadSuccess);
            // Connect to or disconnect from AssetBus for the ImageMipChainAssets of the input StreamingImageAsset
            void HandleMipChainAssetBuses(Data::Asset<Data::AssetData> streamingImageAsset, bool connect);

            struct PendingImageAssetInfo
            {
                Data::Asset<Data::AssetData> m_imageAsset;
                AZStd::vector<u32> m_mipChainAssetSubIds;
            };

            AZStd::mutex m_accessPendingAssetsMutex;
            AZStd::unordered_map<Uuid, PendingImageAssetInfo> m_pendingReloadImageAsset;
        };
    }
}
