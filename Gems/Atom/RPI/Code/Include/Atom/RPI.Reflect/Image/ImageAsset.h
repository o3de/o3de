/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! The base image asset class. 
        //! Image assets are domain specific (e.g. streaming vs. attachments) so
        //! the details of how to source content for an image is defined by the specialized variant. The base
        //! class provides access to the RHI image and default image view.
        class ImageAsset
            : public Data::AssetData
        {
        public:
            static const char* DisplayName;
            static const char* Group;
            static const char* Extension;

            AZ_RTTI(ImageAsset, "{C53AB73A-5BC9-462D-805B-43BAFA8C8167}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(ImageAsset, AZ::SystemAllocator, 0);

            virtual ~ImageAsset();

            static void Reflect(AZ::ReflectContext* context);

            //! Returns the descriptor for the image.
            const RHI::ImageDescriptor& GetImageDescriptor() const;

            //! Returns the default image view descriptor for the image.
            const RHI::ImageViewDescriptor& GetImageViewDescriptor() const;

        protected:
            // Called by image related asset creators to assign the asset to a ready state.
            void SetReady();
            
            // [Serialized] The descriptor used to initialize the RHI image.
            RHI::ImageDescriptor m_imageDescriptor;

            // [Serialized] The descriptor used to initialize the RHI image view.
            RHI::ImageViewDescriptor m_imageViewDescriptor;
        };

        using ImageAssetHandler = AssetHandler<ImageAsset>;
    }
}
