/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI/DeviceResource.h>
#include <Atom/RHI/DeviceBufferView.h>

namespace AZ::RHI
{
    class MemoryStatisticsBuilder;
    class BufferFrameAttachment;
    struct BufferViewDescriptor;
    
    //! A buffer corresponds to a region of linear memory and used for rendering operations. The user
    //! manages the lifecycle of a buffer through a DeviceBufferPool.
    class ATOM_RHI_PUBLIC_API DeviceBuffer
        : public DeviceResource
    {
        using Base = DeviceResource;
        friend class DeviceBufferPoolBase;
    public:
        AZ_RTTI(DeviceBuffer, "{3C918323-F39C-4DC6-BEE9-BC220DBA9414}", DeviceResource);
        virtual ~DeviceBuffer() = default;

        static constexpr uint64_t InvalidDeviceAddress = static_cast<uint64_t>(-1);

        const BufferDescriptor& GetDescriptor() const;
            
        //! This implementation estimates memory usage using the descriptor. Platforms may
        //! override to report more accurate usage metrics.
        void ReportMemoryUsage(MemoryStatisticsBuilder& builder) const override;

        /// Returns the buffer frame attachment if the buffer is currently attached.
        const BufferFrameAttachment* GetFrameAttachment() const;

        Ptr<DeviceBufferView> GetBufferView(const BufferViewDescriptor& bufferViewDescriptor) const;

        // Get the hash associated with the DeviceBuffer
        const HashValue64 GetHash() const;

        virtual uint64_t GetDeviceAddress() const
        {
            return InvalidDeviceAddress;
        }

    protected:
        DeviceBuffer() = default;

        void SetDescriptor(const BufferDescriptor& descriptor);

    private:

        // The RHI descriptor for this buffer.
        BufferDescriptor m_descriptor;

        // A debug reference count to track use of map / unmap operations.
        AZStd::atomic_int m_mapRefCount = {0};
    };
}
