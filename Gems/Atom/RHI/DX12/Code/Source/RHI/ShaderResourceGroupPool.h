/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/ShaderResourceGroup.h>
#include <RHI/MemorySubAllocator.h>
#include <Atom/RHI/FrameEventBus.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>

namespace AZ
{
    namespace DX12
    {
        class BufferView;
        class ImageView;
        class ShaderResourceGroupLayout;
        class DescriptorContext;

        class ShaderResourceGroupPool final
            : public RHI::ShaderResourceGroupPool
        {
            using Base = RHI::ShaderResourceGroupPool;
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupPool, AZ::SystemAllocator, 0);

            static RHI::Ptr<ShaderResourceGroupPool> Create();

            //! Re-Update the descriptor tables for all the cbv/srv/uav views (direct and via unbounded array)
            RHI::ResultCode UpdateDescriptorTableAfterCompaction(RHI::ShaderResourceGroup& groupBase, const RHI::ShaderResourceGroupData& groupData);

        private:
            ShaderResourceGroupPool() = default;

            //////////////////////////////////////////////////////////////////////////
            // Platform API
            RHI::ResultCode InitInternal(RHI::Device& deviceBase, const RHI::ShaderResourceGroupPoolDescriptor& descriptor) override;
            RHI::ResultCode InitGroupInternal(RHI::ShaderResourceGroup& groupBase) override;
            void ShutdownInternal() override;
            RHI::ResultCode CompileGroupInternal(RHI::ShaderResourceGroup& groupBase, const RHI::ShaderResourceGroupData& groupData) override;
            void ShutdownResourceInternal(RHI::Resource& resourceBase) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // FrameEventBus::Handler
            void OnFrameEnd() override;
            //////////////////////////////////////////////////////////////////////////

            void UpdateViewsDescriptorTable(DescriptorTable descriptorTable, const RHI::ShaderResourceGroupData& groupData);
            void UpdateSamplersDescriptorTable(DescriptorTable descriptorTable, const RHI::ShaderResourceGroupData& groupData);
            void UpdateUnboundedArrayDescriptorTables(ShaderResourceGroup& group, const RHI::ShaderResourceGroupData& groupData);

            //! Update all the buffer views for the unbounded array
            void UpdateUnboundedBuffersDescTable(
                DescriptorTable descriptorTable,
                const RHI::ShaderResourceGroupData& groupData,
                uint32_t shaderInputIndex,
                RHI::ShaderInputBufferAccess bufferAccess);

            //! Update all the image views for the unbounded array
            void UpdateUnboundedImagesDescTable(
                DescriptorTable descriptorTable,
                const RHI::ShaderResourceGroupData& groupData,
                uint32_t shaderInputIndex,
                RHI::ShaderInputImageAccess imageAccess,
                RHI::ShaderInputImageType imageType);

            void UpdateDescriptorTableRange(
                DescriptorTable descriptorTable,
                const AZStd::vector<DescriptorHandle>& descriptors,
                RHI::ShaderInputBufferIndex bufferInputIndex);

            void UpdateDescriptorTableRange(
                DescriptorTable descriptorTable,
                const AZStd::vector<DescriptorHandle>& descriptors,
                RHI::ShaderInputImageIndex imageIndex);

            void UpdateDescriptorTableRange(
                DescriptorTable descriptorTable,
                RHI::ShaderInputSamplerIndex samplerIndex,
                AZStd::array_view<RHI::SamplerState> samplerStates);

            //Cache all the gpu handles for the Descriptor tables related to all the views
            void CacheGpuHandlesForViews(ShaderResourceGroup& group);

            DescriptorTable GetBufferTable(DescriptorTable descriptorTable, RHI::ShaderInputBufferIndex bufferIndex) const;
            DescriptorTable GetBufferTableUnbounded(DescriptorTable descriptorTable, RHI::ShaderInputBufferIndex bufferIndex) const;
            DescriptorTable GetImageTable(DescriptorTable descriptorTable, RHI::ShaderInputImageIndex imageIndex) const;
            DescriptorTable GetSamplerTable(DescriptorTable descriptorTable, RHI::ShaderInputSamplerIndex samplerInputIndex) const;

            template<typename T, typename U>
            AZStd::vector<DescriptorHandle> GetSRVsFromImageViews(const AZStd::array_view<RHI::ConstPtr<T>>& imageViews, D3D12_SRV_DIMENSION dimension);

            template<typename T, typename U>
            AZStd::vector<DescriptorHandle> GetUAVsFromImageViews(const AZStd::array_view<RHI::ConstPtr<T>>& bufferViews, D3D12_UAV_DIMENSION dimension);

            AZStd::vector<DescriptorHandle> GetCBVsFromBufferViews(const AZStd::array_view<RHI::ConstPtr<RHI::BufferView>>& bufferViews);

            MemoryPoolSubAllocator m_constantAllocator;
            DescriptorContext* m_descriptorContext = nullptr;
            uint32_t m_constantBufferSize = 0;
            uint32_t m_constantBufferRingSize = 0;
            uint32_t m_viewsDescriptorTableSize = 0;
            uint32_t m_viewsDescriptorTableRingSize = 0;
            uint32_t m_samplersDescriptorTableSize = 0;
            uint32_t m_samplersDescriptorTableRingSize = 0;
            uint32_t m_descriptorTableBufferOffset = 0;
            uint32_t m_descriptorTableImageOffset = 0;
            uint32_t m_unboundedArrayCount = 0;
        };
    }
}
