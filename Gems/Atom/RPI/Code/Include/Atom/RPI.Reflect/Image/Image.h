/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/DeviceImageView.h>

#include <Atom/RPI.Reflect/Configuration.h>

#include <AtomCore/Instance/InstanceData.h>

namespace AZ
{
    namespace RPI
    {
        class ImageAsset;

        //! A base class for images providing access to common image information.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API Image
            : public Data::InstanceData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class ImageSystem;

        public:
            AZ_INSTANCE_DATA(Image, "{4E4B1092-1BEE-4DC4-BE4B-8FBC83B0F48C}");
            AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator);

            virtual ~Image() = default;

            //! Returns whether Image is currently initialized.
            bool IsInitialized() const;

            //! Returns the mutable GPU image instance initialized at asset load time.
            RHI::Image* GetRHIImage();

            //! Returns the immutable GPU image instance initialized at asset load time.
            const RHI::Image* GetRHIImage() const;

            //! Returns the default image view instance, mapping the full (resident) image.
            const RHI::ImageView* GetImageView() const;

            //! Returns the image descriptor which contains some image information
            const RHI::ImageDescriptor& GetDescriptor() const;

            //! Returns the number of mip levels of this image
            uint16_t GetMipLevelCount();
                        
            //! Updates content of a single sub-resource in the image from the CPU.
            virtual RHI::ResultCode UpdateImageContents(const RHI::ImageUpdateRequest& request);

        protected:
            // This is a base class for a derived instance variant.
            Image();

            // The RHI image instance is created at load time. It contains the resident set of mip levels.
            RHI::Ptr<RHI::Image> m_image;

            // The default view instance mapping the full resident set of the image mip levels and array slices.
            RHI::Ptr<RHI::ImageView> m_imageView;
        };
    }
}
