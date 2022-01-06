
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Device.h>

namespace AZ
{
    namespace Null
    {
        class Device final
            : public RHI::Device
        {
            using Base = RHI::Device;
        public:
            AZ_CLASS_ALLOCATOR(Device, AZ::SystemAllocator, 0);
            AZ_RTTI(Device, "{F2AAE1EA-6B35-4870-9C73-8CD7EDC149A8}", Base);

            static RHI::Ptr<Device> Create();

        private:
            Device();
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::Device
            RHI::ResultCode InitInternal([[maybe_unused]] RHI::PhysicalDevice& physicalDevice) override { return RHI::ResultCode::Success; }
            void ShutdownInternal() override {}
            void CompileMemoryStatisticsInternal([[maybe_unused]] RHI::MemoryStatisticsBuilder& builder) override {}
            void UpdateCpuTimingStatisticsInternal() const override {}
            void BeginFrameInternal() override {}
            void EndFrameInternal() override {}
            void WaitForIdleInternal() override {}
            AZStd::chrono::microseconds GpuTimestampToMicroseconds([[maybe_unused]] uint64_t gpuTimestamp, [[maybe_unused]] RHI::HardwareQueueClass queueClass) const override { return AZStd::chrono::microseconds();}
            void FillFormatsCapabilitiesInternal([[maybe_unused]] FormatCapabilitiesList& formatsCapabilities) override;
            RHI::ResultCode InitializeLimits() override { return RHI::ResultCode::Success; }
            void PreShutdown() override {}
            RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const RHI::ImageDescriptor& descriptor) override { return RHI::ResourceMemoryRequirements();}
            RHI::ResourceMemoryRequirements GetResourceMemoryRequirements([[maybe_unused]] const RHI::BufferDescriptor& descriptor) override { return RHI::ResourceMemoryRequirements();}
            void ObjectCollectionNotify(RHI::ObjectCollectorNotifyFunction notifyFunction) override;
            //////////////////////////////////////////////////////////////////////////
        };
    }
}


