/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    private:

        AZ::RHI::ResultCode InitInternal(AZ::RHI::PhysicalDevice&) override { return AZ::RHI::ResultCode::Success; }
        AZ::RHI::ResultCode PostInitInternal(const AZ::RHI::DeviceDescriptor&) override { return AZ::RHI::ResultCode::Success; }

        void ShutdownInternal() override {}

        void BeginFrameInternal() override {}

        void EndFrameInternal() override {}

        void WaitForIdleInternal() override {}

        void CompileMemoryStatisticsInternal(AZ::RHI::MemoryStatisticsBuilder&) override {}

        void UpdateCpuTimingStatisticsInternal([[maybe_unused]] AZ::RHI::CpuTimingStatistics& cpuTimingStatistics) const override {}

        AZStd::chrono::microseconds GpuTimestampToMicroseconds([[maybe_unused]] uint64_t gpuTimestamp, [[maybe_unused]] AZ::RHI::HardwareQueueClass queueClass) const override
        {
            return AZStd::chrono::microseconds();
        }

        void FillFormatsCapabilitiesInternal([[maybe_unused]] FormatCapabilitiesList& formatsCapabilities) override {}
        
        void PreShutdown() override {}

        AZ::RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const AZ::RHI::ImageDescriptor& descriptor) { return AZ::RHI::ResourceMemoryRequirements{}; };

        AZ::RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const AZ::RHI::BufferDescriptor& descriptor) { return AZ::RHI::ResourceMemoryRequirements{}; };
    };

    AZ::RHI::Ptr<AZ::RHI::Device> MakeTestDevice();
}
