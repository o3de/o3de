/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/ImageSubresourceLayoutPlaced.h>
#include <Atom/RHI/Resource.h>

namespace AZ
{
    namespace RHI
    {
        class ShaderResourceGroup;
        class ImageView;
        struct ImageViewDescriptor;
        
         //! Image represents a collection of Image Subresources, where each subresource comprises a one to three
         //! dimensional grid of pixels. Images are divided into an array of mip-map chains. A mip map chain is
         //! a list of subresources, progressively halved on each axis, down to a 1x1 pixel base image. If an array is used,
         //! each array 'slice' is its own mip chain. All mip chains in an array share the same size.
         //!
         //! Subresources are organized by a linear indexing scheme: mipSliceOffset + arraySliceOffset * arraySize. The total
         //! number of subresources is equal to mipLevels * arraySize. All subresources share the same pixel format.
         //!
         //! @see ImageView on how to interpret contents of an image.
        class Image : public Resource
        {
            friend class ImagePool;
            friend class ImageView;
            friend class ImagePoolBase;
            friend class StreamingImagePool;
            friend class SwapChain;
            friend class TransientAttachmentPool;

            using Base = Resource;

        public:
            // Using SystemAllocator here instead of ThreadPoolAllocator as it gets slower when
            // we create thousands of buffer views related to SRGs
            AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator, 0);
            AZ_RTTI(Image, "{39FFE66C-805A-41AD-9092-91327D51F64B}", Resource);
            Image() = default;
            virtual ~Image() = default;

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
            void GetSubresourceLayout(
                ImageSubresourceLayoutPlaced& subresourceLayout, ImageAspectFlags aspectFlags = ImageAspectFlags::All) const;

            inline Ptr<DeviceImage> GetDeviceImage(int deviceIndex) const
            {
                AZ_Assert(
                    m_deviceImages.find(deviceIndex) != m_deviceImages.end(), "No DeviceImage found for device index %d\n", deviceIndex);
                return m_deviceImages.at(deviceIndex);
            }

            //! Returns the most detailed mip level currently resident in memory, where a value of 0
            //! is the highest detailed mip.
            uint32_t GetResidentMipLevel() const;
            
             //! Returns the set of queue classes that are supported for usage as an attachment on the frame scheduler.
             //! Effectively, for a scope of a specific hardware class to use the image as an attachment, the queue must
             //! be present in this mask. This does not apply to non-attachment images on the Compute / Graphics queue.
            HardwareQueueClassMask GetSupportedQueueMask() const;

            Ptr<ImageView> GetImageView(const ImageViewDescriptor& imageViewDescriptor);

            //! Returns the aspects that are included in the image.
            ImageAspectFlags GetAspectFlags() const;

            //! Get the hash associated with the passed view descriptorimage
            const HashValue64 GetHash() const;

        protected:
            virtual void SetDescriptor(const ImageDescriptor& descriptor);

        private:
            /// The RHI descriptor for this image.
            ImageDescriptor m_descriptor;

            /// the set of supported queue classes for this resource.
            HardwareQueueClassMask m_supportedQueueMask = HardwareQueueClassMask::All;

            /// The resident mip level of this image (e.g. 0 being the top, most detailed mip).
            uint32_t m_residentMipLevel = 0;

            /// Aspects supported by the image
            ImageAspectFlags m_aspectFlags = ImageAspectFlags::None;

            AZStd::unordered_map<int, Ptr<DeviceImage>> m_deviceImages;
        };
    } // namespace RHI
}
