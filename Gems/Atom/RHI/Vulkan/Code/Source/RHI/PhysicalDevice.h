/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PhysicalDevice.h>
#include <AzCore/std/containers/bitset.h>
#include <Atom/RHI.Reflect/Format.h>
#include <RHI/Vulkan.h>

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
            BufferDeviceAddress,
            Count // Must be last
        };

        enum class OptionalDeviceExtension : uint32_t
        {
            SampleLocation = 0,
            ConditionalRendering,
            MemoryBudget,
            DepthClipEnable,
            ConservativeRasterization,
            DrawIndirectCount,
            RelaxedBlockLayout,
            Robustness2,
            ShaderFloat16Int8,
            ShaderAtomicInt64,
            ShaderImageAtomicInt64,
            AccelerationStructure,
            RayTracingPipeline,
            RayQuery,
            BufferDeviceAddress,
            DeferredHostOperations,
            DescriptorIndexing,
            Spirv14,
            ShaderFloatControls,
            Count
        };

        class PhysicalDevice final
            : public RHI::PhysicalDevice
        {
            using Base = RHI::PhysicalDevice;
        public:
            AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator, 0);
            AZ_RTTI(PhysicalDevice, "AD5F2BAD-A9B3-48F4-962F-C6D0760EEE17", Base);
            PhysicalDevice() = default;
            ~PhysicalDevice() = default;

            static RHI::PhysicalDeviceList Enumerate();
            void Init(VkPhysicalDevice vkPhysicalDevice);
            const VkPhysicalDevice& GetNativePhysicalDevice() const;
            const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const;
            bool IsFeatureSupported(DeviceFeature feature) const;
            bool IsOptionalDeviceExtensionSupported(OptionalDeviceExtension optionalDeviceExtension) const;
            const VkPhysicalDeviceLimits& GetDeviceLimits() const;
            const VkPhysicalDeviceFeatures& GetPhysicalDeviceFeatures() const;
            const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() const;
            const VkPhysicalDeviceConservativeRasterizationPropertiesEXT& GetPhysicalDeviceConservativeRasterProperties() const;
            const VkPhysicalDeviceDepthClipEnableFeaturesEXT& GetPhysicalDeviceDepthClipEnableFeatures() const;
            const VkPhysicalDeviceRobustness2FeaturesEXT& GetPhysicalDeviceRobutness2Features() const;
            const VkPhysicalDeviceShaderFloat16Int8FeaturesKHR& GetPhysicalDeviceFloat16Int8Features() const;
            const VkPhysicalDeviceDescriptorIndexingFeaturesEXT& GetPhysicalDeviceDescriptorIndexingFeatures() const;
            const VkPhysicalDeviceBufferDeviceAddressFeaturesEXT& GetPhysicalDeviceBufferDeviceAddressFeatures() const;
            const VkPhysicalDeviceVulkan12Features& GetPhysicalDeviceVulkan12Features() const;
            const VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR& GetPhysicalDeviceSeparateDepthStencilFeatures() const;
            const VkPhysicalDeviceShaderAtomicInt64Features& GetShaderAtomicInt64Features() const;
            const VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT& GetShaderImageAtomicInt64Features() const;
            const VkPhysicalDeviceAccelerationStructurePropertiesKHR& GetPhysicalDeviceAccelerationStructureProperties() const;
            const VkPhysicalDeviceAccelerationStructureFeaturesKHR& GetPhysicalDeviceAccelerationStructureFeatures() const;
            const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetPhysicalDeviceRayTracingPipelineProperties() const;
            const VkPhysicalDeviceRayTracingPipelineFeaturesKHR& GetPhysicalDeviceRayTracingPipelineFeatures() const;
            const VkPhysicalDeviceRayQueryFeaturesKHR& GetRayQueryFeatures() const;
            VkFormatProperties GetFormatProperties(RHI::Format format, bool raiseAsserts = true) const;
            StringList GetDeviceLayerNames() const;
            StringList GetDeviceExtensionNames(const char* layerName = nullptr) const;
            bool IsFormatSupported(RHI::Format format, VkImageTiling tiling, VkFormatFeatureFlags features) const;
            void LoadSupportedFeatures(const GladVulkanContext& context);
            //! Filter optional extensions based on what the physics device support.
            RawStringList FilterSupportedOptionalExtensions();
            void CompileMemoryStatistics(const GladVulkanContext& context, RHI::MemoryStatisticsBuilder& builder) const;

        private:
            

            ///////////////////////////////////////////////////////////////////
            // RHI::PhysicalDevice
            void Shutdown() override;
           ///////////////////////////////////////////////////////////////////

            VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
            VkPhysicalDeviceMemoryProperties m_memoryProperty{};

            AZStd::bitset<static_cast<uint32_t>(OptionalDeviceExtension::Count)> m_optionalExtensions;
            AZStd::bitset<static_cast<uint32_t>(DeviceFeature::Count)> m_features;
            VkPhysicalDeviceFeatures m_deviceFeatures{};
            VkPhysicalDeviceProperties m_deviceProperties{};
            VkPhysicalDeviceConservativeRasterizationPropertiesEXT m_conservativeRasterProperties{};
            VkPhysicalDeviceDepthClipEnableFeaturesEXT m_dephClipEnableFeatures{};
            VkPhysicalDeviceRobustness2FeaturesEXT m_robutness2Features{};
            VkPhysicalDeviceShaderFloat16Int8FeaturesKHR m_float16Int8Features{};
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT m_descriptorIndexingFeatures{};
            VkPhysicalDeviceBufferDeviceAddressFeaturesEXT m_bufferDeviceAddressFeatures{};
            VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR m_separateDepthStencilFeatures{};
            VkPhysicalDeviceShaderAtomicInt64Features m_shaderAtomicInt64Features{};
            VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT m_shaderImageAtomicInt64Features{};
            VkPhysicalDeviceAccelerationStructurePropertiesKHR m_accelerationStructureProperties{};
            VkPhysicalDeviceAccelerationStructureFeaturesKHR m_accelerationStructureFeatures{};
            VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rayTracingPipelineProperties{};
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR m_rayTracingPipelineFeatures{};
            VkPhysicalDeviceRayQueryFeaturesKHR m_rayQueryFeatures{};
            VkPhysicalDeviceVulkan12Features m_vulkan12Features{};
        };
    }
}
