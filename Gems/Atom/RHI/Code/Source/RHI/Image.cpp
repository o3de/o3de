/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/ImageView.h>

namespace AZ
{
    namespace RHI
    {
        void Image::SetDescriptor(const ImageDescriptor& descriptor)
        {
            m_descriptor = descriptor;
            m_aspectFlags = GetImageAspectFlags(descriptor.m_format);
        }

        const ImageDescriptor& Image::GetDescriptor() const
        {
            return m_descriptor;
        }

        void Image::GetSubresourceLayout(ImageSubresourceLayoutPlaced& subresourceLayout, ImageAspectFlags aspectFlags) const
        {
            ImageSubresourceRange subresourceRange;
            subresourceRange.m_mipSliceMin = 0;
            subresourceRange.m_mipSliceMax = 0;
            subresourceRange.m_arraySliceMin = 0;
            subresourceRange.m_arraySliceMax = 0;
            subresourceRange.m_aspectFlags = aspectFlags;

            for (auto& [deviceIndex, deviceImage] : m_deviceImages)
            {
                deviceImage->GetSubresourceLayouts(
                    subresourceRange, &subresourceLayout.GetDeviceImageSubresourcePlaced(deviceIndex), nullptr);
            }
        }

        uint32_t Image::GetResidentMipLevel() const
        {
            return m_residentMipLevel;
        }

        Ptr<ImageView> Image::GetImageView(const ImageViewDescriptor& imageViewDescriptor)
        {
            return Base::GetResourceView(imageViewDescriptor);
        }

        ImageAspectFlags Image::GetAspectFlags() const
        {
            return m_aspectFlags;
        }

        const HashValue64 Image::GetHash() const
        {
            HashValue64 hash = HashValue64{ 0 };
            hash = m_descriptor.GetHash();
            hash = TypeHash64(m_supportedQueueMask, hash);
            hash = TypeHash64(m_residentMipLevel, hash);
            hash = TypeHash64(m_aspectFlags, hash);
            return hash;
        }
    }
}
