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
#include <Atom/RHI/XRRenderingInterface.h>

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
            ResultCode InitDevice();

            //! This function initializes the rest of the RHI/RHI backend. 
            void Init();
            void Shutdown();

            //! An external callback to build the frame graph.
            using FrameGraphCallback = AZStd::function<void(RHI::FrameGraphBuilder&)>;

            //! Invokes the frame scheduler. The provided callback is invoked prior to compilation of the graph.
            void FrameUpdate(FrameGraphCallback frameGraphCallback);

            //! Register/Unregister xr system
            bool RegisterXRSystem(XRRenderingInterface* xrRenderingInterface);
            void UnregisterXRSystem();

            //! Get/Set functions for the number of active pipelines in use in a frame 
            void SetNumActiveRenderPipelines(uint16_t numActiveRenderPipelines);
            uint16_t GetNumActiveRenderPipelines() const;

            //////////////////////////////////////////////////////////////////////////
            // RHISystemInterface Overrides
            RHI::Device* GetDevice(int deviceIndex = MultiDevice::DefaultDeviceIndex) override;
            int GetDeviceCount() override;
            RHI::DrawListTagRegistry* GetDrawListTagRegistry() override;
            RHI::PipelineStateCache* GetPipelineStateCache() override;
            void ModifyFrameSchedulerStatisticsFlags(RHI::FrameSchedulerStatisticsFlags statisticsFlags, bool enableFlags) override;
            double GetCpuFrameTime() const override;
            const RHI::TransientAttachmentStatistics* GetTransientAttachmentStatistics() const override;
            const RHI::MemoryStatistics* GetMemoryStatistics() const override;
            const RHI::TransientAttachmentPoolDescriptor* GetTransientAttachmentPoolDescriptor() const override;
            ConstPtr<PlatformLimitsDescriptor> GetPlatformLimitsDescriptor(int deviceIndex = MultiDevice::DefaultDeviceIndex) const override;
            void QueueRayTracingShaderTableForBuild(RayTracingShaderTable* rayTracingShaderTable) override;
            XRRenderingInterface* GetXRSystem() const override;
            //////////////////////////////////////////////////////////////////////////

        private:

            //Enumerates the Physical devices and picks one to be used to initialize the RHI::Device with
            ResultCode InitInternalDevices();

            AZStd::vector<RHI::Ptr<RHI::Device>> m_devices;
            RHI::FrameScheduler m_frameScheduler;
            RHI::FrameSchedulerCompileRequest m_compileRequest;
            RHI::Ptr<RHI::DrawListTagRegistry> m_drawListTagRegistry;
            RHI::Ptr<RHI::PipelineStateCache> m_pipelineStateCache;
            XRRenderingInterface* m_xrSystem = nullptr;

            //Used for better verbosity related to gpu markers
            uint16_t m_numActiveRenderPipelines = 0;
        };
    } // namespace RPI
} // namespace AZ
