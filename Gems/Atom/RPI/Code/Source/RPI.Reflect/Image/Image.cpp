/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/Image.h>

#include <Atom/RPI.Reflect/Image/ImageAsset.h>

namespace AZ
{
    namespace RPI
    {
        Image::Image()
        {
            /**
             * Image views are persistently initialized on their parent image, and
             * shader resource groups hold image view references. If we re-create the image
             * view instance entirely, that will not automatically propagate to dependent
             * shader resource groups.
             *
             * Image views remain valid when their host image shuts down and re-initializes
             * (it will force a rebuild), so the best course of action is to keep a persistent
             * pointer around at all times, and then only initialize the image view once.
             */

            m_image = aznew RHI::Image;
            AZ_Assert(m_image, "Failed to acquire an image instance from the RHI. Is the RHI initialized?");
        }

        bool Image::IsInitialized() const
        {
            return m_image->IsInitialized();
        }

        RHI::Image* Image::GetRHIImage()
        {
            return m_image.get();
        }

        const RHI::Image* Image::GetRHIImage() const
        {
            return m_image.get();
        }

        const RHI::ImageView* Image::GetImageView() const
        {
            return m_imageView.get();
        }

        const RHI::ImageDescriptor& Image::GetDescriptor() const
        {
            return m_image->GetDescriptor();
        }

        uint16_t Image::GetMipLevelCount()
        {
            return m_image->GetDescriptor().m_mipLevels;
        }

        RHI::ResultCode Image::UpdateImageContents(const RHI::ImageUpdateRequest& request)
        {
            RHI::ImagePool* imagePool = azrtti_cast<RHI::ImagePool*>(m_image->GetPool());
            return imagePool->UpdateImageContents(request);
        }     
    }
}
