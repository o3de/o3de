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
#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/MultiDeviceImage.h>
#include <Atom/RHI/MultiDeviceImagePoolBase.h>

namespace AZ::RHI
{
    using MultiDeviceImageInitRequest = ImageInitRequestTemplate<MultiDeviceImage>;
    using MultiDeviceImageUpdateRequest = ImageUpdateRequestTemplate<MultiDeviceImage, MultiDeviceImageSubresourceLayout>;

    //! MultiDeviceImagePool is a pool of images that will be bound as attachments to the frame scheduler.
    //! As a result, they are intended to be produced and consumed by the GPU. Persistent Color / Depth Stencil / Image
    //! attachments should be created from this pool. This pool is not designed for intra-frame aliasing.
    //! If transient images are required, they can be created from the frame scheduler itself.
    class MultiDeviceImagePool : public MultiDeviceImagePoolBase
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceImagePool, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceImagePool, "{11D804D0-8332-490B-8A3E-BE279FCEFB8E}", MultiDeviceImagePoolBase);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(ImagePool);
        MultiDeviceImagePool() = default;
        virtual ~MultiDeviceImagePool() = default;

        //! Initializes the pool. The pool must be initialized before images can be registered with it.
        ResultCode Init(MultiDevice::DeviceMask deviceMask, const ImagePoolDescriptor& descriptor);

        //! Initializes an image onto the pool. The pool provides backing GPU resources to the image.
        ResultCode InitImage(const MultiDeviceImageInitRequest& request);

        //! Updates image content from the CPU.
        ResultCode UpdateImageContents(const MultiDeviceImageUpdateRequest& request);

        //! Returns the descriptor used to initialize the pool.
        const ImagePoolDescriptor& GetDescriptor() const override final;

        void Shutdown() override final;

    private:
        using MultiDeviceImagePoolBase::InitImage;
        using MultiDeviceResourcePool::Init;

        bool ValidateUpdateRequest(const MultiDeviceImageUpdateRequest& updateRequest) const;

        ImagePoolDescriptor m_descriptor;
    };
} // namespace AZ::RHI
