/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI/DeviceBuffer.h>
#include <Atom/RHI/DeviceBufferView.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/DeviceObject.h>
#include <AtomCore/std/containers/small_vector.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/span.h>
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

            static constexpr size_t ViewsFixedsize = 2;

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

            void ReserveUpdateData(size_t numUpdates);

            void UpdateBufferViews(uint32_t index, const AZStd::span<const RHI::ConstPtr<RHI::DeviceBufferView>>& bufViews);
            void UpdateImageViews(uint32_t index, const AZStd::span<const RHI::ConstPtr<RHI::DeviceImageView>>& imageViews, RHI::ShaderInputImageType imageType);
            void UpdateSamplers(uint32_t index, const AZStd::span<const RHI::SamplerState>& samplers);
            void UpdateConstantData(AZStd::span<const uint8_t> data);

            RHI::Ptr<BufferView> GetConstantDataBufferView() const;

        private:
            // we use the small_vector to speed up SRG compilation, but we have to be a bit careful with the size of the pre-allocation:
            // e.g. the StarterGame - Level has about 8000 objects (~4000 meshes with 2 LODs) that get a unique Draw-Srgs for
            // each pass. And with 2 Material-Pipelines we have about 20 passes, so we end up with 160.000 unique Draw-Srgs
            struct WriteDescriptorData
            {
                uint32_t m_layoutIndex = 0;
                AZStd::small_vector<VkDescriptorBufferInfo, ViewsFixedsize> m_bufferViewsInfo;
                AZStd::small_vector<VkDescriptorImageInfo, ViewsFixedsize> m_imageViewsInfo;
                AZStd::small_vector<VkBufferView, ViewsFixedsize> m_texelBufferViews;
                AZStd::small_vector<VkAccelerationStructureKHR, ViewsFixedsize> m_accelerationStructures;
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
            AZStd::small_vector<RHI::Interval, ViewsFixedsize> GetValidDescriptorsIntervals(const AZStd::span<T>& descriptorsInfo) const;

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
        AZStd::small_vector<RHI::Interval, DescriptorSet::ViewsFixedsize> DescriptorSet::GetValidDescriptorsIntervals(
            const AZStd::span<T>& descriptorsInfo) const
        {
            AZStd::small_vector<RHI::Interval, DescriptorSet::ViewsFixedsize> intervals;
            // if Null descriptors are supported, then we just return one interval that covers the whole range.
            if (m_nullDescriptorSupported)
            {
                intervals.push_back(RHI::Interval(0, aznumeric_caster(descriptorsInfo.size())));
                return intervals;
            }

            auto beginInterval = descriptorsInfo.begin();
            auto endInterval = beginInterval;
            bool (*IsNullFuntion)(const T&) = &DescriptorSet::IsNullDescriptorInfo;
            do 
            {
                beginInterval = AZStd::find_if_not(endInterval, descriptorsInfo.end(), IsNullFuntion);
                if (beginInterval != descriptorsInfo.end())
                {
                    endInterval = AZStd::find_if(beginInterval, descriptorsInfo.end(), IsNullFuntion);

                    intervals.push_back({});
                    RHI::Interval& interval = intervals.span().back();
                    interval.m_min = aznumeric_caster(AZStd::distance(descriptorsInfo.begin(), beginInterval));
                    interval.m_max = endInterval == descriptorsInfo.end() ? static_cast<uint32_t>(descriptorsInfo.size()) : static_cast<uint32_t>(AZStd::distance(descriptorsInfo.begin(), endInterval));
                }
            } while (beginInterval != descriptorsInfo.end() && endInterval != descriptorsInfo.end());

            return intervals;
        }
    }
}
