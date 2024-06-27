/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceShaderResourceGroup.h>
#include <RHI/Descriptor.h>
#include <RHI/MemoryView.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace DX12
    {
        struct ShaderResourceGroupCompiledData
        {
            /// The GPU descriptor handle for views to bind to the command list.
            GpuDescriptorHandle m_gpuViewsDescriptorHandle = {};

            /// The GPU descriptor handle for unbounded arrays to bind to the command list.
            /// Note that one SRG can only contain at most two unbounded arrays, one SRV and one UAV.
            // TODO(bindless): The new bindless handling does not require this member. This and all usages can be removed after
            // terrain/ray-tracing shaders are ported
            static const uint32_t MaxUnboundedArrays = 2;
            GpuDescriptorHandle m_gpuUnboundedArraysDescriptorHandles[MaxUnboundedArrays] = {};

            /// The GPU descriptor handle for samplers to bind to the command list.
            GpuDescriptorHandle m_gpuSamplersDescriptorHandle = {};

            /// The constant buffer GPU virtual address.
            GpuVirtualAddress m_gpuConstantAddress = {};

            /// The constant buffer CPU virtual address.
            CpuVirtualAddress m_cpuConstantAddress = {};
        };

        class ShaderResourceGroup final
            : public RHI::DeviceShaderResourceGroup
        {
            using Base = RHI::DeviceShaderResourceGroup;
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator);

            static RHI::Ptr<ShaderResourceGroup> Create();

            const ShaderResourceGroupCompiledData& GetCompiledData() const;

        private:
            ShaderResourceGroup() = default;

            friend class ShaderResourceGroupPool;
            friend class DescriptorContext;

            /// The current index into the compiled data array.
            uint32_t m_compiledDataIndex = 0;

            /// The array of compiled SRG data, N buffered for CPU updates.
            AZStd::array<ShaderResourceGroupCompiledData, RHI::Limits::Device::FrameCountMax> m_compiledData;

            /// The mapped memory view to constant memory.
            MemoryView m_constantMemoryView;

            /// The allocated descriptor table for vews.
            DescriptorTable m_viewsDescriptorTable;

            /// The allocated descriptor table for samplers.
            DescriptorTable m_samplersDescriptorTable; 

            /// The descriptor tables for unbounded arrays.  Allocated on demand.
            AZStd::array<DescriptorTable, ShaderResourceGroupCompiledData::MaxUnboundedArrays * RHI::Limits::Device::FrameCountMax> m_unboundedDescriptorTables;
        };
    }
}
