/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Descriptor.h>
#include <Atom/RHI/BufferView.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace DX12
    {
        class Buffer;

        class BufferView final
            : public RHI::BufferView
        {
            using Base = RHI::BufferView;
        public:
            AZ_CLASS_ALLOCATOR(BufferView, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(BufferView, "{F83C1982-68ED-42B8-8A00-E9D7908B2792}", Base);

            static RHI::Ptr<BufferView> Create();

            const Buffer& GetBuffer() const;

            DescriptorHandle GetReadDescriptor() const;
            DescriptorHandle GetReadWriteDescriptor() const;
            DescriptorHandle GetClearDescriptor() const;
            DescriptorHandle GetConstantDescriptor() const;

            GpuVirtualAddress GetGpuAddress() const;
            ID3D12Resource* GetMemory() const;

        private:
            BufferView() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::BufferView
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::Resource& resourceBase) override;
            RHI::ResultCode InvalidateInternal() override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            DescriptorHandle m_readDescriptor;
            DescriptorHandle m_readWriteDescriptor;
            DescriptorHandle m_clearDescriptor;
            DescriptorHandle m_constantDescriptor;
            GpuVirtualAddress m_gpuAddress = 0;
            ID3D12Resource* m_memory = nullptr;
        };
    }
}
