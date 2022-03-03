/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>

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
    
        void Image::GetSubresourceLayouts(
            const ImageSubresourceRange& subresourceRange,
            ImageSubresourceLayoutPlaced* subresourceLayouts,
            size_t* totalSizeInBytes) const
        {
            const RHI::ImageDescriptor& imageDescriptor = GetDescriptor();

            ImageSubresourceRange subresourceRangeClamped;
            subresourceRangeClamped.m_mipSliceMin = subresourceRange.m_mipSliceMin;
            subresourceRangeClamped.m_mipSliceMax = AZStd::clamp<uint16_t>(subresourceRange.m_mipSliceMax, subresourceRange.m_mipSliceMin, imageDescriptor.m_mipLevels - 1);
            subresourceRangeClamped.m_arraySliceMin = subresourceRange.m_arraySliceMin;
            subresourceRangeClamped.m_arraySliceMax = AZStd::clamp<uint16_t>(subresourceRange.m_arraySliceMax, subresourceRange.m_arraySliceMin, imageDescriptor.m_arraySize - 1);
            GetSubresourceLayoutsInternal(subresourceRangeClamped, subresourceLayouts, totalSizeInBytes);
        }

        uint32_t Image::GetResidentMipLevel() const
        {
            return m_residentMipLevel;
        }

        const ImageFrameAttachment* Image::GetFrameAttachment() const
        {
            return static_cast<const ImageFrameAttachment*>(Resource::GetFrameAttachment());
        }

        void Image::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
        {
            const ImageDescriptor& descriptor = GetDescriptor();

            MemoryStatistics::Image* imageStats = builder.AddImage();
            imageStats->m_name = GetName();
            imageStats->m_bindFlags = descriptor.m_bindFlags;

            ImageSubresourceRange subresourceRange;
            subresourceRange.m_mipSliceMin = static_cast<uint16_t>(GetResidentMipLevel());
            GetSubresourceLayouts(subresourceRange, nullptr, &imageStats->m_sizeInBytes);
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
