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

namespace AZ
{
    namespace RHI
    {
        class ResourceView;
    }

    namespace Vulkan
    {
        class BufferView;
        class DescriptorSetLayout;
        class Device;
        class ImageView;
        class DescriptorPool;

        class DescriptorSet final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;
            friend class DescriptorPool;

        public:
            
            //Using SystemAllocator here instead of ThreadPoolAllocator as it gets slower when
            //we create thousands of DescriptorSets related to SRGs
            AZ_CLASS_ALLOCATOR(DescriptorSet, AZ::SystemAllocator);
            AZ_RTTI(DescriptorSet, "06D7FC0A-B53E-46D9-975D-D4E445356645", Base);

            struct Descriptor
            {
                Device* m_device = nullptr;
                const DescriptorPool* m_descriptorPool = nullptr;
                const DescriptorSetLayout* m_descriptorSetLayout = nullptr;
            };
            ~DescriptorSet() = default;

            static RHI::Ptr<DescriptorSet> Create();
            VkResult Init(const Descriptor& descriptor);
            const Descriptor& GetDescriptor() const;
            VkDescriptorSet GetNativeDescriptorSet() const;

            void CommitUpdates();

            void UpdateBufferViews(uint32_t index, const AZStd::span<const RHI::ConstPtr<RHI::DeviceBufferView>>& bufViews);
            void UpdateImageViews(uint32_t index, const AZStd::span<const RHI::ConstPtr<RHI::DeviceImageView>>& imageViews, RHI::ShaderInputImageType imageType);
            void UpdateSamplers(uint32_t index, const AZStd::span<const RHI::SamplerState>& samplers);
            void UpdateConstantData(AZStd::span<const uint8_t> data);

            RHI::Ptr<BufferView> GetConstantDataBufferView() const;

        private:
            struct WriteDescriptorData
            {
                uint32_t m_layoutIndex = 0;
                AZStd::vector<VkDescriptorBufferInfo> m_bufferViewsInfo;
                AZStd::vector<VkDescriptorImageInfo> m_imageViewsInfo;
                AZStd::vector<VkBufferView> m_texelBufferViews;
                AZStd::vector<VkAccelerationStructureKHR> m_accelerationStructures;
            };

            DescriptorSet() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            void UpdateNativeDescriptorSet();
            void AllocateDescriptorSetWithUnboundedArray();

            template<typename T>
            AZStd::vector<RHI::Interval> GetValidDescriptorsIntervals(const AZStd::vector<T>& descriptorsInfo) const;

            static bool IsNullDescriptorInfo(const VkDescriptorBufferInfo& descriptorInfo);
            static bool IsNullDescriptorInfo(const VkDescriptorImageInfo& descriptorInfo);
            static bool IsNullDescriptorInfo(const VkBufferView& descriptorInfo);

            Descriptor m_descriptor;

            VkDescriptorSet m_nativeDescriptorSet = VK_NULL_HANDLE;
            AZStd::vector<WriteDescriptorData> m_updateData;
            RHI::Ptr<Buffer> m_constantDataBuffer;
            RHI::Ptr<BufferView> m_constantDataBufferView;
            bool m_nullDescriptorSupported = false;
            uint32_t m_currentUnboundedArrayAllocation = 0;
        };

        template<typename T>
        AZStd::vector<RHI::Interval> DescriptorSet::GetValidDescriptorsIntervals(const AZStd::vector<T>& descriptorsInfo) const
        {
            // if Null descriptors are supported, then we just return one interval that covers the whole range.
            if (m_nullDescriptorSupported)
            {
                return { RHI::Interval(0, aznumeric_caster(descriptorsInfo.size())) };
            }

            AZStd::vector<RHI::Interval> intervals;
            auto beginInterval = descriptorsInfo.begin();
            auto endInterval = beginInterval;
            bool (*IsNullFuntion)(const T&) = &DescriptorSet::IsNullDescriptorInfo;
            do 
            {
                beginInterval = AZStd::find_if_not(endInterval, descriptorsInfo.end(), IsNullFuntion);
                if (beginInterval != descriptorsInfo.end())
                {
                    endInterval = AZStd::find_if(beginInterval, descriptorsInfo.end(), IsNullFuntion);

                    intervals.emplace_back();
                    RHI::Interval& interval = intervals.back();
                    interval.m_min = aznumeric_caster(AZStd::distance(descriptorsInfo.begin(), beginInterval));
                    interval.m_max = endInterval == descriptorsInfo.end() ? static_cast<uint32_t>(descriptorsInfo.size()) : static_cast<uint32_t>(AZStd::distance(descriptorsInfo.begin(), endInterval));
                }
            } while (beginInterval != descriptorsInfo.end() && endInterval != descriptorsInfo.end());

            return intervals;
        }
    }
}
