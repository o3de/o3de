/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI/SingleDeviceImage.h>
#include <Atom/RHI/SingleDeviceImageView.h>
#include <Atom/RHI/MultiDeviceResource.h>
#include <Atom/RHI/SingleDeviceImage.h>

namespace AZ::RHI
{
    class ImageFrameAttachment;
    class MultiDeviceShaderResourceGroup;
    class MultiDeviceImageView;

    //! MultiDeviceImage represents a collection of MultiDeviceImage Subresources, where each subresource comprises a one to three
    //! dimensional grid of pixels. Images are divided into an array of mip-map chains. A mip map chain is
    //! a list of subresources, progressively halved on each axis, down to a 1x1 pixel base image. If an array is used,
    //! each array 'slice' is its own mip chain. All mip chains in an array share the same size.
    //!
    //! Subresources are organized by a linear indexing scheme: mipSliceOffset + arraySliceOffset * arraySize. The total
    //! number of subresources is equal to mipLevels * arraySize. All subresources share the same pixel format.
    //!
    //! @see SingleDeviceImageView on how to interpret contents of an image.
    class MultiDeviceImage : public MultiDeviceResource
    {
        friend class MultiDeviceImagePoolBase;
        friend class MultiDeviceImagePool;
        friend class MultiDeviceTransientAttachmentPool;
        friend class MultiDeviceStreamingImagePool;
        friend class MultiDeviceSwapChain;

        using Base = MultiDeviceResource;

    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceImage, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceImage, "{39FFE66C-805A-41AD-9092-91327D51F64B}", MultiDeviceResource);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(Image);
        MultiDeviceImage() = default;
        virtual ~MultiDeviceImage() = default;

        //! Returns the image descriptor used to initialize the image. If the image is uninitialized, the contents
        //! are considered undefined.
        const ImageDescriptor& GetDescriptor() const;

        //! Returns the multi-device SingleDeviceImageView
        Ptr<MultiDeviceImageView> BuildImageView(const ImageViewDescriptor& imageViewDescriptor);

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
        void GetSubresourceLayout(MultiDeviceImageSubresourceLayout& subresourceLayout, ImageAspectFlags aspectFlags = ImageAspectFlags::All) const;

        //! Returns the set of queue classes that are supported for usage as an attachment on the frame scheduler.
        //! Effectively, for a scope of a specific hardware class to use the image as an attachment, the queue must
        //! be present in this mask. This does not apply to non-attachment images on the Compute / Graphics queue.
        HardwareQueueClassMask GetSupportedQueueMask() const;

        //! Returns the image frame attachment if the image is currently attached. This is assigned when the image
        //! is imported into the frame scheduler (which is reset every frame). This value will be null for non-attachment
        //! images.
        const ImageFrameAttachment* GetFrameAttachment() const;

        //! Returns the most detailed mip level currently resident in memory on any device, where a value of 0
        //! is the highest detailed mip.
        uint32_t GetResidentMipLevel() const;

        //! Returns whether the image has sub-resources which can be evicted from or streamed into the device memory
        bool IsStreamable() const;

        //! Returns the aspects that are included in the image.
        ImageAspectFlags GetAspectFlags() const;

        //! Get the hash associated with the passed image descriptor
        const HashValue64 GetHash() const;

        //! Shuts down the resource by detaching it from its parent pool.
        void Shutdown() override final;

        //! Returns true if the SingleDeviceResourceView is in the cache of all single device images
        bool IsInResourceCache(const ImageViewDescriptor& imageViewDescriptor);

    protected:
        virtual void SetDescriptor(const ImageDescriptor& descriptor);

    private:
        //! The RHI descriptor for this image.
        ImageDescriptor m_descriptor;

        //! the set of supported queue classes for this resource.
        HardwareQueueClassMask m_supportedQueueMask = HardwareQueueClassMask::All;

        //! Aspects supported by the image
        ImageAspectFlags m_aspectFlags = ImageAspectFlags::None;
    };

    //! A MultiDeviceImageView is a light-weight representation of a view onto a multi-device image.
    //! It holds a raw pointer to a multi-device image as well as an ImageViewDescriptor
    //! Using both, single-device ImageViews can be retrieved
    class MultiDeviceImageView : public MultiDeviceResourceView
    {
    public:
        AZ_RTTI(MultiDeviceImageView, "{AB366B8F-F1B7-45C6-A0D8-475D4834FAD2}", MultiDeviceResourceView);
        virtual ~MultiDeviceImageView() = default;

        MultiDeviceImageView(const RHI::MultiDeviceImage* image, ImageViewDescriptor descriptor)
            : m_image{ image }
            , m_descriptor{ descriptor }
        {
        }

        //! Given a device index, return the corresponding SingleDeviceImageView for the selected device
        const RHI::Ptr<RHI::SingleDeviceImageView> GetDeviceImageView(int deviceIndex) const;

        //! Return the contained multi-device image
        const RHI::MultiDeviceImage* GetImage() const
        {
            return m_image.get();
        }

        //! Return the contained ImageViewDescriptor
        const ImageViewDescriptor& GetDescriptor() const
        {
            return m_descriptor;
        }

        const MultiDeviceResource* GetResource() const override
        {
            return m_image.get();
        }

        const SingleDeviceResourceView* GetDeviceResourceView(int deviceIndex) const override
        {
            return GetDeviceImageView(deviceIndex).get();
        }

    private:
        //! Safe-guard access to SingleDeviceImageView cache during parallel access
        mutable AZStd::mutex m_imageViewMutex;
        //! A raw pointer to a multi-device image
        ConstPtr<RHI::MultiDeviceImage> m_image;
        //! The corresponding ImageViewDescriptor for this view.
        ImageViewDescriptor m_descriptor;
        //! SingleDeviceImageView cache
        //! This cache is necessary as the caller receives raw pointers from the ResourceCache,
        //! which now, with multi-device objects in use, need to be held in memory as long as
        //! the multi-device view is held.
        mutable AZStd::unordered_map<int, Ptr<RHI::SingleDeviceImageView>> m_cache;
    };
} // namespace AZ::RHI
