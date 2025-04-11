/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI/DeviceResource.h>

namespace AZ::RHI
{
    class ImageFrameAttachment;
    class DeviceShaderResourceGroup;
    class DeviceImageView;
    struct ImageViewDescriptor;
        
    //! DeviceImage represents a collection of DeviceImage Subresources, where each subresource comprises a one to three
    //! dimensional grid of pixels. Images are divided into an array of mip-map chains. A mip map chain is
    //! a list of subresources, progressively halved on each axis, down to a 1x1 pixel base image. If an array is used,
    //! each array 'slice' is its own mip chain. All mip chains in an array share the same size.
    //!
    //! Subresources are organized by a linear indexing scheme: mipSliceOffset + arraySliceOffset * arraySize. The total
    //! number of subresources is equal to mipLevels * arraySize. All subresources share the same pixel format.
    //!
    //! @see DeviceImageView on how to interpret contents of an image.
    class DeviceImage
        : public DeviceResource
    {
        friend class DeviceImagePoolBase;
        friend class DeviceStreamingImagePool;
            
        using Base = DeviceResource;
    public:
        AZ_RTTI(DeviceImage, "{39FFE66C-805A-41AD-9092-91327D51F64B}", DeviceResource);
        virtual ~DeviceImage() = default;
            
        //! Returns the image descriptor used to initialize the image. If the image is uninitialized, the contents
        //! are considered undefined.
        const ImageDescriptor& GetDescriptor() const;
            
        //! Computes the subresource layouts and total size of the image contents, if represented linearly. Effectively,
        //! this data represents how to store the image in a buffer resource. Naturally, if the image contents
        //! are swizzled in device memory, the layouts will differ from the actual physical memory footprint. Use this data
        //! to facilitate transfers between buffers and images.
        //!
        //!  @param subresourceRange The range of subresources in the image to consider when computing subresource layouts.
        //!  @param subresourceLayouts
        //!      [Optional] If specified, fills the provided array with computed subresource layout results. The size of the
        //!      array must be at least the number of subresources specified in the subresource range (number of mip slices *
        //!      number of array slices).
        //!  @param totalSizeInBytes
        //!      [Optional] If specified, will be filled with the total size necessary to contain all subresources.
        void GetSubresourceLayouts(
            const ImageSubresourceRange& subresourceRange,
            DeviceImageSubresourceLayout* subresourceLayouts,
            size_t* totalSizeInBytes) const;
            
        //! This implementation estimates memory usage using the descriptor. Platforms may
        //! override to report more accurate memory usage.
        void ReportMemoryUsage(MemoryStatisticsBuilder& builder) const override;
            
        //! Returns the most detailed mip level currently resident in memory, where a value of 0
        //! is the highest detailed mip.
        uint32_t GetResidentMipLevel() const;
            
        //! Returns the set of queue classes that are supported for usage as an attachment on the frame scheduler.
        //! Effectively, for a scope of a specific hardware class to use the image as an attachment, the queue must
        //! be present in this mask. This does not apply to non-attachment images on the Compute / Graphics queue.
        HardwareQueueClassMask GetSupportedQueueMask() const;
            
        //! Returns the image frame attachment if the image is currently attached. This is assigned when the image
        //! is imported into the frame scheduler (which is reset every frame). This value will be null for non-attachment
        //! images.
        const ImageFrameAttachment* GetFrameAttachment() const;
            
        Ptr<DeviceImageView> GetImageView(const ImageViewDescriptor& imageViewDescriptor);

        //! Returns the aspects that are included in the image.
        ImageAspectFlags GetAspectFlags() const;

        //! Get the hash associated with the passed image descriptor
        const HashValue64 GetHash() const;

        //! Returns whether the image has sub-resources which can be evicted from or streamed into the device memory
        bool IsStreamable() const;

    protected:
        DeviceImage() = default;

        virtual void SetDescriptor(const ImageDescriptor& descriptor);

    private:

        ///////////////////////////////////////////////////////////////////
        // Platform API

        /// Called by GetSubresourceLayouts. The subresource range is clamped and validated beforehand.
        virtual void GetSubresourceLayoutsInternal(
            const ImageSubresourceRange& subresourceRange,
            DeviceImageSubresourceLayout* subresourceLayouts,
            size_t* totalSizeInBytes) const = 0;

        //! Returns whether the image has sub-resources which can be evicted from or streamed into the device memory
        virtual bool IsStreamableInternal() const { return false;};
        ///////////////////////////////////////////////////////////////////

        /// The RHI descriptor for this image.
        ImageDescriptor m_descriptor;

        /// the set of supported queue classes for this resource.
        HardwareQueueClassMask m_supportedQueueMask = HardwareQueueClassMask::All;

        /// The resident mip level of this image (e.g. 0 being the top, most detailed mip).
        uint32_t m_residentMipLevel = 0;

        /// Aspects supported by the image
        ImageAspectFlags m_aspectFlags = ImageAspectFlags::None;
    };
}
