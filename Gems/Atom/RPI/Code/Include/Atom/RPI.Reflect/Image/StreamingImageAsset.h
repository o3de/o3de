/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImagePoolAsset.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RPI
    {
        enum class StreamingImageFlags : uint32_t
        {
            None = 0,

            //! The streaming image with NotStreamable flag is fixed to the tail mip chain. It cannot be evicted or expanded.
            //! Note: streaming image that only has one mip chain can be streamable or non-streamable. Non-streamable streaming image must have one and only one mipchain.
            NotStreamable = AZ_BIT(1)
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RPI::StreamingImageFlags)

        //! Flat data associated with a streaming image.
        //! A streaming image contains a flat list of image mip chains. The first (0 index) mip chain in the list
        //! is called the 'Head'. This is the most detailed set of mips. The last index in the list is called the 'Tail'.
        //! Each streaming image defines its own list of mip chains, which are then atomic units of streaming. This
        //! is done to allow both the platform and individual image asset to control streaming granularity.
        //! On modern GPU hardware, the last N mips in the GPU mip chain are 'packed' into a single hardware page. The
        //! recommended pattern is to group the lowest detail mips into their own mip chain asset, up to the page size.
        //! Other approaches may include grouping mips into 'Head', 'Middle', and 'Tail' sets; this is because the amount
        //! of memory gained by dropping the first mip is so much higher than all the other mips combined. The design of
        //! the streaming image asset allows you to combine mips ideally for your platform, allowing the streaming
        //! controller to fetch optimized batches of data.
        //! Each streaming image is directly associated with a streaming image pool asset, which defines its own budget
        //! and streaming controller.
        //! This is an immutable, serialized asset. It can be either serialized-in or created dynamically using StreamingImageAssetCreator.
        //! See RPI::StreamingImage for runtime features based on this asset.
        class ATOM_RPI_REFLECT_API StreamingImageAsset final
            : public ImageAsset
        {
            friend class StreamingImageAssetCreator;
            friend class StreamingImageAssetTester;
            friend class StreamingImageAssetHandler;

        public:
            static constexpr const char* DisplayName{ "StreamingImage" };
            static constexpr const char* Group{ "Image" };
            static constexpr const char* Extension{ "streamingimage" };

            using Allocator = StreamingImageAssetAllocator_for_std_t;

            AZ_RTTI(StreamingImageAsset, "{3C96A826-9099-4308-A604-7B19ADBF8761}", ImageAsset);
            AZ_CLASS_ALLOCATOR_DECL

            static void Reflect(ReflectContext* context);

            StreamingImageAsset() = default;

            //! Returns an immutable reference to the mip chain associated by index into the array of mip chains.
            const Data::Asset<ImageMipChainAsset>& GetMipChainAsset(size_t mipChainIndex) const; 

            //! Get the last mip chain asset data which contains lowest level of mips.
            const ImageMipChainAsset& GetTailMipChain() const;

            //! Returns the total number of mip chains in the image.
            size_t GetMipChainCount() const;

            //! Returns the mip chain index associated with the provided mip level.
            size_t GetMipChainIndex(size_t mipLevel) const;

            //! Given a mip chain index, returns the highest detail mip level associated with the mip chain.
            size_t GetMipLevel(size_t mipChainIndex) const;

            //! Given a mip chain index, returns the number of mip levels in the chain.
            size_t GetMipCount(size_t mipChainIndex) const;

            //! Get image data for specified mip and slice. It may trigger mipchain asset loading if the asset wasn't loaded
            AZStd::span<const uint8_t> GetSubImageData(uint32_t mip, uint32_t slice);

            //! Returns streaming image pool asset id of the pool that will be used to create the streaming image.
            const Data::AssetId& GetPoolAssetId() const;

            //!  Returns the set of flags assigned to the image.
            StreamingImageFlags GetFlags() const;

            //!  Streaming image assets are subId 1000, mipchain assets are 1001 + n
            static constexpr uint32_t GetImageAssetSubId() { return 1000; };

            //! Returns the total size of pixel data across all mips, both in this StreamingImageAsset and in all child ImageMipChainAssets. 
            size_t GetTotalImageDataSize() const;

            //! Returns the average color of this image (alpha-weighted in case of 4-component images).
            Color GetAverageColor() const;

            //! Returns the image descriptor for the specified mip level.
            RHI::ImageDescriptor GetImageDescriptorForMipLevel(AZ::u32 mipLevel) const;

            //! Whether the image has all referenced ImageMipChainAssets loaded
            bool HasFullMipChainAssets() const;

            //! Returns the image tags
            using TagList = AZStd::vector<AZ::Name, Allocator>;
            const TagList& GetTags() const;

            //! Removes up to `mipChainLevel` mipchains, reducing quality (used by the image tag system).
            //! The last mipchain won't be removed
            void RemoveFrontMipchains(size_t mipChainLevel);

        private:
            struct MipChain
            {
                AZ_TYPE_INFO(MipChain, "{5BE0B445-7B4A-451A-91FF-81033467FD68}");

                uint16_t m_mipOffset = 0;
                uint16_t m_mipCount = 0;
                Data::Asset<ImageMipChainAsset> m_asset;
            };

            // A simple lookup table that maps the image mip slice to a mip chain asset.
            AZStd::array<uint16_t, RHI::Limits::Image::MipCountMax> m_mipLevelToChainIndex;

            // A flat list of mip chains, which combine to form the complete mip chain of the parent image.
            // The tail mip chain asset reference is empty since the data is embedded in m_tailMipChain.
            AZStd::fixed_vector<MipChain, RHI::Limits::Image::MipCountMax> m_mipChains;

            // The tail mip chain data which is embedded in this StreamingImageAsset
            // The tail mip chain is required at initialization time. This is so the pool can initialize the RHI image with valid,
            // albeit low-resolution, content.
            ImageMipChainAsset m_tailMipChain;

            // The asset id of the streaming image pool to use when initializing a streaming image.
            // Note: this asset id is optional and it's not part of serialization context.
            // It is a member so we have the ability to assign it at runtime to select the preferred pool for image initialization
            Data::AssetId m_poolAssetId;

            uint32_t m_totalImageDataSize = 0;

            StreamingImageFlags m_flags = StreamingImageFlags::None;

            // Cached value of the average color of this image (alpha-weighted average in case of 4-component images)
            AZ::Color m_averageColor = AZ::Color(AZStd::numeric_limits<float>::quiet_NaN());

            //! Helper method for retrieving the ImageMipChainAsset for a given
            //! mip level. The public method GetMipChainAsset takes in the
            //! mip chain index, not the level. And will fail for any level
            //! that resides in the tail mip chain.
            const ImageMipChainAsset* GetImageMipChainAsset(AZ::u32 mipLevel) const;

            TagList m_tags;
        };
    }
}
