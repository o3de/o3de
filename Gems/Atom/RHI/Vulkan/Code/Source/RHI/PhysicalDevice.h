/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PhysicalDevice.h>
#include <AzCore/std/containers/bitset.h>
#include <Atom/RHI.Reflect/Format.h>

namespace AZ
{
    namespace RHI
    {
        class MemoryStatisticsBuilder;
    }

    namespace Vulkan
    {
        enum class DeviceFeature : uint32_t
        {
            Compatible2dArrayTexture = 0,
            CustomSampleLocation,
            Predication,
            DepthClipEnable,
            ConservativeRaster,
            DrawIndirectCount,
            NullDescriptor,
            SeparateDepthStencil,
            DescriptorIndexing,
            Count // Must be last
        };

        class PhysicalDevice final
            : public RHI::PhysicalDevice
        {
            using Base = RHI::PhysicalDevice;
        public:
            AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator, 0);
            AZ_RTTI(PhysicalDevice, "AD5F2BAD-A9B3-48F4-962F-C6D0760EEE17", Base);
            ~PhysicalDevice() = default;

            static RHI::PhysicalDeviceList Enumerate();
            const VkPhysicalDevice& GetNativePhysicalDevice() const;
            const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const;
            bool IsFeatureSupported(DeviceFeature feature) const;
            const VkPhysicalDeviceLimits& GetDeviceLimits() const;
            const VkPhysicalDeviceFeatures& GetPhysicalDeviceFeatures() const;
            const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() const;
            const VkPhysicalDeviceConservativeRasterizationPropertiesEXT& GetPhysicalDeviceConservativeRasterProperties() const;
            const VkPhysicalDeviceDepthClipEnableFeaturesEXT& GetPhysicalDeviceDepthClipEnableFeatures() const;
            const VkPhysicalDeviceRobustness2FeaturesEXT& GetPhysicalDeviceRobutness2Features() const;
            const VkPhysicalDeviceShaderFloat16Int8FeaturesKHR& GetPhysicalDeviceFloat16Int8Features() const;
            const VkPhysicalDeviceDescriptorIndexingFeaturesEXT& GetPhysicalDeviceDescriptorIndexingFeatures() const;
            const VkPhysicalDeviceVulkan12Features& GetPhysicalDeviceVulkan12Features() const;
            const VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR& GetPhysicalDeviceSeparateDepthStencilFeatures() const;
            const VkPhysicalDeviceAccelerationStructurePropertiesKHR& GetPhysicalDeviceAccelerationStructureProperties() const;
            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetPhysicalDeviceRayTracingPipelineProperties() const;
            VkFormatProperties GetFormatProperties(RHI::Format format, bool raiseAsserts = true) const;
            StringList GetDeviceLayerNames() const;
            StringList GetDeviceExtensionNames(const char* layerName = nullptr) const;
            bool IsFormatSupported(RHI::Format format, VkImageTiling tiling, VkFormatFeatureFlags features) const;
            void LoadSupportedFeatures();
            void CompileMemoryStatistics(RHI::MemoryStatisticsBuilder& builder) const;

        private:
            PhysicalDevice() = default;
            void Init(VkPhysicalDevice vkPhysicalDevice);

            ///////////////////////////////////////////////////////////////////
            // RHI::PhysicalDevice
            void Shutdown() override;
           ///////////////////////////////////////////////////////////////////

            VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
            VkPhysicalDeviceMemoryProperties m_memoryProperty{};

            AZStd::bitset<static_cast<uint32_t>(DeviceFeature::Count)> m_features;
            VkPhysicalDeviceFeatures m_deviceFeatures{};
            VkPhysicalDeviceProperties m_deviceProperties{};
            VkPhysicalDeviceConservativeRasterizationPropertiesEXT m_conservativeRasterProperties{};
            VkPhysicalDeviceDepthClipEnableFeaturesEXT m_dephClipEnableFeatures{};
            VkPhysicalDeviceRobustness2FeaturesEXT m_robutness2Features{};
            VkPhysicalDeviceShaderFloat16Int8FeaturesKHR m_float16Int8Features{};
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT m_descriptorIndexingFeatures{};
            VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR m_separateDepthStencilFeatures{};
            VkPhysicalDeviceAccelerationStructurePropertiesKHR m_accelerationStructureProperties{};
            VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rayTracingPipelineProperties{};
            VkPhysicalDeviceVulkan12Features m_vulkan12Features{};
        };
    }
}
