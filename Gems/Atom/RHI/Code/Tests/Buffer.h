/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/RHI/SingleDeviceBufferPool.h>
#include <Atom/RHI/SingleDeviceBufferView.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class BufferView
        : public AZ::RHI::SingleDeviceBufferView
    {
    public:
        AZ_CLASS_ALLOCATOR(BufferView, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device& device, const AZ::RHI::SingleDeviceResource&) override;
        AZ::RHI::ResultCode InvalidateInternal() override;
        void ShutdownInternal() override;
    };

    class BufferPool;

    class Buffer
        : public AZ::RHI::SingleDeviceBuffer
    {
        friend class BufferPool;
    public:
        AZ_CLASS_ALLOCATOR(Buffer, AZ::SystemAllocator);

        bool IsMapped() const;

        void* Map();

        void Unmap();

        const AZStd::vector<uint8_t>& GetData() const;

    private:
        bool m_isMapped = false;
        AZStd::vector<uint8_t> m_data;
    };

    class BufferPool
        : public AZ::RHI::SingleDeviceBufferPool
    {
    public:
        AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device& device, const AZ::RHI::BufferPoolDescriptor& descriptor) override;

        void ShutdownInternal() override;

        AZ::RHI::ResultCode InitBufferInternal(AZ::RHI::SingleDeviceBuffer& bufferBase, const AZ::RHI::BufferDescriptor& descriptor) override;

        void ShutdownResourceInternal(AZ::RHI::SingleDeviceResource&) override;

        AZ::RHI::ResultCode OrphanBufferInternal(AZ::RHI::SingleDeviceBuffer& buffer) override;

        AZ::RHI::ResultCode MapBufferInternal(const AZ::RHI::SingleDeviceBufferMapRequest& request, AZ::RHI::SingleDeviceBufferMapResponse& response) override;

        void UnmapBufferInternal(AZ::RHI::SingleDeviceBuffer& bufferBase) override;

        AZ::RHI::ResultCode StreamBufferInternal(const AZ::RHI::SingleDeviceBufferStreamRequest& request) override;

        void ComputeFragmentation() const override {}
    };
}
