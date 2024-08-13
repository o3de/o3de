/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <Atom/RHI/DeviceImagePool.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImagePoolBase.h>

namespace AZ::RHI
{
    //! @brief The data structure used to update the device mask of an RHI::Image.
    struct ImageDeviceMaskRequest
    {
        ImageDeviceMaskRequest() = default;

        ImageDeviceMaskRequest(
            Image& image, MultiDevice::DeviceMask deviceMask = MultiDevice::AllDevices, const ClearValue* optimizedClearValue = nullptr)
            : m_image{ &image }
            , m_deviceMask{ deviceMask }
            , m_optimizedClearValue{ optimizedClearValue }
        {
        }

        /// The image to initialize.
        Image* m_image = nullptr;

        /// The device mask used for the image.
        /// Note: Only devices in the mask of the image pool will be considered.
        MultiDevice::DeviceMask m_deviceMask = MultiDevice::AllDevices;

        /// An optional, optimized clear value for the image. Certain
        /// platforms may use this value to perform fast clears when this
        /// clear value is used.
        const ClearValue* m_optimizedClearValue = nullptr;
    };

    //! @brief The data structure used to initialize an RHI::Image on an RHI::ImagePool.
    struct ImageInitRequest : public ImageDeviceMaskRequest
    {
        ImageInitRequest() = default;

        ImageInitRequest(
            Image& image,
            const ImageDescriptor& descriptor,
            const ClearValue* optimizedClearValue = nullptr,
            MultiDevice::DeviceMask deviceMask = MultiDevice::AllDevices)
            : ImageDeviceMaskRequest{ image, deviceMask, optimizedClearValue }
            , m_descriptor{ descriptor }
        {
        }

        /// The descriptor used to initialize the image.
        ImageDescriptor m_descriptor;
    };

    using ImageUpdateRequest = ImageUpdateRequestTemplate<Image, ImageSubresourceLayout>;

    //! ImagePool is a pool of images that will be bound as attachments to the frame scheduler.
    //! As a result, they are intended to be produced and consumed by the GPU. Persistent Color / Depth Stencil / Image
    //! attachments should be created from this pool. This pool is not designed for intra-frame aliasing.
    //! If transient images are required, they can be created from the frame scheduler itself.
    class ImagePool : public ImagePoolBase
    {
    public:
        AZ_CLASS_ALLOCATOR(ImagePool, AZ::SystemAllocator, 0);
        AZ_RTTI(ImagePool, "{11D804D0-8332-490B-8A3E-BE279FCEFB8E}", ImagePoolBase);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(ImagePool);
        ImagePool() = default;
        virtual ~ImagePool() = default;

        //! Initializes the pool. The pool must be initialized before images can be registered with it.
        ResultCode Init(const ImagePoolDescriptor& descriptor);

        //! Initializes an image onto the pool. The pool provides backing GPU resources to the image.
        ResultCode InitImage(const ImageInitRequest& request);

        //! Updates the device mask of an image instance created from this pool.
        ResultCode UpdateImageDeviceMask(const ImageDeviceMaskRequest& request);

        //! Updates image content from the CPU.
        ResultCode UpdateImageContents(const ImageUpdateRequest& request);

        //! Returns the descriptor used to initialize the pool.
        const ImagePoolDescriptor& GetDescriptor() const override final;

        void Shutdown() override final;

    private:
        using ImagePoolBase::InitImage;
        using ResourcePool::Init;

        bool ValidateUpdateRequest(const ImageUpdateRequest& updateRequest) const;

        ImagePoolDescriptor m_descriptor;
    };
} // namespace AZ::RHI
