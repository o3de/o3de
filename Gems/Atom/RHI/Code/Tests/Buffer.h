/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class BufferView
        : public AZ::RHI::DeviceBufferView
    {
    public:
        AZ_CLASS_ALLOCATOR(BufferView, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device& device, const AZ::RHI::DeviceResource&) override;
        AZ::RHI::ResultCode InvalidateInternal() override;
        void ShutdownInternal() override;
    };

    class BufferPool;

    class Buffer
        : public AZ::RHI::DeviceBuffer
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
        : public AZ::RHI::DeviceBufferPool
    {
    public:
        AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device& device, const AZ::RHI::BufferPoolDescriptor& descriptor) override;

        void ShutdownInternal() override;

        AZ::RHI::ResultCode InitBufferInternal(AZ::RHI::DeviceBuffer& bufferBase, const AZ::RHI::BufferDescriptor& descriptor) override;

        void ShutdownResourceInternal(AZ::RHI::DeviceResource&) override;

        AZ::RHI::ResultCode OrphanBufferInternal(AZ::RHI::DeviceBuffer& buffer) override;

        AZ::RHI::ResultCode MapBufferInternal(const AZ::RHI::DeviceBufferMapRequest& request, AZ::RHI::DeviceBufferMapResponse& response) override;

        void UnmapBufferInternal(AZ::RHI::DeviceBuffer& bufferBase) override;

        AZ::RHI::ResultCode StreamBufferInternal(const AZ::RHI::DeviceBufferStreamRequest& request) override;

        void ComputeFragmentation() const override {}
    };
}
