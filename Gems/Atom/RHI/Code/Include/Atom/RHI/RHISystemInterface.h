/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Budget.h>
#include <AzCore/Name/Name.h>
#include <AzCore/EBus/EBus.h>
#include <Atom/RHI.Reflect/FrameSchedulerEnums.h>
#include <Atom/RHI.Reflect/MemoryStatistics.h>
#include <Atom/RHI/DrawListTagRegistry.h>

AZ_DECLARE_BUDGET(RHI);

namespace AZ
{
    namespace RHI
    {
        class Device;
        class FrameGraphBuilder;
        class PipelineState;
        class PipelineStateCache;
        class PlatformLimitsDescriptor;
        class RayTracingShaderTable;
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

            virtual RHI::Device* GetDevice() = 0;

            virtual RHI::DrawListTagRegistry* GetDrawListTagRegistry() = 0;

            virtual RHI::PipelineStateCache* GetPipelineStateCache() = 0;

            virtual const RHI::FrameSchedulerCompileRequest& GetFrameSchedulerCompileRequest() const = 0;

            virtual void ModifyFrameSchedulerStatisticsFlags(RHI::FrameSchedulerStatisticsFlags statisticsFlags, bool enableFlags) = 0;

            virtual double GetCpuFrameTime() const = 0;

            virtual const RHI::TransientAttachmentStatistics* GetTransientAttachmentStatistics() const = 0;

            virtual const RHI::MemoryStatistics* GetMemoryStatistics() const = 0;

            virtual const RHI::TransientAttachmentPoolDescriptor* GetTransientAttachmentPoolDescriptor() const = 0;

            virtual ConstPtr<PlatformLimitsDescriptor> GetPlatformLimitsDescriptor() const = 0;

            virtual void QueueRayTracingShaderTableForBuild(RayTracingShaderTable* rayTracingShaderTable) = 0;
        };

        //! This bus exists to give RHI samples the ability to slot in scopes manually
        //! before anything else is processed.
        class RHISystemNotificiationInterface
            : public AZ::EBusTraits
        {
        public:
            virtual void OnFramePrepare(RHI::FrameGraphBuilder& frameGraphBuilder) = 0;
        };

        using RHISystemNotificationBus = AZ::EBus<RHISystemNotificiationInterface>;
    }
}
