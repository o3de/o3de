/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceBuffer.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/parallel/atomic.h>
#include <RHI/BufferMemoryView.h>
#include <RHI/MemoryView.h>

namespace AZ
{
    namespace DX12
    {
        class Device;
        class BufferPool;
        class AliasedHeap;

        class Buffer final : public RHI::DeviceBuffer
        {
            using Base = RHI::DeviceBuffer;

        public:
            AZ_CLASS_ALLOCATOR(Buffer, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(Buffer, "{EFBC5B3C-84BB-43E8-8C68-A44EC30ADC39}", Base);
            ~Buffer() = default;

            static RHI::Ptr<Buffer> Create();

            // Returns the memory view allocated to this buffer.
            const MemoryView& GetMemoryView() const;
            MemoryView& GetMemoryView();

            // The initial state for the graph compiler to use when compiling the resource transition chain.
            D3D12_RESOURCE_STATES m_initialAttachmentState = D3D12_RESOURCE_STATE_COMMON;

            // Override that returns the DX12 device.
            Device& GetDevice();
            const Device& GetDevice() const;

        private:
            Buffer() = default;

            friend class BufferPool;
            friend class AliasedHeap;
            friend class BufferPoolResolver;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Resource
            void ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Buffer
            using Base::SetDescriptor;
            //////////////////////////////////////////////////////////////////////////

            // The buffer memory allocation on the primary heap.
            BufferMemoryView m_memoryView;

            // The number of resolve operations pending for this buffer.
            AZStd::atomic<uint32_t> m_pendingResolves = 0;
        };
    }
}
