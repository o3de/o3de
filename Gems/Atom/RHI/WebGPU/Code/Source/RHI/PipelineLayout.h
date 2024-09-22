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
    class PipelineLayoutDescriptor;
}

namespace AZ::WebGPU
{
    class Device;
    class BindGroupLayout;

    class PipelineLayout final
        : public RHI::DeviceObject
    {
        using Base = RHI::DeviceObject;

    public:
        AZ_CLASS_ALLOCATOR(PipelineLayout, AZ::ThreadPoolAllocator);
        AZ_RTTI(PipelineLayout, "{99E77BE8-B5E1-48CE-972F-9ED124AA34EC}", Base);

        struct Descriptor 
        {
            size_t GetHash() const;

            RHI::ConstPtr<RHI::PipelineLayoutDescriptor> m_pipelineLayoutDescriptor;
        };

        static RHI::Ptr<PipelineLayout> Create();
        RHI::ResultCode Init(Device& device, const Descriptor& descriptor);
        ~PipelineLayout() = default;

        //! Returns the SRG binding slot associated with the SRG flat index.
        uint32_t GetSlotByIndex(uint32_t index) const;

        //! Returns the SRG flat index associated with the SRG binding slot.
        uint32_t GetIndexBySlot(uint32_t slot) const;

        //! Returns the size in bytes of root constants being used
        uint32_t GetRootConstantSize() const;

        //! Returns the binding group index used for root constants.
        uint32_t GetRootConstantIndex() const;

        //! Returns the pipeline layout descriptor being used
        const RHI::PipelineLayoutDescriptor& GetPipelineLayoutDescriptor() const;

        const wgpu::PipelineLayout& GetNativePipelineLayout() const;
        wgpu::PipelineLayout& GetNativePipelineLayout();

    private:
        PipelineLayout() = default;

        //////////////////////////////////////////////////////////////////////////
        // RHI::Object
        void SetNameInternal(const AZStd::string_view& name) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RHI::DeviceObject
        void Shutdown() override;
        //////////////////////////////////////////////////////////////////////////

        RHI::ResultCode BuildNativePipelineLayout();

        /// Tables for mapping between SRG slots (sparse) to SRG indices (packed).
        AZStd::array<uint8_t, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_slotToIndexTable;
        AZStd::fixed_vector<uint8_t, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_indexToSlotTable;

        uint32_t m_rootConstantSize = 0;
        uint32_t m_rootConstantIndex = ~0u;
        RHI::ConstPtr<RHI::PipelineLayoutDescriptor> m_layoutDescriptor;

        //! Native handle
        wgpu::PipelineLayout m_wgpuPipelineLayout = nullptr;
        AZStd::vector<RHI::Ptr<BindGroupLayout>> m_bindGroupLayouts;
    };
}
