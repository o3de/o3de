/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Allocators.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>

#include <Atom/RHI/StreamingImagePool.h>

#include <AzCore/std/containers/span.h>

#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    namespace RPI
    {
        class ImageMipChainAssetTester;

        //! A container of packed image data.
        //! This asset is designed to represent image data located on disk. It may contain multiple mip levels, each with an
        //! array of sub-images. Support for multiple mip levels allows the streaming system to partition mip levels into groups.
        //! For example, the lowest N mips can be streamed at once and loaded as a unit.
        //! The mip data is defined independently from any parent image asset. Only the topology of the sub-images is known (i.e.
        //! the number of mip levels and the array size). The first slice (index 0) is the highest detail mip. The lowest detail mip is
        //! N-1. Since the mip chain is independent, the slice index is local to the container. That means you will have to translate
        //! a parent image mip slice to the local container slice index.
        //! This is an immutable, serialized asset. It can be either serialized-in or created dynamically using ImageMipChainAssetCreator.
        //! See RPI::ImageMipChain for runtime features based on this asset.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API ImageMipChainAsset final
            : public Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class ImageMipChainAssetCreator;
            friend class ImageMipChainAssetHandler;
            friend class ImageMipChainAssetTester;
            friend class StreamingImageAssetCreator;
            friend class StreamingImageAssetHandler;

        public:
            static constexpr const char* DisplayName{ "ImageMipChain" };
            static constexpr const char* Group{ "Image" };
            static constexpr const char* Extension{ "imagemipchain" };

            using Allocator = ImageMipChainAssetAllocator_for_std_t;

            AZ_RTTI(ImageMipChainAsset, "{CB403C8A-6982-4C9F-8090-78C9C36FBEDB}", Data::AssetData);
            AZ_CLASS_ALLOCATOR_DECL

            static void Reflect(AZ::ReflectContext* context);

            ImageMipChainAsset() = default;

            //! Returns the number of mip levels in the group.
            uint16_t GetMipLevelCount() const;

            //! Returns the number of array slices in the group.
            uint16_t GetArraySize() const;

            //! Returns the number of sub-images in the group.
            size_t GetSubImageCount() const;

            //! Returns the sub-image data blob for a given mip slice and array slice (local to the group).
            AZStd::span<const uint8_t> GetSubImageData(uint32_t mipSlice, uint32_t arraySlice) const;

            //! Returns the sub-image data blob for a linear index (local to the group).
            AZStd::span<const uint8_t> GetSubImageData(uint32_t subImageIndex) const;
            
            //! Returns the sub-image layout for a single sub-image by index.
            const RHI::DeviceImageSubresourceLayout& GetSubImageLayout(uint32_t subImageIndex) const;

            using MipSliceList = AZStd::fixed_vector<RHI::StreamingImageMipSlice, RHI::Limits::Image::MipCountMax>;

            //! Returns the array of streaming image mip slices used to update RHI image content.
            const MipSliceList& GetMipSlices() const;

            //! Returns the total size of pixel data across all mips in this chain. 
            size_t GetImageDataSize() const;

        protected:
            // AssetData overrides...
            bool HandleAutoReload() override { return false; }

        private:
            // Copy content from another ImageMipChainAsset
            void CopyFrom(const ImageMipChainAsset& source);

            // Initializes mip chain data after serialization.
            void Init();

            /// Called by asset creators to assign the asset to a ready state.
            void SetReady();

            // Array of mip slice pointers; initialized after serialization. Used for constructing the RHI update request.
            MipSliceList m_mipSlices;

            // The list of subresource data, fixed up from serialization.
            AZStd::vector<RHI::StreamingImageSubresourceData, Allocator> m_subImageDatas;

            // [Serialized] Topology of sub-images in the mip group.
            uint16_t m_mipLevels = 0;
            uint16_t m_arraySize = 0;

            // [Serialized] Maps the local mip level to a region of the sub-image array.
            AZStd::array<uint16_t, RHI::Limits::Image::MipCountMax> m_mipToSubImageOffset;

            // [Serialized] Maps the local mip level to a sub resource layout.
            AZStd::array<RHI::DeviceImageSubresourceLayout, RHI::Limits::Image::MipCountMax> m_subImageLayouts;

            // [Serialized] Contains a flat list of sub-images which reference the flat data blob.
            AZStd::vector<AZ::u64, Allocator> m_subImageDataOffsets;

            // [Serialized] Flat image data interpreted by m_subImages.
            AZStd::vector<uint8_t, Allocator> m_imageData;
        };

        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API ImageMipChainAssetHandler final
            : public AssetHandler<ImageMipChainAsset>
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            using Base = AssetHandler<ImageMipChainAsset>;
        public:
            ImageMipChainAssetHandler() = default;

        protected:
            Data::AssetHandler::LoadResult LoadAssetData(
                const Data::Asset<Data::AssetData>& asset,
                AZStd::shared_ptr<Data::AssetDataStream> stream,
                const Data::AssetFilterCB& assetLoadFilterCB) override;
        };
    }
}
