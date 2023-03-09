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
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/bitset.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <RHI/DescriptorSetLayout.h>

namespace AZ
{
    namespace RHI
    {
        class PipelineLayoutDescriptor;
    }

    namespace Vulkan
    {
        class Device;
        class DescriptorSetLayout;
        class MergedShaderResourceGroupPool;

        class PipelineLayout final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(PipelineLayout, AZ::ThreadPoolAllocator);
            AZ_RTTI(PipelineLayout, "3791D9FB-1CD6-4A8A-80B7-717962ECDC28", Base);

            struct Descriptor 
            {
                size_t GetHash() const;

                Device* m_device = nullptr;
                RHI::ConstPtr<RHI::PipelineLayoutDescriptor> m_pipelineLayoutDescriptor;
            };

            using ShaderResourceGroupBitset = AZStd::bitset<RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;

            static RHI::Ptr<PipelineLayout> Create();
            RHI::ResultCode Init(const Descriptor& descriptor);
            ~PipelineLayout() = default;

            VkPipelineLayout GetNativePipelineLayout() const;

            size_t GetDescriptorSetLayoutCount() const;
            RHI::Ptr<DescriptorSetLayout> GetDescriptorSetLayout(uint32_t index) const;
            ShaderResourceGroupBitset GetAZSLBindingSlotsOfIndex(uint32_t index) const;
            uint32_t GetIndexFromAZSLBindingSlot(uint32_t slot) const;
            uint32_t GetPushContantsSize() const;
            const RHI::PipelineLayoutDescriptor& GetPipelineLayoutDescriptor() const;
            MergedShaderResourceGroupPool* GetMergedShaderResourceGroupPool(uint32_t index) const;
            bool IsMergedDescriptorSetLayout(uint32_t index) const;

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
            void BuildPushConstantRanges();

            // Build the shader resource group pools for the merged SRGs (if there's any).
            RHI::ResultCode BuildMergedShaderResourceGroupPools();

            // Creates a merged SRG layout from a list of SRG layouts.
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> MergeShaderResourceGroupLayouts(const AZStd::vector<const RHI::ShaderResourceGroupLayout*>& srgLayoutList) const;

            VkPipelineLayout m_nativePipelineLayout = VK_NULL_HANDLE;

            AZStd::vector<RHI::Ptr<DescriptorSetLayout>> m_descriptorSetLayouts;
            AZStd::vector<VkPushConstantRange> m_pushConstantRanges;
            AZStd::array<uint8_t, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_slotToIndex;
            AZStd::fixed_vector<ShaderResourceGroupBitset, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_indexToSlot;
            uint32_t m_pushConstantsSize = 0;
            RHI::ConstPtr<RHI::PipelineLayoutDescriptor> m_layoutDescriptor;

            AZStd::array<RHI::Ptr<MergedShaderResourceGroupPool>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_mergedShaderResourceGroupPools;
        };
    }
}
