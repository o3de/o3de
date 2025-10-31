/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

namespace AZ
{
    namespace RPI
    {
        //! Constructs an instance of an StreamingImageAsset.
        //! (Note this class generally follows the builder design pattern, but is called a "creator" rather 
        //! than a "builder" to avoid confusion with the AssetBuilderSDK).
        class ATOM_RPI_REFLECT_API StreamingImageAssetCreator
            : public AssetCreator<StreamingImageAsset>
        {
        public:
            StreamingImageAssetCreator() = default;

            //! Begins construction of a new streaming image asset instance. Resets the builder to a fresh state.
            //! assetId the unique id to use when creating the asset.
            void Begin(const Data::AssetId& assetId);

            //! Assigns the default image descriptor.
            void SetImageDescriptor(const RHI::ImageDescriptor& imageDescriptor);

            //! Assigns the default image view descriptor.
            void SetImageViewDescriptor(const RHI::ImageViewDescriptor& imageViewDescriptor);

            //! Adds a mip chain asset to the image. Mip chains stack, starting from the most detailed to the least.
            //! @param mipChainAsset the mip chain data to add.
            void AddMipChainAsset(ImageMipChainAsset& mipChainAsset);

            //! Assigns asset id of the streaming image pool, which the runtime streaming image will allocate from.
            //! Note: the pool asset id won't be serialized but it's useful when creating streaming image from data in memory
            void SetPoolAssetId(const Data::AssetId& poolAssetId);

            //! Set streaming image asset's flags.
            void SetFlags(StreamingImageFlags flag);

            //! Set the average color of the image.
            void SetAverageColor(Color avgColor);

            void AddTag(AZ::Name tag);

            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<StreamingImageAsset>& result);

        private:
            uint16_t m_mipLevels = 0;
        };
    }
}
