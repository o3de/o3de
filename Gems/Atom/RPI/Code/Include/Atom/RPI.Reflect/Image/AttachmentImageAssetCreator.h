/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/AssetCreator.h>

namespace AZ
{
    namespace RPI
    {
        //! The class to create an attachment image asset
        class AttachmentImageAssetCreator
            : public AssetCreator<AttachmentImageAsset>
        {
        public:
            //! Begins construction of a new attachment image asset instance. Resets the builder to a fresh state.
            //! assetId the unique id to use when creating the asset.
            void Begin(const Data::AssetId& assetId);

            //! Assigns the default image descriptor. Required.
            void SetImageDescriptor(const RHI::ImageDescriptor& imageDescriptor);

            //! Assigns the default image view descriptor. Optional
            void SetImageViewDescriptor(const RHI::ImageViewDescriptor& imageViewDescriptor);

            //! Assigns clear value for the image asset. Optional.
            void SetOptimizedClearValue(const RHI::ClearValue& clearValue);

            //! Assigns the attachment image pool, which the runtime attachment image will allocate from. Required.
            void SetPoolAsset(const Data::Asset<ResourcePoolAsset>& poolAsset);

            //! Set a string to asset's hint. This info is only kept for runtime generated asset.
            //! For asset on disc, the asset system will assign asset path to asset hint
            void SetAssetHint(AZStd::string_view hint);

            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<AttachmentImageAsset>& result);
        };
    }
}
