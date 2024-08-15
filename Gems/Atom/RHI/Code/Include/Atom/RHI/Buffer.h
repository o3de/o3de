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

        //! Returns true if the DeviceResourceView is in the cache of all single device buffers
        bool IsInResourceCache(const BufferViewDescriptor& bufferViewDescriptor);

    protected:
        void SetDescriptor(const BufferDescriptor& descriptor);

    private:
        void Invalidate();

        //! The RHI descriptor for this Buffer.
        BufferDescriptor m_descriptor;
    };

    //! A BufferView is a light-weight representation of a view onto a multi-device buffer.
    //! It holds a raw pointer to a multi-device buffer as well as a BufferViewDescriptor
    //! Using both, single-device BufferViews can be retrieved
    class BufferView : public ResourceView
    {
    public:
        AZ_RTTI(BufferView, "{AB366B8F-F1B7-45C6-A0D8-475D4834FAD2}", ResourceView);
        virtual ~BufferView() = default;

        BufferView(const RHI::Buffer* buffer, BufferViewDescriptor descriptor, MultiDevice::DeviceMask deviceMask)
            : m_buffer{ buffer }
            , m_descriptor{ descriptor }
            , m_deviceMask{ deviceMask }
        {
        }

        //! Given a device index, return the corresponding DeviceBufferView for the selected device
        const RHI::Ptr<RHI::DeviceBufferView> GetDeviceBufferView(int deviceIndex) const;

        //! Return the contained multi-device buffer
        const RHI::Buffer* GetBuffer() const
        {
            return m_buffer.get();
        }

        //! Return the contained BufferViewDescriptor
        const BufferViewDescriptor& GetDescriptor() const
        {
            return m_descriptor;
        }

        AZStd::unordered_map<int, uint32_t> GetBindlessReadIndex() const;

        const Resource* GetResource() const override
        {
            return m_buffer.get();
        }

        const DeviceResourceView* GetDeviceResourceView(int deviceIndex) const override
        {
            return GetDeviceBufferView(deviceIndex).get();
        }

    private:
        //! Safe-guard access to DeviceBufferView cache during parallel access
        mutable AZStd::mutex m_bufferViewMutex;
        //! A raw pointer to a multi-device buffer
        ConstPtr<RHI::Buffer> m_buffer;
        //! The corresponding BufferViewDescriptor for this view.
        BufferViewDescriptor m_descriptor;
        //! The device mask of the buffer stored for comparison to figure out when cache entries need to be freed.
        mutable MultiDevice::DeviceMask m_deviceMask = MultiDevice::AllDevices;
        //! DeviceBufferView cache
        //! This cache is necessary as the caller receives raw pointers from the ResourceCache, 
        //! which now, with multi-device objects in use, need to be held in memory as long as
        //! the multi-device view is held.
        mutable AZStd::unordered_map<int, Ptr<RHI::DeviceBufferView>> m_cache;
    };
} // namespace AZ::RHI
