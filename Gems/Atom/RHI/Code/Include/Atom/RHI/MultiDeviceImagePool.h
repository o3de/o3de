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
#include <Atom/RHI/MultiDeviceImage.h>
#include <Atom/RHI/MultiDeviceImagePoolBase.h>
#include <Atom/RHI/ImagePool.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * @brief The data structure used to initialize an RHI::MultiDeviceImage on an RHI::MultiDeviceImagePool.
         */
        struct MultiDeviceImageInitRequest
        {
            MultiDeviceImageInitRequest() = default;

            MultiDeviceImageInitRequest(
                MultiDeviceImage& image, const ImageDescriptor& descriptor, const ClearValue* optimizedClearValue = nullptr);

            /// The image to initialize.
            MultiDeviceImage* m_image = nullptr;

            /// The descriptor used to initialize the image.
            ImageDescriptor m_descriptor;

            /// An optional, optimized clear value for the image. Certain
            /// platforms may use this value to perform fast clears when this
            /// clear value is used.
            const ClearValue* m_optimizedClearValue = nullptr;
        };

        /**
         * @brief The data structure used to update contents of an RHI::MultiDeviceImage on an RHI::MultiDeviceImagePool.
         */
        struct MultiDeviceImageUpdateRequest
        {
            MultiDeviceImageUpdateRequest() = default;

            /// A pointer to an initialized image, whose contents will be updated.
            MultiDeviceImage* m_image = nullptr;

            /// The image subresource to update.
            ImageSubresource m_imageSubresource;

            /// The offset in pixels from the start of the sub-resource in the destination image.
            Origin m_imageSubresourcePixelOffset;

            /// The source data pointer
            const void* m_sourceData = nullptr;

            /// The source sub-resource layout.
            MultiDeviceImageSubresourceLayout m_sourceSubresourceLayout;
        };

        /**
         * MultiDeviceImagePool is a pool of images that will be bound as attachments to the frame scheduler.
         * As a result, they are intended to be produced and consumed by the GPU. Persistent Color / Depth Stencil / Image
         * attachments should be created from this pool. This pool is not designed for intra-frame aliasing.
         * If transient images are required, they can be created from the frame scheduler itself.
         */
        class MultiDeviceImagePool : public MultiDeviceImagePoolBase
        {
        public:
            AZ_CLASS_ALLOCATOR(MultiDeviceImagePool, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDeviceImagePool, "{11D804D0-8332-490B-8A3E-BE279FCEFB8E}", MultiDeviceImagePoolBase);
            MultiDeviceImagePool() = default;
            virtual ~MultiDeviceImagePool() = default;

            inline const Ptr<ImagePool>& GetDeviceImagePool(int deviceIndex) const
            {
                AZ_Error(
                    "MultiDeviceImagePool",
                    m_deviceImagePools.find(deviceIndex) != m_deviceImagePools.end(),
                    "No DeviceImagePool found for device index %d\n",
                    deviceIndex);
                return m_deviceImagePools.at(deviceIndex);
            }

            /// Initializes the pool. The pool must be initialized before images can be registered with it.
            ResultCode Init(MultiDevice::DeviceMask deviceMask, const ImagePoolDescriptor& descriptor);

            /// Initializes an image onto the pool. The pool provides backing GPU resources to the image.
            ResultCode InitImage(const MultiDeviceImageInitRequest& request);

            /// Updates image content from the CPU.
            ResultCode UpdateImageContents(const MultiDeviceImageUpdateRequest& request);

            /// Returns the descriptor used to initialize the pool.
            const ImagePoolDescriptor& GetDescriptor() const override final;

            void Shutdown() override final;

        private:
            using MultiDeviceImagePoolBase::InitImage;
            using MultiDeviceResourcePool::Init;

            bool ValidateUpdateRequest(const MultiDeviceImageUpdateRequest& updateRequest) const;

            ImagePoolDescriptor m_descriptor;

            AZStd::unordered_map<int, Ptr<ImagePool>> m_deviceImagePools;
        };
    } // namespace RHI
} // namespace AZ
