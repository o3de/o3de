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
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/std/string/conversions.h>

AZ_DEFINE_BUDGET(RHI);

namespace AZ
{
    namespace RHI
    {
        RHISystemInterface* RHISystemInterface::Get()
        {
            return Interface<RHISystemInterface>::Get();
        }

        void RHISystem::InitDevice()
        {
            Interface<RHISystemInterface>::Register(this);
            m_device = InitInternalDevice();
        }
    
        void RHISystem::Init()
        {
            Ptr<RHI::PlatformLimitsDescriptor> platformLimitsDescriptor = m_device->GetDescriptor().m_platformLimitsDescriptor;

            RHI::FrameSchedulerDescriptor frameSchedulerDescriptor;

            m_drawListTagRegistry = RHI::DrawListTagRegistry::Create();
            m_pipelineStateCache = RHI::PipelineStateCache::Create(*m_device);

            frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_renderTargetBudgetInBytes = platformLimitsDescriptor->m_transientAttachmentPoolBudgets.m_renderTargetBudgetInBytes;
            frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_imageBudgetInBytes = platformLimitsDescriptor->m_transientAttachmentPoolBudgets.m_imageBudgetInBytes;
            frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_bufferBudgetInBytes = platformLimitsDescriptor->m_transientAttachmentPoolBudgets.m_bufferBudgetInBytes;

            switch (platformLimitsDescriptor->m_heapAllocationStrategy)
            {
                case HeapAllocationStrategy::Fixed:
                {
                    frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_heapParameters = RHI::HeapAllocationParameters();
                    break;
                }
                case  HeapAllocationStrategy::Paging:
                {
                    RHI::HeapPagingParameters heapAllocationParameters;
                    heapAllocationParameters.m_collectLatency = platformLimitsDescriptor->m_pagingParameters.m_collectLatency;
                    heapAllocationParameters.m_initialAllocationPercentage = platformLimitsDescriptor->m_pagingParameters.m_initialAllocationPercentage;
                    heapAllocationParameters.m_pageSizeInBytes = platformLimitsDescriptor->m_pagingParameters.m_pageSizeInBytes;
                    frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_heapParameters = RHI::HeapAllocationParameters(heapAllocationParameters);
                    break;
                }
                case HeapAllocationStrategy::MemoryHint:
                {
                    RHI::HeapMemoryHintParameters heapAllocationParameters;
                    heapAllocationParameters.m_heapSizeScaleFactor = platformLimitsDescriptor->m_usageHintParameters.m_heapSizeScaleFactor;
                    heapAllocationParameters.m_collectLatency = platformLimitsDescriptor->m_usageHintParameters.m_collectLatency;
                    heapAllocationParameters.m_maxHeapWastedPercentage = platformLimitsDescriptor->m_usageHintParameters.m_maxHeapWastedPercentage;
                    heapAllocationParameters.m_minHeapSizeInBytes = platformLimitsDescriptor->m_usageHintParameters.m_minHeapSizeInBytes;
                    frameSchedulerDescriptor.m_transientAttachmentPoolDescriptor.m_heapParameters = RHI::HeapAllocationParameters(heapAllocationParameters);
                    break;
                }
                default:
                {
                    AZ_Assert(false, "UnSupported type");
                    break;
                }
            }
                
            frameSchedulerDescriptor.m_platformLimitsDescriptor = platformLimitsDescriptor;
            m_frameScheduler.Init(*m_device, frameSchedulerDescriptor);
        }

        RHI::Ptr<RHI::Device> RHISystem::InitInternalDevice()
        {
            RHI::PhysicalDeviceList physicalDevices = RHI::Factory::Get().EnumeratePhysicalDevices();

            AZ_Printf("RHISystem", "Initializing RHI...\n");

            if (physicalDevices.empty())
            {
                AZ_Printf("RHISystem", "Unable to initialize RHI! No supported physical device found.\n");
                return nullptr;
            }

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
                physicalDriverValidator.ValidateDriverVersion(physicalDeviceFound->GetDescriptor());
            }

            AZ_Printf("RHISystem", "\tUsing physical device: %s\n", physicalDeviceFound->GetDescriptor().m_description.c_str());

            RHI::Ptr<RHI::Device> device = RHI::Factory::Get().CreateDevice();
            if (device->Init(*physicalDeviceFound) == RHI::ResultCode::Success)
            {
                PlatformLimitsDescriptor::Create();
                return device;
            }

            AZ_Error("RHISystem", false, "Failed to initialize RHI device.");
            return nullptr;
        }

        void RHISystem::Shutdown()
        {
            Interface<RHISystemInterface>::Unregister(this);
            m_frameScheduler.Shutdown();

            m_pipelineStateCache = nullptr;
            if (m_device)
            {
                m_device->PreShutdown();
                AZ_Assert(m_device->use_count()==1, "The ref count for Device is %i but it should be 1 here to ensure all the resources are released", m_device->use_count());
                m_device = nullptr;
            }
        }

        void RHISystem::FrameUpdate(FrameGraphCallback frameGraphCallback)
        {
            AZ_PROFILE_SCOPE(RHI, "RHISystem: FrameUpdate");

            {
                AZ_PROFILE_SCOPE(RHI, "main per-frame work");
                m_frameScheduler.BeginFrame();

                frameGraphCallback(m_frameScheduler);

                /**
                 * This exists as a hook to enable RHI sample tests, which are allowed to queue their
                 * own RHI scopes to the frame scheduler. This happens prior to the RPI pass graph registration.
                 */
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

            m_frameScheduler.EndFrame();
        }

        RHI::Device* RHISystem::GetDevice()
        {
            return m_device.get();
        }

        RHI::PipelineStateCache* RHISystem::GetPipelineStateCache()
        {
            return m_pipelineStateCache.get();
        }

        RHI::DrawListTagRegistry* RHISystem::GetDrawListTagRegistry()
        {
            return m_drawListTagRegistry.get();
        }

        const RHI::FrameSchedulerCompileRequest& RHISystem::GetFrameSchedulerCompileRequest() const
        {
            return m_compileRequest;
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

        const RHI::TransientAttachmentStatistics* RHISystem::GetTransientAttachmentStatistics() const
        {
            return m_frameScheduler.GetTransientAttachmentStatistics();
        }

        const RHI::MemoryStatistics* RHISystem::GetMemoryStatistics() const
        {
            return m_frameScheduler.GetMemoryStatistics();
        }

        const AZ::RHI::TransientAttachmentPoolDescriptor* RHISystem::GetTransientAttachmentPoolDescriptor() const
        {
            return m_frameScheduler.GetTransientAttachmentPoolDescriptor();
        }

        ConstPtr<PlatformLimitsDescriptor> RHISystem::GetPlatformLimitsDescriptor() const
        {
            return m_device->GetDescriptor().m_platformLimitsDescriptor;
        }

        void RHISystem::QueueRayTracingShaderTableForBuild(RayTracingShaderTable* rayTracingShaderTable)
        {
            m_frameScheduler.QueueRayTracingShaderTableForBuild(rayTracingShaderTable);
        }
    } //namespace RPI
} //namespace AZ
