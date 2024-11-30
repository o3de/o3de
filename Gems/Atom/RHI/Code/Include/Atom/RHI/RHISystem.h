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
#include <Atom/RHI/RHIMemoryStatisticsInterface.h>
#include <Atom/RHI/XRRenderingInterface.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    class Scene;

    //! Initialization flags passed to RHISystem::InitDevices to signal if all available devices should be initialized 
    //! or just one (potentially preferred) adapter
    enum class InitDevicesFlags : uint32_t
    {
        Device = 0,
        MultiDevice
    };

    class RHISystem final
        : public RHISystemInterface
        , public RHIMemoryStatisticsInterface
    {
    public:
        //! This function just initializes the native devices and RHI::Device as a result.
        //! We can use these devices to then query for device capabilities.
        ResultCode InitDevices(int deviceCount = 1);

        //! This function initializes the rest of the RHI/RHI backend.
        //! bindlessSrgLayout in this case is layout associated with the bindless srg (Bindless.azsli).
        void Init(RHI::Ptr<RHI::ShaderResourceGroupLayout> bindlessSrgLayout = nullptr);
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
        uint16_t GetNumActiveRenderPipelines() const override;

        //////////////////////////////////////////////////////////////////////////
        // RHISystemInterface Overrides
        RHI::Device* GetDevice(int deviceIndex = MultiDevice::DefaultDeviceIndex) override;
        const RHI::Device* GetDevice(int deviceIndex = MultiDevice::DefaultDeviceIndex) const override;
        //! Add a new virtual device (referencing the same physical device as an existing device marked by deviceIndexToVirtualize)
        [[nodiscard]] AZStd::optional<int> AddVirtualDevice(int deviceIndexToVirtualize = MultiDevice::DefaultDeviceIndex) override;
        int GetDeviceCount() override;
        MultiDevice::DeviceMask GetRayTracingSupport() override;
        RHI::DrawListTagRegistry* GetDrawListTagRegistry() override;
        RHI::PipelineStateCache* GetPipelineStateCache() override;
        void ModifyFrameSchedulerStatisticsFlags(RHI::FrameSchedulerStatisticsFlags statisticsFlags, bool enableFlags) override;
        double GetCpuFrameTime() const override;
        const AZStd::unordered_map<int, TransientAttachmentPoolDescriptor>* GetTransientAttachmentPoolDescriptor() const override;
        ConstPtr<PlatformLimitsDescriptor> GetPlatformLimitsDescriptor(int deviceIndex = MultiDevice::DefaultDeviceIndex) const override;
        void QueueRayTracingShaderTableForBuild(DeviceRayTracingShaderTable* rayTracingShaderTable) override;
        XRRenderingInterface* GetXRSystem() const override;
        void SetDrawListTagEnabledByDefault(DrawListTag drawListTag, bool enabled) override;
        const AZStd::vector<DrawListTag>& GetDrawListTagsDisabledByDefault() const override;
        bool GpuMarkersEnabled() const override;
        bool CanMergeSubpasses() const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RHIMemoryStatisticsInterface Overrides
        AZStd::unordered_map<int, TransientAttachmentStatistics> GetTransientAttachmentStatistics() const override;
        const RHI::MemoryStatistics* GetMemoryStatistics() const override;
        void WriteResourcePoolInfoToJson(
            const AZStd::vector<RHI::MemoryStatistics::Pool>& pools,
            rapidjson::Document& doc) const override;
        AZ::Outcome<void, AZStd::string> LoadResourcePoolInfoFromJson(
            AZStd::vector<RHI::MemoryStatistics::Pool>& pools,
            AZStd::vector<RHI::MemoryStatistics::Heap>& heaps,
            rapidjson::Document& doc,
            const AZStd::string& fileName) const override;
        void TriggerResourcePoolAllocInfoDump() const override;
        //////////////////////////////////////////////////////////////////////////

    private:

        //! Enumerates the Physical devices and picks one (or multiple) to be used to initialize the RHI::Device(s) with
        ResultCode InitInternalDevices(int deviceCount);

        AZStd::vector<DrawListTag> m_drawListTagsDisabledByDefault;
        AZStd::vector<RHI::Ptr<RHI::Device>> m_devices;
        RHI::FrameScheduler m_frameScheduler;
        RHI::FrameSchedulerCompileRequest m_compileRequest;
        RHI::Ptr<RHI::DrawListTagRegistry> m_drawListTagRegistry;
        RHI::Ptr<RHI::PipelineStateCache> m_pipelineStateCache;
        XRRenderingInterface* m_xrSystem = nullptr;

        //Used for better verbosity related to gpu markers
        uint16_t m_numActiveRenderPipelines = 0;
        bool m_gpuMarkersEnabled = true;
    };
}
