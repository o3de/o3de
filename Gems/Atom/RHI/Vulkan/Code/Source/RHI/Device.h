/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Loader/LoaderContext.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI/Device.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI/ObjectCache.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/ThreadLocalContext.h>
#include <AtomCore/std/containers/lru_cache.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/utils.h>
#include <RHI/BindlessDescriptorPool.h>
#include <RHI/Buffer.h>
#include <RHI/CommandList.h>
#include <RHI/CommandListAllocator.h>
#include <RHI/CommandPool.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/DescriptorSetLayout.h>
#include <RHI/Framebuffer.h>
#include <RHI/NullDescriptorManager.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/Queue.h>
#include <RHI/ReleaseQueue.h>
#include <RHI/RenderPass.h>
#include <RHI/Sampler.h>
#include <RHI/SemaphoreAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        class BufferPool;
        class ImagePool;
        class GraphicsPipeline;
        class SwapChain;
        class AsyncUploadQueue;

        //! Helper class to contain a vulkan create info structure
        //! Since the create info points into memory arrays, we need to keep the
        //! arrays alive when returning the create info from a function.
        template<class T, class ExtT>
        struct CreateInfoContainer
        {
            CreateInfoContainer() = default;
            AZ_DEFAULT_COPY_MOVE(CreateInfoContainer);

            void SetCreateInfo(const T& createInfo)
            {
                m_vkCreateInfo = createInfo;
            }
            void SetExternalCreateInfo(const ExtT& createInfo)
            {
                m_vkExtCreateInfo = createInfo;
            }

            T* GetCreateInfo()
            {
                if (m_vkExtCreateInfo.sType != 0)
                {
                    // Add the external info to the createInfo if it exists
                    // We do this here, so the pointer is always valid, even after copying this struct
                    m_vkCreateInfo.pNext = &m_vkExtCreateInfo;
                }
                return &m_vkCreateInfo;
            }

            //! Vector of queue families that the create info structure points to
            AZStd::vector<uint32_t> m_queueFamilyIndices;

        private:
            //! Vulkan create info structure
            T m_vkCreateInfo = {};
            ExtT m_vkExtCreateInfo = {};
        };

        using BufferCreateInfo = CreateInfoContainer<VkBufferCreateInfo, VkExternalMemoryBufferCreateInfo>;
        using ImageCreateInfo = CreateInfoContainer<VkImageCreateInfo, VkExternalMemoryImageCreateInfo>;

        class Device final
            : public RHI::Device
            , public RHI::RHISystemNotificationBus::Handler
        {
            using Base = RHI::Device;
        public:
            AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator);
            AZ_RTTI(Device, "C77D578F-841F-41B0-84BB-EE5430FCF8BC", Base);

            static RHI::Ptr<Device> Create();
            ~Device();

            static StringList GetRequiredLayers();
            static StringList GetRequiredExtensions();

            VkDevice GetNativeDevice() const;

            const GladVulkanContext& GetContext() const;

            uint32_t FindMemoryTypeIndex(VkMemoryPropertyFlags memoryPropertyFlags, uint32_t memoryTypeBits) const;

            VkMemoryRequirements GetImageMemoryRequirements(const RHI::ImageDescriptor& descriptor);
            VkMemoryRequirements GetBufferMemoryRequirements(const RHI::BufferDescriptor& descriptor);

            const VkPhysicalDeviceFeatures& GetEnabledDevicesFeatures() const;

            VkPipelineStageFlags GetSupportedPipelineStageFlags() const;

            // Some capability of image is restricted by a combination of physical device (GPU) and image format.
            // For example, a GPU cannot use a image of BC1_UNORM format for storage image
            // (and a certain kind of GPU might be able to use it).
            // GetImageUsageFromFormat gives capabilities of images of the given format for this device.
            VkImageUsageFlags GetImageUsageFromFormat(RHI::Format format) const;

            CommandQueueContext& GetCommandQueueContext();
            const CommandQueueContext& GetCommandQueueContext() const;
            SemaphoreAllocator& GetSemaphoreAllocator();
            SwapChainSemaphoreAllocator& GetSwapChainSemaphoreAllocator();

            const AZStd::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() const;

            AsyncUploadQueue& GetAsyncUploadQueue();

            BindlessDescriptorPool& GetBindlessDescriptorPool();

            RHI::Ptr<Buffer> AcquireStagingBuffer(AZStd::size_t byteCount, AZStd::size_t alignment = 1);

            void QueueForRelease(RHI::Ptr<RHI::Object> object);

            RHI::Ptr<RenderPass> AcquireRenderPass(const RenderPass::Descriptor& descriptor);
            RHI::Ptr<Framebuffer> AcquireFramebuffer(const Framebuffer::Descriptor& descriptor);
            RHI::Ptr<DescriptorSetLayout> AcquireDescriptorSetLayout(const DescriptorSetLayout::Descriptor& descriptor);
            RHI::Ptr<Sampler> AcquireSampler(const Sampler::Descriptor& descriptor);
            RHI::Ptr<PipelineLayout> AcquirePipelineLayout(const PipelineLayout::Descriptor& descriptor);

            RHI::Ptr<CommandList> AcquireCommandList(uint32_t familyQueueIndex, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            RHI::Ptr<CommandList> AcquireCommandList(RHI::HardwareQueueClass queueClass, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

            uint32_t GetCurrentFrameIndex() const;

            NullDescriptorManager& GetNullDescriptorManager();

            //! Fills a vulkan buffer create info with the provided descriptor
            BufferCreateInfo BuildBufferCreateInfo(const RHI::BufferDescriptor& descriptor) const;
            //! Fills a vulkan image create info with the provided descriptor
            ImageCreateInfo BuildImageCreateInfo(const RHI::ImageDescriptor& descriptor) const;

            // Supported modes when specifiying the shading rate through an image.
            enum class ShadingRateImageMode : uint32_t
            {
                None,           // Not supported
                ImageAttachment,// Using the VK_KHR_fragment_shading_rate extension
                DensityMap      // Using VK_EXT_fragment_density_map extension
            };

            ShadingRateImageMode GetImageShadingRateMode() const;

            RHI::Ptr<BufferPool> GetConstantBufferPool();

            //! Returns the VMA allocator used by this device.
            VmaAllocator& GetVmaAllocator();

        protected:
            // RHI::RHISystemNotificationBus::Handler overrides...
            void OnRHISystemInitialized() override;

        private:
            Device();

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Device
            RHI::ResultCode InitInternal(RHI::PhysicalDevice& physicalDevice) override;
            RHI::ResultCode InitInternalBindlessSrg(const AZ::RHI::BindlessSrgDescriptor& bindlessSrgDesc) override;
            void ShutdownInternal() override;
            RHI::ResultCode BeginFrameInternal() override;
            void EndFrameInternal() override;
            void WaitForIdleInternal() override;
            void CompileMemoryStatisticsInternal(RHI::MemoryStatisticsBuilder& builder) override;
            void UpdateCpuTimingStatisticsInternal() const override;
            AZStd::vector<RHI::Format> GetValidSwapChainImageFormats(const RHI::WindowHandle& windowHandle) const override;
            AZStd::chrono::microseconds GpuTimestampToMicroseconds(uint64_t gpuTimestamp, RHI::HardwareQueueClass queueClass) const override;
            AZStd::pair<uint64_t, uint64_t> GetCalibratedTimestamp(RHI::HardwareQueueClass queueClass) override;
            void FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities) override;
            RHI::ResultCode InitializeLimits() override;
            void PreShutdown() override;
            RHI::ResourceMemoryRequirements GetResourceMemoryRequirements(const RHI::ImageDescriptor& descriptor) override;
            RHI::ResourceMemoryRequirements GetResourceMemoryRequirements(const RHI::BufferDescriptor& descriptor) override;
            void ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction) override;
            RHI::ShadingRateImageValue ConvertShadingRate(RHI::ShadingRate rate) const override;
            RHI::Ptr<RHI::XRDeviceDescriptor> BuildXRDescriptor() const override;
            //////////////////////////////////////////////////////////////////////////

            void InitFeaturesAndLimits(const PhysicalDevice& physicalDevice);
            void BuildDeviceQueueInfo(const PhysicalDevice& physicalDevice);

            template<typename ObjectType>
            using ObjectCache = std::pair<RHI::ObjectCache<ObjectType>, AZStd::mutex>;

            template <typename ObjectType, typename... Args>
            RHI::Ptr<ObjectType> AcquireObjectFromCache(ObjectCache<ObjectType>& cache, const size_t hash, Args... args);

            //! Get the vulkan buffer usage flags from buffer bind flags.
            //! Flags will be corrected if required features or extensions are not enabled.
            VkBufferUsageFlags GetBufferUsageFlagBitsUnderRestrictions(RHI::BufferBindFlags bindFlags) const;

            // Initialize the VMA allocator for this device
            RHI::ResultCode InitVmaAllocator(RHI::PhysicalDevice& physicalDevice);
            // Shutdown VMA allocator of this device
            void ShutdownVmaAllocator();

            VkImageUsageFlags CalculateImageUsageFlags(const RHI::ImageDescriptor& descriptor) const;
            VkImageCreateFlags CalculateImageCreateFlags(const RHI::ImageDescriptor& descriptor) const;

            //! Calibrated Timestamps
            void InitializeTimeDomains();

            VkDevice m_nativeDevice = VK_NULL_HANDLE;
            VmaAllocator m_vmaAllocator = VK_NULL_HANDLE;
            VkPhysicalDeviceFeatures m_enabledDeviceFeatures{};
            VkPipelineStageFlags m_supportedPipelineStageFlagsMask = std::numeric_limits<VkPipelineStageFlags>::max();

            AZStd::unique_ptr<LoaderContext> m_loaderContext;

            AZStd::vector<VkQueueFamilyProperties> m_queueFamilyProperties;
            RHI::Ptr<AsyncUploadQueue> m_asyncUploadQueue;
            CommandListAllocator m_commandListAllocator;
            SemaphoreAllocator m_semaphoreAllocator;
            SwapChainSemaphoreAllocator m_swapChainSemaphoreAllocator;

            // New VkImageUsageFlags are inserted in the map in a lazy way.
            // Because of this, the map containing the usages per formar is mutable to keep the
            // constness of the GetImageUsageFromFormat function.
            mutable AZStd::unordered_map<RHI::Format, VkImageUsageFlags> m_imageUsageOfFormat;

            RHI::Ptr<BufferPool> m_stagingBufferPool;

            RHI::Ptr<BufferPool> m_constantBufferPool;

            ReleaseQueue m_releaseQueue;
            CommandQueueContext m_commandQueueContext;

            static const uint32_t RenderPassCacheCapacity = 10000;
            static const uint32_t FrameBufferCacheCapacity = 1000;
            static const uint32_t DescriptorLayoutCacheCapacity = 1000;
            static const uint32_t SamplerCacheCapacity = 1000;
            static const uint32_t PipelineLayoutCacheCapacity = 1000;

            ObjectCache<RenderPass> m_renderPassCache;
            ObjectCache<Framebuffer> m_framebufferCache;
            ObjectCache<DescriptorSetLayout> m_descriptorSetLayoutCache;
            ObjectCache<Sampler> m_samplerCache;
            ObjectCache<PipelineLayout> m_pipelineLayoutCache;

            static const uint32_t MemoryRequirementsCacheSize = 100;
            RHI::ThreadLocalContext<AZStd::lru_cache<uint64_t, VkMemoryRequirements>> m_imageMemoryRequirementsCache;
            RHI::ThreadLocalContext<AZStd::lru_cache<uint64_t, VkMemoryRequirements>> m_bufferMemoryRequirementsCache;

            RHI::Ptr<NullDescriptorManager> m_nullDescriptorManager;

            BindlessDescriptorPool m_bindlessDescriptorPool;
            ShadingRateImageMode m_imageShadingRateMode = ShadingRateImageMode::None;

            VkTimeDomainEXT m_hostTimeDomain = VK_TIME_DOMAIN_MAX_ENUM_EXT;
        };

        template<typename ObjectType, typename ...Args>
        inline RHI::Ptr<ObjectType> Device::AcquireObjectFromCache(ObjectCache<ObjectType>& cache, const size_t hash, Args... args)
        {
            AZStd::lock_guard<AZStd::mutex> lock(cache.second);
            auto& cacheContainer = cache.first;
            ObjectType* objectRawPtr = cacheContainer.Find(hash);
            if (!objectRawPtr)
            {
                RHI::Ptr<ObjectType> objectPtr = ObjectType::Create();
                if (objectPtr->Init(AZStd::forward<Args>(args)...) == RHI::ResultCode::Success)
                {
                    objectRawPtr = objectPtr.get();
                    cacheContainer.Insert(hash, AZStd::move(objectPtr));
                }
                else
                {
                    AZ_Error("Vulkan", false, "Failed to create a cached object");
                }
            }
            return RHI::Ptr<ObjectType>(objectRawPtr);
        }
    }
}
