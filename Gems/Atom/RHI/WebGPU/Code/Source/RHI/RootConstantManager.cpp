/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/BindGroup.h>
#include <RHI/BindGroupLayout.h>
#include <RHI/BufferView.h>
#include <RHI/BufferPool.h>
#include <RHI/Device.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/RootConstantManager.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>

namespace AZ::WebGPU
{
    RHI::Ptr<RootConstantManager> RootConstantManager::Create()
    {
        return aznew RootConstantManager();
    }

    RHI::ResultCode RootConstantManager::Init(Device& device)
    {
        DeviceObject::Init(device);
        m_bufferPool = BufferPool::Create();
        m_bufferPool->SetName(Name("RootConstantBufferPool"));

        // Initialize the buffer pool for constants
        RHI::BufferPoolDescriptor bufferPoolDescriptor;
        bufferPoolDescriptor.m_bindFlags = RHI::BufferBindFlags::Constant;
        bufferPoolDescriptor.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
        RHI::ResultCode result = m_bufferPool->Init(device, bufferPoolDescriptor);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        // Create an RHI::ShaderResourceGroupLayout that we use for building the BindGroupLayout
        RHI::ShaderInputBufferDescriptor bufferDesc(
            AZ::Name{ "RootConstants" },
            RHI::ShaderInputBufferAccess::Constant,
            RHI::ShaderInputBufferType::Constant,
            1,
            0,
            0,
            0);
        auto srgLayout = RHI::ShaderResourceGroupLayout::Create();
        srgLayout->SetBindingSlot(0);
        srgLayout->AddShaderInput(bufferDesc);
        if (!srgLayout->Finalize())
        {
            AZ_Error("WebGPU", false, "Failed to create ShaderResourceGroupLayout for root constants");
            return RHI::ResultCode::Fail;
        }
        
        srgLayout->SetName(AZ::Name{ "RootConstants" });

        // Creates the BindGroup layout
        BindGroupLayout::Descriptor layoutDesc{ srgLayout, { RHI::ShaderInputBufferIndex(0) } };
        m_bindGroupLayout = BindGroupLayout::Create();
        result = m_bindGroupLayout->Init(device, layoutDesc);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);
        
        return result;
    } 

    void RootConstantManager::Shutdown()
    {
        m_allocations.clear();
        m_bindGroupLayout = nullptr;
        m_bufferPool = nullptr;
        RHI::DeviceObject::Shutdown();
    }

    RootConstantManager::Allocation RootConstantManager::Allocate(uint32_t size)
    {
        const PhysicalDevice& physicalDevice = static_cast<const PhysicalDevice&>(GetDevice().GetPhysicalDevice());        
        Allocation alloc;
        // Check if we need to allocate a new buffer
        if (m_allocations.empty() ||
            m_allocations.back().m_bufferOffset + size > m_allocations.back().m_bufferView->GetBuffer().GetDescriptor().m_byteCount)
        {
            // Allocate a buffer, create a bind group and update it to point to the newly created buffer.
            Device& device = static_cast<Device&>(GetDevice());
            constexpr size_t BufferSize = 65536; 
            RHI::Ptr<Buffer> rootConstantBuffer = Buffer::Create();
            RHI::BufferDescriptor bufferDesc(RHI::BufferBindFlags::Constant, BufferSize);
            RHI::DeviceBufferInitRequest initRequest(*rootConstantBuffer, bufferDesc);
            RHI::ResultCode result = m_bufferPool->InitBuffer(initRequest);
            if (result != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "Failed to create root constant buffer.");
                return alloc;
            }
            RHI::Ptr<BindGroup> bindGroup = BindGroup::Create();
            result = bindGroup->Init(device, BindGroup::Descriptor{ m_bindGroupLayout.get() });
            if (result != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "Failed to create bind group for root constant.");
                return alloc;
            }
            bindGroup->SetName(AZ::Name("RootConstant"));

            constexpr size_t RootConstantViewSize = 128;
            auto bufferView = BufferView::Create();
            result = bufferView->Init(
                *rootConstantBuffer, RHI::BufferViewDescriptor::CreateTyped(0, RootConstantViewSize, RHI::Format::R8_UINT));
            RHI::ConstPtr<RHI::DeviceBufferView> constBufferView = bufferView;
            bindGroup->UpdateBufferViews(0, AZStd::span<const RHI::ConstPtr<RHI::DeviceBufferView>>(&constBufferView, 1));
            bindGroup->CommitUpdates();

            Allocation& newAlloc = m_allocations.emplace_back();
            newAlloc.m_bindGroup = bindGroup;
            newAlloc.m_bufferView = bufferView;
            newAlloc.m_bufferOffset = 0;
        }

        Allocation& currentAlloc = m_allocations.back();
        alloc = currentAlloc;
        currentAlloc.m_bufferOffset += size;
        currentAlloc.m_bufferOffset =
            RHI::AlignUp(currentAlloc.m_bufferOffset, physicalDevice.GetLimits().limits.minUniformBufferOffsetAlignment);
        return alloc;
    }

    void RootConstantManager::Collect()
    {
        m_allocations.clear();
    }

    const BindGroupLayout& RootConstantManager::GetBindGroupLayout() const
    {
        return *m_bindGroupLayout;
    }
}
