/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/string/conversions.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Instance.h>
#include <RHI/PhysicalDevice.h>
#include <Vulkan_Traits_Platform.h>

namespace AZ
{
    namespace Vulkan
    {
        [[maybe_unused]] static constexpr uint32_t VendorID_Nvidia = 0x10DE;
        [[maybe_unused]] static constexpr uint32_t VendorID_AMD = 0x1002;
        [[maybe_unused]] static constexpr uint32_t VendorID_Intel = 0x8086;

        RHI::PhysicalDeviceList PhysicalDevice::Enumerate()
        {
            RHI::PhysicalDeviceList physicalDeviceList;
            VkResult result = VK_SUCCESS;

            auto& instance = Instance::GetInstance();

            uint32_t physicalDeviceCount = 0;
            result = instance.GetContext().EnumeratePhysicalDevices(instance.GetNativeInstance(), &physicalDeviceCount, nullptr);
            AssertSuccess(result);
            if (physicalDeviceCount == 0)
            {
                AZ_Error("Vulkan", false, "No Vulkan compatible physical devices were found!");
                return physicalDeviceList;
            }

            AZStd::vector<VkPhysicalDevice> physicalDevices;
            physicalDevices.resize(physicalDeviceCount);

            result =
                instance.GetContext().EnumeratePhysicalDevices(instance.GetNativeInstance(), &physicalDeviceCount, physicalDevices.data());
            AssertSuccess(result);

            if (ConvertResult(result) != RHI::ResultCode::Success)
            {
                AZ_Error("Vulkan", false, GetResultString(result));
                return physicalDeviceList;
            }

            if (physicalDevices.empty())
            {
                AZ_Error("Vulkan", false, "No suitable Vulkan devices were found!");
            }

            physicalDeviceList.reserve(physicalDevices.size());
            for (const VkPhysicalDevice& device : physicalDevices)
            {
                RHI::Ptr<PhysicalDevice> physicalDevice = aznew PhysicalDevice;
                physicalDevice->Init(device);
                physicalDeviceList.emplace_back(physicalDevice);
            }

            return physicalDeviceList;
        }

        const VkPhysicalDevice& PhysicalDevice::GetNativePhysicalDevice() const
        {
            return m_vkPhysicalDevice;
        }

        const VkPhysicalDeviceMemoryProperties& PhysicalDevice::GetMemoryProperties() const
        {
            return m_memoryProperty;
        }

        const VkPhysicalDeviceLimits& PhysicalDevice::GetDeviceLimits() const
        {
            return m_deviceProperties.limits;
        }

        const VkPhysicalDeviceFeatures& PhysicalDevice::GetPhysicalDeviceFeatures() const
        {
            return m_deviceFeatures;
        }

        const VkPhysicalDeviceProperties& PhysicalDevice::GetPhysicalDeviceProperties() const
        {
            return m_deviceProperties;
        }

        const VkPhysicalDeviceConservativeRasterizationPropertiesEXT& PhysicalDevice::GetPhysicalDeviceConservativeRasterProperties() const
        {
            return m_conservativeRasterProperties;
        }

        const VkPhysicalDeviceDepthClipEnableFeaturesEXT& PhysicalDevice::GetPhysicalDeviceDepthClipEnableFeatures() const
        {
            return m_dephClipEnableFeatures;
        }

        const VkPhysicalDeviceRobustness2FeaturesEXT& PhysicalDevice::GetPhysicalDeviceRobutness2Features() const
        {
            return m_robutness2Features;
        }

        const VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR& PhysicalDevice::GetPhysicalDeviceSeparateDepthStencilFeatures() const
        {
            return m_separateDepthStencilFeatures;
        }

        const VkPhysicalDeviceShaderAtomicInt64Features& PhysicalDevice::GetShaderAtomicInt64Features() const
        {
            return m_shaderAtomicInt64Features;
        }

        const VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT& PhysicalDevice::GetShaderImageAtomicInt64Features() const
        {
            return m_shaderImageAtomicInt64Features;
        }

        const VkPhysicalDeviceAccelerationStructurePropertiesKHR& PhysicalDevice::GetPhysicalDeviceAccelerationStructureProperties() const
        {
            return m_accelerationStructureProperties;
        }

        const VkPhysicalDeviceAccelerationStructureFeaturesKHR& PhysicalDevice::GetPhysicalDeviceAccelerationStructureFeatures() const
        {
            return m_accelerationStructureFeatures;
        }

        const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& PhysicalDevice::GetPhysicalDeviceRayTracingPipelineProperties() const
        {
            return m_rayTracingPipelineProperties;
        }

        const VkPhysicalDeviceRayTracingPipelineFeaturesKHR& PhysicalDevice::GetPhysicalDeviceRayTracingPipelineFeatures() const
        {
            return m_rayTracingPipelineFeatures;
        }

        const VkPhysicalDeviceRayQueryFeaturesKHR& PhysicalDevice::GetRayQueryFeatures() const
        {
            return m_rayQueryFeatures;
        }

        const VkPhysicalDeviceFragmentShadingRateFeaturesKHR& PhysicalDevice::GetPhysicalDeviceFragmentShadingRateFeatures() const
        {
            return m_shadingRateFeatures;
        }

        const VkPhysicalDeviceFragmentDensityMapFeaturesEXT& PhysicalDevice::GetPhysicalDeviceFragmentDensityMapFeatures() const
        {
            return m_fragmentDensityMapFeatures;
        }

        const VkPhysicalDeviceFragmentDensityMapPropertiesEXT& PhysicalDevice::GetPhysicalDeviceFragmentDensityMapProperties() const
        {
            return m_fragmentDensityMapProperties;
        }

        const VkPhysicalDeviceFragmentShadingRatePropertiesKHR& PhysicalDevice::GetPhysicalDeviceFragmentShadingRateProperties() const
        {
            return m_fragmentShadingRateProperties;
        }

        const VkPhysicalDeviceShaderFloat16Int8FeaturesKHR& PhysicalDevice::GetPhysicalDeviceFloat16Int8Features() const
        {
            return m_float16Int8Features;
        }

        const VkPhysicalDeviceDescriptorIndexingFeaturesEXT& PhysicalDevice::GetPhysicalDeviceDescriptorIndexingFeatures() const
        {
            return m_descriptorIndexingFeatures;
        }

        const VkPhysicalDeviceBufferDeviceAddressFeaturesEXT& PhysicalDevice::GetPhysicalDeviceBufferDeviceAddressFeatures() const
        {
            return m_bufferDeviceAddressFeatures;
        }

        const VkPhysicalDeviceTimelineSemaphoreFeatures& PhysicalDevice::GetPhysicalDeviceTimelineSemaphoreFeatures() const
        {
            return m_timelineSemaphoreFeatures;
        }

        const VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT& PhysicalDevice::GetPhysicalSubpassMergeFeedbackFeatures() const
        {
            return m_subpassMergeFeedbackFeatures;
        }

        const VkPhysicalDeviceVulkan12Features& PhysicalDevice::GetPhysicalDeviceVulkan12Features() const
        {
            return m_vulkan12Features;
        }

        VkFormatProperties PhysicalDevice::GetFormatProperties(RHI::Format format, bool raiseAsserts) const
        {
            VkFormatProperties formatProperties{};
            VkFormat vkFormat = ConvertFormat(format, raiseAsserts);
            if (vkFormat != VK_FORMAT_UNDEFINED)
            {
                auto& instance = Instance::GetInstance();
                instance.GetContext().GetPhysicalDeviceFormatProperties(GetNativePhysicalDevice(), vkFormat, &formatProperties);
            }
            return formatProperties;
        }

        StringList PhysicalDevice::GetDeviceLayerNames() const
        {
            auto& instance = Instance::GetInstance();
            StringList layerNames;
            uint32_t layerPropertyCount = 0;
            VkResult result = instance.GetContext().EnumerateDeviceLayerProperties(m_vkPhysicalDevice, &layerPropertyCount, nullptr);
            if (IsError(result) || layerPropertyCount == 0)
            {
                return layerNames;
            }

            AZStd::vector<VkLayerProperties> layerProperties(layerPropertyCount);
            result = instance.GetContext().EnumerateDeviceLayerProperties(m_vkPhysicalDevice, &layerPropertyCount, layerProperties.data());
            if (IsError(result))
            {
                return layerNames;
            }

            layerNames.reserve(layerNames.size() + layerProperties.size());
            for (uint32_t layerPropertyIndex = 0; layerPropertyIndex < layerPropertyCount; ++layerPropertyIndex)
            {
                layerNames.emplace_back(layerProperties[layerPropertyIndex].layerName);
            }

            return layerNames;
        }

        StringList PhysicalDevice::GetDeviceExtensionNames(const char* layerName /*=nullptr*/) const
        {
            StringList extensionNames;
            uint32_t extPropertyCount = 0;
            auto& instance = Instance::GetInstance();
            VkResult result =
                instance.GetContext().EnumerateDeviceExtensionProperties(m_vkPhysicalDevice, layerName, &extPropertyCount, nullptr);
            if (IsError(result) || extPropertyCount == 0)
            {
                return extensionNames;
            }

            AZStd::vector<VkExtensionProperties> extProperties;
            extProperties.resize(extPropertyCount);

            result = instance.GetContext().EnumerateDeviceExtensionProperties(
                m_vkPhysicalDevice, layerName, &extPropertyCount, extProperties.data());
            if (IsError(result))
            {
                return extensionNames;
            }

            extensionNames.reserve(extensionNames.size() + extProperties.size());
            for (uint32_t extPropertyIndex = 0; extPropertyIndex < extPropertyCount; extPropertyIndex++)
            {
                extensionNames.emplace_back(extProperties[extPropertyIndex].extensionName);
            }
            return extensionNames;
        }

        bool PhysicalDevice::IsFormatSupported(RHI::Format format, VkImageTiling tiling, VkFormatFeatureFlags features) const
        {
            VkFormatProperties properties = GetFormatProperties(format);
            switch (tiling)
            {
            case VK_IMAGE_TILING_OPTIMAL:
                return RHI::CheckBitsAll(properties.optimalTilingFeatures, features);
            case VK_IMAGE_TILING_LINEAR:
                return RHI::CheckBitsAll(properties.linearTilingFeatures, features);
            default:
                AZ_Assert(false, "Invalid image tiling type %d", tiling);
                return false;
            }
        }

        void PhysicalDevice::LoadSupportedFeatures(const GladVulkanContext& context)
        {
            uint32_t majorVersion = VK_VERSION_MAJOR(GetVulkanVersion());
            uint32_t minorVersion = VK_VERSION_MINOR(GetVulkanVersion());

            m_features.reset();
            m_features.set(static_cast<size_t>(DeviceFeature::Compatible2dArrayTexture), (majorVersion >= 1 && minorVersion >= 1) || VK_DEVICE_EXTENSION_SUPPORTED(context, KHR_maintenance1));
            m_features.set(static_cast<size_t>(DeviceFeature::CustomSampleLocation), VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_sample_locations));
            m_features.set(static_cast<size_t>(DeviceFeature::Predication), VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_conditional_rendering));
            m_features.set(static_cast<size_t>(DeviceFeature::ConservativeRaster), VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_conservative_rasterization));
            m_features.set(static_cast<size_t>(DeviceFeature::DepthClipEnable), VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_depth_clip_enable) && m_dephClipEnableFeatures.depthClipEnable);
            m_features.set(static_cast<size_t>(DeviceFeature::DrawIndirectCount), (majorVersion >= 1 && minorVersion >= 2 && m_vulkan12Features.drawIndirectCount) || VK_DEVICE_EXTENSION_SUPPORTED(context, KHR_draw_indirect_count));
            m_features.set(static_cast<size_t>(DeviceFeature::NullDescriptor), m_robutness2Features.nullDescriptor && VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_robustness2));
            m_features.set(static_cast<size_t>(DeviceFeature::SeparateDepthStencil),
                (m_separateDepthStencilFeatures.separateDepthStencilLayouts && VK_DEVICE_EXTENSION_SUPPORTED(context, KHR_separate_depth_stencil_layouts)) ||
                (m_vulkan12Features.separateDepthStencilLayouts));
            m_features.set(static_cast<size_t>(DeviceFeature::DescriptorIndexing), VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_descriptor_indexing));
            m_features.set(static_cast<size_t>(DeviceFeature::BufferDeviceAddress), VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_buffer_device_address));
            // Disable memory budget extension for now since it's crashing the driver when the VkPhysicalDeviceMemoryBudgetPropertiesEXT structure
            // is included in the pNext chain of VkPhysicalDeviceMemoryProperties2
            m_features.set(
                static_cast<size_t>(DeviceFeature::MemoryBudget),
                VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_memory_budget) && m_deviceProperties.vendorID != VendorID_Intel);
            m_features.set(static_cast<size_t>(DeviceFeature::SubgroupOperation), (majorVersion >= 1 && minorVersion >= 1));
            m_features.set(static_cast<size_t>(DeviceFeature::LoadNoneOp), VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_load_store_op_none));
            m_features.set(
                static_cast<size_t>(DeviceFeature::StoreNoneOp),
                VK_DEVICE_EXTENSION_SUPPORTED(context, EXT_load_store_op_none) || (majorVersion >= 1 && minorVersion >= 3));
        }

        RawStringList PhysicalDevice::FilterSupportedOptionalExtensions()
        {
            // The order must match the enum OptionalDeviceExtensions
            RawStringList optionalExtensions = { { VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME,
                                                   VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME,
                                                   VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
                                                   VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME,
                                                   VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,
                                                   VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
                                                   VK_KHR_RELAXED_BLOCK_LAYOUT_EXTENSION_NAME,
                                                   VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
                                                   VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
                                                   VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME,
                                                   VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME,

                                                   // ray tracing extensions
                                                   VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                                                   VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                                                   VK_KHR_RAY_QUERY_EXTENSION_NAME,
                                                   VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
                                                   VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                                                   VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
                                                   VK_KHR_SPIRV_1_4_EXTENSION_NAME,
                                                   VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,

                                                   VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
                                                   VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME,
                                                   VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
                                                   VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
                                                   VK_EXT_LOAD_STORE_OP_NONE_EXTENSION_NAME,
                                                   VK_EXT_SUBPASS_MERGE_FEEDBACK_EXTENSION_NAME,
                                                   VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME } };

            [[maybe_unused]] uint32_t optionalExtensionCount = aznumeric_cast<uint32_t>(optionalExtensions.size());

            AZ_Assert(optionalExtensionCount == static_cast<uint32_t>(OptionalDeviceExtension::Count), "The order and size must match the enum OptionalDeviceExtensions.");

            // Optional device extensions are filtered based on what the device support.
            // It returns in the same order as in the original list.
            StringList deviceExtensions = GetDeviceExtensionNames();
            RawStringList filteredOptionalExtensions = FilterList(optionalExtensions, deviceExtensions);

            // Mark the supported optional extensions in the bitset for faster look up compared to string search.
            uint32_t originalIndex = 0;
            for (const auto& extension : filteredOptionalExtensions)
            {
                AZ_Assert(originalIndex < optionalExtensionCount, "Out of range index. Check FilterList algorithm if list is returned in the original order.");
                while (strcmp(extension, optionalExtensions[originalIndex]) != 0)
                {
                    ++originalIndex;
                }
                m_optionalExtensions.set(originalIndex);
                ++originalIndex;
            }

            return filteredOptionalExtensions;
        }

        AZStd::vector<VkTimeDomainEXT> PhysicalDevice::GetCalibratedTimeDomains(const GladVulkanContext& context) const
        {
            AZStd::vector<VkTimeDomainEXT> time_domains;

            // Initialize time domain count:
            auto time_domain_count{ 0u };
            // Update time domain count:
            auto result = context.GetPhysicalDeviceCalibrateableTimeDomainsEXT(m_vkPhysicalDevice, &time_domain_count, nullptr);

            if (result == VK_SUCCESS)
            {
                // Resize time domains vector:
                time_domains.resize(time_domain_count);
                // Update time_domain vector:
                result = context.GetPhysicalDeviceCalibrateableTimeDomainsEXT(m_vkPhysicalDevice, &time_domain_count, time_domains.data());
            }

            return time_domains;
        }

        uint32_t PhysicalDevice::GetVulkanVersion() const
        {
            return m_vulkanVersion;
        }

        void PhysicalDevice::Init(VkPhysicalDevice vkPhysicalDevice)
        {
            m_vkPhysicalDevice = vkPhysicalDevice;
            const auto& context = Instance::GetInstance().GetContext();

            if (VK_INSTANCE_EXTENSION_SUPPORTED(context, KHR_get_physical_device_properties2))
            {
                // Features
                m_descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
                m_bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
                m_dephClipEnableFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
                m_shaderAtomicInt64Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
                m_shaderImageAtomicInt64Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT;
                m_rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
                m_robutness2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
                m_float16Int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR;
                m_separateDepthStencilFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
                m_vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
                m_accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
                m_rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
                m_shadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
                m_fragmentDensityMapFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT;
                m_timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
                m_subpassMergeFeedbackFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_MERGE_FEEDBACK_FEATURES_EXT;

                VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
                deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                AppendVkStruct(
                    deviceFeatures2,
                    { &m_descriptorIndexingFeatures,
                      &m_bufferDeviceAddressFeatures,
                      &m_dephClipEnableFeatures,
                      &m_shaderAtomicInt64Features,
                      &m_shaderImageAtomicInt64Features,
                      &m_rayQueryFeatures,
                      &m_robutness2Features,
                      &m_float16Int8Features,
                      &m_separateDepthStencilFeatures,
                      &m_vulkan12Features,
                      &m_accelerationStructureFeatures,
                      &m_rayTracingPipelineFeatures,
                      &m_shadingRateFeatures,
                      &m_fragmentDensityMapFeatures,
                      &m_timelineSemaphoreFeatures,
                      &m_subpassMergeFeedbackFeatures });

                context.GetPhysicalDeviceFeatures2KHR(vkPhysicalDevice, &deviceFeatures2);
                m_deviceFeatures = deviceFeatures2.features;

                // Properties
                m_conservativeRasterProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
                m_rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
                m_accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
                m_fragmentDensityMapProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT;
                m_fragmentShadingRateProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
                
                VkPhysicalDeviceProperties2 deviceProps2 = {};
                deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
                AppendVkStruct(
                    deviceProps2,
                    { &m_conservativeRasterProperties,
                      &m_rayTracingPipelineProperties,
                      &m_accelerationStructureProperties,
                      &m_fragmentDensityMapProperties,
                      &m_fragmentShadingRateProperties });

                context.GetPhysicalDeviceProperties2KHR(vkPhysicalDevice, &deviceProps2);
                m_deviceProperties = deviceProps2.properties;
            }
            else
            {
                context.GetPhysicalDeviceFeatures(vkPhysicalDevice, &m_deviceFeatures);
                context.GetPhysicalDeviceProperties(vkPhysicalDevice, &m_deviceProperties);
            }

            m_descriptor.m_description = m_deviceProperties.deviceName;

            switch (m_deviceProperties.deviceType)
            {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                m_descriptor.m_type = RHI::PhysicalDeviceType::GpuDiscrete;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                m_descriptor.m_type = RHI::PhysicalDeviceType::GpuIntegrated;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                m_descriptor.m_type = RHI::PhysicalDeviceType::GpuVirtual;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                m_descriptor.m_type = RHI::PhysicalDeviceType::Cpu;
                break;
            default:
                m_descriptor.m_type = RHI::PhysicalDeviceType::Unknown;
                break;
            }

            m_descriptor.m_vendorId = static_cast<RHI::VendorId>(m_deviceProperties.vendorID);
            m_descriptor.m_deviceId = m_deviceProperties.deviceID;
            m_descriptor.m_driverVersion = m_deviceProperties.driverVersion;

            context.GetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &m_memoryProperty);

            AZStd::set<uint32_t> heapIndicesDevice;
            AZStd::set<uint32_t> heapIndicesHost;
            for (uint32_t typeIndex = 0; typeIndex < m_memoryProperty.memoryTypeCount; ++typeIndex)
            {
                const VkMemoryPropertyFlags propertyFlags = m_memoryProperty.memoryTypes[typeIndex].propertyFlags;
                if (RHI::CheckBitsAny(propertyFlags, static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)))
                {
                    const uint32_t heapIndex = m_memoryProperty.memoryTypes[typeIndex].heapIndex;
                    AZ_Assert(heapIndex < m_memoryProperty.memoryHeapCount, "Heap Index is wrong.");
                    heapIndicesDevice.emplace(heapIndex);
                }
                else if (RHI::CheckBitsAny(propertyFlags, static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)))
                {
                    const uint32_t hidx = m_memoryProperty.memoryTypes[typeIndex].heapIndex;
                    AZ_Assert(hidx < m_memoryProperty.memoryHeapCount, "Heap Index is wrong.");
                    heapIndicesHost.emplace(hidx);
                }
            }

            VkDeviceSize memsize_device = 0;
            for (uint32_t heapIndex : heapIndicesDevice)
            {
                const VkMemoryHeap& heap = m_memoryProperty.memoryHeaps[heapIndex];
                AZ_Assert(RHI::CheckBitsAny(heap.flags, static_cast<VkMemoryHeapFlags>(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)), "Device local heap does not have device local bit.");
                memsize_device += heap.size;
            }

            VkDeviceSize memsize_host = 0;
            for (uint32_t heapIndex : heapIndicesHost)
            {
                const VkMemoryHeap& heap = m_memoryProperty.memoryHeaps[heapIndex];
                AZ_Assert(!RHI::CheckBitsAny(heap.flags, static_cast<VkMemoryHeapFlags>(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)), "Host heap have device local bit.");
                memsize_host += heap.size;
            }

            m_descriptor.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Device)] = static_cast<size_t>(memsize_device);
            m_descriptor.m_heapSizePerLevel[static_cast<size_t>(RHI::HeapMemoryLevel::Host)] = static_cast<size_t>(memsize_host);
            // We need to consider the application's vulkan version, since we cannot use a higher version than that, even though
            // the physical device might support a higher one.
            m_vulkanVersion = AZStd::min(Instance::GetInstance().GetVkAppInfo().apiVersion, m_deviceProperties.apiVersion);
        }

        void PhysicalDevice::Shutdown()
        {
            m_vkPhysicalDevice = VK_NULL_HANDLE;
        }

        bool PhysicalDevice::IsFeatureSupported(DeviceFeature feature) const
        {
            uint32_t index = static_cast<uint32_t>(feature);
            AZ_Assert(index < m_features.size(), "Invalid feature %d", index);
            return m_features.test(index);
        }

        bool PhysicalDevice::IsOptionalDeviceExtensionSupported(OptionalDeviceExtension optionalDeviceExtension) const
        {
            uint32_t index = static_cast<uint32_t>(optionalDeviceExtension);
            AZ_Assert(index < m_optionalExtensions.size(), "Invalid feature %d", index);
            return m_optionalExtensions.test(index);
        }

        void PhysicalDevice::DisableOptionalDeviceExtension(OptionalDeviceExtension optionalDeviceExtension)
        {
            uint32_t index = static_cast<uint32_t>(optionalDeviceExtension);
            AZ_Assert(index < m_optionalExtensions.size(), "Invalid feature %d", index);
            m_optionalExtensions.set(index, false);
        }
    }
}
