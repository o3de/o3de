/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/FrameSchedulerEnums.h>
#include <Atom/RHI.Reflect/MemoryStatistics.h>
#include <Atom/RHI/Device.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/XRRenderingInterface.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Name/Name.h>

AZ_DECLARE_BUDGET(RHI);

namespace AZ
{
    namespace RHI
    {
        class FrameGraphBuilder;
        class PipelineStateCache;
        class PlatformLimitsDescriptor;
        class PhysicalDeviceDescriptor;
        class DeviceRayTracingShaderTable;
        struct FrameSchedulerCompileRequest;
        struct TransientAttachmentStatistics;
        struct TransientAttachmentPoolDescriptor;

        class RHISystemInterface
        {
        public:
            AZ_RTTI(RHISystemInterface, "{B70BB184-D7D5-4C15-9C82-C9459F552F13}");

            static RHISystemInterface* Get();

            RHISystemInterface() = default;
            virtual ~RHISystemInterface() = default;

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(RHISystemInterface);

            virtual RHI::Device* GetDevice(size_t index = 0) = 0;

            virtual int GetDeviceCount() = 0;

            virtual RHI::DrawListTagRegistry* GetDrawListTagRegistry() = 0;

            virtual RHI::PipelineStateCache* GetPipelineStateCache() = 0;

            virtual void ModifyFrameSchedulerStatisticsFlags(RHI::FrameSchedulerStatisticsFlags statisticsFlags, bool enableFlags) = 0;

            virtual double GetCpuFrameTime() const = 0;

            virtual const RHI::TransientAttachmentStatistics* GetTransientAttachmentStatistics() const = 0;

            virtual const RHI::MemoryStatistics* GetMemoryStatistics() const = 0;

            virtual const RHI::TransientAttachmentPoolDescriptor* GetTransientAttachmentPoolDescriptor() const = 0;

            virtual ConstPtr<PlatformLimitsDescriptor> GetPlatformLimitsDescriptor() const = 0;

            virtual void QueueRayTracingShaderTableForBuild(DeviceRayTracingShaderTable* rayTracingShaderTable) = 0;

            virtual XRRenderingInterface* GetXRSystem() const = 0;
        };

        //! This bus exists to give RHI samples the ability to slot in scopes manually
        //! before anything else is processed.
        class RHISystemNotificiationInterface
            : public AZ::EBusTraits
        {
        public:
            virtual void OnFramePrepare(RHI::FrameGraphBuilder& ) {};
            
            //! Notify that the input device was removed
            virtual void OnDeviceRemoved(Device* ) {};
        };

        using RHISystemNotificationBus = AZ::EBus<RHISystemNotificiationInterface>;
    } // namespace RHI
}
