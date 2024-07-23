/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Descriptor.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace DX12
    {
        class Buffer;

        class BufferView final
            : public RHI::DeviceBufferView
        {
            using Base = RHI::DeviceBufferView;
        public:
            AZ_CLASS_ALLOCATOR(BufferView, AZ::ThreadPoolAllocator);
            AZ_RTTI(BufferView, "{F83C1982-68ED-42B8-8A00-E9D7908B2792}", Base);

            static RHI::Ptr<BufferView> Create();

            const Buffer& GetBuffer() const;

            DescriptorHandle GetReadDescriptor() const;
            DescriptorHandle GetReadWriteDescriptor() const;
            DescriptorHandle GetClearDescriptor() const;
            DescriptorHandle GetConstantDescriptor() const;

            GpuVirtualAddress GetGpuAddress() const;
            ID3D12Resource* GetMemory() const;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceBufferView
            uint32_t GetBindlessReadIndex() const override;
            uint32_t GetBindlessReadWriteIndex() const override;
            //////////////////////////////////////////////////////////////////////////

        private:
            BufferView() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceBufferView
            RHI::ResultCode InitInternal(RHI::Device& device, const RHI::DeviceResource& resourceBase) override;
            RHI::ResultCode InvalidateInternal() override;
            void ShutdownInternal() override;
            //////////////////////////////////////////////////////////////////////////

            DescriptorHandle m_readDescriptor;
            DescriptorHandle m_readWriteDescriptor;
            DescriptorHandle m_clearDescriptor;
            DescriptorHandle m_constantDescriptor;
            GpuVirtualAddress m_gpuAddress = 0;

            // The following indicies are offsets to the static descriptor associated with this
            // resource view in the static region of the shader-visible descriptor heap
            DescriptorHandle m_staticReadDescriptor;
            DescriptorHandle m_staticReadWriteDescriptor;
            DescriptorHandle m_staticConstantDescriptor;

            ID3D12Resource* m_memory = nullptr;
        };
    }
}
