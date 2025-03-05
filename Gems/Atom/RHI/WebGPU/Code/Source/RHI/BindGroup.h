/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/Buffer.h>

namespace AZ::RHI
{
    class ResourceView;
}

namespace AZ::WebGPU
{
    class BufferView;
    class BindGroupLayout;
    class Device;
    class ImageView;
    class Sampler;

    //! Encapsulate a WebGPU BindGroup.
    class BindGroup final
        : public RHI::DeviceObject
    {
        using Base = RHI::DeviceObject;
        friend class DescriptorPool;

    public:
            
        AZ_CLASS_ALLOCATOR(BindGroup, AZ::SystemAllocator);
        AZ_RTTI(BindGroup, "{FF47A529-4114-4B35-AE74-70447288503D}", Base);

        struct Descriptor
        {
            const BindGroupLayout* m_bindGroupLayout = nullptr;
        };
        ~BindGroup() = default;

        static RHI::Ptr<BindGroup> Create();
        RHI::ResultCode Init(Device& device, const Descriptor& descriptor);
        const Descriptor& GetDescriptor() const;

        void CommitUpdates();

        //! Updates the buffer views of the bind group
        void UpdateBufferViews(uint32_t binding, const AZStd::span<const RHI::ConstPtr<RHI::DeviceBufferView>>& bufViews);
        //! Updates the iamge views of the bind group
        void UpdateImageViews(
            uint32_t binding,
            const AZStd::span<const RHI::ConstPtr<RHI::DeviceImageView>>& imageViews,
            RHI::ShaderInputImageType imageType);
        //! Updates the samplers of the bind group
        void UpdateSamplers(uint32_t index, const AZStd::span<const RHI::SamplerState>& samplers);
        //! Updates the constant data of the bind group
        void UpdateConstantData(AZStd::span<const uint8_t> data);

        //! Returns a buffer view of the constant buffer
        RHI::Ptr<BufferView> GetConstantDataBufferView() const;

        //! Returns the native bind group object
        const wgpu::BindGroup& GetNativeBindGroup() const;

    private:
        BindGroup() = default;

        //////////////////////////////////////////////////////////////////////////
        // RHI::Object
        void SetNameInternal(const AZStd::string_view& name) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RHI::DeviceObject
        void Shutdown() override;
        //////////////////////////////////////////////////////////////////////////

        Descriptor m_descriptor;
        wgpu::BindGroup m_wgpuBindGroup = nullptr;
        AZStd::vector<wgpu::BindGroupEntry> m_wgpuEntries;
        RHI::Ptr<Buffer> m_constantDataBuffer;
        RHI::Ptr<BufferView> m_constantDataBufferView;
    };
}
