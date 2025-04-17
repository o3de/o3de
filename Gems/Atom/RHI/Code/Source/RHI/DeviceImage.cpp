/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>

namespace AZ::RHI
{
    void DeviceImage::SetDescriptor(const ImageDescriptor& descriptor)
    {
        m_descriptor = descriptor;
        m_aspectFlags = GetImageAspectFlags(descriptor.m_format);
    }

    const ImageDescriptor& DeviceImage::GetDescriptor() const
    {
        return m_descriptor;
    }
    
    void DeviceImage::GetSubresourceLayouts(
        const ImageSubresourceRange& subresourceRange,
        DeviceImageSubresourceLayout* subresourceLayouts,
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

    uint32_t DeviceImage::GetResidentMipLevel() const
    {
        return m_residentMipLevel;
    }

    const ImageFrameAttachment* DeviceImage::GetFrameAttachment() const
    {
        return static_cast<const ImageFrameAttachment*>(DeviceResource::GetFrameAttachment());
    }

    void DeviceImage::ReportMemoryUsage(MemoryStatisticsBuilder& builder) const
    {
        const ImageDescriptor& descriptor = GetDescriptor();

        MemoryStatistics::Image* imageStats = builder.AddImage();
        imageStats->m_name = GetName();
        imageStats->m_bindFlags = descriptor.m_bindFlags;

        ImageSubresourceRange subresourceRange;
        subresourceRange.m_mipSliceMin = static_cast<uint16_t>(GetResidentMipLevel());
        GetSubresourceLayouts(subresourceRange, nullptr, &imageStats->m_sizeInBytes);
        imageStats->m_minimumSizeInBytes = imageStats->m_minimumSizeInBytes;
    }
    
    Ptr<DeviceImageView> DeviceImage::GetImageView(const ImageViewDescriptor& imageViewDescriptor)
    {
        return Base::GetResourceView(imageViewDescriptor);
    }

    ImageAspectFlags DeviceImage::GetAspectFlags() const
    {
        return m_aspectFlags;
    }

    const HashValue64 DeviceImage::GetHash() const
    {
        HashValue64 hash = HashValue64{ 0 };
        hash = m_descriptor.GetHash();
        hash = TypeHash64(m_supportedQueueMask, hash);
        hash = TypeHash64(m_residentMipLevel, hash);
        hash = TypeHash64(m_aspectFlags, hash);
        return hash;
    }

    bool DeviceImage::IsStreamable() const
    {
        return IsStreamableInternal();
    }
}
