/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ::RHI
{
    class ShaderResourceGroupLayout;
    class ShaderInputBufferDescriptor;
}

namespace AZ::WebGPU
{
    class Device;

    //! Encapsulates a WebGPU BindGroupLayout
    class BindGroupLayout final
        : public RHI::DeviceObject
    {
        using Base = RHI::DeviceObject;

    public:
        AZ_CLASS_ALLOCATOR(BindGroupLayout, AZ::ThreadPoolAllocator);
        AZ_RTTI(BindGroupLayout, "{88891AFA-D73A-49FF-8B73-F41D3A9142FB}", Base);

        struct Descriptor
        {
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> m_shaderResouceGroupLayout;
            AZStd::vector<RHI::ShaderInputBufferIndex> m_dynamicOffsetBuffers; // Buffers bindings that will use a dynamic offset
            HashValue64 GetHash() const;
        };

        static RHI::Ptr<BindGroupLayout> Create();
        RHI::ResultCode Init(Device& device, const Descriptor& descriptor);
        ~BindGroupLayout();

        const wgpu::BindGroupLayout& GetNativeBindGroupLayout() const;
        uint32_t GetConstantDataSize() const;
        const RHI::ShaderResourceGroupLayout* GetShaderResourceGroupLayout() const;
        const wgpu::BindGroupLayoutEntry& GetEntryByBinding(const uint32_t binding) const;

    private:
        BindGroupLayout() = default;

        //////////////////////////////////////////////////////////////////////////
        // RHI::Object
        void SetNameInternal(const AZStd::string_view& name) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RHI::DeviceObject
        void Shutdown() override;
        //////////////////////////////////////////////////////////////////////////

        RHI::ResultCode BuildNativeDescriptorSetLayout(const Descriptor& descriptor);
        RHI::ResultCode BuildBindGroupLayoutBindings(const Descriptor& descriptor);

        wgpu::BindGroupLayout m_wgpuBindGroupLayout = nullptr;
        AZStd::vector<wgpu::BindGroupLayoutEntry> m_wgpuEntries;
        RHI::ConstPtr<RHI::ShaderResourceGroupLayout> m_shaderResourceGroupLayout;
        uint32_t m_constantDataSize = 0;
    };
}
