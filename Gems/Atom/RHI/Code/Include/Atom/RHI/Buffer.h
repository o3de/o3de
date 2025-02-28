/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/Resource.h>

namespace AZ::RHI
{
    class BufferFrameAttachment;
    struct BufferViewDescriptor;
    class BufferView;

    //! A Buffer holds all Buffers across multiple devices.
    //! The buffer descriptor will be shared across all the buffers.
    //! The user manages the lifecycle of a Buffer through a BufferPool
    class Buffer : public Resource
    {
        using Base = Resource;
        friend class BufferPoolBase;
        friend class BufferPool;
        friend class RayTracingTlas;
        friend class TransientAttachmentPool;

    public:
        AZ_CLASS_ALLOCATOR(Buffer, AZ::SystemAllocator, 0);
        AZ_RTTI(Buffer, "{8B8A544D-7819-4677-9C47-943B821DE619}", Resource);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(Buffer);
        Buffer() = default;
        virtual ~Buffer() = default;

        const BufferDescriptor& GetDescriptor() const;

        //! Returns the buffer frame attachment if the buffer is currently attached.
        const BufferFrameAttachment* GetFrameAttachment() const;

        Ptr<BufferView> BuildBufferView(const BufferViewDescriptor& bufferViewDescriptor);

        //! Get the hash associated with the Buffer
        const HashValue64 GetHash() const;

        //! Shuts down the resource by detaching it from its parent pool.
        void Shutdown() override final;

    protected:
        void SetDescriptor(const BufferDescriptor& descriptor);

    private:
        void Invalidate();

        //! The RHI descriptor for this Buffer.
        BufferDescriptor m_descriptor;
    };
} // namespace AZ::RHI
