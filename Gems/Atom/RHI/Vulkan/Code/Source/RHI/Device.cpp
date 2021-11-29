/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Loader/FunctionLoader.h>
#include <Atom/RHI.Reflect/Vulkan/PlatformLimitsDescriptor.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/TransientAttachmentPool.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Debug/Trace.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/Conversion.h>
#include <RHI/CommandList.h>
#include <RHI/CommandQueue.h>
#include <RHI/Device.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/ImagePool.h>
#include <RHI/Instance.h>
#include <RHI/Pipeline.h>
#include <RHI/SwapChain.h>
#include <RHI/WSISurface.h>
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

        RHI::Ptr<Device> Device::Create()
        {
            return aznew Device();
        }

        RawStringList Device::GetRequiredLayers()
        {
            // No required layers for now
            return RawStringList();
        }

        RawStringList Device::GetRequiredExtensions()
        {
            return RawStringList{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
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
            auto& physicalDevice = static_cast<Vulkan::PhysicalDevice&>(physicalDeviceBase);
            RawStringList requiredLayers = GetRequiredLayers();
            RawStringList requiredExtensions = GetRequiredExtensions();

            RawStringList optionalExtensions = physicalDevice.FilterSupportedOptionalExtensions();
            requiredExtensions.insert(requiredExtensions.end(), optionalExtensions.begin(), optionalExtensions.end());

            //We now need to find the queues that the physical device has available and make sure 
            //it has what we need. We're just expecting a Graphics queue for now.

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
            queueCreateInfo.pNext = nullptr;
            queueCreateInfo.flags = 0;

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

            const auto& physicalProperties = physicalDevice.GetPhysicalDeviceProperties();
            uint32_t majorVersion = VK_VERSION_MAJOR(physicalProperties.apiVersion);
            uint32_t minorVersion = VK_VERSION_MINOR(physicalProperties.apiVersion);

            // unbounded array functionality
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures = {};
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

            VkPhysicalDeviceBufferDeviceAddressFeaturesEXT bufferDeviceAddressFeatures = {};
            bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
            const VkPhysicalDeviceBufferDeviceAddressFeaturesEXT& physicalDeviceBufferDeviceAddressFeatures =
                physicalDevice.GetPhysicalDeviceBufferDeviceAddressFeatures();
            bufferDeviceAddressFeatures.bufferDeviceAddress = physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddress;
            bufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay;
            bufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice = physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice;
            descriptorIndexingFeatures.pNext = &bufferDeviceAddressFeatures;

            VkPhysicalDeviceDepthClipEnableFeaturesEXT depthClipEnabled = {};
            depthClipEnabled.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
            depthClipEnabled.depthClipEnable = physicalDevice.GetPhysicalDeviceDepthClipEnableFeatures().depthClipEnable;
            bufferDeviceAddressFeatures.pNext = &depthClipEnabled;

            VkPhysicalDeviceRobustness2FeaturesEXT robustness2 = {};
            robustness2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
            robustness2.nullDescriptor = physicalDevice.GetPhysicalDeviceRobutness2Features().nullDescriptor;
            depthClipEnabled.pNext = &robustness2;

            VkPhysicalDeviceVulkan12Features vulkan12Features = {};
            VkPhysicalDeviceShaderFloat16Int8FeaturesKHR float16Int8 = {};
            VkPhysicalDeviceSeparateDepthStencilLayoutsFeaturesKHR separateDepthStencil = {};

            VkDeviceCreateInfo deviceInfo = {};
            deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

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
                vulkan12Features.bufferDeviceAddress = physicalDevice.GetPhysicalDeviceVulkan12Features().bufferDeviceAddress;
                vulkan12Features.bufferDeviceAddressMultiDevice = physicalDevice.GetPhysicalDeviceVulkan12Features().bufferDeviceAddressMultiDevice;
                vulkan12Features.runtimeDescriptorArray = physicalDevice.GetPhysicalDeviceVulkan12Features().runtimeDescriptorArray;
                robustness2.pNext = &vulkan12Features;
                deviceInfo.pNext = &depthClipEnabled;
            }
            else
            {
                float16Int8.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR;
                float16Int8.shaderFloat16 = physicalDevice.GetPhysicalDeviceFloat16Int8Features().shaderFloat16;

                separateDepthStencil.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES;
                separateDepthStencil.separateDepthStencilLayouts = physicalDevice.GetPhysicalDeviceSeparateDepthStencilFeatures().separateDepthStencilLayouts;
                float16Int8.pNext = &separateDepthStencil;

                robustness2.pNext = &float16Int8;


                deviceInfo.pNext = &descriptorIndexingFeatures;
            }

            deviceInfo.flags = 0;
            deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreationInfo.size());
            deviceInfo.pQueueCreateInfos = queueCreationInfo.data();
            deviceInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
            deviceInfo.ppEnabledLayerNames = requiredLayers.data();
            deviceInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
            deviceInfo.ppEnabledExtensionNames = requiredExtensions.data();
            deviceInfo.pEnabledFeatures = &m_enabledDeviceFeatures;

            const VkResult vkResult = vkCreateDevice(physicalDevice.GetNativePhysicalDevice(),
                &deviceInfo, nullptr, &m_nativeDevice);
            AssertSuccess(vkResult);
            RETURN_RESULT_IF_UNSUCCESSFUL(ConvertResult(vkResult));

            for (const VkDeviceQueueCreateInfo& queueInfo : queueCreationInfo)
            {
                delete queueInfo.pQueuePriorities;
            }

            Instance& instance = Instance::GetInstance();
            instance.GetFunctionLoader().LoadProcAddresses(instance.GetNativeInstance(), physicalDevice.GetNativePhysicalDevice(), m_nativeDevice);

            //Load device features now that we have loaded all extension info
            physicalDevice.LoadSupportedFeatures();
            return RHI::ResultCode::Success;
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

            m_imageMemoryRequirementsCache.SetInitFunction([](auto& cache) { cache.set_capacity(MemoryRequirementsCacheSize); });
            m_bufferMemoryRequirementsCache.SetInitFunction([](auto& cache) { cache.set_capacity(MemoryRequirementsCacheSize); });

            m_stagingBufferPool = BufferPool::Create();
            RHI::BufferPoolDescriptor poolDesc;
            poolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
            poolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
            poolDesc.m_bindFlags = RHI::BufferBindFlags::CopyRead;
            poolDesc.m_budgetInBytes = m_descriptor.m_platformLimitsDescriptor->m_platformDefaultValues.m_stagingBufferBudgetInBytes;
            result = m_stagingBufferPool->Init(*this, poolDesc);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            if (!physicalDevice.IsFeatureSupported(DeviceFeature::NullDescriptor))
            {
                m_nullDescriptorManager = NullDescriptorManager::Create();
                result = m_nullDescriptorManager->Init(*this);
                RETURN_RESULT_IF_UNSUCCESSFUL(result);
            }

            SetName(GetName());
            return result;
        }

        VkDevice Device::GetNativeDevice() const
        {
            return m_nativeDevice;
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
                Image image;
                [[maybe_unused]] RHI::ResultCode result = image.Init(*this, descriptor);
                AZ_Assert(result == RHI::ResultCode::Success, "Failed to get memory requirements");
                auto it2 = cache.insert(hash, image.m_memoryRequirements);
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
                VkBuffer vkBuffer = CreateBufferResouce(descriptor);
                AZ_Assert(vkBuffer != VK_NULL_HANDLE, "Failed to get memory requirements");
                VkMemoryRequirements memoryRequirements = {};
                vkGetBufferMemoryRequirements(GetNativeDevice(), vkBuffer, &memoryRequirements);
                auto it2 = cache.insert(hash, memoryRequirements);
                DestroyBufferResource(vkBuffer);
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

        VkImageUsageFlags Device::GetImageUsageFromFormat(RHI::Format format)
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

        const AZStd::vector<VkQueueFamilyProperties>& Device::GetQueueFamilyProperties() const
        {
            return m_queueFamilyProperties;
        }

        AsyncUploadQueue& Device::GetAsyncUploadQueue()
        {
            if (!m_asyncUploadQueue)
            {
                m_asyncUploadQueue = aznew AsyncUploadQueue();
                AsyncUploadQueue::Descriptor asyncUploadQueueDescriptor(RHI::RHISystemInterface::Get()->GetPlatformLimitsDescriptor()->m_platformDefaultValues.m_asyncQueueStagingBufferSizeInBytes);
                asyncUploadQueueDescriptor.m_device = this;
                m_asyncUploadQueue->Init(asyncUploadQueueDescriptor);
            }

            return *m_asyncUploadQueue;
        }        

        RHI::Ptr<Buffer> Device::AcquireStagingBuffer(AZStd::size_t byteCount)
        {
            RHI::Ptr<Buffer> stagingBuffer = Buffer::Create();
            RHI::BufferDescriptor bufferDesc(RHI::BufferBindFlags::CopyRead, byteCount);
            RHI::BufferInitRequest initRequest(*stagingBuffer, bufferDesc);
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

            m_commandQueueContext.Shutdown();

            m_stagingBufferPool.reset();
            m_renderPassCache.first.Clear();
            m_framebufferCache.first.Clear();
            m_descriptorSetLayoutCache.first.Clear();
            m_samplerCache.first.Clear();
            m_pipelineLayoutCache.first.Clear();
            m_semaphoreAllocator.Shutdown();
            m_asyncUploadQueue.reset();
            m_commandListAllocator.Shutdown();

            m_nullDescriptorManager.reset();

            // Make sure this is last to flush any objects released in the above calls.
            m_releaseQueue.Shutdown();
        }

        void Device::ShutdownInternal()
        {
            m_imageMemoryRequirementsCache.Clear();
            m_bufferMemoryRequirementsCache.Clear();

            if ( m_nativeDevice != VK_NULL_HANDLE )
            {
                vkDestroyDevice( m_nativeDevice, nullptr );
                m_nativeDevice = VK_NULL_HANDLE;
            }
        }

        void Device::BeginFrameInternal() 
        {
            // We call to collect on the release queue on the begin frame because some objects (like resource pools)
            // cannot be shutdown during the frame scheduler execution. At this point the frame has not yet started.
            m_releaseQueue.Collect();
            m_commandQueueContext.Begin();
        }
        
        void Device::EndFrameInternal() 
        {
            m_commandQueueContext.End();
            m_commandListAllocator.Collect();
            m_semaphoreAllocator.Collect();
        }

        void Device::WaitForIdleInternal() 
        {
            m_commandQueueContext.WaitForIdle();
            m_releaseQueue.Collect(true);
        }

        void Device::CompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder) 
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            physicalDevice.CompileMemoryStatistics(builder);
        }

        void Device::UpdateCpuTimingStatisticsInternal() const
        {
            m_commandQueueContext.UpdateCpuTimingStatistics();
        }

        AZStd::vector<RHI::Format> Device::GetValidSwapChainImageFormats(const RHI::WindowHandle& windowHandle) const
        {
            AZStd::vector<RHI::Format> formatsList;
            WSISurface::Descriptor surfaceDescriptor{windowHandle};
            RHI::Ptr<WSISurface> surface = WSISurface::Create();
            const RHI::ResultCode result = surface->Init(surfaceDescriptor);
            if (result != RHI::ResultCode::Success)
            {
                return formatsList;
            }
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            uint32_t surfaceFormatCount = 0;
            AssertSuccess(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.GetNativePhysicalDevice(), surface->GetNativeSurface(), &surfaceFormatCount, nullptr));
            if (surfaceFormatCount == 0)
            {
                AZ_Assert(false, "Surface support no format.");
                return formatsList;
            }

            AZStd::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
            AssertSuccess(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.GetNativePhysicalDevice(), surface->GetNativeSurface(), &surfaceFormatCount, surfaceFormats.data()));

            AZStd::set<RHI::Format> formats;
            for (const VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
            {
                formats.insert(ConvertFormat(surfaceFormat.format));
            }
            formatsList.assign(formats.begin(), formats.end());
            return formatsList;
        }

        void Device::FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities)
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
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
            }
        }

        AZStd::chrono::microseconds Device::GpuTimestampToMicroseconds(uint64_t gpuTimestamp, [[maybe_unused]] RHI::HardwareQueueClass queueClass) const
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            auto timeInNano = AZStd::chrono::nanoseconds(static_cast<AZStd::chrono::nanoseconds::rep>(physicalDevice.GetDeviceLimits().timestampPeriod * gpuTimestamp));
            return AZStd::chrono::microseconds(timeInNano);
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

        void Device::InitFeaturesAndLimits(const PhysicalDevice& physicalDevice)
        {
            m_features.m_tessellationShader = (m_enabledDeviceFeatures.tessellationShader == VK_TRUE);
            m_features.m_geometryShader = (m_enabledDeviceFeatures.geometryShader == VK_TRUE);
            m_features.m_computeShader = true;
            m_features.m_independentBlend = (m_enabledDeviceFeatures.independentBlend == VK_TRUE);
            m_features.m_customResolvePositions = physicalDevice.IsFeatureSupported(DeviceFeature::CustomSampleLocation);
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
            m_features.m_renderTargetSubpassInputSupport = RHI::SubpassInputSupportType::Native;
            m_features.m_depthStencilSubpassInputSupport = RHI::SubpassInputSupportType::Native;

            // check for the VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME in the list of physical device extensions
            // to determine if ray tracing is supported on this device
            StringList deviceExtensions = physicalDevice.GetDeviceExtensionNames();
            StringList::iterator itRayTracingExtension = AZStd::find(deviceExtensions.begin(), deviceExtensions.end(), VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            m_features.m_rayTracing = (itRayTracingExtension != deviceExtensions.end());
            m_features.m_unboundedArrays = physicalDevice.GetPhysicalDeviceDescriptorIndexingFeatures().shaderStorageTexelBufferArrayNonUniformIndexing;

            const auto& deviceLimits = physicalDevice.GetDeviceLimits();
            m_limits.m_maxImageDimension1D = deviceLimits.maxImageDimension1D;
            m_limits.m_maxImageDimension2D = deviceLimits.maxImageDimension2D;
            m_limits.m_maxImageDimension3D = deviceLimits.maxImageDimension3D;
            m_limits.m_maxImageDimensionCube = deviceLimits.maxImageDimensionCube;
            m_limits.m_maxImageArraySize = deviceLimits.maxImageArrayLayers;
            m_limits.m_minConstantBufferViewOffset = static_cast<uint32_t>(deviceLimits.minUniformBufferOffsetAlignment);
            m_limits.m_maxIndirectDrawCount = deviceLimits.maxDrawIndirectCount;
        }

        void Device::BuildDeviceQueueInfo(const PhysicalDevice& physicalDevice)
        {
            m_queueFamilyProperties.clear();
            VkPhysicalDevice nativePhysicalDevice = physicalDevice.GetNativePhysicalDevice();

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(nativePhysicalDevice, &queueFamilyCount, nullptr);
            AZ_Assert(queueFamilyCount, "No queue families were found for physical device %s", physicalDevice.GetName().GetCStr());

            m_queueFamilyProperties.resize(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(nativePhysicalDevice, &queueFamilyCount, m_queueFamilyProperties.data());            
        }

        RHI::Ptr<Memory> Device::AllocateMemory(uint64_t sizeInBytes, const uint32_t memoryTypeMask, const VkMemoryPropertyFlags flags, const RHI::BufferBindFlags bufferBindFlags)
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(GetPhysicalDevice());
            const VkPhysicalDeviceMemoryProperties& memProp = physicalDevice.GetMemoryProperties();

            RHI::Ptr<Memory> memory = Memory::Create();
            Memory::Descriptor memoryDesc;
            memoryDesc.m_sizeInBytes = sizeInBytes;

            // Flags that we remove in each new allocation try.
            VkMemoryPropertyFlags filterFlags[] =
            {
                VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // Try removing the cache/coherent flags.
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,    // Remove the host visible flag in case we run out of host memory.
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,    // Try to remove the device local flag (so we fallback to the host visible memory).
                ~0u                                     // This will remove all flags
            };

            VkMemoryPropertyFlags memoryFlags = flags;
            uint32_t memoryTypesToUseMask = memoryTypeMask;
            for (uint32_t filterIndex = 0; filterIndex < AZ_ARRAY_SIZE(filterFlags); filterIndex++)
            {
                // Try to allocate from all supported memory types that have necessary flags.
                for (uint32_t memoryIndex = 0; memoryIndex < memProp.memoryTypeCount; ++memoryIndex)
                {
                    const uint32_t memoryTypeBit = AZ_BIT(memoryIndex);
                    if (RHI::CheckBitsAll(memProp.memoryTypes[memoryIndex].propertyFlags, memoryFlags) &&
                        RHI::CheckBitsAll(memoryTypesToUseMask, memoryTypeBit))
                    {
                        memoryDesc.m_memoryTypeIndex = memoryIndex;
                        memoryDesc.m_bufferBindFlags = bufferBindFlags;
                        auto result = memory->Init(*this, memoryDesc);
                        if (result == RHI::ResultCode::Success)
                        {
                            AZ_Warning("Vulkan", memoryFlags == flags, "Could not allocate memory using VkMemoryPropertyFlags %u, fallback to using VkMemoryPropertyFlags %u instead", flags, memoryFlags);
                            return memory;
                        }
                        memoryTypesToUseMask = RHI::ResetBits(memoryTypesToUseMask, memoryTypeBit);
                    }
                }

                // Since we couldn't allocate with the current flags,
                // remove some of them and try again.
                memoryFlags = RHI::ResetBits(memoryFlags, filterFlags[filterIndex]);
            }

            AZ_Assert(memory, "Failed to allocate memory size %llu bytes with flags %u, and memory types %u", static_cast<unsigned long long>(sizeInBytes), flags, memoryTypeMask);
            return memory;
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

        VkBuffer Device::CreateBufferResouce(const RHI::BufferDescriptor& descriptor) const
        {
            AZ_Assert(descriptor.m_sharedQueueMask != RHI::HardwareQueueClassMask::None, "Invalid shared queue mask");
            AZStd::vector<uint32_t> queueFamilies(GetCommandQueueContext().GetQueueFamilyIndices(descriptor.m_sharedQueueMask));

            VkBufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.size = descriptor.m_byteCount;
            createInfo.usage = GetBufferUsageFlagBitsUnderRestrictions(descriptor.m_bindFlags);
            // Trying to guess here if the buffers are going to be used as attachments. Maybe it would be better to add an explicit flag in the descriptor.
            createInfo.sharingMode = 
                (RHI::CheckBitsAny(
                    descriptor.m_bindFlags, 
                    RHI::BufferBindFlags::ShaderWrite | RHI::BufferBindFlags::Predication | RHI::BufferBindFlags::Indirect) || 
                    (queueFamilies.size()) <= 1) 
                ? VK_SHARING_MODE_EXCLUSIVE 
                : VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilies.size());
            createInfo.pQueueFamilyIndices = queueFamilies.empty() ? nullptr : queueFamilies.data();

            VkBuffer vkBuffer = VK_NULL_HANDLE;
            VkResult vkResult = vkCreateBuffer(GetNativeDevice(), &createInfo, nullptr, &vkBuffer);
            AssertSuccess(vkResult);
            return vkBuffer;
        }

        void Device::DestroyBufferResource(VkBuffer vkBuffer) const
        {
            vkDestroyBuffer(GetNativeDevice(), vkBuffer, nullptr);
        }
    }
}
