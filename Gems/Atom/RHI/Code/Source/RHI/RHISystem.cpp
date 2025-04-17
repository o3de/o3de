/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Device.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RHISystem.h>
#include <Atom/RHI/RHIUtils.h>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Interface/Interface.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/CommandLine/CommandLine.h>
#include <AzFramework/Device/DeviceAttributeInterface.h>
#include <AzFramework/Device/DeviceAttributeGPUModel.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>

AZ_DEFINE_BUDGET(RHI);

namespace AZ::RHI
{
    RHISystemInterface* RHISystemInterface::Get()
    {
        return Interface<RHISystemInterface>::Get();
    }

    RHIMemoryStatisticsInterface* RHIMemoryStatisticsInterface::Get()
    {
        return Interface<RHIMemoryStatisticsInterface>::Get();
    }

    ResultCode RHISystem::InitDevices(int deviceCount)
    {
        Interface<RHISystemInterface>::Register(this);
        Interface<RHIMemoryStatisticsInterface>::Register(this);
        return InitInternalDevices(deviceCount);
    }
    
    void RHISystem::Init(RHI::Ptr<RHI::ShaderResourceGroupLayout> bindlessSrgLayout)
    {
        //! If a bindless srg layout is not provided we simply skip initialization with the assumption that no one will use bindless srg
        if (bindlessSrgLayout)
        {
            [[maybe_unused]] bool success = true;

            for (auto device : m_devices)
            {
                success &= device->InitBindlessSrg(bindlessSrgLayout) == RHI::ResultCode::Success;
            }

            AZ_Assert(success, "RHISystem", "Bindless SRG was not initialized.\n");
        }

        RHI::FrameSchedulerDescriptor frameSchedulerDescriptor;

        m_drawListTagRegistry = RHI::DrawListTagRegistry::Create();
        m_pipelineStateCache = RHI::PipelineStateCache::Create(RHI::MultiDevice::AllDevices);

        m_gpuMarkersEnabled = !RHI::QueryCommandLineOption("rhi-disable-gpu-markers");

        for (int deviceIndex{0}; deviceIndex < GetDeviceCount(); ++deviceIndex)
        {
            Ptr<RHI::PlatformLimitsDescriptor> platformLimitsDescriptor = m_devices[deviceIndex]->GetDescriptor().m_platformLimitsDescriptor;

            frameSchedulerDescriptor.m_transientAttachmentPoolDescriptors[deviceIndex].m_renderTargetBudgetInBytes = platformLimitsDescriptor->m_transientAttachmentPoolBudgets.m_renderTargetBudgetInBytes;
            frameSchedulerDescriptor.m_transientAttachmentPoolDescriptors[deviceIndex].m_imageBudgetInBytes = platformLimitsDescriptor->m_transientAttachmentPoolBudgets.m_imageBudgetInBytes;
            frameSchedulerDescriptor.m_transientAttachmentPoolDescriptors[deviceIndex].m_bufferBudgetInBytes = platformLimitsDescriptor->m_transientAttachmentPoolBudgets.m_bufferBudgetInBytes;

            switch (platformLimitsDescriptor->m_heapAllocationStrategy)
            {
                case HeapAllocationStrategy::Fixed:
                {
                    frameSchedulerDescriptor.m_transientAttachmentPoolDescriptors[deviceIndex].m_heapParameters = RHI::HeapAllocationParameters();
                    break;
                }
                case  HeapAllocationStrategy::Paging:
                {
                    RHI::HeapPagingParameters heapAllocationParameters;
                    heapAllocationParameters.m_collectLatency = platformLimitsDescriptor->m_pagingParameters.m_collectLatency;
                    heapAllocationParameters.m_initialAllocationPercentage = platformLimitsDescriptor->m_pagingParameters.m_initialAllocationPercentage;
                    heapAllocationParameters.m_pageSizeInBytes = platformLimitsDescriptor->m_pagingParameters.m_pageSizeInBytes;
                    frameSchedulerDescriptor.m_transientAttachmentPoolDescriptors[deviceIndex].m_heapParameters = RHI::HeapAllocationParameters(heapAllocationParameters);
                    break;
                }
                case HeapAllocationStrategy::MemoryHint:
                {
                    RHI::HeapMemoryHintParameters heapAllocationParameters;
                    heapAllocationParameters.m_heapSizeScaleFactor = platformLimitsDescriptor->m_usageHintParameters.m_heapSizeScaleFactor;
                    heapAllocationParameters.m_collectLatency = platformLimitsDescriptor->m_usageHintParameters.m_collectLatency;
                    heapAllocationParameters.m_maxHeapWastedPercentage = platformLimitsDescriptor->m_usageHintParameters.m_maxHeapWastedPercentage;
                    heapAllocationParameters.m_minHeapSizeInBytes = platformLimitsDescriptor->m_usageHintParameters.m_minHeapSizeInBytes;
                    frameSchedulerDescriptor.m_transientAttachmentPoolDescriptors[deviceIndex].m_heapParameters = RHI::HeapAllocationParameters(heapAllocationParameters);
                    break;
                }
                default:
                {
                    AZ_Assert(false, "UnSupported type");
                    break;
                }
            }

            frameSchedulerDescriptor.m_platformLimitsDescriptors[deviceIndex] = platformLimitsDescriptor;
        }

        m_frameScheduler.Init(MultiDevice::AllDevices, frameSchedulerDescriptor);

        RHISystemNotificationBus::Broadcast(&RHISystemNotificationBus::Events::OnRHISystemInitialized);
    }

    ResultCode RHISystem::InitInternalDevices(int deviceCount)
    {
        RHI::PhysicalDeviceList physicalDevices = RHI::Factory::Get().EnumeratePhysicalDevices();

        AZ_Printf("RHISystem", "Initializing RHI...\n");

        if (physicalDevices.empty())
        {
            AZ_Printf("RHISystem", "Unable to initialize RHI! No supported physical device found.\n");
            return ResultCode::Fail;
        }

        RHI::PhysicalDeviceList usePhysicalDevices;

        if (deviceCount > 1)
        {
            AZ_Printf("RHISystem", "\tUsing multiple devices\n");

            for(auto i {0}; (i < deviceCount) && (i < static_cast<int>(AZStd::size(physicalDevices))); ++i)
            {
                usePhysicalDevices.emplace_back(physicalDevices[i]);
            }
        }
        else
        {
            AZStd::string preferredUserAdapterName = RHI::GetCommandLineValue("forceAdapter");
            AZStd::to_lower(preferredUserAdapterName.begin(), preferredUserAdapterName.end());
            bool findPreferredUserDevice = preferredUserAdapterName.size() > 0;

            RHI::PhysicalDevice* preferredUserDevice{};
            RHI::PhysicalDevice* preferredVendorDevice{};

            for (RHI::Ptr<RHI::PhysicalDevice>& physicalDevice : physicalDevices)
            {
                const RHI::PhysicalDeviceDescriptor& descriptor = physicalDevice->GetDescriptor();

                AZ_Printf("RHISystem", "\tEnumerated physical device: %s\n", descriptor.m_description.c_str());
                if (findPreferredUserDevice)
                {
                    AZStd::string descriptorLowerCase = descriptor.m_description;
                    AZStd::to_lower( descriptorLowerCase.begin(), descriptorLowerCase.end());
                    if (!preferredUserDevice && descriptorLowerCase.contains(preferredUserAdapterName))
                    {
                        preferredUserDevice = physicalDevice.get();
                    }
                }
                // Record the first nVidia or AMD device we find.
                if (!preferredVendorDevice && (descriptor.m_vendorId == RHI::VendorId::AMD || descriptor.m_vendorId == RHI::VendorId::nVidia))
                {
                    preferredVendorDevice = physicalDevice.get();
                }
            }

            AZ_Warning("RHISystem", preferredUserAdapterName.empty() || preferredUserDevice, "Specified adapter name not found: '%s'", preferredUserAdapterName.c_str());

            RHI::PhysicalDevice* physicalDeviceFound{};

            if (preferredUserDevice)
            {
                // First, prefer the user specified device if found.
                physicalDeviceFound = preferredUserDevice;
            }
            else if (preferredVendorDevice)
            {
                // Second, prefer specific vendor devices.
                physicalDeviceFound = preferredVendorDevice;
            }
            else
            {
                // Default to first device if no other preferred device is found.
                physicalDeviceFound = physicalDevices.front().get();
            }

            usePhysicalDevices.emplace_back(physicalDeviceFound);
        }

        for (RHI::Ptr<RHI::PhysicalDevice>& physicalDevice : usePhysicalDevices)
        {
            // Validate the GPU driver version.
            // Some GPU drivers have known issues and it is recommended to update or use other versions.
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            PhysicalDeviceDriverValidator physicalDriverValidator;
            if (!(settingsRegistry && settingsRegistry->GetObject(physicalDriverValidator, "/O3DE/Atom/RHI/PhysicalDeviceDriverInfo")))
            {
                AZ_Printf("RHISystem", "Failed to get settings registry for GPU driver Info.");
            }
            else
            {
                physicalDriverValidator.ValidateDriverVersion(physicalDevice->GetDescriptor());
            }

            AZ_Printf("RHISystem", "\tUsing physical device: %s\n", physicalDevice->GetDescriptor().m_description.c_str());

            RHI::Ptr<RHI::Device> device = RHI::Factory::Get().CreateDevice();
            if (device->Init(static_cast<int>(m_devices.size()), *physicalDevice) == RHI::ResultCode::Success)
            {
                m_devices.emplace_back(AZStd::move(device));
            }
        }

        for (auto index{ 0 }; m_devices.size() < deviceCount; index++)
        {
            // We do not have enough physical devices for the requested device count
            // Virtualize the existing devices up to the required number
            auto deviceIndex{ AddVirtualDevice(m_devices[index]->GetDeviceIndex()) };
            AZ_Printf("RHISystem", "\tVirtualized device %d from device %d\n", deviceIndex.value(), m_devices[index]->GetDeviceIndex());
        }

        if (m_devices.empty())
        {
            AZ_Error("RHISystem", false, "Failed to initialize RHI device.");
            return ResultCode::Fail;
        }

        // Register device GPUs attributes
        if (auto deviceRegistrar = AzFramework::DeviceAttributeRegistrar::Get())
        {
            AZStd::vector<AZStd::string_view> gpuList;
            AZStd::transform(
                m_devices.begin(),
                m_devices.end(),
                std::back_inserter(gpuList),
                [](const auto& device)
                {
                    return device->GetPhysicalDevice().GetDescriptor().m_description.c_str();
                });
            deviceRegistrar->RegisterDeviceAttribute(AZStd::make_shared<AzFramework::DeviceAttributeGPUModel>(gpuList));
        }
        return ResultCode::Success;
    }

    void RHISystem::Shutdown()
    {
        m_frameScheduler.Shutdown();
        m_pipelineStateCache = nullptr;

        while (!m_devices.empty())
        {
            m_devices.back()->PreShutdown();
            AZ_Assert(m_devices.back()->use_count()==1, "The ref count for Device is %i but it should be 1 here to ensure all the resources are released", m_devices.back()->use_count());
            m_devices.pop_back();
        }
        Interface<RHISystemInterface>::Unregister(this);
    }

    void RHISystem::FrameUpdate(FrameGraphCallback frameGraphCallback)
    {
        AZ_PROFILE_SCOPE(RHI, "RHISystem: FrameUpdate");
        {
            AZ_PROFILE_SCOPE(RHI, "main per-frame work");
            if (m_frameScheduler.BeginFrame() == ResultCode::Success)
            {
                frameGraphCallback(m_frameScheduler);

                // This exists as a hook to enable RHI sample tests, which are allowed to queue their
                // own RHI scopes to the frame scheduler. This happens prior to the RPI pass graph registration.
                {
                    AZ_PROFILE_SCOPE(RHI, "RHISystem: FrameUpdate: OnFramePrepare");
                    RHISystemNotificationBus::Broadcast(&RHISystemNotificationBus::Events::OnFramePrepare, m_frameScheduler);
                }

                RHI::MessageOutcome outcome = m_frameScheduler.Compile(m_compileRequest);
                if (outcome.IsSuccess())
                {
                    m_frameScheduler.Execute(RHI::JobPolicy::Parallel);
                }
                else
                {
                    AZ_Error("RHISystem", false, "Frame Scheduler Compilation Failure: %s", outcome.GetError().c_str());
                }

                m_pipelineStateCache->Compact();
            }
        }

        m_frameScheduler.EndFrame();
    }

    AZStd::optional<int> RHISystem::AddVirtualDevice(int deviceIndexToVirtualize)
    {
        if (deviceIndexToVirtualize >= m_devices.size())
        {
            AZ_Error("RHISystem", false, "Invalid device index, cannot add virtual device");
            return AZStd::nullopt;
        }

        RHI::Ptr<RHI::Device> device = RHI::Factory::Get().CreateDevice();
        auto virtualDeviceIndex{ static_cast<int>(m_devices.size()) };
        auto& selectedPhysicalDevice = const_cast<RHI::PhysicalDevice&>(m_devices[deviceIndexToVirtualize]->GetPhysicalDevice());
        if (device->Init(virtualDeviceIndex, selectedPhysicalDevice) == RHI::ResultCode::Success)
        {
            m_devices.emplace_back(AZStd::move(device));
            return virtualDeviceIndex;
        }
        else
        {
            AZ_Error("RHISystem", false, "Failed to initialize virtual device");
            return AZStd::nullopt;
        }
    }

    RHI::Device* RHISystem::GetDevice(int deviceIndex)
    {
        if (deviceIndex < m_devices.size())
        {
            return m_devices.at(deviceIndex).get();
        }

        return nullptr;
    }

    const RHI::Device* RHISystem::GetDevice(int deviceIndex) const
    {
        if (deviceIndex < m_devices.size())
        {
            return m_devices.at(deviceIndex).get();
        }

        return nullptr;
    }

    int RHISystem::GetDeviceCount()
    {
        return static_cast<int>(m_devices.size());
    }

    MultiDevice::DeviceMask RHISystem::GetRayTracingSupport()
    {
        MultiDevice::DeviceMask result{0};

        for (int deviceIndex{0}; deviceIndex < m_devices.size(); ++deviceIndex)
        {
            if (m_devices[deviceIndex]->GetFeatures().m_rayTracing)
            {
                result |= static_cast<MultiDevice::DeviceMask>(1 << deviceIndex);
            }
        }

        return result;
    }

    RHI::PipelineStateCache* RHISystem::GetPipelineStateCache()
    {
        return m_pipelineStateCache.get();
    }

    RHI::DrawListTagRegistry* RHISystem::GetDrawListTagRegistry()
    {
        return m_drawListTagRegistry.get();
    }

    void RHISystem::ModifyFrameSchedulerStatisticsFlags(RHI::FrameSchedulerStatisticsFlags statisticsFlags, bool enableFlags)
    {
        m_compileRequest.m_statisticsFlags =
            enableFlags
            ? RHI::SetBits(m_compileRequest.m_statisticsFlags, statisticsFlags)
            : RHI::ResetBits(m_compileRequest.m_statisticsFlags, statisticsFlags);
    }

    double RHISystem::GetCpuFrameTime() const
    {
        return m_frameScheduler.GetCpuFrameTime();
    }


    const AZStd::unordered_map<int, TransientAttachmentPoolDescriptor>* RHISystem::GetTransientAttachmentPoolDescriptor() const
    {
        return m_frameScheduler.GetTransientAttachmentPoolDescriptor();
    }

    void RHISystem::SetNumActiveRenderPipelines(uint16_t numActiveRenderPipelines)
    {
        m_numActiveRenderPipelines = numActiveRenderPipelines;
    }

    uint16_t RHISystem::GetNumActiveRenderPipelines() const
    {
        return m_numActiveRenderPipelines;
    }

    ConstPtr<PlatformLimitsDescriptor> RHISystem::GetPlatformLimitsDescriptor(int deviceIndex) const
    {
        return m_devices[deviceIndex]->GetDescriptor().m_platformLimitsDescriptor;
    }

    void RHISystem::QueueRayTracingShaderTableForBuild(DeviceRayTracingShaderTable* rayTracingShaderTable)
    {
        m_frameScheduler.QueueRayTracingShaderTableForBuild(rayTracingShaderTable);
    }

    bool RHISystem::RegisterXRSystem(XRRenderingInterface* xrRenderingInterface)
    {
        AZ_Assert(!m_xrSystem, "XR System is already registered");
        if (RHI::Factory::Get().SupportsXR())
        {
            m_xrSystem = xrRenderingInterface;
            return true;
        }
        return false;
    }

    void RHISystem::UnregisterXRSystem()
    {
        AZ_Assert(m_xrSystem, "XR System is already null");
        m_xrSystem = nullptr;
    }

    RHI::XRRenderingInterface* RHISystem::GetXRSystem() const
    {
        return m_xrSystem;
    }

    void RHISystem::SetDrawListTagEnabledByDefault(DrawListTag drawListTag, bool enabled)
    {
        if (enabled)
        {
            AZStd::remove(m_drawListTagsDisabledByDefault.begin(),
                          m_drawListTagsDisabledByDefault.end(),
                          drawListTag);
        }
        else
        {
            m_drawListTagsDisabledByDefault.push_back(drawListTag);
        }
    }

    const AZStd::vector<DrawListTag>& RHISystem::GetDrawListTagsDisabledByDefault() const
    {
        return m_drawListTagsDisabledByDefault;
    }

    bool RHISystem::GpuMarkersEnabled() const
    {
        return m_gpuMarkersEnabled;
    }

    bool RHISystem::CanMergeSubpasses() const
    {
        const auto devicePtr = GetDevice();
        return devicePtr && (devicePtr->GetFeatures().m_subpassInputSupport != AZ::RHI::SubpassInputSupportType::None );
    }

    /////////////////////////////////////////////////////////////////////////////
    // RHIMemoryStatisticsInterface overrides
    AZStd::unordered_map<int, TransientAttachmentStatistics> RHISystem::GetTransientAttachmentStatistics() const
    {
        return m_frameScheduler.GetTransientAttachmentStatistics();
    }

    const RHI::MemoryStatistics* RHISystem::GetMemoryStatistics() const
    {
        return m_frameScheduler.GetMemoryStatistics();
    }
        
    void RHISystem::WriteResourcePoolInfoToJson(
        const AZStd::vector<RHI::MemoryStatistics::Pool>& pools, 
        rapidjson::Document& doc) const
    {
        AZ::RHI::WritePoolsToJson(pools, doc);
    }

    AZ::Outcome<void, AZStd::string> RHISystem::LoadResourcePoolInfoFromJson(
        AZStd::vector<RHI::MemoryStatistics::Pool>& pools, 
        AZStd::vector<RHI::MemoryStatistics::Heap>& heaps, 
        rapidjson::Document& doc, 
        const AZStd::string& fileName) const
    {
        return AZ::RHI::LoadPoolsFromJson(pools, heaps, doc, fileName);
    }

    void RHISystem::TriggerResourcePoolAllocInfoDump() const
    {
        AZ::RHI::DumpPoolInfoToJson();
    }
    /////////////////////////////////////////////////////////////////////////////
}
