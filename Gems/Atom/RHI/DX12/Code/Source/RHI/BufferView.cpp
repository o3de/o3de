/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/BufferView.h>
#include <RHI/Buffer.h>
#include <RHI/Conversions.h>
#include <RHI/DescriptorContext.h>
#include <RHI/Device.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<BufferView> BufferView::Create()
        {
            return aznew BufferView();
        }

        RHI::ResultCode BufferView::InitInternal(RHI::Device& deviceBase, const RHI::DeviceResource& resourceBase)
        {
            Device& device = static_cast<Device&>(deviceBase);

            const Buffer& buffer = static_cast<const Buffer&>(resourceBase);
            const RHI::BufferViewDescriptor& viewDescriptor = GetDescriptor();
            DescriptorContext& descriptorContext = device.GetDescriptorContext();

            // By default, if no bind flags are specified on the view descriptor, attempt to create all views that are compatible with the underlying buffer's bind flags
            // If bind flags are specified on the view descriptor, only create the views for the specified bind flags.
            bool hasOverrideFlags = viewDescriptor.m_overrideBindFlags != RHI::BufferBindFlags::None;
            const RHI::BufferBindFlags bindFlags = hasOverrideFlags ? viewDescriptor.m_overrideBindFlags : buffer.GetDescriptor().m_bindFlags;

            m_memory = buffer.GetMemoryView().GetMemory();
            m_gpuAddress = buffer.GetMemoryView().GetGpuAddress() + viewDescriptor.m_elementOffset * viewDescriptor.m_elementSize;

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::ShaderRead | RHI::BufferBindFlags::RayTracingAccelerationStructure))
            {
                descriptorContext.CreateShaderResourceView(buffer, viewDescriptor, m_readDescriptor, m_staticReadDescriptor);
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::ShaderWrite))
            {
                descriptorContext.CreateUnorderedAccessView(
                    buffer, viewDescriptor, m_readWriteDescriptor, m_clearDescriptor, m_staticReadWriteDescriptor);
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::Constant))
            {
                descriptorContext.CreateConstantBufferView(buffer, viewDescriptor, m_constantDescriptor, m_staticConstantDescriptor);
            }

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode BufferView::InvalidateInternal()
        {
            return InitInternal(GetDevice(), GetResource());
        }

        void BufferView::ShutdownInternal()
        {
            Device& device = static_cast<Device&>(GetDevice());

            DescriptorContext& descriptorContext = device.GetDescriptorContext();
            descriptorContext.ReleaseDescriptor(m_readDescriptor);
            descriptorContext.ReleaseDescriptor(m_readWriteDescriptor);
            descriptorContext.ReleaseDescriptor(m_clearDescriptor);
            descriptorContext.ReleaseDescriptor(m_constantDescriptor);
            descriptorContext.ReleaseStaticDescriptor(m_staticReadDescriptor);
            descriptorContext.ReleaseStaticDescriptor(m_staticReadWriteDescriptor);
            descriptorContext.ReleaseStaticDescriptor(m_staticConstantDescriptor);
            m_memory = nullptr;
            m_gpuAddress = 0;
        }

        DescriptorHandle BufferView::GetReadDescriptor() const
        {
            return m_readDescriptor;
        }

        DescriptorHandle BufferView::GetReadWriteDescriptor() const
        {
            return m_readWriteDescriptor;
        }

        DescriptorHandle BufferView::GetClearDescriptor() const
        {
            return m_clearDescriptor;
        }

        DescriptorHandle BufferView::GetConstantDescriptor() const
        {
            return m_constantDescriptor;
        }

        GpuVirtualAddress BufferView::GetGpuAddress() const
        {
            return m_gpuAddress;
        }

        ID3D12Resource* BufferView::GetMemory() const
        {
            return m_memory;
        }

        const Buffer& BufferView::GetBuffer() const
        {
            return static_cast<const Buffer&>(Base::GetBuffer());
        }

        uint32_t BufferView::GetBindlessReadIndex() const
        {
            return m_staticReadDescriptor.m_index;
        }

        uint32_t BufferView::GetBindlessReadWriteIndex() const
        {
            return m_staticReadWriteDescriptor.m_index;
        }
    }
}
