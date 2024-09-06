
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/CommandQueueContext.h>
#include <Atom/RHI/Device.h>

namespace AZ::WebGPU
{
    class CommandList;

    class Device final
        : public RHI::Device
    {
        using Base = RHI::Device;
    public:
        AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator);
        AZ_RTTI(Device, "{32DEC5E3-493C-4645-8270-8090350279EB}", Base);

        static RHI::Ptr<Device> Create();
        const wgpu::Device& GetNativeDevice() const;
        wgpu::Device& GetNativeDevice();

        CommandList* AcquireCommandList(RHI::HardwareQueueClass hardwareQueueClass);
        CommandQueueContext& GetCommandQueueContext();

    private:
        Device();
            
        //////////////////////////////////////////////////////////////////////////
        // RHI::Device
        RHI::ResultCode InitInternal(RHI::PhysicalDevice& physicalDevice) override;
        RHI::ResultCode InitInternalBindlessSrg([[maybe_unused]] const RHI::BindlessSrgDescriptor& bindlessSrgDesc) override { return RHI::ResultCode::Success;}
        void ShutdownInternal() override {}
        void CompileMemoryStatisticsInternal([[maybe_unused]] RHI::MemoryStatisticsBuilder& builder) override {}
        void UpdateCpuTimingStatisticsInternal() const override {}
        RHI::ResultCode BeginFrameInternal() override;
        void EndFrameInternal() override;
        void WaitForIdleInternal() override {}
        AZStd::chrono::microseconds GpuTimestampToMicroseconds([[maybe_unused]] uint64_t gpuTimestamp, [[maybe_unused]] RHI::HardwareQueueClass queueClass) const override { return AZStd::chrono::microseconds();}
        void FillFormatsCapabilitiesInternal([[maybe_unused]] FormatCapabilitiesList& formatsCapabilities) override;
        RHI::ResultCode InitializeLimits() override;
        void PreShutdown() override;
        RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const RHI::ImageDescriptor& descriptor) override { return RHI::ResourceMemoryRequirements();}
        RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const RHI::BufferDescriptor& descriptor) override { return RHI::ResourceMemoryRequirements();}
        void ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction) override;
        RHI::ShadingRateImageValue ConvertShadingRate(RHI::ShadingRate rate) const override;
        //////////////////////////////////////////////////////////////////////////

        static void DeviceLostCallback(const wgpu::Device&, wgpu::DeviceLostReason reason, const char* message);
        static void ErrorCallback(const wgpu::Device& , wgpu::ErrorType type, const char* message);

        wgpu::DeviceDescriptor m_wgpuDeviceDesc = {};
        wgpu::Device m_wgpuDevice = nullptr;

        CommandQueueContext m_commandQueueContext;
        AZStd::vector<RHI::Ptr<CommandList>> m_frameCommandLists;
    };
}


