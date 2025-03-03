/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 #pragma once

 #include <Atom/RHI.Reflect/BufferViewDescriptor.h>
 #include <Atom/RHI/Resource.h>
 #include <Atom/RHI/Buffer.h>

 namespace AZ::RHI
{
    class DeviceBuffer;
    class DeviceBufferView;

    //! A BufferView is a light-weight representation of a view onto a multi-device buffer.
    //! It holds a ConstPtr to a multi-device buffer as well as a BufferViewDescriptor
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
        //! From RHI::Object
        void Shutdown() final;

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
}