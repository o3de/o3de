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
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/MultiDeviceResource.h>

namespace AZ::RHI
{
    class BufferFrameAttachment;
    struct BufferViewDescriptor;
    class MultiDeviceBufferView;

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

        Ptr<MultiDeviceBufferView> BuildBufferView(const BufferViewDescriptor& bufferViewDescriptor);

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

    //! A MultiDeviceBufferView is a light-weight representation of a view onto a multi-device buffer.
    //! It holds a raw pointer to a multi-device buffer as well as a BufferViewDescriptor object.
    //! Using both, single-device BufferViews can be retrieved.
    class MultiDeviceBufferView : public Object
    {
    public:
        AZ_RTTI(MultiDeviceBufferView, "{CD764967-6AC1-4B9D-9573-498FA61F8F84}", Object);
        virtual ~MultiDeviceBufferView() = default;

        MultiDeviceBufferView(const RHI::MultiDeviceBuffer* buffer, BufferViewDescriptor descriptor)
            : m_buffer{ buffer }
            , m_descriptor{ descriptor }
        {
        }

        //! Given a device index, return the corresponding BufferView for the selected device
        const RHI::Ptr<RHI::BufferView> GetDeviceBufferView(int deviceIndex) const;

        //! Return the contained multi-device buffer
        const RHI::MultiDeviceBuffer* GetBuffer() const
        {
            return m_buffer;
        }

        //! Return the contained BufferViewDescriptor
        const BufferViewDescriptor& GetDescriptor() const
        {
            return m_descriptor;
        }

    private:
        //! A raw pointer to a multi-device buffer
        const RHI::MultiDeviceBuffer* m_buffer;
        //! The corresponding BufferViewDescriptor for this view.
        BufferViewDescriptor m_descriptor;
    };
} // namespace AZ::RHI
