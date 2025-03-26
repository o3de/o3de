/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <Atom/RHI/Device.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI.Reflect/Limits.h>

namespace UnitTest
{
    static constexpr auto DeviceCount{8};
    static constexpr AZ::RHI::MultiDevice::DeviceMask DeviceMask{AZ::RHI::MultiDevice::AllDevices};

    class PhysicalDevice
        : public AZ::RHI::PhysicalDevice
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator);

        static AZ::RHI::PhysicalDeviceList Enumerate();

    private:
        PhysicalDevice();
    };

    class Device
        : public AZ::RHI::Device
    {
    public:
        AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator);

        Device();

    private:

        AZ::RHI::ResultCode InitInternal(AZ::RHI::PhysicalDevice&) override { return AZ::RHI::ResultCode::Success; }
        AZ::RHI::ResultCode InitInternalBindlessSrg([[maybe_unused]] const AZ::RHI::BindlessSrgDescriptor& bindlessSrgDesc) override { return AZ::RHI::ResultCode::Success;}

        void ShutdownInternal() override {}

        AZ::RHI::ResultCode BeginFrameInternal() override { return AZ::RHI::ResultCode::Success; }

        void EndFrameInternal() override {}

        void WaitForIdleInternal() override {}

        void CompileMemoryStatisticsInternal(AZ::RHI::MemoryStatisticsBuilder&) override {}

        void UpdateCpuTimingStatisticsInternal() const override {}

        AZStd::chrono::microseconds GpuTimestampToMicroseconds([[maybe_unused]] uint64_t gpuTimestamp, [[maybe_unused]] AZ::RHI::HardwareQueueClass queueClass) const override
        {
            return AZStd::chrono::microseconds();
        }

        AZStd::pair<uint64_t, uint64_t> GetCalibratedTimestamp([[maybe_unused]] AZ::RHI::HardwareQueueClass queueClass) override
        {
            return { 0ull, AZStd::chrono::microseconds().count() };
        }

        void FillFormatsCapabilitiesInternal([[maybe_unused]] FormatCapabilitiesList& formatsCapabilities) override {}

        AZ::RHI::ResultCode InitializeLimits() override { return AZ::RHI::ResultCode::Success; }

        void PreShutdown() override {}

        AZ::RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const AZ::RHI::ImageDescriptor& descriptor) { return AZ::RHI::ResourceMemoryRequirements{}; };

        AZ::RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const AZ::RHI::BufferDescriptor& descriptor) { return AZ::RHI::ResourceMemoryRequirements{}; };

        void ObjectCollectionNotify(AZ::RHI::ObjectCollectorNotifyFunction notifyFunction) override {}

        AZ::RHI::ShadingRateImageValue ConvertShadingRate([[maybe_unused]] AZ::RHI::ShadingRate rate) const override
        {
            return AZ::RHI::ShadingRateImageValue{};
        }
    };

    AZ::RHI::Ptr<AZ::RHI::Device> MakeTestDevice();
}
