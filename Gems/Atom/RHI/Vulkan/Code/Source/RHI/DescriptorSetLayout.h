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
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/tuple.h>
#include <RHI/Sampler.h>

namespace AZ
{
    namespace RHI
    {
        class ShaderResourceGroupLayout;
        class ShaderInputBufferDescriptor;
    }

    namespace Vulkan
    {
        class Device;

        class DescriptorSetLayout final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(DescriptorSetLayout, AZ::ThreadPoolAllocator);
            AZ_RTTI(DescriptorSetLayout, "25C09E30-F46B-424D-B97A-7F32592A76D7", Base);

            struct Descriptor
            {
                Device* m_device = nullptr;
                RHI::ConstPtr<RHI::ShaderResourceGroupLayout> m_shaderResouceGroupLayout;
                Name m_name;
                HashValue64 GetHash() const;
            };

            enum class ResourceType : uint32_t
            {
                ConstantData,
                BufferView,
                ImageView,
                BufferViewUnboundedArray,
                ImageViewUnboundedArray,
                Sampler,
                Count
            };

            const static uint32_t ResourceTypeSize = static_cast<uint32_t>(ResourceType::Count);

            static RHI::Ptr<DescriptorSetLayout> Create();
            RHI::ResultCode Init(const Descriptor& descriptor);
            ~DescriptorSetLayout();

            VkDescriptorSetLayout GetNativeDescriptorSetLayout() const;
            size_t GetDescriptorSetLayoutBindingsCount() const;
            VkDescriptorType GetDescriptorType(size_t index) const;
            uint32_t GetBindingIndex(uint32_t index) const;

            const static uint32_t InvalidLayoutIndex;
            uint32_t GetLayoutIndexFromGroupIndex(uint32_t groupIndex, ResourceType type) const;

            uint32_t GetDescriptorCount(size_t descIndex) const;
            uint32_t GetConstantDataSize() const;
            const AZStd::vector<VkDescriptorSetLayoutBinding>& GetNativeLayoutBindings() const;
            const AZStd::vector<VkDescriptorBindingFlags>& GetNativeBindingFlags() const;
            const RHI::ShaderResourceGroupLayout* GetShaderResourceGroupLayout() const;

            static const uint32_t MaxUnboundedArrayDescriptors = 100000; //Using this number as it needs to be less than maxDescriptorSetSampledImages limit of 1048576
            bool GetHasUnboundedArray() const { return m_hasUnboundedArray; }

            //! Returns true if the resource in the specified layoutIndex is using a depth format.
            //! Only valid for image resources.
            bool UsesDepthFormat(uint32_t layoutIndex) const;

        private:
            DescriptorSetLayout() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::ResultCode BuildNativeDescriptorSetLayout();
            RHI::ResultCode BuildDescriptorSetLayoutBindings();
            bool IsBindlessSRGLayout();

            bool ValidateUniformBufferDeviceLimits(const RHI::ShaderInputBufferDescriptor& desc);

            // Contains information about binding and layout of the resources in the descriptor set
            struct LayoutBindingInfo
            {
            public:
                LayoutBindingInfo() = default;
                LayoutBindingInfo(size_t size);

                using Element = AZStd::tuple<VkDescriptorSetLayoutBinding&, VkDescriptorBindingFlags&, bool&>;

                Element Add();
                AZStd::size_t GetSize() const;
                const AZStd::vector<VkDescriptorSetLayoutBinding>& GetLayoutBindings() const;
                const AZStd::vector<VkDescriptorBindingFlags>& GetBindingFlags() const;
                const AZStd::vector<bool>& GetUseDepthFormat() const;

            private:
                AZStd::vector<VkDescriptorSetLayoutBinding> m_descriptorSetLayoutBindings;
                AZStd::vector<VkDescriptorBindingFlags> m_descriptorBindingFlags;
                AZStd::vector<bool> m_useDepthFormat;
            };

            VkDescriptorSetLayout m_nativeDescriptorSetLayout = VK_NULL_HANDLE;
            LayoutBindingInfo m_layoutBindingInfo;
            uint32_t m_constantDataSize = 0;
            AZStd::array<uint32_t, ResourceTypeSize> m_layoutIndexOffset;
            AZStd::vector<VkSampler> m_nativeSamplers;
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> m_shaderResourceGroupLayout;
            bool m_hasUnboundedArray = false;
        };
    }
}
