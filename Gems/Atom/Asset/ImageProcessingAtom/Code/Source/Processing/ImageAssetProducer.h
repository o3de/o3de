/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <Atom/ImageProcessing/ImageObject.h>

namespace ImageProcessingAtom
{
    using namespace AZ;

    /**
     * The ImageAssetProducer saves an ImageObject to a StreamingImageAsset and several MipChainAsset. And save them to files on disk.
     * It also generats a list of AssetBuilderSDK::JobProduct which can be used for Image Builder's ProcessJob's result.
     */
    class ImageAssetProducer
    {
    public:
        /**
         * Constructor with all required initialization parameters.
         * @param imageObject is the image object to be processed and saved
         * @param saveFolder is the path of the folder where the image asset files be saved
         * @param sourceAssetId is the asset id of this image. It's used to generate full asset id for MipChainAssets which will be referenced in StreamingImageAsset.
         * @param sourceAssetName is the name of source asset file. It doesn't include path.
         * @param subId is the product subId to use for the output product.
         * @param tags list of tags to save in the image asset.
         */
        ImageAssetProducer(
            const IImageObjectPtr imageObject,
            AZStd::string_view saveFolder,
            const Data::AssetId& sourceAssetId,
            AZStd::string_view sourceAssetName,
            uint8_t numResidentMips,
            uint32_t subId,
            AZStd::set<AZStd::string> tags = AZStd::set<AZStd::string>{});

        /// Build image assets for the image object and save them to asset files. It also generates AssetBuilderSDK jobProducts if it success.
        bool BuildImageAssets();

        /// Return the list of JobProducts. The list could be empty of the build process failed.
        const AZStd::vector<AssetBuilderSDK::JobProduct>& GetJobProducts() const;

    protected:
        enum class ImageAssetType
        {
            Image,
            MipChain,
        };

        // Generate one MipChainAsset
        bool BuildMipChainAsset(const Data::AssetId& chainAssetId, uint32_t startMip, uint32_t mipLevels,
            Data::Asset<RPI::ImageMipChainAsset>& outAsset, bool saveAsProduct);

        // Generate all job products which can be used for AssetProcessor job response
        void GenerateJobProducts();

        // Save all generated assets to files
        void SaveAssetsToFile();

        // Generate product assets' full path
        AZStd::string GenerateAssetFullPath(ImageAssetType assetType, uint32_t assetSubId);

    private:
        ImageAssetProducer() = default;

        // All inputs. They shouldn't be modified.
        const IImageObjectPtr m_imageObject;
        const AZStd::string m_productFolder;
        const Data::AssetId m_sourceAssetId;
        const AZStd::string m_fileName;
        const uint32_t m_subId = AZ::RPI::StreamingImageAsset::GetImageAssetSubId();
        const uint8_t m_numResidentMips = 0u;
        AZStd::set<AZStd::string> m_tags;

        AZStd::vector<AssetBuilderSDK::JobProduct> m_jobProducts;

    };
}
