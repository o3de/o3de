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

namespace UnitTest
{
    class PhysicalDevice
        : public AZ::RHI::PhysicalDevice
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysicalDevice, AZ::SystemAllocator, 0);

        static AZ::RHI::PhysicalDeviceList Enumerate();

    private:
        PhysicalDevice();
    };

    class Device
        : public AZ::RHI::Device
    {
    public:
        AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator, 0);

        Device();

    private:

        AZ::RHI::ResultCode InitInternal(AZ::RHI::PhysicalDevice&) override { return AZ::RHI::ResultCode::Success; }

        void ShutdownInternal() override {}

        void BeginFrameInternal() override {}

        void EndFrameInternal() override {}

        void WaitForIdleInternal() override {}

        void CompileMemoryStatisticsInternal(AZ::RHI::MemoryStatisticsBuilder&) override {}

        void UpdateCpuTimingStatisticsInternal() const override {}

        AZStd::chrono::microseconds GpuTimestampToMicroseconds([[maybe_unused]] uint64_t gpuTimestamp, [[maybe_unused]] AZ::RHI::HardwareQueueClass queueClass) const override
        {
            return AZStd::chrono::microseconds();
        }

        void FillFormatsCapabilitiesInternal([[maybe_unused]] FormatCapabilitiesList& formatsCapabilities) override {}

        AZ::RHI::ResultCode InitializeLimits() override { return AZ::RHI::ResultCode::Success; }

        void PreShutdown() override {}

        AZ::RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const AZ::RHI::ImageDescriptor& descriptor) { return AZ::RHI::ResourceMemoryRequirements{}; };

        AZ::RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const AZ::RHI::BufferDescriptor& descriptor) { return AZ::RHI::ResourceMemoryRequirements{}; };

        void ObjectCollectionNotify(AZ::RHI::ObjectCollectorNotifyFunction notifyFunction) override {}
    };

    AZ::RHI::Ptr<AZ::RHI::Device> MakeTestDevice();
}
