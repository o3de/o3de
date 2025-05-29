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
#include <Atom/RHI/FrameGraphBuilder.h>
#include <Atom/RHI/FrameGraphExecuter.h>
#include <Atom/RHI/FrameGraphCompiler.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/DeviceRayTracingShaderTable.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI/ScopeProducerEmpty.h>
#include <Atom/RHI/TransientAttachmentPool.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class Job;
    class TaskGraphActiveInterface;
}

namespace AZ::RHI
{
    class DeviceShaderResourceGroupPool;
    class FrameGraphExecuteGroup;

    //! @brief Fill this descriptor when initializing a FrameScheduler instance.
    struct FrameSchedulerDescriptor
    {
        // The descriptor used to initialize the transient attachment pool.
        AZStd::unordered_map<int, TransientAttachmentPoolDescriptor> m_transientAttachmentPoolDescriptors;

        // Platform specific limits
        AZStd::unordered_map<int, ConstPtr<PlatformLimitsDescriptor>> m_platformLimitsDescriptors;
    };

    //! @brief Fill and provide this request structure when invoking FrameScheduler::Compile.
    struct FrameSchedulerCompileRequest
    {
        /// Specifies the debug log verbosity for the compile phase.
        FrameSchedulerLogVerbosity m_logVerbosity = FrameSchedulerLogVerbosity::None;

        /// Specifies a set of flags for the compile phase.
        FrameSchedulerCompileFlags m_compileFlags = FrameSchedulerCompileFlags::None;

        /// Controls which statistics are gathered over the course of the frame.
        FrameSchedulerStatisticsFlags m_statisticsFlags = FrameSchedulerStatisticsFlags::None;

        /// Controls whether the phase is allowed to use jobs.
        JobPolicy m_jobPolicy = JobPolicy::Parallel;

        /// Controls the number of ShaderResourceGroups compiled per job.
        uint32_t m_shaderResourceGroupCompilesPerJob = 256;
    };

    //! == Overview ==
    //!
    //! The frame scheduler is a system for facilitating efficient GPU work submission. It provides a
    //! user-facing API for preparing (constructing), compiling, and executing a frame graph. The graph
    //! provides knowledge of the whole frame and is processed through phases down to platform-specific
    //! actions. Because the graph is known up front, hazard tracking, memory aliasing, and cross-queue
    //! synchronization become much simpler problems. The frame becomes fully deterministic.
    //!
    //! The graph is constructed from ScopeProducers--user overridden classes which declare information
    //! to the graph. ScopeProducers own and maintain a Scope, which contains the generated graph node data.
    //! ScopeProducer is overridden by the end-user (feature author), while Scope is overridden by
    //! the internal platform implementation. Effectively, scopes contain private data, while ScopeProducers are
    //! public producers of that data.
    //!
    //! In addition to scopes, the frame graph supports attachments. An attachment is effectively some
    //! metadata around a buffer / image resource which tracks its usage across all scopes in a frame.
    //! This usage is vital for controlling low-level resource transitions or memory aliasing on the GPU.
    //!
    //! FrameScheduler delegates most of the heavy lifting to the FrameGraphCompiler and FrameGraphExecuter
    //! classes, which are the platform-overridden interfaces for graph construction / execution, respectively. It
    //! effectively ties everything together by owning the frame graph and all the necessary sub-components. The class
    //! also facilitates jobification of command list recording.
    //!
    //! == Usage ==
    //!
    //! To use the frame scheduler:
    //! 1) Instantiate a FrameScheduler instance with a valid RHI device.
    //! 2) Override and instantiate ScopeProducers.
    //! 3) Once per frame:
    //! 3.1) Call BeginFrame().
    //! 3.2) Import ScopeProducers with ImportScopeProducer. You may also directly import / create
    //! attachments via GetAttachmentDatabase.
    //! 3.3) Call Compile (and validate the return code).
    //! 3.4) Call Execute (and validate the return code).
    //! 3.5) Call EndFrame() to complete execution.
    //!
    //! == Statistics ==
    //!
    //! Statistics may be gathered for a frame after EndFrame completes. The following statistics are reported:
    //! 1) Transient attachment usages with scope timeline. This data represents a grid where one axis is the scope
    //! execution order for the current frame, and the other axis is the internal aliased heap (i.e. starting at
    //! 0 bytes). The grid communicates the start and end points for each attachment. This data is useful when
    //! visualized to show overlap between attachments.
    //!   2) GPU timing information of each scope for each queue. GPU timing accuracy depends on the platform; certain
    //!      platforms (like mobile) do not have a way to extract exact GPU timings. Thus, they may instead represent
    //!      approximations.
    //!   3) GPU memory usage across the RHI associated with the device.
    //!
    //! The platform may or may not publish this information. If not, the method will return a null pointer.
    //!
    //! == Pool Resolves ==
    //!
    //! FrameScheduler contains a single "root" Graphics scope which is always the first scope added to the graph. All
    //! subsequent scopes take on a dependency to this root scope. The reason for this is twofold:
    //! 1) DeviceResourcePool implementations need a scope to perform resolves (DMA uploads) to GPU memory. These operations
    //! occur first in the frame to avoid complicating pool / scope dependencies. Hence, this is done synchronously
    //!       on the Graphics queue.
    //! 2) To make resource transitions and aliasing easier, the first scope in an attachment chain should be a
    //! Graphics scope. The root scope guarantees this to be true for any scenario.
    //!
    //! == Restrictions ==
    //!
    //! Currently, only a one frame scheduler instance is supported. This restriction can be lifted
    //! if the ResourceEventBus is replaced with a non-singleton queue data structure. Currently, it
    //! is only possible to flush this queue globally, which is incompatible with multiple frame schedulers.
    //! See [LY-83241] for more information.
    class FrameScheduler final
        : public FrameGraphBuilder
    {
    public:
        AZ_CLASS_ALLOCATOR(FrameScheduler, AZ::SystemAllocator);
        virtual ~FrameScheduler() = default;
        FrameScheduler() = default;
        FrameScheduler(const FrameScheduler&) = delete;

        bool IsInitialized() const;

        /// Initializes the frame scheduler and connects it to the buses.
        ResultCode Init(MultiDevice::DeviceMask deviceMask, const FrameSchedulerDescriptor& descriptor);

        /// Shuts down the frame scheduler.
        void Shutdown();

        //! Begin GPU frame. Any GPU-related operations should occur between this call and EndFrame.
        ResultCode BeginFrame();

        //! Ends GPU frame. Must be called after Execute if Compile was called.
        ResultCode EndFrame();

        //////////////////////////////////////////////////////////////////////////
        // FrameGraphBuilder - Methods should be called after BeginFrame, but before Compile.
        FrameGraphAttachmentInterface GetAttachmentDatabase() override;
        ResultCode ImportScopeProducer(ScopeProducer& scopeProducer) override;
        //////////////////////////////////////////////////////////////////////////

        //! Compiles the schedule. This should be called after successive calls to RegisterScope, and before
        //! calling Execute.
        MessageOutcome Compile(const FrameSchedulerCompileRequest& compileRequest);

        //! Executes the compiled schedule. Must be called after Compile. This will jobify recording and
        //! of command lists associated with each scope in the dependency graph.
        //! \param jobPolicy The global job policy for the current frame. If serial, it will force
        //!                   serial execution even if the platform supports parallel dispatch. If parallel,
        //!                   it will defer to the platform for parallel dispatch support.
        void Execute(JobPolicy jobPolicy);

        //! Returns the timing statistics for the previous frame.
        AZStd::unordered_map<int, TransientAttachmentStatistics> GetTransientAttachmentStatistics() const;

        //! Returns current CPU frame to frame time in milliseconds.
        double GetCpuFrameTime() const;

        //! Returns memory statistics for the previous frame.
        const MemoryStatistics* GetMemoryStatistics() const;

        //! Returns the implicit root scope id for the given deviceIndex.
        ScopeId GetRootScopeId(int deviceIndex = 0);

        //! Returns the descriptor which has information on the properties of a TransientAttachmentPool.
        const AZStd::unordered_map<int, TransientAttachmentPoolDescriptor>* GetTransientAttachmentPoolDescriptor() const;

        //! Adds a DeviceRayTracingShaderTable to be built this frame
        void QueueRayTracingShaderTableForBuild(DeviceRayTracingShaderTable* rayTracingShaderTable);

        //! Returns PhysicalDeviceDescriptor which can be used to extract vendor/driver information
        const PhysicalDeviceDescriptor& GetPhysicalDeviceDescriptor();

    private:
        AZStd::unordered_map<int, ScopeId> m_rootScopeIds;

        bool ValidateIsInitialized() const;
        bool ValidateIsProcessing() const;

        void PrepareProducers();
        void CompileProducers();
        void CompileShaderResourceGroups();
        void BuildRayTracingShaderTables();

        ScopeProducer* FindScopeProducer(const ScopeId& scopeId);

        //! This method executes a single context on a scope. First find the scope and
        //! then call the external Execute method. This may happen within a single
        //! job.
        void ExecuteContextInternal(FrameGraphExecuteGroup& group, uint32_t index);

        //! This method executes an entire group of contexts. Each context may result in a job dispatch depending
        //! on the respective job policies.
        void ExecuteGroupInternal(AZ::Job* parentJob, uint32_t groupIndex);

        bool m_isProcessing = false;

        MultiDevice::DeviceMask m_deviceMask = static_cast<MultiDevice::DeviceMask>(0);

        AZStd::unique_ptr<FrameGraph> m_frameGraph;
        FrameGraphAttachmentDatabase* m_frameGraphAttachmentDatabase = nullptr;

        Ptr<FrameGraphCompiler> m_frameGraphCompiler;
        Ptr<FrameGraphExecuter> m_frameGraphExecuter;

        Ptr<TransientAttachmentPool> m_transientAttachmentPool;

        AZStd::sys_time_t m_lastFrameEndTime{};
        MemoryStatistics m_memoryStatistics;

        FrameSchedulerCompileRequest m_compileRequest;

        AZStd::unordered_map<int, Scope*> m_rootScopes;
        AZStd::unordered_map<int, AZStd::unique_ptr<ScopeProducerEmpty>> m_rootScopeProducers;
        AZStd::vector<ScopeProducer*> m_scopeProducers;
        AZStd::unordered_map<ScopeId, ScopeProducer*> m_scopeProducerLookup;

        // list of RayTracingShaderTables that should be built this frame
        AZStd::vector<RHI::Ptr<DeviceRayTracingShaderTable>> m_rayTracingShaderTablesToBuild;

        AZ::TaskGraphActiveInterface* m_taskGraphActive = nullptr;
    };
}
