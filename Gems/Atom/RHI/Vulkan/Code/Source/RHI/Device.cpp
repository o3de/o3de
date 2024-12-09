/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/VkAllocator.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI.Reflect/Vulkan/PlatformLimitsDescriptor.h>
#include <Atom/RHI.Reflect/Vulkan/VulkanBus.h>
#include <Atom/RHI.Reflect/Vulkan/XRVkDescriptors.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHIMemoryStatisticsInterface.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DeviceTransientAttachmentPool.h>
#include <Atom_RHI_Vulkan_Platform.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/CommandList.h>
#include <RHI/CommandQueue.h>
#include <RHI/Device.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/ImagePool.h>
#include <RHI/Instance.h>
#include <RHI/Pipeline.h>
#include <RHI/SwapChain.h>
#include <RHI/WSISurface.h>
#include <RHI/WindowSurfaceBus.h>
#include <Vulkan_Traits_Platform.h>

namespace AZ
{
    namespace Vulkan
    {
        Device::Device()
        {
            RHI::Ptr<PlatformLimitsDescriptor> platformLimitsDescriptor = aznew PlatformLimitsDescriptor();
            platformLimitsDescriptor->LoadPlatformLimitsDescriptor(RHI::Factory::Get().GetName().GetCStr());
            m_descriptor.m_platformLimitsDescriptor = RHI::Ptr<RHI::PlatformLimitsDescriptor>(platformLimitsDescriptor);
        }

        Device::~Device() = default;

        RHI::Ptr<Device> Device::Create()
        {
            return aznew Device();
        }

        StringList Device::GetRequiredLayers()
        {
            // No required layers for now
            return StringList();
        }

        StringList Device::GetRequiredExtensions()
        {
            StringList requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
            AZStd::vector<AZStd::string> collectedExtensions;
            DeviceRequirementBus::Broadcast(&DeviceRequirementBus::Events::CollectAdditionalRequiredDeviceExtensions, collectedExtensions);
            requiredExtensions.insert(requiredExtensions.end(), collectedExtensions.begin(), collectedExtensions.end());
            return requiredExtensions;
        }

        void Device::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeDevice), name.data(), VK_OBJECT_TYPE_DEVICE, *this);
            }
        }

        RHI::ResultCode Device::InitInternal(RHI::PhysicalDevice& physicalDeviceBase)
        {
            m_loaderContext = LoaderContext::Create();
            if (!m_loaderContext)
            {
                return RHI::ResultCode::Fail;
            }

            auto& physicalDevice = static_cast<Vulkan::PhysicalDevice&>(physicalDeviceBase);
            StringList requiredLayerStrings = GetRequiredLayers();
            StringList requiredExtensionStrings = GetRequiredExtensions();
            RawStringList requiredLayers, requiredExtensions;
            ToRawStringList(requiredLayerStrings, requiredLayers);
            ToRawStringList(requiredExtensionStrings, requiredExtensions);

            RawStringList optionalExtensions = physicalDevice.FilterSupportedOptionalExtensions();
            requiredExtensions.insert(requiredExtensions.end(), optionalExtensions.begin(), optionalExtensions.end());

            // We now need to find the queues that the physical device has available and make sure
            // it has what we need. We're just expecting a Graphics queue for now.
            BuildDeviceQueueInfo(physicalDevice);

            m_supportedPipelineStageFlagsMask = std::numeric_limits<VkPipelineStageFlags>::max();

            const auto& deviceFeatures = physicalDevice.GetPhysicalDeviceFeatures();
            m_enabledDeviceFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;
            m_enabledDeviceFeatures.textureCompressionETC2 = deviceFeatures.textureCompressionETC2;
            m_enabledDeviceFeatures.textureCompressionBC = deviceFeatures.textureCompressionBC;
            m_enabledDeviceFeatures.fragmentStoresAndAtomics = deviceFeatures.fragmentStoresAndAtomics;
            m_enabledDeviceFeatures.dualSrcBlend = deviceFeatures.dualSrcBlend;
            m_enabledDeviceFeatures.imageCubeArray = deviceFeatures.imageCubeArray;
            m_enabledDeviceFeatures.pipelineStatisticsQuery = deviceFeatures.pipelineStatisticsQuery;
            m_enabledDeviceFeatures.occlusionQueryPrecise = deviceFeatures.occlusionQueryPrecise;
            m_enabledDeviceFeatures.depthClamp = deviceFeatures.depthClamp;
            m_enabledDeviceFeatures.depthBiasClamp = deviceFeatures.depthBiasClamp;
            m_enabledDeviceFeatures.depthBounds = deviceFeatures.depthBounds;
            m_enabledDeviceFeatures.fillModeNonSolid = deviceFeatures.fillModeNonSolid;
            m_enabledDeviceFeatures.multiDrawIndirect = deviceFeatures.multiDrawIndirect;
            m_enabledDeviceFeatures.sampleRateShading = deviceFeatures.sampleRateShading;
            m_enabledDeviceFeatures.shaderImageGatherExtended = deviceFeatures.shaderImageGatherExtended;
            m_enabledDeviceFeatures.shaderInt64 = deviceFeatures.shaderInt64;
            m_enabledDeviceFeatures.shaderInt16 = deviceFeatures.shaderInt16;
            m_enabledDeviceFeatures.sparseBinding = deviceFeatures.sparseBinding;
            m_enabledDeviceFeatures.sparseResidencyImage2D = deviceFeatures.sparseResidencyImage2D;
            m_enabledDeviceFeatures.sparseResidencyImage3D = deviceFeatures.sparseResidencyImage3D;
            m_enabledDeviceFeatures.sparseResidencyAliased = deviceFeatures.sparseResidencyAliased;
            m_enabledDeviceFeatures.independentBlend = deviceFeatures.independentBlend;
            m_enabledDeviceFeatures.shaderStorageImageMultisample = deviceFeatures.shaderStorageImageMultisample;
            m_enabledDeviceFeatures.shaderStorageImageReadWithoutFormat = deviceFeatures.shaderStorageImageReadWithoutFormat;

            if (deviceFeatures.geometryShader)
            {
                m_enabledDeviceFeatures.geometryShader = VK_TRUE;
            }
            else
            {
                m_supportedPipelineStageFlagsMask &= ~VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
            }

            if (deviceFeatures.tessellationShader)
            {
                m_enabledDeviceFeatures.tessellationShader = VK_TRUE;
            }
            else
            {
                m_supportedPipelineStageFlagsMask &= ~(VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT);
            }

            AZStd::vector<VkDeviceQueueCreateInfo> queueCreationInfo;
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

            for (size_t familyIndex = 0; familyIndex < m_queueFamilyProperties.size(); ++familyIndex)
            {
                const VkQueueFamilyProperties& familyProperties = m_queueFamilyProperties[familyIndex];

                // [GFX TODO][ATOM-286] Figure out if we care about queue priority
                float* queuePriorities = new float[familyProperties.queueCount];
                for (size_t index = 0; index < familyProperties.queueCount; ++index)
                {
                    queuePriorities[index] = 1.0f;
                }

                queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(familyIndex);
                queueCreateInfo.queueCount = familyProperties.queueCount;
                queueCreateInfo.pQueuePriorities = queuePriorities;

                queueCreationInfo.push_back(queueCreateInfo);
            }

            uint32_t physicalDeviceVersion = physicalDevice.GetVulkanVersion();
            uint32_t majorVersion = VK_VERSION_MAJOR(physicalDeviceVersion);
            uint32_t minorVersion = VK_VERSION_MINOR(physicalDeviceVersion);

            // unbounded array functionality
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures = {};
            VkBaseOutStructure& chainInit = reinterpret_cast<VkBaseOutStructure&>(descriptorIndexingFeatures);
            descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            const VkPhysicalDeviceDescriptorIndexingFeaturesEXT& physicalDeviceDescriptorIndexingFeatures =
                physicalDevice.GetPhysicalDeviceDescriptorIndexingFeatures();
            descriptorIndexingFeatures.shaderInputAttachmentArrayDynamicIndexing = physicalDeviceDescriptorIndexingFeatures.shaderInputAttachmentArrayDynamicIndexing;
            descriptorIndexingFeatures.shaderUniformTexelBufferArrayDynamicIndexing = physicalDeviceDescriptorIndexingFeatures.shaderUniformTexelBufferArrayDynamicIndexing;
            descriptorIndexingFeatures.shaderStorageTexelBufferArrayDynamicIndexing = physicalDeviceDescriptorIndexingFeatures.shaderStorageTexelBufferArrayDynamicIndexing;
            descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = physicalDeviceDescriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing;
            descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = physicalDeviceDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing;
            descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = physicalDeviceDescriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing;
            descriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = physicalDeviceDescriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing;
            descriptorIndexingFeatures.shaderInputAttachmentArrayNonUniformIndexing = physicalDeviceDescriptorIndexingFeatures.shaderInputAttachmentArrayNonUniformIndexing;
            descriptorIndexingFeatures.shaderUniformTexelBufferArrayNonUniformIndexing = physicalDeviceDescriptorIndexingFeatures.shaderUniformTexelBufferArrayNonUniformIndexing;
            descriptorIndexingFeatures.shaderStorageTexelBufferArrayNonUniformIndexing = physicalDeviceDescriptorIndexingFeatures.shaderStorageTexelBufferArrayNonUniformIndexing;
            descriptorIndexingFeatures.descriptorBindingPartiallyBound = physicalDeviceDescriptorIndexingFeatures.shaderStorageTexelBufferArrayNonUniformIndexing;
            descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = physicalDeviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount;
            descriptorIndexingFeatures.runtimeDescriptorArray = physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray;
            descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind =
                physicalDeviceDescriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind;
            descriptorIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind =
                physicalDeviceDescriptorIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind;
            descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind =
                physicalDeviceDescriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind;

            auto bufferDeviceAddressFeatures = physicalDevice.GetPhysicalDeviceBufferDeviceAddressFeatures();
            auto depthClipEnabled = physicalDevice.GetPhysicalDeviceDepthClipEnableFeatures();
            auto rayQueryFeatures = physicalDevice.GetRayQueryFeatures();
            auto shaderImageAtomicInt64 = physicalDevice.GetShaderImageAtomicInt64Features();

            VkPhysicalDeviceRobustness2FeaturesEXT robustness2 = {};
            robustness2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
            robustness2.nullDescriptor = physicalDevice.GetPhysicalDeviceRobutness2Features().nullDescriptor;

            bufferDeviceAddressFeatures.pNext = nullptr;
            depthClipEnabled.pNext = nullptr;
            rayQueryFeatures.pNext = nullptr;
            shaderImageAtomicInt64.pNext = nullptr;
            robustness2.pNext = nullptr;

            AppendVkStruct(
                chainInit, { &bufferDeviceAddressFeatures, &depthClipEnabled, &rayQueryFeatures, &shaderImageAtomicInt64, &robustness2 });

            auto subpassMergeFeedback = physicalDevice.GetPhysicalSubpassMergeFeedbackFeatures();

#if !defined(AZ_RELEASE_BUILD)
            subpassMergeFeedback.subpassMergeFeedback = false;
#endif
            if (!subpassMergeFeedback.subpassMergeFeedback &&
                physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::SubpassMergeFeedback))
            {
                physicalDevice.DisableOptionalDeviceExtension(OptionalDeviceExtension::SubpassMergeFeedback);
                subpassMergeFeedback.pNext = nullptr;
                AppendVkStruct(chainInit, &subpassMergeFeedback);
            }

            auto fragmenDensityMapFeatures = physicalDevice.GetPhysicalDeviceFragmentDensityMapFeatures();
            auto fragmenShadingRateFeatures = physicalDevice.GetPhysicalDeviceFragmentShadingRateFeatures();

            if (fragmenShadingRateFeatures.attachmentFragmentShadingRate)
            {
                // Must disable the "FragmentDensityMap" usage if "attachmentFragmentShadingRate" is enabled.
                physicalDevice.DisableOptionalDeviceExtension(OptionalDeviceExtension::FragmentDensityMap);
                fragmenShadingRateFeatures.pNext = nullptr;
                AppendVkStruct(chainInit, &fragmenShadingRateFeatures);
            }
            else if (fragmenDensityMapFeatures.fragmentDensityMap && fragmenDensityMapFeatures.fragmentDensityMapNonSubsampledImages)
            {
                // We only support NonSubsampledImages when using fragment density map
                // Must disable the "FragmentShadingRate" usage if "fragmentDensityMap" is enabled.
                physicalDevice.DisableOptionalDeviceExtension(OptionalDeviceExtension::FragmentShadingRate);
                fragmenDensityMapFeatures.pNext = nullptr;
                AppendVkStruct(chainInit, &fragmenDensityMapFeatures);
            }

            VkPhysicalDeviceVulkan12Features vulkan12Features = {};
            VkPhysicalDeviceShaderFloat16Int8FeaturesKHR float16Int8 = {};
            VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR separateDepthStencil = {};
            VkPhysicalDeviceShaderAtomicInt64Features shaderAtomicInt64 = {};
            VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {};

            VkDeviceCreateInfo deviceInfo = {};
            deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

            auto timelineSemaphore = physicalDevice.GetPhysicalDeviceTimelineSemaphoreFeatures();
            // If we are running Vulkan >= 1.2, then we must use VkPhysicalDeviceVulkan12Features instead
            // of VkPhysicalDeviceShaderFloat16Int8FeaturesKHR or VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR.
            if (majorVersion >= 1 && minorVersion >= 2)
            {
                vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
                vulkan12Features.drawIndirectCount = physicalDevice.GetPhysicalDeviceVulkan12Features().drawIndirectCount;
                vulkan12Features.shaderFloat16 = physicalDevice.GetPhysicalDeviceVulkan12Features().shaderFloat16;
                vulkan12Features.shaderInt8 = physicalDevice.GetPhysicalDeviceVulkan12Features().shaderInt8;
                vulkan12Features.separateDepthStencilLayouts = physicalDevice.GetPhysicalDeviceVulkan12Features().separateDepthStencilLayouts;
                vulkan12Features.descriptorBindingPartiallyBound = physicalDevice.GetPhysicalDeviceVulkan12Features().separateDepthStencilLayouts;
                vulkan12Features.descriptorIndexing = physicalDevice.GetPhysicalDeviceVulkan12Features().separateDepthStencilLayouts;
                vulkan12Features.descriptorBindingVariableDescriptorCount = physicalDevice.GetPhysicalDeviceVulkan12Features().separateDepthStencilLayouts;
                // We use the "VkPhysicalDeviceBufferDeviceAddressFeatures" instead of the "VkPhysicalDeviceVulkan12Features" for buffer device address
                // because some drivers (e.g. Intel) don't report any features of buffer device address through the "PhysicalDeviceVulkan12Features" but they do
                // through the "VK_EXT_buffer_device_address" extension.
                vulkan12Features.bufferDeviceAddress = physicalDevice.GetPhysicalDeviceBufferDeviceAddressFeatures().bufferDeviceAddress;
                vulkan12Features.bufferDeviceAddressMultiDevice = physicalDevice.GetPhysicalDeviceBufferDeviceAddressFeatures().bufferDeviceAddressMultiDevice;
                vulkan12Features.runtimeDescriptorArray = physicalDevice.GetPhysicalDeviceVulkan12Features().runtimeDescriptorArray;
                vulkan12Features.shaderSharedInt64Atomics = physicalDevice.GetPhysicalDeviceVulkan12Features().shaderSharedInt64Atomics;
                vulkan12Features.shaderBufferInt64Atomics = physicalDevice.GetPhysicalDeviceVulkan12Features().shaderBufferInt64Atomics;
                vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = physicalDevice.GetPhysicalDeviceVulkan12Features().descriptorBindingSampledImageUpdateAfterBind;
                vulkan12Features.descriptorBindingStorageImageUpdateAfterBind = physicalDevice.GetPhysicalDeviceVulkan12Features().descriptorBindingStorageImageUpdateAfterBind;
                vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind = physicalDevice.GetPhysicalDeviceVulkan12Features().descriptorBindingStorageBufferUpdateAfterBind;
                vulkan12Features.descriptorBindingPartiallyBound = physicalDevice.GetPhysicalDeviceVulkan12Features().descriptorBindingPartiallyBound;
                vulkan12Features.descriptorBindingUpdateUnusedWhilePending = physicalDevice.GetPhysicalDeviceVulkan12Features().descriptorBindingUpdateUnusedWhilePending;
                vulkan12Features.shaderOutputViewportIndex = physicalDevice.GetPhysicalDeviceVulkan12Features().shaderOutputViewportIndex;
                vulkan12Features.shaderOutputLayer = physicalDevice.GetPhysicalDeviceVulkan12Features().shaderOutputLayer;
                vulkan12Features.timelineSemaphore = timelineSemaphore.timelineSemaphore;

                accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
                accelerationStructureFeatures.accelerationStructure = physicalDevice.GetPhysicalDeviceAccelerationStructureFeatures().accelerationStructure;

                rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
                rayTracingPipelineFeatures.rayTracingPipeline = physicalDevice.GetPhysicalDeviceRayTracingPipelineFeatures().rayTracingPipeline;
                rayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect = physicalDevice.GetPhysicalDeviceRayTracingPipelineFeatures().rayTracingPipelineTraceRaysIndirect;

                AppendVkStruct(chainInit, { &vulkan12Features, &accelerationStructureFeatures, &rayTracingPipelineFeatures });
                // Do not start from the chainInit, but from the depthClipEnabled struct
                deviceInfo.pNext = &depthClipEnabled;
            }
            else
            {
                float16Int8.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR;
                float16Int8.shaderFloat16 = physicalDevice.GetPhysicalDeviceFloat16Int8Features().shaderFloat16;

                separateDepthStencil.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
                separateDepthStencil.separateDepthStencilLayouts = physicalDevice.GetPhysicalDeviceSeparateDepthStencilFeatures().separateDepthStencilLayouts;

                shaderAtomicInt64.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
                shaderAtomicInt64.shaderBufferInt64Atomics = physicalDevice.GetShaderAtomicInt64Features().shaderBufferInt64Atomics;
                shaderAtomicInt64.shaderSharedInt64Atomics = physicalDevice.GetShaderAtomicInt64Features().shaderSharedInt64Atomics;

                AppendVkStruct(chainInit, { &float16Int8, &separateDepthStencil, &shaderAtomicInt64, &timelineSemaphore });
                deviceInfo.pNext = &chainInit;
            }

#if defined(USE_NSIGHT_AFTERMATH)
            requiredExtensions.push_back(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);

            // Set up device creation info for Aftermath feature flag configuration.
            VkDeviceDiagnosticsConfigFlagsNV aftermathFlags =
                VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV | // Enable tracking of resources.
                VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV | // Capture call stacks for all draw calls, compute
                                                                                   // dispatches, and resource copies.
                VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV; // Generate debug information for shaders.

            VkDeviceDiagnosticsConfigCreateInfoNV aftermathInfo = {};
            aftermathInfo.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
            aftermathInfo.flags = aftermathFlags;
            aftermathInfo.pNext = deviceInfo.pNext;
            deviceInfo.pNext = &aftermathInfo;
#endif

            deviceInfo.flags = 0;
            deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreationInfo.size());
            deviceInfo.pQueueCreateInfos = queueCreationInfo.data();
            deviceInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
            deviceInfo.ppEnabledLayerNames = requiredLayers.data();
            deviceInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
            deviceInfo.ppEnabledExtensionNames = requiredExtensions.data();
            deviceInfo.pEnabledFeatures = &m_enabledDeviceFeatures;

            Instance& instance = Instance::GetInstance();
            const VkResult vkResult = instance.GetContext().CreateDevice(
                physicalDevice.GetNativePhysicalDevice(),
                &deviceInfo,
                VkSystemAllocator::Get(),
                &m_nativeDevice);
            AssertSuccess(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));

            LoaderContext::Descriptor loaderDescriptor;
            loaderDescriptor.m_instance = instance.GetNativeInstance();
            loaderDescriptor.m_physicalDevice = physicalDevice.GetNativePhysicalDevice();
            loaderDescriptor.m_device = m_nativeDevice;
            loaderDescriptor.m_loadedExtensions = instance.GetLoadedExtensions();
            loaderDescriptor.m_loadedLayers = instance.GetLoadedLayers();
            if (!m_loaderContext->Init(loaderDescriptor))
            {
                AZ_Warning("Vulkan", false, "Could not initialize function loader.");
                return RHI::ResultCode::Fail;
            }

            if (physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::CalibratedTimestamps))
            {
                InitializeTimeDomains();
            }

            for (const VkDeviceQueueCreateInfo& queueInfo : queueCreationInfo)
            {
                delete[] queueInfo.pQueuePriorities;
            }

            //Load device features now that we have loaded all extension info
            physicalDevice.LoadSupportedFeatures(GetContext());

            if (!physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::FragmentShadingRate))
            {
                m_supportedPipelineStageFlagsMask &= ~VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
            }

            if (!physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::FragmentDensityMap))
            {
                m_supportedPipelineStageFlagsMask &= ~VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
            }

            RHI::ResultCode resultCode = InitVmaAllocator(physicalDeviceBase);
            RHI::RHISystemNotificationBus::Handler::BusConnect();
            return resultCode;
        }

        RHI::ResultCode Device::InitInternalBindlessSrg(const AZ::RHI::BindlessSrgDescriptor& bindlessSrgDesc)
        {
            RHI::ResultCode result = m_bindlessDescriptorPool.Init(*this, bindlessSrgDesc);
            return result;
        }

        RHI::ResultCode Device::InitializeLimits()
        {
            CommandQueueContext::Descriptor commandQueueContextDescriptor;
            commandQueueContextDescriptor.m_frameCountMax = RHI::Limits::Device::FrameCountMax;
            RHI::ResultCode result = m_commandQueueContext.Init(*this, commandQueueContextDescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            InitFeaturesAndLimits(static_cast<const Vulkan::PhysicalDevice&>(GetPhysicalDevice()));

            // Initialize member variables.
            ReleaseQueue::Descriptor releaseQueueDescriptor;
            releaseQueueDescriptor.m_collectLatency = m_descriptor.m_frameCountMax - 1;
            m_releaseQueue.Init(releaseQueueDescriptor);


            // Set the cache sizes.
            m_renderPassCache.first.SetCapacity(RenderPassCacheCapacity);
            m_framebufferCache.first.SetCapacity(FrameBufferCacheCapacity);
            m_descriptorSetLayoutCache.first.SetCapacity(DescriptorLayoutCacheCapacity);
            m_samplerCache.first.SetCapacity(SamplerCacheCapacity);
            m_pipelineLayoutCache.first.SetCapacity(PipelineLayoutCacheCapacity);

            CommandListAllocator::Descriptor cmdPooldescriptor;
            cmdPooldescriptor.m_device = this;
            cmdPooldescriptor.m_frameCountMax = RHI::Limits::Device::FrameCountMax;
            cmdPooldescriptor.m_familyQueueCount = static_cast<uint32_t>(m_queueFamilyProperties.size());
            result = m_commandListAllocator.Init(cmdPooldescriptor);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            SemaphoreAllocator::Descriptor semaphoreAllocDescriptor;
            semaphoreAllocDescriptor.m_device = this;
            semaphoreAllocDescriptor.m_collectLatency = RHI::Limits::Device::FrameCountMax;
            m_semaphoreAllocator.Init(semaphoreAllocDescriptor);

            SwapChainSemaphoreAllocator::Descriptor swapChainSemaphoreAllocDescriptor;
            swapChainSemaphoreAllocDescriptor.m_device = this;
            swapChainSemaphoreAllocDescriptor.m_collectLatency = RHI::Limits::Device::FrameCountMax;
            m_swapChainSemaphoreAllocator.Init(swapChainSemaphoreAllocDescriptor);

            m_imageMemoryRequirementsCache.SetInitFunction([](auto& cache) { cache.set_capacity(MemoryRequirementsCacheSize); });
            m_bufferMemoryRequirementsCache.SetInitFunction([](auto& cache) { cache.set_capacity(MemoryRequirementsCacheSize); });

            m_stagingBufferPool = BufferPool::Create();
            RHI::BufferPoolDescriptor poolDesc;
            poolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
            poolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
            poolDesc.m_bindFlags = RHI::BufferBindFlags::CopyRead;
            poolDesc.m_budgetInBytes = m_descriptor.m_platformLimitsDescriptor->m_platformDefaultValues.m_stagingBufferBudgetInBytes;
            m_stagingBufferPool->SetName(AZ::Name("Device_StagingBufferPool"));
            result = m_stagingBufferPool->Init(*this, poolDesc);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            {
                m_constantBufferPool = BufferPool::Create();
                static int index = 0;
                m_constantBufferPool->SetName(Name(AZStd::string::format("ConstantPool_%d", ++index)));

                RHI::BufferPoolDescriptor bufferPoolDescriptor;
                bufferPoolDescriptor.m_bindFlags = RHI::BufferBindFlags::Constant;
                bufferPoolDescriptor.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
                result = m_constantBufferPool->Init(*this, bufferPoolDescriptor);
                RETURN_RESULT_IF_UNSUCCESSFUL(result);
            }

            SetName(GetName());
            return result;
        }

        VkDevice Device::GetNativeDevice() const
        {
            return m_nativeDevice;
        }

        const GladVulkanContext& Device::GetContext() const
        {
            return m_loaderContext->GetContext();
        }

        uint32_t Device::FindMemoryTypeIndex(VkMemoryPropertyFlags memoryPropertyFlags, uint32_t memoryTypeBits) const
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            const VkPhysicalDeviceMemoryProperties& memProp = physicalDevice.GetMemoryProperties();
            for ( uint32_t index = 0; index < memProp.memoryTypeCount; ++index)
            {
                if (RHI::CheckBitsAll(memProp.memoryTypes[index].propertyFlags, memoryPropertyFlags) &&
                    ((1 << index) & memoryTypeBits))
                {
                    return index;
                }
            }

            return VK_MAX_MEMORY_TYPES; // not found
        }

        VkMemoryRequirements Device::GetImageMemoryRequirements(const RHI::ImageDescriptor& descriptor)
        {
            auto& cache = m_imageMemoryRequirementsCache.GetStorage();

            const uint64_t hash = static_cast<uint64_t>(descriptor.GetHash());
            auto it = cache.get(hash);
            if (it != cache.end())
            {
                return it->second;
            }
            else
            {
                // Need to create an image to get the requirements.
                // This will not allocate or bind memory.
                ImageCreateInfo createInfo = BuildImageCreateInfo(descriptor);
                VkImage vkImage = VK_NULL_HANDLE;
                VkResult vkResult =
                    GetContext().CreateImage(GetNativeDevice(), createInfo.GetCreateInfo(), VkSystemAllocator::Get(), &vkImage);
                AssertSuccess(vkResult);

                VkMemoryRequirements memoryRequirements = {};
                GetContext().GetImageMemoryRequirements(GetNativeDevice(), vkImage, &memoryRequirements);
                auto it2 = cache.insert(hash, memoryRequirements);
                GetContext().DestroyImage(GetNativeDevice(), vkImage, VkSystemAllocator::Get());
                return it2.first->second;
            }
        }

        VkMemoryRequirements Device::GetBufferMemoryRequirements(const RHI::BufferDescriptor& descriptor)
        {
            auto& cache = m_bufferMemoryRequirementsCache.GetStorage();

            const uint64_t hash = static_cast<uint64_t>(descriptor.GetHash());
            auto it = cache.get(hash);
            if (it != cache.end())
            {
                return it->second;
            }
            else
            {
                // Need to create a buffer to get the requirements.
                // This will not allocate or bind memory.
                BufferCreateInfo createInfo = BuildBufferCreateInfo(descriptor);
                VkBuffer vkBuffer = VK_NULL_HANDLE;
                VkResult vkResult =
                    GetContext().CreateBuffer(GetNativeDevice(), createInfo.GetCreateInfo(), VkSystemAllocator::Get(), &vkBuffer);
                AssertSuccess(vkResult);

                VkMemoryRequirements memoryRequirements = {};
                GetContext().GetBufferMemoryRequirements(GetNativeDevice(), vkBuffer, &memoryRequirements);
                auto it2 = cache.insert(hash, memoryRequirements);
                GetContext().DestroyBuffer(GetNativeDevice(), vkBuffer, VkSystemAllocator::Get());
                return it2.first->second;
            }
        }

        const VkPhysicalDeviceFeatures& Device::GetEnabledDevicesFeatures() const
        {
            return m_enabledDeviceFeatures;
        }

        VkPipelineStageFlags Device::GetSupportedPipelineStageFlags() const
        {
            return m_supportedPipelineStageFlagsMask;
        }

        VkImageUsageFlags Device::GetImageUsageFromFormat(RHI::Format format) const
        {
            auto it = m_imageUsageOfFormat.find(format);
            if (it != m_imageUsageOfFormat.end())
            {
                return it->second;
            }

            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            const VkFormatProperties formatProperties = physicalDevice.GetFormatProperties(format);
            const VkImageUsageFlags imageUsageFlags = ImageUsageFlagsOfFormatFeatureFlags(formatProperties.optimalTilingFeatures);
            m_imageUsageOfFormat[format] = imageUsageFlags;

            return imageUsageFlags;
        }

        CommandQueueContext& Device::GetCommandQueueContext()
        {
            return m_commandQueueContext;
        }

        const CommandQueueContext& Device::GetCommandQueueContext() const
        {
            return m_commandQueueContext;
        }

        SemaphoreAllocator& Device::GetSemaphoreAllocator()
        {
            return m_semaphoreAllocator;
        }

        SwapChainSemaphoreAllocator& Device::GetSwapChainSemaphoreAllocator()
        {
            return m_swapChainSemaphoreAllocator;
        }

        const AZStd::vector<VkQueueFamilyProperties>& Device::GetQueueFamilyProperties() const
        {
            return m_queueFamilyProperties;
        }

        AsyncUploadQueue& Device::GetAsyncUploadQueue()
        {
            if (!m_asyncUploadQueue)
            {
                m_asyncUploadQueue = aznew AsyncUploadQueue();
                AsyncUploadQueue::Descriptor asyncUploadQueueDescriptor(RHI::RHISystemInterface::Get()
                                                                            ->GetPlatformLimitsDescriptor(GetDeviceIndex())
                                                                            ->m_platformDefaultValues.m_asyncQueueStagingBufferSizeInBytes);
                asyncUploadQueueDescriptor.m_device = this;
                m_asyncUploadQueue->Init(asyncUploadQueueDescriptor);
            }

            return *m_asyncUploadQueue;
        }

        BindlessDescriptorPool& Device::GetBindlessDescriptorPool()
        {
            return m_bindlessDescriptorPool;
        }

        RHI::Ptr<Buffer> Device::AcquireStagingBuffer(AZStd::size_t byteCount, AZStd::size_t alignment /* = 1*/)
        {
            RHI::Ptr<Buffer> stagingBuffer = Buffer::Create();
            RHI::BufferDescriptor bufferDesc(RHI::BufferBindFlags::CopyRead, byteCount);
            bufferDesc.m_alignment = alignment;
            RHI::DeviceBufferInitRequest initRequest(*stagingBuffer, bufferDesc);
            const RHI::ResultCode result = m_stagingBufferPool->InitBuffer(initRequest);
            if (result != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "Initialization of staging Buffer fails.");
                return nullptr;
            }

            return stagingBuffer;
        }

        void Device::QueueForRelease(RHI::Ptr<RHI::Object> object)
        {
            if (object)
            {
                m_releaseQueue.QueueForCollect(object);
            }
        }

        RHI::Ptr<RenderPass> Device::AcquireRenderPass(const RenderPass::Descriptor& descriptor)
        {
            return AcquireObjectFromCache(m_renderPassCache, descriptor.GetHash(), descriptor);
        }

        RHI::Ptr<Framebuffer> Device::AcquireFramebuffer(const Framebuffer::Descriptor& descriptor)
        {
            return AcquireObjectFromCache(m_framebufferCache, descriptor.GetHash(), descriptor);
        }

        RHI::Ptr<DescriptorSetLayout> Device::AcquireDescriptorSetLayout(const DescriptorSetLayout::Descriptor& descriptor)
        {
            return AcquireObjectFromCache(m_descriptorSetLayoutCache, static_cast<uint64_t>(descriptor.GetHash()), descriptor);
        }

        RHI::Ptr<Sampler> Device::AcquireSampler(const Sampler::Descriptor& descriptor)
        {
            return AcquireObjectFromCache(m_samplerCache, descriptor.GetHash(), descriptor);
        }

        RHI::Ptr<PipelineLayout> Device::AcquirePipelineLayout(const PipelineLayout::Descriptor& descriptor)
        {
            return AcquireObjectFromCache(m_pipelineLayoutCache, descriptor.GetHash(), descriptor);
        }

        RHI::Ptr<CommandList> Device::AcquireCommandList(uint32_t familyQueueIndex, VkCommandBufferLevel level /*=VK_COMMAND_BUFFER_LEVEL_PRIMARY*/)
        {
            return m_commandListAllocator.Allocate(familyQueueIndex, level);
        }

        RHI::Ptr<CommandList> Device::AcquireCommandList(RHI::HardwareQueueClass queueClass, VkCommandBufferLevel level /*=VK_COMMAND_BUFFER_LEVEL_PRIMARY*/)
        {
            return m_commandListAllocator.Allocate(m_commandQueueContext.GetQueueFamilyIndex(queueClass), level);
        }

        uint32_t Device::GetCurrentFrameIndex() const
        {
            return m_commandQueueContext.GetCurrentFrameIndex();
        }

        void Device::PreShutdown()
        {
            // Any containers that maintain references to DeviceObjects need to be cleared here to ensure the device
            // refcount reaches 0 before shutdown.

            m_nullDescriptorManager.reset();

            m_bindlessDescriptorPool.Shutdown();
            m_stagingBufferPool.reset();
            m_constantBufferPool.reset();
            m_renderPassCache.first.Clear();
            m_framebufferCache.first.Clear();
            m_descriptorSetLayoutCache.first.Clear();
            m_samplerCache.first.Clear();
            m_pipelineLayoutCache.first.Clear();

            m_asyncUploadQueue.reset();
            m_commandQueueContext.Shutdown();
            m_commandListAllocator.Shutdown();

            m_semaphoreAllocator.Shutdown();
            m_swapChainSemaphoreAllocator.Shutdown();

            // Flush any objects released in the above calls.
            m_releaseQueue.Shutdown();
        }

        void Device::ShutdownInternal()
        {
            RHI::RHISystemNotificationBus::Handler::BusDisconnect();

            m_imageMemoryRequirementsCache.Clear();
            m_bufferMemoryRequirementsCache.Clear();

            ShutdownVmaAllocator();

            if (m_nativeDevice != VK_NULL_HANDLE)
            {
                GetContext().DestroyDevice(m_nativeDevice, VkSystemAllocator::Get());
                m_nativeDevice = VK_NULL_HANDLE;
            }

            if (m_loaderContext)
            {
                m_loaderContext->Shutdown();
                m_loaderContext = nullptr;
            }
        }

        RHI::ResultCode Device::BeginFrameInternal()
        {
            // We call to collect on the release queue on the begin frame because some objects (like resource pools)
            // cannot be shutdown during the frame scheduler execution. At this point the frame has not yet started.
            m_releaseQueue.Collect();
            m_commandQueueContext.Begin();

            RHI::XRRenderingInterface* xrSystem = RHI::RHISystemInterface::Get()->GetXRSystem();
            if (xrSystem)
            {
                // Begin Frame can make XR related calls which we need to make sure happens
                // from the thread related to the presentation queue or drivers will complain
                auto& presentationQueue = m_commandQueueContext.GetPresentationCommandQueue();
                auto presentCommand = [xrSystem](void*)
                {
                    xrSystem->BeginFrame();
                };

                presentationQueue.QueueCommand(AZStd::move(presentCommand));
                presentationQueue.FlushCommands();
            }
            return RHI::ResultCode::Success;
        }

        void Device::EndFrameInternal()
        {
            RHI::XRRenderingInterface* xrSystem = RHI::RHISystemInterface::Get()->GetXRSystem();
            if (xrSystem)
            {
                // End Frame can make XR related calls which we need to make sure happens
                // from the thread related to the presentation queue or drivers will complain
                auto& presentationQueue = m_commandQueueContext.GetPresentationCommandQueue();
                auto presentCommand = [xrSystem](void*)
                {
                    xrSystem->EndFrame();
                };

                presentationQueue.QueueCommand(AZStd::move(presentCommand));
                presentationQueue.FlushCommands();

                xrSystem->PostFrame();
            }

            m_commandQueueContext.End();
            m_commandListAllocator.Collect();
            m_semaphoreAllocator.Collect();
            m_swapChainSemaphoreAllocator.Collect();
            m_bindlessDescriptorPool.GarbageCollect();
        }

        void Device::WaitForIdleInternal()
        {
            m_commandQueueContext.WaitForIdle();
            m_releaseQueue.Collect(true);
        }

        void Device::CompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder)
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            const auto& memProps = physicalDevice.GetMemoryProperties();
            VmaBudget budgets[VK_MAX_MEMORY_HEAPS];
            vmaGetHeapBudgets(GetVmaAllocator(), budgets);

            VmaTotalStatistics detailStats;
            bool detail = builder.GetReportFlags() == RHI::MemoryStatisticsReportFlags::Detail;
            if (detail)
            {
                vmaCalculateStatistics(GetVmaAllocator(), &detailStats);
            }

            for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i)
            {
                RHI::MemoryStatistics::Heap* heapStats = builder.AddHeap();
                heapStats->m_name = AZStd::string::format("Heap %d", static_cast<int>(i));
                heapStats->m_heapMemoryType =
                    RHI::CheckBitsAll(memProps.memoryHeaps[i].flags, static_cast<VkMemoryHeapFlags>(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT))
                    ? RHI::HeapMemoryLevel::Device
                    : RHI::HeapMemoryLevel::Host;
                heapStats->m_memoryUsage.m_budgetInBytes = budgets[i].budget;
                heapStats->m_memoryUsage.m_totalResidentInBytes = budgets[i].usage;
                heapStats->m_memoryUsage.m_usedResidentInBytes = budgets[i].statistics.allocationBytes;

                if (detail)
                {
                    const VmaStatistics& stats = budgets[i].statistics;
                    VkDeviceSize unusedBytes = stats.blockBytes - stats.allocationBytes;
                    heapStats->m_memoryUsage.m_fragmentation =
                        unusedBytes > 0 ? 1.0f - (detailStats.memoryHeap[i].unusedRangeSizeMax / float(unusedBytes)) : 1.0f;

                    char* jsonString = nullptr;
                    vmaBuildStatsString(GetVmaAllocator(), &jsonString, true);
                    heapStats->m_memoryUsage.m_extraStats = jsonString;
                    vmaFreeStatsString(GetVmaAllocator(), jsonString);
                }
            }
        }

        void Device::UpdateCpuTimingStatisticsInternal() const
        {
            m_commandQueueContext.UpdateCpuTimingStatistics();
        }

        AZStd::vector<RHI::Format> Device::GetValidSwapChainImageFormats(const RHI::WindowHandle& windowHandle) const
        {
            AZStd::vector<RHI::Format> formatsList;
            // On some platforms (e.g. Android) we cannot create a new WSISurface if the window is already connected to one, so we first
            // check if the window is connected to one.
            VkSurfaceKHR vkSurface = VK_NULL_HANDLE;
            WindowSurfaceRequestsBus::EventResult(vkSurface, windowHandle, &WindowSurfaceRequestsBus::Events::GetNativeSurface);
            RHI::Ptr<WSISurface> surface;
            if (vkSurface == VK_NULL_HANDLE)
            {
                // Window is not connected to a WSISurface, so we create a temporary one to be able to get the valid device surface formats.
                WSISurface::Descriptor surfaceDescriptor{ windowHandle };
                surface = WSISurface::Create();
                const RHI::ResultCode result = surface->Init(surfaceDescriptor);
                if (result == RHI::ResultCode::Success)
                {
                    vkSurface = surface->GetNativeSurface();
                }
            }

            if (vkSurface == VK_NULL_HANDLE)
            {
                return formatsList;
            }

            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            uint32_t surfaceFormatCount = 0;
            AssertSuccess(GetContext().GetPhysicalDeviceSurfaceFormatsKHR(
                physicalDevice.GetNativePhysicalDevice(), vkSurface, &surfaceFormatCount, nullptr));
            if (surfaceFormatCount == 0)
            {
                AZ_Assert(false, "Surface support no format.");
                return formatsList;
            }

            AZStd::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
            AssertSuccess(GetContext().GetPhysicalDeviceSurfaceFormatsKHR(
                physicalDevice.GetNativePhysicalDevice(), vkSurface, &surfaceFormatCount, surfaceFormats.data()));

            AZStd::set<RHI::Format> formats;
            for (const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
            {
                // Don't expose formats for HDR output when the extension is missing
                // This can happen on Linux with Wayland.
                if (surfaceFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 &&
                    m_loaderContext->GetContext().SetHdrMetadataEXT == nullptr)
                {
                    continue;
                }
                formats.insert(ConvertFormat(surfaceFormat.format));
            }
            formatsList.assign(formats.begin(), formats.end());
            return formatsList;
        }

        void Device::FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities)
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            const bool isFragmentShadingRateSupported = physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::FragmentShadingRate);
            for (uint32_t i = 0; i < formatsCapabilities.size(); ++i)
            {
                RHI::Format format = static_cast<RHI::Format>(i);
                VkFormatProperties properties = physicalDevice.GetFormatProperties(format, false);
                RHI::FormatCapabilities& flags = formatsCapabilities[i];
                flags = RHI::FormatCapabilities::None;

                if (RHI::CheckBitsAll(properties.bufferFeatures, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT)))
                {
                    flags |= RHI::FormatCapabilities::VertexBuffer;
                }

                if (format == RHI::Format::R32_UINT || format == RHI::Format::R16_UINT)
                {
                    flags |= RHI::FormatCapabilities::IndexBuffer;
                }

                if (RHI::CheckBitsAll(properties.optimalTilingFeatures, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)))
                {
                    flags |= RHI::FormatCapabilities::RenderTarget;
                }

                if (RHI::CheckBitsAll(properties.optimalTilingFeatures, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)))
                {
                    flags |= RHI::FormatCapabilities::DepthStencil;
                }

                if (RHI::CheckBitsAll(properties.optimalTilingFeatures, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)))
                {
                    flags |= RHI::FormatCapabilities::Blend;
                }

                if (RHI::CheckBitsAll(properties.linearTilingFeatures, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) ||
                    RHI::CheckBitsAll(properties.optimalTilingFeatures, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)))
                {
                    flags |= RHI::FormatCapabilities::Sample;
                }

                if (RHI::CheckBitsAll(properties.bufferFeatures, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)))
                {
                    flags |= RHI::FormatCapabilities::TypedLoadBuffer;
                }

                if (RHI::CheckBitsAll(properties.bufferFeatures, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT)))
                {
                    flags |= RHI::FormatCapabilities::TypedStoreBuffer;
                }

                if (RHI::CheckBitsAll(properties.bufferFeatures, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT)))
                {
                    flags |= RHI::FormatCapabilities::AtomicBuffer;
                }

                // "Fragment Shading Rate" is preferable over "Fragment Density Map".
                // By checking only one of them, we avoid color format selection errors when looking
                // for color formats compatible with RHI::FormatCapabilities::ShadingRate.
                // Otherwise the runtime may end up using color formats that are ONLY supported for "Fragment Density Map"
                // when trying to use "Fragment Shading Rate" feature.
                if (isFragmentShadingRateSupported)
                {
                    if (RHI::CheckBitsAll(
                            properties.optimalTilingFeatures,
                            static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)))
                    {
                        flags |= RHI::FormatCapabilities::ShadingRate;
                    }
                }
                else
                {
                    if (RHI::CheckBitsAll(
                            properties.optimalTilingFeatures,
                            static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_FRAGMENT_DENSITY_MAP_BIT_EXT)))
                    {
                        flags |= RHI::FormatCapabilities::ShadingRate;
                    }
                }
            }
        }

        AZStd::chrono::microseconds Device::GpuTimestampToMicroseconds(uint64_t gpuTimestamp, [[maybe_unused]] RHI::HardwareQueueClass queueClass) const
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            auto timeInNano = AZStd::chrono::nanoseconds(static_cast<AZStd::chrono::nanoseconds::rep>(physicalDevice.GetDeviceLimits().timestampPeriod * gpuTimestamp));
            return AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(timeInNano);
        }

        AZStd::pair<uint64_t, uint64_t> Device::GetCalibratedTimestamp([[maybe_unused]] RHI::HardwareQueueClass queueClass)
        {
            if (!static_cast<const PhysicalDevice&>(GetPhysicalDevice())
                     .IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::CalibratedTimestamps))
            {
                return { 0ull, AZStd::chrono::microseconds().count() };
            }

            uint64_t maxDeviation;
            AZStd::pair<uint64_t, uint64_t> result;

            AZStd::array<VkCalibratedTimestampInfoEXT, 2> timestampsInfos{
                VkCalibratedTimestampInfoEXT{ VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT, 0, VK_TIME_DOMAIN_DEVICE_EXT },
                VkCalibratedTimestampInfoEXT{ VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_EXT, 0, m_hostTimeDomain }
            };

            GetContext().GetCalibratedTimestampsEXT(
                m_nativeDevice, static_cast<uint32_t>(timestampsInfos.size()), timestampsInfos.data(), &result.first, &maxDeviation);
            return result;
        }

        void Device::InitializeTimeDomains()
        {
            auto timeDomains{
                static_cast<const PhysicalDevice&>(GetPhysicalDevice()).GetCalibratedTimeDomains(m_loaderContext->GetContext())
            };

            bool deviceTimeDomainFound{ false };

            for (VkTimeDomainEXT timeDomain : timeDomains)
            {
                if (timeDomain == VK_TIME_DOMAIN_DEVICE_EXT)
                {
                    deviceTimeDomainFound = true;
                }
                // we prioritize VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT of the host time domains
                else if (m_hostTimeDomain != VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT)
                {
                    m_hostTimeDomain = timeDomain;
                }
            }

            // if there is no device time domain we reset the host one as this is pointless then
            if (!deviceTimeDomainFound)
            {
                m_hostTimeDomain = VK_TIME_DOMAIN_MAX_ENUM_EXT;
            }
        }

        RHI::ResourceMemoryRequirements Device::GetResourceMemoryRequirements(const RHI::ImageDescriptor& descriptor)
        {
            auto vkRequirements = GetImageMemoryRequirements(descriptor);
            return RHI::ResourceMemoryRequirements{ vkRequirements.alignment, vkRequirements.size };
        }

        RHI::ResourceMemoryRequirements Device::GetResourceMemoryRequirements(const RHI::BufferDescriptor& descriptor)
        {
            auto vkRequirements = GetBufferMemoryRequirements(descriptor);
            return RHI::ResourceMemoryRequirements{ vkRequirements.alignment, vkRequirements.size };
        }

        void Device::ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction)
        {
            m_releaseQueue.Notify(notifyFunction);
        }

        RHI::ShadingRateImageValue Device::ConvertShadingRate(RHI::ShadingRate rate) const
        {
            ShadingRateImageMode mode = GetImageShadingRateMode();
            if (mode == ShadingRateImageMode::ImageAttachment)
            {
                // Fragment sizes are encoded in a single texel as follows:
                // size(w) = 2^((texel/4) & 3)
                // size(h) = 2^(texel & 3)

                uint8_t encoded_rate_w, encoded_rate_h;
                switch (rate)
                {
                case RHI::ShadingRate::Rate1x1:
                    encoded_rate_w = encoded_rate_h = 0;
                    break;
                case RHI::ShadingRate::Rate1x2:
                    encoded_rate_w = 0;
                    encoded_rate_h = 1;
                    break;
                case RHI::ShadingRate::Rate2x1:
                    encoded_rate_w = 1;
                    encoded_rate_h = 0;
                    break;
                case RHI::ShadingRate::Rate2x2:
                    encoded_rate_w = encoded_rate_h = 1;
                    break;
                case RHI::ShadingRate::Rate2x4:
                    encoded_rate_w = 1;
                    encoded_rate_h = 2;
                    break;
                case RHI::ShadingRate::Rate4x2:
                    encoded_rate_w = 2;
                    encoded_rate_h = 1;
                    break;
                case RHI::ShadingRate::Rate4x1:
                    encoded_rate_w = 2;
                    encoded_rate_h = 0;
                    break;
                case RHI::ShadingRate::Rate1x4:
                    encoded_rate_w = 0;
                    encoded_rate_h = 2;
                    break;
                case RHI::ShadingRate::Rate4x4:
                    encoded_rate_w = encoded_rate_h = 2;
                    break;
                default:
                    AZ_Assert(false, "Invalid shading rate enum %d", rate);
                    encoded_rate_w = encoded_rate_h = 0;
                }
                uint8_t encodedRate = encoded_rate_w << 2 | encoded_rate_h;
                return RHI::ShadingRateImageValue{ encodedRate, 0 };
            }
            else if (mode == ShadingRateImageMode::DensityMap)
            {
                // Horizontal rate is encoded in the first texel component.
                // Vertical rate is encoded in the second texl component.
                // Final density is calculated as (1/rate) and valid values must be in range (0, 1]
                uint8_t encoded_rate_w, encoded_rate_h;
                switch (rate)
                {
                case RHI::ShadingRate::Rate1x1:
                    encoded_rate_w = encoded_rate_h = 0;
                    break;
                case RHI::ShadingRate::Rate1x2:
                    encoded_rate_w = 0;
                    encoded_rate_h = 1;
                    break;
                case RHI::ShadingRate::Rate2x1:
                    encoded_rate_w = 1;
                    encoded_rate_h = 0;
                    break;
                case RHI::ShadingRate::Rate2x2:
                    encoded_rate_w = encoded_rate_h = 1;
                    break;
                case RHI::ShadingRate::Rate2x4:
                    encoded_rate_w = 1;
                    encoded_rate_h = 2;
                    break;
                case RHI::ShadingRate::Rate4x2:
                    encoded_rate_w = 2;
                    encoded_rate_h = 1;
                    break;
                case RHI::ShadingRate::Rate1x4:
                    encoded_rate_w = 0;
                    encoded_rate_h = 2;
                    break;
                case RHI::ShadingRate::Rate4x1:
                    encoded_rate_w = 2;
                    encoded_rate_h = 0;
                    break;
                case RHI::ShadingRate::Rate4x4:
                    encoded_rate_w = encoded_rate_h = 2;
                    break;
                default:
                    AZ_Assert(false, "Invalid shading rate enum %d", rate);
                    encoded_rate_w = encoded_rate_h = 0;
                }
                uint8_t density_w = 0xFF >> encoded_rate_w;
                uint8_t density_h = 0xFF >> encoded_rate_h;
                return RHI::ShadingRateImageValue{ density_w, density_h };
            }
            AZ_Error("Vulkan", false, "Shading Rate Image is not supported on this platform");
            return RHI::ShadingRateImageValue{};
        }

        RHI::Ptr<RHI::XRDeviceDescriptor> Device::BuildXRDescriptor() const
        {
            XRDeviceDescriptor* xrDeviceDescriptor = aznew XRDeviceDescriptor;
            xrDeviceDescriptor->m_inputData.m_xrVkDevice = m_nativeDevice;
            xrDeviceDescriptor->m_inputData.m_xrVkPhysicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice()).GetNativePhysicalDevice();
            for (int i = 0; i < RHI::HardwareQueueClassCount; ++i)
            {
                const auto& queueDescriptor =
                    m_commandQueueContext.GetCommandQueue(static_cast<RHI::HardwareQueueClass>(i)).GetQueueDescriptor();
                xrDeviceDescriptor->m_inputData.m_xrQueueBinding[i].m_queueFamilyIndex = queueDescriptor.m_familyIndex;
                xrDeviceDescriptor->m_inputData.m_xrQueueBinding[i].m_queueIndex = queueDescriptor.m_queueIndex;
            }
            return xrDeviceDescriptor;
        }

        void Device::InitFeaturesAndLimits(const PhysicalDevice& physicalDevice)
        {
            m_features.m_geometryShader = (m_enabledDeviceFeatures.geometryShader == VK_TRUE);
            m_features.m_computeShader = true;
            m_features.m_independentBlend = (m_enabledDeviceFeatures.independentBlend == VK_TRUE);
            m_features.m_customSamplePositions = physicalDevice.IsFeatureSupported(DeviceFeature::CustomSampleLocation);
#if AZ_TRAIT_ATOM_VULKAN_DISABLE_DUAL_SOURCE_BLENDING
            // [ATOM-1448] Dual source blending may not work on certain devices due to driver issues.
            m_features.m_dualSourceBlending = false;
#else
            m_features.m_dualSourceBlending = m_enabledDeviceFeatures.dualSrcBlend == VK_TRUE;
#endif
            m_features.m_queryTypesMask[static_cast<uint32_t>(RHI::HardwareQueueClass::Graphics)] = RHI::QueryTypeFlags::Occlusion;
            if (m_enabledDeviceFeatures.pipelineStatisticsQuery)
            {
                m_features.m_queryTypesMask[static_cast<uint32_t>(RHI::HardwareQueueClass::Graphics)] |= RHI::QueryTypeFlags::PipelineStatistics;
                m_features.m_queryTypesMask[static_cast<uint32_t>(RHI::HardwareQueueClass::Compute)] |= RHI::QueryTypeFlags::PipelineStatistics;
            }
            if (physicalDevice.GetDeviceLimits().timestampComputeAndGraphics)
            {
                m_features.m_queryTypesMask[static_cast<uint32_t>(RHI::HardwareQueueClass::Graphics)] |= RHI::QueryTypeFlags::Timestamp;
                m_features.m_queryTypesMask[static_cast<uint32_t>(RHI::HardwareQueueClass::Compute)] |= RHI::QueryTypeFlags::Timestamp;
            }
            else
            {
                for (uint32_t i = 0; i < RHI::HardwareQueueClassCount; ++i)
                {
                    QueueId id = m_commandQueueContext.GetCommandQueue(static_cast<RHI::HardwareQueueClass>(i)).GetId();
                    if (m_queueFamilyProperties[id.m_familyIndex].timestampValidBits)
                    {
                        m_features.m_queryTypesMask[i] |= RHI::QueryTypeFlags::Timestamp;
                    }
                }
            }

            m_features.m_occlusionQueryPrecise = m_enabledDeviceFeatures.occlusionQueryPrecise == VK_TRUE;
            m_features.m_predication = physicalDevice.IsFeatureSupported(DeviceFeature::Predication);
            m_features.m_indirectCommandTier = RHI::IndirectCommandTiers::Tier1;
            m_features.m_indirectDrawCountBufferSupported = physicalDevice.IsFeatureSupported(DeviceFeature::DrawIndirectCount);
            m_features.m_indirectDispatchCountBufferSupported = false;
            m_features.m_indirectDrawStartInstanceLocationSupported = m_enabledDeviceFeatures.drawIndirectFirstInstance == VK_TRUE;
            m_features.m_subpassInputSupport = RHI::SubpassInputSupportType::Color | RHI::SubpassInputSupportType::DepthStencil;

            const VkPhysicalDeviceProperties& deviceProperties = physicalDevice.GetPhysicalDeviceProperties();
            // Our sparse image implementation requires the device support sparse binding and particle residency for 2d and 3d images
            // And it should use standard block shape (64k).
            // It also requires memory alias support so resources can use the same block repeatedly (this may reduce performance based on implementation)
           const auto& copyQueue = m_commandQueueContext.GetCommandQueue(RHI::HardwareQueueClass::Copy);
            m_features.m_tiledResource = m_enabledDeviceFeatures.sparseBinding
                && m_enabledDeviceFeatures.sparseResidencyImage2D
                && m_enabledDeviceFeatures.sparseResidencyImage3D
                && m_enabledDeviceFeatures.sparseResidencyAliased
                && deviceProperties.sparseProperties.residencyStandard2DBlockShape
                && deviceProperties.sparseProperties.residencyStandard3DBlockShape
                && ((m_queueFamilyProperties[copyQueue.GetQueueDescriptor().m_familyIndex].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) == VK_QUEUE_SPARSE_BINDING_BIT);

            // Check if the Vulkan device support subgroup operations
            m_features.m_waveOperation = physicalDevice.IsFeatureSupported(DeviceFeature::SubgroupOperation);

            // check for the VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME in the list of physical device extensions
            // to determine if ray tracing is supported on this device
            StringList deviceExtensions = physicalDevice.GetDeviceExtensionNames();
            StringList::iterator itRayTracingExtension = AZStd::find(deviceExtensions.begin(), deviceExtensions.end(), VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            m_features.m_unboundedArrays = physicalDevice.GetPhysicalDeviceDescriptorIndexingFeatures().shaderStorageTexelBufferArrayNonUniformIndexing;
            if (m_features.m_unboundedArrays)
            {
                // Ray tracing needs raytracing extensions and unbounded arrays to work
                m_features.m_rayTracing = (itRayTracingExtension != deviceExtensions.end());
            }

            m_features.m_float16 = physicalDevice.GetPhysicalDeviceFloat16Int8Features().shaderFloat16;

            if (physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::FragmentShadingRate))
            {
                const auto& shadingRateFeatures = physicalDevice.GetPhysicalDeviceFragmentShadingRateFeatures();
                if (shadingRateFeatures.attachmentFragmentShadingRate)
                {
                    m_features.m_shadingRateTypeMask |= RHI::ShadingRateTypeFlags::PerRegion;
                    m_imageShadingRateMode = ShadingRateImageMode::ImageAttachment;
                    m_features.m_dynamicShadingRateImage = true;
                }

                if (shadingRateFeatures.primitiveFragmentShadingRate)
                {
                    m_features.m_shadingRateTypeMask |= RHI::ShadingRateTypeFlags::PerPrimitive;
                }
                if (shadingRateFeatures.pipelineFragmentShadingRate)
                {
                    m_features.m_shadingRateTypeMask |= RHI::ShadingRateTypeFlags::PerDraw;
                }

                auto nativeDevice = physicalDevice.GetNativePhysicalDevice();
                AZStd::vector<VkPhysicalDeviceFragmentShadingRateKHR> rates{};
                uint32_t rateCount = 0;
                GetContext().GetPhysicalDeviceFragmentShadingRatesKHR(nativeDevice, &rateCount, nullptr);
                if (rateCount > 0)
                {
                    rates.resize(rateCount);
                    for (VkPhysicalDeviceFragmentShadingRateKHR& fragment_shading_rate : rates)
                    {
                        // As per spec, the sType member of each shading rate array entry must be set
                        fragment_shading_rate.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_KHR;
                    }
                    GetContext().GetPhysicalDeviceFragmentShadingRatesKHR(nativeDevice, &rateCount, rates.data());
                    for (const auto& vkRate : rates)
                    {
                        m_features.m_shadingRateMask |= static_cast<RHI::ShadingRateFlags>(
                            AZ_BIT(static_cast<uint32_t>(ConvertFragmentShadingRate(vkRate.fragmentSize))));
                    }
                }
            }
            else if (physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::FragmentDensityMap))
            {
                const auto& densityFeatures = physicalDevice.GetPhysicalDeviceFragmentDensityMapFeatures();
                if (densityFeatures.fragmentDensityMap && densityFeatures.fragmentDensityMapNonSubsampledImages)
                {
                    m_features.m_shadingRateTypeMask |= RHI::ShadingRateTypeFlags::PerRegion;
                    m_imageShadingRateMode = ShadingRateImageMode::DensityMap;
                    m_features.m_dynamicShadingRateImage = densityFeatures.fragmentDensityMapDynamic;
                    for (uint32_t i = 0; i < static_cast<uint32_t>(RHI::ShadingRate::Count); ++i)
                    {
                        m_features.m_shadingRateMask |= static_cast<RHI::ShadingRateFlags>(AZ_BIT(i));
                    }
                }
            }
            m_features.m_swapchainScalingFlags = AZ_TRAIT_ATOM_VULKAN_SWAPCHAIN_SCALING_FLAGS;

#ifdef DISABLE_TIMELINE_SEMAPHORES
            m_features.m_signalFenceFromCPU = false;
#else
            m_features.m_signalFenceFromCPU = physicalDevice.GetPhysicalDeviceTimelineSemaphoreFeatures().timelineSemaphore;
#endif

            const auto& deviceLimits = physicalDevice.GetDeviceLimits();
            m_limits.m_maxImageDimension1D = deviceLimits.maxImageDimension1D;
            m_limits.m_maxImageDimension2D = deviceLimits.maxImageDimension2D;
            m_limits.m_maxImageDimension3D = deviceLimits.maxImageDimension3D;
            m_limits.m_maxImageDimensionCube = deviceLimits.maxImageDimensionCube;
            m_limits.m_maxImageArraySize = deviceLimits.maxImageArrayLayers;
            m_limits.m_minConstantBufferViewOffset = static_cast<uint32_t>(deviceLimits.minUniformBufferOffsetAlignment);
            m_limits.m_minTexelBufferOffsetAlignment = static_cast<uint32_t>(deviceLimits.minTexelBufferOffsetAlignment);
            m_limits.m_minStorageBufferOffsetAlignment = static_cast<uint32_t>(deviceLimits.minStorageBufferOffsetAlignment);
            m_limits.m_maxMemoryAllocationCount = deviceLimits.maxMemoryAllocationCount;
            m_limits.m_maxIndirectDrawCount = deviceLimits.maxDrawIndirectCount;
            m_limits.m_maxConstantBufferSize = deviceLimits.maxUniformBufferRange;
            m_limits.m_maxBufferSize = deviceLimits.maxStorageBufferRange;

            VkExtent2D tileSize{ 1, 1 };
            switch (m_imageShadingRateMode)
            {
            case ShadingRateImageMode::ImageAttachment:
                tileSize = physicalDevice.GetPhysicalDeviceFragmentShadingRateProperties().minFragmentShadingRateAttachmentTexelSize;
                break;
            case ShadingRateImageMode::DensityMap:
                tileSize = physicalDevice.GetPhysicalDeviceFragmentDensityMapProperties().minFragmentDensityTexelSize;
                break;
            default:
                break;
            }
            m_limits.m_shadingRateTileSize = RHI::Size(tileSize.width, tileSize.height, 1);
        }

        void Device::BuildDeviceQueueInfo(const PhysicalDevice& physicalDevice)
        {
            Instance& instance = Instance::GetInstance();

            m_queueFamilyProperties.clear();
            VkPhysicalDevice nativePhysicalDevice = physicalDevice.GetNativePhysicalDevice();

            uint32_t queueFamilyCount = 0;
            instance.GetContext().GetPhysicalDeviceQueueFamilyProperties(nativePhysicalDevice, &queueFamilyCount, nullptr);
            AZ_Assert(queueFamilyCount, "No queue families were found for physical device %s", physicalDevice.GetName().GetCStr());

            m_queueFamilyProperties.resize(queueFamilyCount);
            instance.GetContext().GetPhysicalDeviceQueueFamilyProperties(
                nativePhysicalDevice, &queueFamilyCount, m_queueFamilyProperties.data());
        }

        NullDescriptorManager& Device::GetNullDescriptorManager()
        {
            AZ_Assert(m_nullDescriptorManager, "NullDescriptorManager was not created. Check device capabilities.");
            return *m_nullDescriptorManager;
        }

        VkBufferUsageFlags Device::GetBufferUsageFlagBitsUnderRestrictions(RHI::BufferBindFlags bindFlags) const
        {
            VkBufferUsageFlags bufferUsageFlags = GetBufferUsageFlagBits(bindFlags);

            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());

            // VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT require bufferDeviceAddress enabled.
            if (!physicalDevice.GetPhysicalDeviceBufferDeviceAddressFeatures().bufferDeviceAddress)
            {
                bufferUsageFlags &= ~VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            }
            // VK_KHR_acceleration_structure provides VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
            // Otherwise unrecognized flag.
            if (!physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::AccelerationStructure))
            {
                bufferUsageFlags &= ~VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
            }

            return bufferUsageFlags;
        }

        RHI::ResultCode Device::InitVmaAllocator(RHI::PhysicalDevice & physicalDeviceBase)
        {
            auto& physicalDevice = static_cast<Vulkan::PhysicalDevice&>(physicalDeviceBase);

            auto& context = GetContext();
            // We pass the function pointers from the Glad context since we already loaded them.
            VmaVulkanFunctions vulkanFunctions =
            {
                context.GetInstanceProcAddr,
                context.GetDeviceProcAddr,
                context.GetPhysicalDeviceProperties,
                context.GetPhysicalDeviceMemoryProperties,
                context.AllocateMemory,
                context.FreeMemory,
                context.MapMemory,
                context.UnmapMemory,
                context.FlushMappedMemoryRanges,
                context.InvalidateMappedMemoryRanges,
                context.BindBufferMemory,
                context.BindImageMemory,
                context.GetBufferMemoryRequirements,
                context.GetImageMemoryRequirements,
                context.CreateBuffer,
                context.DestroyBuffer,
                context.CreateImage,
                context.DestroyImage,
                context.CmdCopyBuffer,
                context.GetBufferMemoryRequirements2,
                context.GetImageMemoryRequirements2,
                context.BindBufferMemory2,
                context.BindImageMemory2,
                context.GetPhysicalDeviceMemoryProperties2,
                context.GetDeviceBufferMemoryRequirements,
                context.GetDeviceImageMemoryRequirements
            };

            auto& instance = Instance::GetInstance();

            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = physicalDevice.GetNativePhysicalDevice();
            allocatorInfo.device = m_nativeDevice;
            allocatorInfo.instance = instance.GetNativeInstance();
            allocatorInfo.vulkanApiVersion = physicalDevice.GetVulkanVersion();
            allocatorInfo.pVulkanFunctions = &vulkanFunctions;
            allocatorInfo.pAllocationCallbacks = VkSystemAllocator::Get();

            VkExternalMemoryHandleTypeFlagsKHR externalHandleTypeFlags = 0;
            ExternalHandleRequirementBus::Broadcast(
                &ExternalHandleRequirementBus::Events::CollectExternalMemoryRequirements, externalHandleTypeFlags);
            AZStd::vector<VkExternalMemoryHandleTypeFlagsKHR> externalTypes;
            if (externalHandleTypeFlags != 0)
            {
                externalTypes = AZStd::vector<VkExternalMemoryHandleTypeFlagsKHR>(
                    physicalDevice.GetMemoryProperties().memoryTypeCount, externalHandleTypeFlags);
                allocatorInfo.pTypeExternalMemoryHandleTypes = externalTypes.data();
            }

            if (GetContext().GetBufferMemoryRequirements2 && GetContext().GetImageMemoryRequirements2)
            {
                allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
            }

            if (GetContext().BindBufferMemory2 && GetContext().BindImageMemory2)
            {
                allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;
            }

            if (physicalDevice.IsFeatureSupported(DeviceFeature::MemoryBudget))
            {
                allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
            }

            if (physicalDevice.GetPhysicalDeviceBufferDeviceAddressFeatures().bufferDeviceAddress)
            {
                allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
            }

            VkResult errorCode = vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator);
            AssertSuccess(errorCode);

            return ConvertResult(errorCode);
        }

        void Device::ShutdownVmaAllocator()
        {
            if (m_vmaAllocator != VK_NULL_HANDLE)
            {
                vmaDestroyAllocator(m_vmaAllocator);
            }
        }

        VkImageUsageFlags Device::CalculateImageUsageFlags(const RHI::ImageDescriptor& descriptor) const
        {
            const RHI::ImageBindFlags bindFlags = descriptor.m_bindFlags;
            VkImageUsageFlags usageFlags{};

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShaderRead))
            {
                usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
            }
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShaderWrite))
            {
                usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
            }
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::Color))
            {
                usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::DepthStencil))
            {
                usageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::CopyWrite))
            {
                usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }
            if (RHI::CheckBitsAll(bindFlags, RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderRead) ||
                RHI::CheckBitsAll(bindFlags, RHI::ImageBindFlags::DepthStencil | RHI::ImageBindFlags::ShaderRead))
            {
                usageFlags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
            }
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShadingRate))
            {
                switch (GetImageShadingRateMode())
                {
                case Device::ShadingRateImageMode::DensityMap:
                    {
                        usageFlags |= VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
                    }
                    break;
                case Device::ShadingRateImageMode::ImageAttachment:
                    {
                        usageFlags |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
                    }
                    break;
                default:
                    {
                        AZ_Error("Image", false, "Image Shading Rate mode not supported on this platform");
                    }
                    break;
                }
            }

            // add transfer src usage for all images since we may want them to be copied for preview or readback
            usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            const VkImageUsageFlags usageMask = GetImageUsageFromFormat(descriptor.m_format);

            auto finalFlags = usageFlags & usageMask;

            // Output a warning about desired usages that are not supported
            if (finalFlags != usageFlags)
            {
                AZ_Warning("Vulkan", false, "Missing usage bit flags (unsupported): %x", usageFlags & ~finalFlags);
            }

            return finalFlags;
        }

        VkImageCreateFlags Device::CalculateImageCreateFlags(const RHI::ImageDescriptor& descriptor) const
        {
            VkImageCreateFlags flags{};

            // Spec. of VkImageCreate:
            // "If imageType is VK_IMAGE_TYPE_2D and flags contains VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
            //  extent.width and extent.height must be equal and arrayLayers must be greater than or equal to 6"
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkImageCreateInfo.html
            if (descriptor.m_dimension == RHI::ImageDimension::Image2D && descriptor.m_size.m_width == descriptor.m_size.m_height &&
                descriptor.m_arraySize >= 6)
            {
                flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            }

            // For required condition of VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT, refer the spec. of VkImageViewCreate:
            // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkImageViewCreateInfo.html
            auto& physicalDevice = static_cast<const Vulkan::PhysicalDevice&>(GetPhysicalDevice());
            if (physicalDevice.IsFeatureSupported(DeviceFeature::Compatible2dArrayTexture) &&
                (descriptor.m_dimension == RHI::ImageDimension::Image3D))
            {
                // The KHR value will map to the core one in case compatible 2D array is part of core.
                flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
            }

            if (physicalDevice.IsFeatureSupported(DeviceFeature::CustomSampleLocation) &&
                descriptor.m_multisampleState.m_customPositionsCount > 0 &&
                RHI::CheckBitsAny(descriptor.m_bindFlags, RHI::ImageBindFlags::DepthStencil))
            {
                flags |= VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT;
            }

            flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

            return flags;
        }

        BufferCreateInfo Device::BuildBufferCreateInfo(const RHI::BufferDescriptor& descriptor) const
        {
            BufferCreateInfo createInfo;
            AZ_Assert(descriptor.m_sharedQueueMask != RHI::HardwareQueueClassMask::None, "Invalid shared queue mask");
            createInfo.m_queueFamilyIndices = GetCommandQueueContext().GetQueueFamilyIndices(descriptor.m_sharedQueueMask);

            VkBufferCreateInfo vkCreateInfo = {};
            vkCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            vkCreateInfo.pNext = nullptr;
            vkCreateInfo.flags = 0;
            vkCreateInfo.size = descriptor.m_byteCount;
            vkCreateInfo.usage = GetBufferUsageFlagBitsUnderRestrictions(descriptor.m_bindFlags);
            // Trying to guess here if the buffers are going to be used as attachments. Maybe it would be better to add an explicit flag in
            // the descriptor.
            vkCreateInfo.sharingMode =
                (RHI::CheckBitsAny(
                     descriptor.m_bindFlags,
                     RHI::BufferBindFlags::ShaderWrite | RHI::BufferBindFlags::Predication | RHI::BufferBindFlags::Indirect) ||
                 (createInfo.m_queueFamilyIndices.size()) <= 1)
                ? VK_SHARING_MODE_EXCLUSIVE
                : VK_SHARING_MODE_CONCURRENT;
            vkCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(createInfo.m_queueFamilyIndices.size());
            vkCreateInfo.pQueueFamilyIndices = createInfo.m_queueFamilyIndices.empty() ? nullptr : createInfo.m_queueFamilyIndices.data();
            createInfo.SetCreateInfo(vkCreateInfo);

            VkExternalMemoryHandleTypeFlagsKHR externalHandleTypeFlags = 0;
            ExternalHandleRequirementBus::Broadcast(
                &ExternalHandleRequirementBus::Events::CollectExternalMemoryRequirements, externalHandleTypeFlags);
            if (externalHandleTypeFlags != 0)
            {
                VkExternalMemoryBufferCreateInfo externalInfo = {};
                externalInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
                externalInfo.handleTypes = externalHandleTypeFlags;
                externalInfo.pNext = nullptr;
                createInfo.SetExternalCreateInfo(externalInfo);
            }

            return createInfo;
        }

        ImageCreateInfo Device::BuildImageCreateInfo(const RHI::ImageDescriptor& descriptor) const
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());

            ImageCreateInfo createInfo;

            VkImageCreateInfo vkCreateInfo = {};
            vkCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            vkCreateInfo.pNext = nullptr;
            vkCreateInfo.format = ConvertFormat(descriptor.m_format);
            vkCreateInfo.flags = CalculateImageCreateFlags(descriptor);
            vkCreateInfo.imageType = ConvertToImageType(descriptor.m_dimension);
            vkCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            vkCreateInfo.usage = CalculateImageUsageFlags(descriptor);

            VkImageFormatProperties formatProps{};
            AssertSuccess(GetContext().GetPhysicalDeviceImageFormatProperties(
                physicalDevice.GetNativePhysicalDevice(),
                vkCreateInfo.format,
                vkCreateInfo.imageType,
                vkCreateInfo.tiling,
                vkCreateInfo.usage,
                vkCreateInfo.flags,
                &formatProps));

            AZ_Assert(descriptor.m_sharedQueueMask != RHI::HardwareQueueClassMask::None, "Invalid shared queue mask");
            createInfo.m_queueFamilyIndices = GetCommandQueueContext().GetQueueFamilyIndices(descriptor.m_sharedQueueMask);

            bool exclusiveOwnership =
                createInfo.m_queueFamilyIndices.size() == 1; // Only supports one queue.
                                           // If it's writable, then we assume that this will be used as an ImageAttachment and all proper
                                           // ownership transfers will be handled by the FrameGraph.
            exclusiveOwnership |= RHI::CheckBitsAny(
                descriptor.m_bindFlags, RHI::ImageBindFlags::ShaderWrite | RHI::ImageBindFlags::Color | RHI::ImageBindFlags::DepthStencil);
            exclusiveOwnership |=
                RHI::CheckBitsAny(descriptor.m_sharedQueueMask, RHI::HardwareQueueClassMask::Copy) && // Supports copy queue
                RHI::CountBitsSet(static_cast<uint32_t>(descriptor.m_sharedQueueMask)) ==
                    2; // And ONLY copy + another queue. This means that the
                       // copy queue can transition the resource to the correct queue
                       // after finishing copying.

            VkExtent3D extent = ConvertToExtent3D(descriptor.m_size);
            extent.width = AZStd::min<uint32_t>(extent.width, formatProps.maxExtent.width);
            extent.height = AZStd::min<uint32_t>(extent.height, formatProps.maxExtent.height);
            extent.depth = AZStd::min<uint32_t>(extent.depth, formatProps.maxExtent.depth);
            vkCreateInfo.extent = extent;
            vkCreateInfo.mipLevels = AZStd::min<uint32_t>(descriptor.m_mipLevels, formatProps.maxMipLevels);
            vkCreateInfo.arrayLayers = AZStd::min<uint32_t>(descriptor.m_arraySize, formatProps.maxArrayLayers);
            VkSampleCountFlagBits sampleCountFlagBits = static_cast<VkSampleCountFlagBits>(RHI::FilterBits(
                static_cast<VkSampleCountFlags>(ConvertSampleCount(descriptor.m_multisampleState.m_samples)), formatProps.sampleCounts));
            vkCreateInfo.samples = (static_cast<uint32_t>(sampleCountFlagBits) > 0) ? sampleCountFlagBits : VK_SAMPLE_COUNT_1_BIT;
            vkCreateInfo.sharingMode = exclusiveOwnership ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
            vkCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(createInfo.m_queueFamilyIndices.size());
            vkCreateInfo.pQueueFamilyIndices = createInfo.m_queueFamilyIndices.empty() ? nullptr : createInfo.m_queueFamilyIndices.data();
            vkCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            createInfo.SetCreateInfo(vkCreateInfo);

            VkExternalMemoryHandleTypeFlagsKHR externalHandleTypeFlags = 0;
            ExternalHandleRequirementBus::Broadcast(
                &ExternalHandleRequirementBus::Events::CollectExternalMemoryRequirements, externalHandleTypeFlags);
            if (externalHandleTypeFlags != 0)
            {
                VkExternalMemoryImageCreateInfo externalInfo = {};
                externalInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
                externalInfo.handleTypes = externalHandleTypeFlags;
                externalInfo.pNext = nullptr;
                createInfo.SetExternalCreateInfo(externalInfo);
            }

            return createInfo;
        }

        Device::ShadingRateImageMode Device::GetImageShadingRateMode() const
        {
            return m_imageShadingRateMode;
        }

        RHI::Ptr<BufferPool> Device::GetConstantBufferPool()
        {
            return m_constantBufferPool;
        }

        VmaAllocator& Device::GetVmaAllocator()
        {
            return m_vmaAllocator;
        }

        void Device::OnRHISystemInitialized()
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            if (!physicalDevice.IsFeatureSupported(DeviceFeature::NullDescriptor))
            {
                // Need to initialize the NullDescriptorManager AFTER the bindless descriptor pool, since we create images and buffers
                // during the initialization of the NullDescriptorManager.
                m_nullDescriptorManager = NullDescriptorManager::Create();
                [[maybe_unused]] RHI::ResultCode result = m_nullDescriptorManager->Init(*this);
                AZ_Error("Vulkan", result == RHI::ResultCode::Success, "Failed to initialize Null descriptor manager");
            }
        }
    }
}
