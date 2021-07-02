/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomCore/Instance/Instance.h>
#include <Atom/Feature/Utils/IndexableList.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <AtomCore/std/containers/array_view.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>


namespace AZ
{
    namespace RPI
    {
        class ImageMipChainAsset;
        class StreamingImageAsset;
        class MaterialAsset;
    }

    namespace Render
    {
        //! Helper class used by DecalTextureArrayFeatureProcessor.
        //! Given a set of images (all with the same dimensions and format), it can pack them together into a single textureArray that can be sent to the GPU.
        class DecalTextureArray : public Data::AssetBus::MultiHandler
        {
        public:

            int AddMaterial(const AZ::Data::AssetId materialAssetId);
            void RemoveMaterial(const int index);
            size_t NumMaterials() const;

            AZ::Data::AssetId GetMaterialAssetId(const int index) const;

            void Pack();
            const Data::Instance<RPI::StreamingImage>& GetPackedTexture() const;

            static bool IsValidDecalMaterial(const RPI::MaterialAsset& materialAsset);

        private:

            struct MaterialData
            {
                AZ::Data::AssetId m_materialAssetId;
                // We will clear this to nullptr as soon as it is packed in order to release the memory. Note that we might need to reload it in order to repack it.
                AZ::Data::Asset<Data::AssetData> m_materialAssetData;
            };

            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;

            int FindMaterial(const AZ::Data::AssetId materialAssetId) const;

            // packs the contents of the source images into a texture array readable by the GPU and returns it
            AZ::Data::Asset<AZ::RPI::ImageMipChainAsset> BuildPackedMipChainAsset(const size_t numTexturesToCreate);

            RHI::ImageDescriptor CreatePackedImageDescriptor(const uint16_t arraySize, const uint16_t mipLevels) const;

            uint16_t GetNumMipLevels() const;
            RHI::Size GetImageDimensions() const;
            RHI::Format GetFormat() const;
            RHI::ImageSubresourceLayout GetLayout(int mip) const;
            AZStd::array_view<uint8_t> GetRawImageData(int arrayLevel, int mip) const;

            bool AreAllAssetsReady() const;
            bool IsAssetReady(const MaterialData& materialData) const;

            void ClearAssets();
            void ClearAsset(MaterialData& materialData);

            void QueueAssetLoads();
            void QueueAssetLoad(MaterialData& materialData);

            bool NeedsPacking() const;

            IndexableList<MaterialData> m_materials;
            Data::Instance<RPI::StreamingImage> m_textureArrayPacked;
             
            AZStd::unordered_set<AZ::Data::AssetId> m_assetsCurrentlyLoading;
        };

    } // namespace Render
} // namespace AZ
