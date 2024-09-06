/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Buffer.h>
#include <RHI/Conversions.h>
#include <RHI/Device.h>

namespace AZ::WebGPU
{
    RHI::Ptr<Buffer> Buffer::Create()
    {
        return aznew Buffer();
    }

    const wgpu::Buffer& Buffer::GetNativeBuffer() const
    {
        return m_wgpuBuffer;
    }

    bool Buffer::CanBeMap() const
    {
        return m_wgpuBufferUsage & (wgpu::BufferUsage::MapRead | wgpu::BufferUsage::MapWrite);
    }

    RHI::ResultCode Buffer::Init(Device& device, const RHI::BufferDescriptor& bufferDescriptorBase, InitFlags initFlags)
    {
        wgpu::BufferDescriptor descriptor = {};
        descriptor.size = bufferDescriptorBase.m_byteCount;
        descriptor.usage = ConvertBufferBindFlags(bufferDescriptorBase.m_bindFlags);
        descriptor.label = GetName().GetCStr();
        if (RHI::CheckBitsAll(initFlags, InitFlags::MapRead))
        {
            descriptor.usage |= wgpu::BufferUsage::MapRead;
        }
        if (RHI::CheckBitsAll(initFlags, InitFlags::MapWrite))
        {
            descriptor.usage |= wgpu::BufferUsage::MapWrite;
        }
        if (RHI::CheckBitsAll(initFlags, InitFlags::MappedAtCreation))
        {
            descriptor.mappedAtCreation = true;
        }
        m_wgpuBuffer = device.GetNativeDevice().CreateBuffer(&descriptor);
        m_wgpuBufferUsage = descriptor.usage;
        AZ_Assert(m_wgpuBuffer, "Failed to create buffer");
        return m_wgpuBuffer ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }

    void Buffer::Invalidate()
    {
        if (m_wgpuBuffer)
        {
            m_wgpuBuffer.Destroy();
            m_wgpuBuffer = nullptr;
        }
    }

    void Buffer::SetNameInternal(const AZStd::string_view& name)
    {
        if (m_wgpuBuffer && !name.empty())
        {
            m_wgpuBuffer.SetLabel(name.data());
        }
    }
} // namespace AZ::WebGPU
