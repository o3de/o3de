/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            AZ_CLASS_ALLOCATOR(DescriptorSetLayout, AZ::ThreadPoolAllocator, 0);
            AZ_RTTI(DescriptorSetLayout, "25C09E30-F46B-424D-B97A-7F32592A76D7", Base);

            struct Descriptor
            {
                Device* m_device = nullptr;
                RHI::ConstPtr<RHI::ShaderResourceGroupLayout> m_shaderResouceGroupLayout;

                HashValue64 GetHash() const;
            };

            enum class ResourceType : uint32_t
            {
                ConstantData,
                BufferView,
                ImageView,
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

            const RHI::ShaderResourceGroupLayout* GetShaderResourceGroupLayout() const;

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

            bool ValidateUniformBufferDeviceLimits(const RHI::ShaderInputBufferDescriptor& desc);

            VkDescriptorSetLayout m_nativeDescriptorSetLayout = VK_NULL_HANDLE;
            AZStd::vector<VkDescriptorSetLayoutBinding> m_descriptorSetLayoutBindings;
            uint32_t m_constantDataSize = 0;
            AZStd::array<uint32_t, ResourceTypeSize> m_layoutIndexOffset;
            AZStd::vector<VkSampler> m_nativeSamplers;
            RHI::ConstPtr<RHI::ShaderResourceGroupLayout> m_shaderResourceGroupLayout;
        };
    }
}
