/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/Image.h>

#include <Atom/RPI.Reflect/Image/ImageAsset.h>

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>  // aefimov font hack
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>  // aefimov font hack

#include <Atom/RHI/Factory.h>

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

            auto& factory = RHI::Factory::Get();
            m_image = factory.CreateImage();
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
            RHI::ImagePool* imagePool = azrtti_cast<RHI::ImagePool*> (m_image->GetPool());

            // aefimov font hack to update a streaming image
            if (!imagePool)
            {
                Data::Instance<AZ::RPI::AttachmentImagePool> attachmentImagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
                imagePool = attachmentImagePool->GetRHIPool();
                auto oldPool = m_image->GetPool();
                m_image->SetPool(imagePool);  // to fool the image pool validation
                RHI::ResultCode result = imagePool->UpdateImageContents(request);
                m_image->SetPool(oldPool);
                return result;
            }

            return imagePool->UpdateImageContents(request);
        }     
    }
}
