/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/MultiDeviceResource.h>

namespace AZ::RHI
{
    class BufferFrameAttachment;
    struct BufferViewDescriptor;

    //! A MultiDeviceBuffer holds all Buffers across multiple devices.
    //! The buffer descriptor will be shared across all the buffers.
    //! The user manages the lifecycle of a MultiDeviceBuffer through a MultiDeviceBufferPool
    class MultiDeviceBuffer : public MultiDeviceResource
    {
        using Base = MultiDeviceResource;
        friend class MultiDeviceBufferPoolBase;
        friend class MultiDeviceBufferPool;
        friend class MultiDeviceRayTracingTlas;
        friend class MultiDeviceTransientAttachmentPool;

    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceBuffer, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceBuffer, "{8B8A544D-7819-4677-9C47-943B821DE619}", MultiDeviceResource);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(Buffer);
        MultiDeviceBuffer() = default;
        virtual ~MultiDeviceBuffer() = default;

        const BufferDescriptor& GetDescriptor() const;

        //! Returns the buffer frame attachment if the buffer is currently attached.
        const BufferFrameAttachment* GetFrameAttachment() const;

        //! Get the hash associated with the MultiDeviceBuffer
        const HashValue64 GetHash() const;

        //! Shuts down the resource by detaching it from its parent pool.
        void Shutdown() override final;

        void InvalidateViews() override final;

    protected:
        void SetDescriptor(const BufferDescriptor& descriptor);

    private:
        void Invalidate();

        //! The RHI descriptor for this MultiDeviceBuffer.
        BufferDescriptor m_descriptor;
    };
} // namespace AZ::RHI
