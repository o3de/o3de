/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/Device.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/PipelineStateCache.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    class ReflectContext;

    namespace RHI
    {
        class Scene;

        class RHISystem final
            : public RHISystemInterface
        {
        public:
            //! This function just initializes the native device and RHI::Device as a result.
            //! We can use this device to then query for device capabilities.
            void InitDevice();

            //! This function initializes the rest of the RHI/RHI backend. 
            void Init();
            void Shutdown();

            //! An external callback to build the frame graph.
            using FrameGraphCallback = AZStd::function<void(RHI::FrameGraphBuilder&)>;

            //! Invokes the frame scheduler. The provided callback is invoked prior to compilation of the graph.
            void FrameUpdate(FrameGraphCallback frameGraphCallback);

            //////////////////////////////////////////////////////////////////////////
            // RHISystemInterface Overrides
            RHI::Device* GetDevice() override;
            RHI::DrawListTagRegistry* GetDrawListTagRegistry() override;
            RHI::PipelineStateCache* GetPipelineStateCache() override;
            const RHI::FrameSchedulerCompileRequest& GetFrameSchedulerCompileRequest() const override;
            void ModifyFrameSchedulerStatisticsFlags(RHI::FrameSchedulerStatisticsFlags statisticsFlags, bool enableFlags) override;
            double GetCpuFrameTime() const override;
            const RHI::TransientAttachmentStatistics* GetTransientAttachmentStatistics() const override;
            const RHI::MemoryStatistics* GetMemoryStatistics() const override;
            const RHI::TransientAttachmentPoolDescriptor* GetTransientAttachmentPoolDescriptor() const override;
            ConstPtr<PlatformLimitsDescriptor> GetPlatformLimitsDescriptor() const override;
            void QueueRayTracingShaderTableForBuild(RayTracingShaderTable* rayTracingShaderTable) override;
            //////////////////////////////////////////////////////////////////////////

        private:

            //Enumerates the Physical devices and picks one to be used to initialize the RHI::Device with
            RHI::Ptr<RHI::Device> InitInternalDevice();

            RHI::Ptr<RHI::Device> m_device;
            RHI::Ptr<RHI::DrawListTagRegistry> m_drawListTagRegistry;
            RHI::Ptr<RHI::PipelineStateCache> m_pipelineStateCache;
            RHI::FrameScheduler m_frameScheduler;
            RHI::FrameSchedulerCompileRequest m_compileRequest;
        };
    } // namespace RPI
} // namespace AZ
