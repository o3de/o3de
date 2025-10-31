/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/ImageProcessing/ImageObject.h>
#include <AzCore/Asset/AssetCommon.h>

namespace ImageProcessingAtom
{
    namespace Utils
    {
        EPixelFormat RHIFormatToPixelFormat(AZ::RHI::Format rhiFormat, bool& isSRGB);

        AZ::RHI::Format PixelFormatToRHIFormat(EPixelFormat pixelFormat, bool isSrgb);

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> LoadImageAsset(const AZ::Data::AssetId& assetId);

        IImageObjectPtr LoadImageFromImageAsset(const AZ::Data::AssetId& assetId);

        IImageObjectPtr LoadImageFromImageAsset(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& asset);

        bool SaveImageToDdsFile(IImageObjectPtr image, AZStd::string_view filePath);

        bool NeedAlphaChannel(EAlphaContent alphaContent);

        //! Load image assets in the background and execute callbacks when complete.
        class AsyncImageAssetLoader : public AZ::Data::AssetBus::MultiHandler
        {
        public:
            using Callback = AZStd::function<void(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>&)>;

            AsyncImageAssetLoader() = default;
            ~AsyncImageAssetLoader();

            //! Queue an asset to be loaded asynchronously. The callback will be executed on the main thread once the asset is ready or
            //! fails.
            //! @param assetId ID of the image asset to be loaded.
            //! @param callback Callback function to execute once the asset is ready or fails.
            void QueueAsset(const AZ::Data::AssetId& assetId, const Callback& callback);

        private:
            // AZ::Data::AssetBus::MultiHandler overrides..
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            void HandleAssetNotification(AZ::Data::Asset<AZ::Data::AssetData> asset);

            using AssetCallbackPair = AZStd::pair<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>, Callback>;
            using AssetCallbackMap = AZStd::unordered_map<AZ::Data::AssetId, AssetCallbackPair>;
            AssetCallbackMap m_assetCallbackMap;
        };
    } // namespace Utils
} // namespace ImageProcessingAtom
