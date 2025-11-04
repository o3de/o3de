/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>

namespace AZ
{
    namespace RPI
    {
        //! This class constructs an instance of an ImageMipChainAsset. 
        //! It is designed to be easy-to-use in order to abstract away details of how data is packed. 
        //! The API exists separately from the asset in order to promote immutability at runtime. 
        //! The builder also does extensive validation to ensure that data is packed properly.
        //! (Note this class generally follows the builder design pattern, but is called a "creator" rather 
        //! than a "builder" to avoid confusion with the AssetBuilderSDK).
        class ATOM_RPI_REFLECT_API ImageMipChainAssetCreator
            : public AssetCreator<ImageMipChainAsset>
        {
        public:
            ImageMipChainAssetCreator() = default;

            //! Begins the build process for a ImageMipChainAsset instance. Resets the builder to a fresh state.
            //! @param mipLevels The number of mip levels in the mip chain.
            //! @param arraySize the number of sub-images within a mip level.
            void Begin(const Data::AssetId& assetId, uint16_t mipLevels, uint16_t arraySize);

            //! Begins construction of a new mip level in the group. The number of mips in the chain must
            //! exactly match mipLevels passed to Begin().
            void BeginMip(const RHI::DeviceImageSubresourceLayout& layout);

            //! Inserts a sub-image into the current mip level. You must call this method for each array element in the mip.
            //! Every mip level must have the same number of array elements matching arraySize passed in Begin().
            void AddSubImage(const void* data, size_t dataSize);

            //! Ends construction of the current mip level. This must be called after adding all sub images.
            void EndMip();
            
            //! Finalizes and assigns ownership of the asset to result, if successful.
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<ImageMipChainAsset>& result);

        private:
            bool IsBuildingMip() const;
            bool ValidateIsBuildingMip();

            uint16_t m_mipLevelsPending = 0;
            uint16_t m_mipLevelsCompleted = 0;
            uint16_t m_arraySlicesCompleted = 0;
            uint16_t m_subImageOffset = 0;
        };
    }
}
