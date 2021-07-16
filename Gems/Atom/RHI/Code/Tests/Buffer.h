/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/BufferView.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    class BufferView
        : public AZ::RHI::BufferView
    {
    public:
        AZ_CLASS_ALLOCATOR(BufferView, AZ::SystemAllocator, 0);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device& device, const AZ::RHI::Resource&) override;
        AZ::RHI::ResultCode InvalidateInternal() override;
        void ShutdownInternal() override;
    };

    class BufferPool;

    class Buffer
        : public AZ::RHI::Buffer
    {
        friend class BufferPool;
    public:
        AZ_CLASS_ALLOCATOR(Buffer, AZ::SystemAllocator, 0);

        bool IsMapped() const;

        void* Map();

        void Unmap();

        const AZStd::vector<uint8_t>& GetData() const;

    private:
        bool m_isMapped = false;
        AZStd::vector<uint8_t> m_data;
    };

    class BufferPool
        : public AZ::RHI::BufferPool
    {
    public:
        AZ_CLASS_ALLOCATOR(BufferPool, AZ::SystemAllocator, 0);

    private:
        AZ::RHI::ResultCode InitInternal(AZ::RHI::Device& device, const AZ::RHI::BufferPoolDescriptor& descriptor) override;

        void ShutdownInternal() override;

        AZ::RHI::ResultCode InitBufferInternal(AZ::RHI::Buffer& bufferBase, const AZ::RHI::BufferDescriptor& descriptor) override;

        void ShutdownResourceInternal(AZ::RHI::Resource&) override;

        AZ::RHI::ResultCode OrphanBufferInternal(AZ::RHI::Buffer& buffer) override;

        AZ::RHI::ResultCode MapBufferInternal(const AZ::RHI::BufferMapRequest& request, AZ::RHI::BufferMapResponse& response) override;

        void UnmapBufferInternal(AZ::RHI::Buffer& bufferBase) override;

        AZ::RHI::ResultCode StreamBufferInternal(const AZ::RHI::BufferStreamRequest& request) override;
    };
}
