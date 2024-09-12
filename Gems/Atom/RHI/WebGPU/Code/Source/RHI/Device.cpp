/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/Device.h>
#include <RHI/Instance.h>
#include <RHI/Conversions.h>
#include <RHI/PhysicalDevice.h>
#include <RHI/NullDescriptorManager.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI.Reflect/WebGPU/PlatformLimitsDescriptor.h>

namespace AZ::WebGPU
{
    RHI::Ptr<Device> Device::Create()
    {
        return aznew Device();
    }

    Device::Device()
    {
        RHI::Ptr<PlatformLimitsDescriptor> platformLimitsDescriptor = aznew PlatformLimitsDescriptor();
        platformLimitsDescriptor->LoadPlatformLimitsDescriptor(RHI::Factory::Get().GetName().GetCStr());
        m_descriptor.m_platformLimitsDescriptor = RHI::Ptr<RHI::PlatformLimitsDescriptor>(platformLimitsDescriptor);
    }

    RHI::ResultCode Device::InitInternal(RHI::PhysicalDevice& physicalDeviceBase)
    {
        PhysicalDevice& physicalDevice = static_cast<PhysicalDevice&>(physicalDeviceBase);
        const auto& wgpuPhysicalDevice = physicalDevice.GetNativeAdapter();
        wgpu::SupportedLimits supportedLimits = {};
        wgpuPhysicalDevice.GetLimits(&supportedLimits);
        wgpu::AdapterInfo adapterInfo = {};
        wgpuPhysicalDevice.GetInfo(&adapterInfo);
        AZStd::vector<wgpu::FeatureName> requiredFeatures = { wgpu::FeatureName::TextureCompressionBC,  };
        wgpu::RequiredLimits requiredLImits = {};
        requiredLImits.limits.maxStorageBuffersPerShaderStage = AZStd::min(supportedLimits.limits.maxStorageBuffersPerShaderStage, 24u);
        requiredLImits.limits.maxStorageTexturesPerShaderStage = AZStd::min(supportedLimits.limits.maxStorageTexturesPerShaderStage, 24u);
        wgpu::DeviceDescriptor wgpuDeviceDesc = {};
        // Set device callbacks
        wgpuDeviceDesc = {};
        wgpuDeviceDesc.SetDeviceLostCallback(wgpu::CallbackMode::AllowSpontaneous, Device::DeviceLostCallback);
        wgpuDeviceDesc.SetUncapturedErrorCallback(&Device::ErrorCallback);
        wgpuDeviceDesc.requiredFeatureCount = requiredFeatures.size();
        wgpuDeviceDesc.requiredFeatures = requiredFeatures.data();
        wgpuDeviceDesc.requiredLimits = &requiredLImits;

        // Synchronously create the device
        auto& instance = Instance::GetInstance().GetNativeInstance();
        m_wgpuDevice = nullptr;
        instance.WaitAny(
            physicalDevice.GetNativeAdapter().RequestDevice(
                &wgpuDeviceDesc,
                wgpu::CallbackMode::WaitAnyOnly,
                [this](wgpu::RequestDeviceStatus status, wgpu::Device device, const char* message)
                {
                    if (status != wgpu::RequestDeviceStatus::Success)
                    {
                        AZ_Assert(false, "Failed to get a device: %s", message);
                        return;
                    }
                    m_wgpuDevice = std::move(device);
                }),
            UINT64_MAX);

        if (!m_wgpuDevice)
        {
            return RHI::ResultCode::Fail;
        }

        RHI::RHISystemNotificationBus::Handler::BusConnect();
        return RHI::ResultCode::Success;
    }

    void Device::ShutdownInternal()
    {
        RHI::RHISystemNotificationBus::Handler::BusDisconnect();
        m_wgpuDevice = nullptr;
    }

    RHI::ResultCode Device::BeginFrameInternal()
    {
        return RHI::ResultCode::Success;
    }

    void Device::EndFrameInternal()
    {
        m_frameCommandLists.clear();
    }

    const wgpu::Device& Device::GetNativeDevice() const
    {
        return m_wgpuDevice;
    }

    wgpu::Device& Device::GetNativeDevice()
    {
        return m_wgpuDevice;
    }

    CommandList* Device::AcquireCommandList([[maybe_unused]] RHI::HardwareQueueClass hardwareQueueClass)
    {
        RHI::Ptr<CommandList> commandList = CommandList::Create();
        commandList->Init(*this);
        m_frameCommandLists.push_back(commandList);
        return commandList.get();
    }

    CommandQueueContext& Device::GetCommandQueueContext()
    {
        return m_commandQueueContext;
    }

    void Device::FillFormatsCapabilitiesInternal(FormatCapabilitiesList& formatsCapabilities)
    {
        formatsCapabilities.fill(static_cast<RHI::FormatCapabilities>(~0));
    }

    RHI::ResultCode Device::InitializeLimits()
    {
        constexpr uint32_t maxValue = static_cast<uint32_t>(-1);

        m_limits.m_maxImageDimension1D = maxValue;
        m_limits.m_maxImageDimension2D = maxValue;
        m_limits.m_maxImageDimension3D = maxValue;
        m_limits.m_maxImageDimensionCube = maxValue;
        m_limits.m_maxImageArraySize = maxValue;
        m_limits.m_minConstantBufferViewOffset = RHI::Alignment::Constant;
        m_limits.m_maxIndirectDrawCount = maxValue;
        m_limits.m_maxIndirectDispatchCount = maxValue;
        m_limits.m_maxConstantBufferSize = maxValue;
        m_limits.m_maxBufferSize = maxValue;

        m_features.m_resourceAliasing = false;
        m_features.m_multithreading = false;
        // Only 2D images can be used for render attachments
        m_features.m_supportedRenderAttachmentDimensions = RHI::ImageDimensionFlags::Image2D;

        // Set the cache sizes.
        m_samplerCache.first.SetCapacity(SamplerCacheCapacity);
        m_pipelineLayoutCache.first.SetCapacity(PipelineLayoutCacheCapacity);

        m_commandQueueContext.Init(*this);

        m_constantBufferPool = BufferPool::Create();
        static int index = 0;
        m_constantBufferPool->SetName(Name(AZStd::string::format("ConstantPool_%d", ++index)));

        RHI::BufferPoolDescriptor bufferPoolDescriptor;
        bufferPoolDescriptor.m_bindFlags = RHI::BufferBindFlags::Constant;
        bufferPoolDescriptor.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
        RHI::ResultCode result = m_constantBufferPool->Init(*this, bufferPoolDescriptor);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        m_stagingBufferPool = BufferPool::Create();
        BufferPoolDescriptor poolDesc;
        poolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
        poolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
        poolDesc.m_bindFlags = RHI::BufferBindFlags::CopyRead;
        poolDesc.m_budgetInBytes = m_descriptor.m_platformLimitsDescriptor->m_platformDefaultValues.m_stagingBufferBudgetInBytes;
        poolDesc.m_mappedAtCreation = true;
        m_stagingBufferPool->SetName(AZ::Name("Device_StagingBufferPool"));
        result = m_stagingBufferPool->Init(*this, poolDesc);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        return RHI::ResultCode::Success;
    }

    void Device::PreShutdown()
    {
        m_nullDescriptorManager = nullptr;
        m_samplerCache.first.Clear();
        m_pipelineLayoutCache.first.Clear();

        m_stagingBufferPool.reset();
        m_constantBufferPool.reset();
        m_commandQueueContext.Shutdown();

        m_asyncUploadQueue.reset();
    }

    void Device::ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction)
    {
        notifyFunction();
    }

    RHI::ShadingRateImageValue Device::ConvertShadingRate([[maybe_unused]] RHI::ShadingRate rate) const
    {
        return RHI::ShadingRateImageValue{};
    }

    void Device::DeviceLostCallback([[maybe_unused]] const wgpu::Device& wgpuDevice, wgpu::DeviceLostReason reason, const char* message)
    {
        AZ_Error("WebGPU", false, "Device lost because of %s : %s", ToString(reason), message);
    }

    void Device::ErrorCallback([[maybe_unused]] const wgpu::Device& wgpuDevice, wgpu::ErrorType type, const char* message)
    {
        AZ_Error("WebGPU", false, "Device error of type %s : %s", ToString(type), message);
    }

    RHI::Ptr<PipelineLayout> Device::AcquirePipelineLayout(const PipelineLayout::Descriptor& descriptor)
    {
        return AcquireObjectFromCache(m_pipelineLayoutCache, descriptor.GetHash(), descriptor);
    }

    RHI::Ptr<Sampler> Device::AcquireSampler(const Sampler::Descriptor& descriptor)
    {
        return AcquireObjectFromCache(m_samplerCache, descriptor.GetHash(), descriptor);
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

    RHI::Ptr<BufferPool> Device::GetConstantBufferPool()
    {
        return m_constantBufferPool;
    }

    AsyncUploadQueue& Device::GetAsyncUploadQueue()
    {
        if (!m_asyncUploadQueue)
        {
            m_asyncUploadQueue = aznew AsyncUploadQueue();
            m_asyncUploadQueue->Init(*this);
        }

        return *m_asyncUploadQueue;
    }

    NullDescriptorManager& Device::GetNullDescriptorManager()
    {
        AZ_Assert(m_nullDescriptorManager, "NullDescriptorManager was not created.");
        return *m_nullDescriptorManager;
    }

    void Device::OnRHISystemInitialized()
    {
        // Need to initialize the NullDescriptorManager AFTER the bindless descriptor pool, since we create images and buffers
        // during the initialization of the NullDescriptorManager.
        m_nullDescriptorManager = NullDescriptorManager::Create();
        [[maybe_unused]] RHI::ResultCode result = m_nullDescriptorManager->Init(*this);
        AZ_Error("WebGPU", result == RHI::ResultCode::Success, "Failed to initialize Null descriptor manager");
    }

}
