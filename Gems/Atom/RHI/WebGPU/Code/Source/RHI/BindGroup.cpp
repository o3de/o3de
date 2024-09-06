/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/BufferView.h>
#include <RHI/BindGroupLayout.h>
#include <RHI/BindGroup.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/Device.h>
#include <RHI/ImageView.h>

namespace AZ::WebGPU
{
    const BindGroup::Descriptor& BindGroup::GetDescriptor() const
    {
        return m_descriptor;
    }

    void BindGroup::CommitUpdates()
    {
        if (!m_wgpuEntries.empty())
        {
            wgpu::BindGroupDescriptor descriptor;
            descriptor.layout = m_descriptor.m_bindGroupLayout->GetNativeBindGroupLayout();
            descriptor.entryCount = m_wgpuEntries.size();
            descriptor.entries = m_wgpuEntries.data();
            descriptor.label = GetName().GetCStr();

            m_wgpuBindGroup = static_cast<Device&>(GetDevice()).GetNativeDevice().CreateBindGroup(&descriptor);

            m_wgpuEntries.clear();
        }
    }

    void BindGroup::UpdateBufferViews(uint32_t binding, const AZStd::span<const RHI::ConstPtr<RHI::DeviceBufferView>>& bufViews)
    {
        AZ_Assert(bufViews.size() == 1, "WebGPU doesn't support array of buffers yet");
        auto bufferView = bufViews.front();
        wgpu::BindGroupEntry& data = m_wgpuEntries.emplace_back(wgpu::BindGroupEntry{});
        data.binding = binding;
        if (bufferView)
        {
            const RHI::BufferViewDescriptor& descriptor = bufferView->GetDescriptor();
            data.buffer = static_cast<const Buffer&>(bufferView->GetBuffer()).GetNativeBuffer();
            data.offset = descriptor.m_elementOffset * descriptor.m_elementSize;
            data.size = descriptor.m_elementCount * descriptor.m_elementSize;
        }
    }       

    void BindGroup::UpdateImageViews(
        uint32_t binding, const AZStd::span<const RHI::ConstPtr<RHI::DeviceImageView>>& imageViews)
    {
        //AZ_Assert(imageViews.size() == 1, "WebGPU doesn't support array of images yet");
        auto imageView = imageViews.front();
        wgpu::BindGroupEntry& data = m_wgpuEntries.emplace_back(wgpu::BindGroupEntry{});
        data.binding = binding;
        data.textureView = imageView ? static_cast<const ImageView*>(imageView.get())->GetNativeView() : nullptr;
    }

    void BindGroup::UpdateSamplers(uint32_t binding, const AZStd::span<const RHI::SamplerState>& samplers)
    {
        AZ_Assert(samplers.size() == 1, "WebGPU doesn't support array of samplers yet");
        const RHI::SamplerState& descriptor = samplers.front();
        Sampler::Descriptor sampleDesc;
        sampleDesc.m_samplerState = descriptor;
        auto sampler = static_cast<Device&>(GetDevice()).AcquireSampler(sampleDesc);
        wgpu::BindGroupEntry& data = m_wgpuEntries.emplace_back(wgpu::BindGroupEntry{});
        data.binding = binding;
        data.sampler = sampler->GetNativeSampler();
    }

    void BindGroup::UpdateConstantData([[maybe_unused]] AZStd::span<const uint8_t> rawData)
    {
        AZ_Assert(m_constantDataBuffer, "Null constant buffer");
        auto& queue = static_cast<Device&>(GetDevice()).GetCommandQueueContext().GetCommandQueue(RHI::HardwareQueueClass::Copy);
        queue.WriteBuffer(*m_constantDataBuffer, 0, rawData);
        
        wgpu::BindGroupEntry& data = m_wgpuEntries.emplace_back(wgpu::BindGroupEntry{});
        data.binding =
            m_descriptor.m_bindGroupLayout->GetShaderResourceGroupLayout()->GetShaderInputListForConstants().front().m_registerId;
        data.buffer = m_constantDataBuffer->GetNativeBuffer();
        data.offset = 0;
        data.size = m_descriptor.m_bindGroupLayout->GetConstantDataSize();
    }

    RHI::Ptr<BindGroup> BindGroup::Create()
    {
        return aznew BindGroup();
    }

    RHI::ResultCode BindGroup::Init(Device& device, const Descriptor& descriptor)
    {
        m_descriptor = descriptor;
        AZ_Assert(descriptor.m_bindGroupLayout, "BindGroupLayout is null.");
        Base::Init(device);

        // Check if we need to create a uniform buffer for the constants
        size_t constantDataSize = descriptor.m_bindGroupLayout->GetConstantDataSize();
        if (constantDataSize)
        {
            m_constantDataBuffer = Buffer::Create();
            const RHI::BufferDescriptor bufferDescriptor(RHI::BufferBindFlags::Constant, constantDataSize);
            RHI::DeviceBufferInitRequest request(*m_constantDataBuffer, bufferDescriptor);
            RHI::ResultCode result = device.GetConstantBufferPool()->InitBuffer(request);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            auto bufferView = m_constantDataBuffer->GetBufferView(
                RHI::BufferViewDescriptor::CreateStructured(
                    0,
                    1,
                    aznumeric_caster(constantDataSize)));
            m_constantDataBufferView = AZStd::static_pointer_cast<BufferView>(bufferView);
        }

        SetName(GetName());
        return RHI::ResultCode::Success;
    }

    void BindGroup::SetNameInternal(const AZStd::string_view& name)
    {
        if (m_wgpuBindGroup && !name.empty())
        {
            m_wgpuBindGroup.SetLabel(name.data());
        }
    }

    void BindGroup::Shutdown()
    {
        m_wgpuBindGroup = nullptr;
        m_wgpuEntries.clear();
        m_constantDataBufferView = nullptr;
        m_constantDataBuffer = nullptr;
        Base::Shutdown();
    }

    RHI::Ptr<BufferView> BindGroup::GetConstantDataBufferView() const
    {
        return m_constantDataBufferView;
    }

    const wgpu::BindGroup& BindGroup::GetNativeBindGroup() const
    {
        return m_wgpuBindGroup;
    }
}
