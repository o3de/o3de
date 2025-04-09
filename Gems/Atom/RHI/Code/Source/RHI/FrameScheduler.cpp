/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameEventBus.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphCompiler.h>
#include <Atom/RHI/FrameGraphExecuteGroup.h>
#include <Atom/RHI/FrameGraphExecuter.h>
#include <Atom/RHI/FrameGraphLogger.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>
#include <Atom/RHI/MemoryStatisticsBus.h>
#include <Atom/RHI/ResourceInvalidateBus.h>
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI/DeviceShaderResourceGroupPool.h>
#include <Atom/RHI/TransientAttachmentPool.h>
#include <Atom/RHI/ResourcePoolDatabase.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/DeviceRayTracingShaderTable.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzCore/std/time.h>

namespace AZ::RHI
{
    static constexpr const char* frameTimeMetricName = "Frame to Frame Time";
    static constexpr AZ::Crc32 frameTimeMetricId = AZ_CRC_CE(frameTimeMetricName);

    ResultCode FrameScheduler::Init(MultiDevice::DeviceMask deviceMask, const FrameSchedulerDescriptor& descriptor)
    {
        ResultCode resultCode = ResultCode::Success;

        m_frameGraph.reset(aznew FrameGraph());
        m_frameGraphAttachmentDatabase = &m_frameGraph->GetAttachmentDatabase();

        m_frameGraphCompiler = Factory::Get().CreateFrameGraphCompiler();
        resultCode = m_frameGraphCompiler->Init();

        if (resultCode != ResultCode::Success)
        {
            Shutdown();
            return resultCode;
        }

        m_frameGraphExecuter = Factory::Get().CreateFrameGraphExecuter();
        FrameGraphExecuterDescriptor executerDesc;
        executerDesc.m_platformLimitsDescriptors = descriptor.m_platformLimitsDescriptors;
        resultCode = m_frameGraphExecuter->Init(executerDesc);

        if (resultCode != ResultCode::Success)
        {
            Shutdown();
            return resultCode;
        }

        RHI::MultiDevice::DeviceMask transientAttachmentPoolDeviceMask{ 0 };

        for (auto& [deviceIndex, transientAttachmentPoolDescriptor] : descriptor.m_transientAttachmentPoolDescriptors)
        {
            if (DeviceTransientAttachmentPool::NeedsTransientAttachmentPool(transientAttachmentPoolDescriptor))
            {
                transientAttachmentPoolDeviceMask |= static_cast<RHI::MultiDevice::DeviceMask>(1 << deviceIndex);
            }
        }

        if (transientAttachmentPoolDeviceMask != static_cast<RHI::MultiDevice::DeviceMask>(0))
        {
            m_transientAttachmentPool = aznew TransientAttachmentPool();
            resultCode =
                m_transientAttachmentPool->Init(transientAttachmentPoolDeviceMask, descriptor.m_transientAttachmentPoolDescriptors);

            if (resultCode != ResultCode::Success)
            {
                Shutdown();
                return resultCode;
            }
        }

        m_deviceMask = deviceMask;

        MultiDeviceObject::IterateDevices(
            m_deviceMask,
            [this](int deviceIndex)
            {
                m_rootScopeProducers[deviceIndex].reset(aznew ScopeProducerEmpty(GetRootScopeId(deviceIndex), deviceIndex));
                m_rootScopes[deviceIndex] = m_rootScopeProducers[deviceIndex]->GetScope();
                return true;
            });

        m_taskGraphActive = AZ::Interface<AZ::TaskGraphActiveInterface>::Get();

        if (auto statsProfiler = AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get(); statsProfiler)
        {
            statsProfiler->ActivateProfiler(rhiMetricsId, true);

            auto& rhiMetrics = statsProfiler->GetProfiler(rhiMetricsId);
            rhiMetrics.GetStatsManager().AddStatistic(frameTimeMetricId, frameTimeMetricName, /*units=*/"clocks", /*failIfExist=*/false);
        }

        m_lastFrameEndTime = AZStd::GetTimeNowTicks();

        return ResultCode::Success;
    }

    void FrameScheduler::Shutdown()
    {
        m_deviceMask = static_cast<MultiDevice::DeviceMask>(0);
        m_taskGraphActive = nullptr;
        m_rootScopeProducers.clear();
        m_rootScopes.clear();
        m_frameGraphExecuter = nullptr;
        m_frameGraphCompiler = nullptr;
        m_transientAttachmentPool = nullptr;
        m_frameGraphAttachmentDatabase = nullptr;
        m_frameGraph = nullptr;
    }

    bool FrameScheduler::ValidateIsInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized() == false)
            {
                AZ_Error("FrameScheduler", false, "Frame scheduler is not initialized.");
                return false;
            }
        }

        return true;
    }

    bool FrameScheduler::ValidateIsProcessing() const
    {
        if (Validation::IsEnabled())
        {
            if (m_isProcessing == false)
            {
                AZ_Error(
                    "FrameScheduler", false,
                    "This operation is not allowed because the frame scheduler is not currently processing the frame.");
                return false;
            }
        }

        return true;
    }

    bool FrameScheduler::IsInitialized() const
    {
        return AZStd::to_underlying(m_deviceMask) != 0;
    }

    FrameGraphAttachmentInterface FrameScheduler::GetAttachmentDatabase()
    {
        return FrameGraphAttachmentInterface(*m_frameGraphAttachmentDatabase);
    }

    ResultCode FrameScheduler::ImportScopeProducer(ScopeProducer& scopeProducer)
    {
        if (!ValidateIsProcessing())
        {
            return RHI::ResultCode::InvalidOperation;
        }

        if (Validation::IsEnabled())
        {
            if (scopeProducer.GetScopeId().IsEmpty())
            {
                AZ_Error("FrameScheduler", false, "Importing a scope with an empty id!");
                return RHI::ResultCode::InvalidArgument;
            }
        }

        const bool isUnique = m_scopeProducerLookup.emplace(scopeProducer.GetScopeId(), &scopeProducer).second;
        (void)isUnique;

        if (Validation::IsEnabled())
        {
            if (!isUnique)
            {
                AZ_Error("FrameScheduler", false, "Scope %s already exists. You must specify a unique name.", scopeProducer.GetScopeId().GetCStr());
                return RHI::ResultCode::InvalidArgument;
            }
        }

        m_scopeProducers.emplace_back(&scopeProducer);
        return ResultCode::Success;
    }

    MessageOutcome FrameScheduler::Compile(const FrameSchedulerCompileRequest& compileRequest)
    {
        AZ_PROFILE_SCOPE(RHI, "FrameScheduler: Compile");

        //Go through each scope and call their SetupFrameGraphDependencies to build the framegraph
        PrepareProducers();

        m_compileRequest = compileRequest;

        {
            AZ_PROFILE_SCOPE(RHI, "FrameScheduler: Compile: OnFrameCompile");
            FrameEventBus::Broadcast(&FrameEventBus::Events::OnFrameCompile);
        }

        FrameGraphCompileRequest frameGraphCompileRequest;
        frameGraphCompileRequest.m_frameGraph = m_frameGraph.get();
        frameGraphCompileRequest.m_transientAttachmentPool = m_transientAttachmentPool.get();
        frameGraphCompileRequest.m_logVerbosity = compileRequest.m_logVerbosity;
        frameGraphCompileRequest.m_compileFlags = compileRequest.m_compileFlags;
        frameGraphCompileRequest.m_statisticsFlags = compileRequest.m_statisticsFlags;

        const MessageOutcome outcome = m_frameGraphCompiler->Compile(frameGraphCompileRequest);
        if (outcome.IsSuccess())
        {
            {
                AZ_PROFILE_SCOPE(RHI, "FrameScheduler: Compile: OnFrameCompileEnd");
                FrameEventBus::Broadcast(&FrameEventBus::Events::OnFrameCompileEnd, *m_frameGraph);
            }

            FrameGraphLogger::Log(*m_frameGraph, compileRequest.m_logVerbosity);

            // Builds the scope execution schedule using the compiled graph.
            m_frameGraphExecuter->Begin(*m_frameGraph);

            // Compile all producers, which will call out to user code to invalidate any scope-specific shader resource groups.
            CompileProducers();

            // Compile all invalidated shader resource groups.
            CompileShaderResourceGroups();

            // Build RayTracingShaderTables
            BuildRayTracingShaderTables();
        }
        return outcome;
    }

    void FrameScheduler::PrepareProducers()
    {
        AZ_PROFILE_SCOPE(RHI, "FrameScheduler: PrepareProducers");

        for (ScopeProducer* scopeProducer : m_scopeProducers)
        {
            RHI_PROFILE_SCOPE_VERBOSE("FrameScheduler: PrepareProducers: Scope %s", scopeProducer->GetScopeId().GetCStr());

            auto scope = scopeProducer->GetScope();
            auto deviceIndex = scopeProducer->GetDeviceIndex();

            if (deviceIndex == MultiDevice::InvalidDeviceIndex)
            {
                deviceIndex = MultiDevice::DefaultDeviceIndex;
            }

            scope->SetDeviceIndex(deviceIndex);

            m_frameGraph->BeginScope(*scope);
            scopeProducer->SetupFrameGraphDependencies(*m_frameGraph);
                
            // All scopes depend on their device root scope.
            if (scopeProducer->GetScopeId() != GetRootScopeId(deviceIndex))
            {
                m_frameGraph->ExecuteAfter(GetRootScopeId(deviceIndex));
            }

            m_frameGraph->EndScope();
        }
        m_frameGraph->End();
    }

    void FrameScheduler::CompileProducers()
    {
        AZ_PROFILE_SCOPE(RHI, "FrameScheduler: CompileProducers");

        for (auto scope : m_frameGraph->GetScopes())
        {
            auto scopeProducer = FindScopeProducer(scope->GetId());
            const FrameGraphCompileContext context(scopeProducer->GetScopeId(), m_frameGraph->GetAttachmentDatabase());
            scopeProducer->CompileResources(context);
        }
    }

    void FrameScheduler::CompileShaderResourceGroups()
    {
        AZ_PROFILE_SCOPE(RHI, "FrameScheduler: CompileShaderResourceGroups");

        // Execute all queued resource invalidations, which will mark SRG's for compilation.
        {
            ResourceInvalidateBus::ExecuteQueuedEvents();
        }

        MultiDeviceObject::IterateDevices(
            m_deviceMask,
            [this](int deviceIndex)
            {
                Device* device = RHI::RHISystemInterface::Get()->GetDevice(deviceIndex);

                const ResourcePoolDatabase& resourcePoolDatabase = device->GetResourcePoolDatabase();

                if (m_compileRequest.m_jobPolicy == JobPolicy::Parallel)
                {
                    // Iterate over each SRG pool and fork jobs to compile SRGs.
                    const uint32_t compilesPerJob = m_compileRequest.m_shaderResourceGroupCompilesPerJob;
                    if (m_taskGraphActive && m_taskGraphActive->IsTaskGraphActive())
                    {
                        AZ::TaskGraph taskGraph{ "SRG Compilation" };

                        const auto compileIntervalsFunction = [compilesPerJob, &taskGraph](DeviceShaderResourceGroupPool* srgPool)
                        {
                            srgPool->CompileGroupsBegin();
                            const uint32_t compilesInPool = srgPool->GetGroupsToCompileCount();
                            const uint32_t jobCount = AZ::DivideAndRoundUp(compilesInPool, compilesPerJob);
                            AZ::TaskDescriptor srgCompileDesc{ "SrgCompile", "Graphics" };
                            AZ::TaskDescriptor srgCompileEndDesc{ "SrgCompileEnd", "Graphics" };

                            auto srgCompileEndTask = taskGraph.AddTask(
                                srgCompileEndDesc,
                                [srgPool]()
                                {
                                    srgPool->CompileGroupsEnd();
                                });

                            for (uint32_t i = 0; i < jobCount; ++i)
                            {
                                Interval interval;
                                interval.m_min = i * compilesPerJob;
                                interval.m_max = AZStd::min(interval.m_min + compilesPerJob, compilesInPool);

                                auto compileTask = taskGraph.AddTask(
                                    srgCompileDesc,
                                    [srgPool, interval]()
                                    {
                                        AZ_PROFILE_SCOPE(RHI, "FrameScheduler : compileGroupsForIntervalLambda");
                                        srgPool->CompileGroupsForInterval(interval);
                                    });
                                compileTask.Precedes(srgCompileEndTask);
                            }
                        };

                        resourcePoolDatabase.ForEachShaderResourceGroupPool<decltype(compileIntervalsFunction)>(
                            AZStd::move(compileIntervalsFunction));
                        if (!taskGraph.IsEmpty())
                        {
                            AZ::TaskGraphEvent finishedEvent{ "SRG Compile Wait" };
                            taskGraph.Submit(&finishedEvent);
                            finishedEvent.Wait();
                        }
                    }
                    else // use Job system
                    {
                        const auto compileGroupsBeginFunction = [](DeviceShaderResourceGroupPool* srgPool)
                        {
                            srgPool->CompileGroupsBegin();
                        };

                        resourcePoolDatabase.ForEachShaderResourceGroupPool<decltype(compileGroupsBeginFunction)>(
                            compileGroupsBeginFunction);

                        // Iterate over each SRG pool and fork jobs to compile SRGs.
                        AZ::JobCompletion jobCompletion;

                        const auto compileIntervalsFunction = [compilesPerJob, &jobCompletion](DeviceShaderResourceGroupPool* srgPool)
                        {
                            const uint32_t compilesInPool = srgPool->GetGroupsToCompileCount();
                            const uint32_t jobCount = AZ::DivideAndRoundUp(compilesInPool, compilesPerJob);

                            for (uint32_t i = 0; i < jobCount; ++i)
                            {
                                Interval interval;
                                interval.m_min = i * compilesPerJob;
                                interval.m_max = AZStd::min(interval.m_min + compilesPerJob, compilesInPool);

                                const auto compileGroupsForIntervalLambda = [srgPool, interval]()
                                {
                                    AZ_PROFILE_SCOPE(RHI, "FrameScheduler : compileGroupsForIntervalLambda");
                                    srgPool->CompileGroupsForInterval(interval);
                                };

                                AZ::Job* executeGroupJob =
                                    AZ::CreateJobFunction(AZStd::move(compileGroupsForIntervalLambda), true, nullptr);
                                executeGroupJob->SetDependent(&jobCompletion);
                                executeGroupJob->Start();
                            }
                        };

                        resourcePoolDatabase.ForEachShaderResourceGroupPool<decltype(compileIntervalsFunction)>(
                            AZStd::move(compileIntervalsFunction));

                        jobCompletion.StartAndWaitForCompletion();

                        const auto compileGroupsEndFunction = [](DeviceShaderResourceGroupPool* srgPool)
                        {
                            srgPool->CompileGroupsEnd();
                        };

                        resourcePoolDatabase.ForEachShaderResourceGroupPool<decltype(compileGroupsEndFunction)>(compileGroupsEndFunction);
                    }
                }
                else
                {
                    const auto compileAllLambda = [](DeviceShaderResourceGroupPool* srgPool)
                    {
                        srgPool->CompileGroupsBegin();
                        srgPool->CompileGroupsForInterval(Interval(0, srgPool->GetGroupsToCompileCount()));
                        srgPool->CompileGroupsEnd();
                    };

                    resourcePoolDatabase.ForEachShaderResourceGroupPool<decltype(compileAllLambda)>(compileAllLambda);
                }

                // It is possible for certain back ends to run out of SRG memory (due to fragmentation) in which case
                // we try to compact and re-compile SRGs.
                [[maybe_unused]] RHI::ResultCode resultCode = device->CompactSRGMemory();
                AZ_Assert(resultCode == RHI::ResultCode::Success, "SRG compaction failed and this can lead to a gpu crash.");
                return true;
            });
    }

    void FrameScheduler::BuildRayTracingShaderTables()
    {
        AZ_PROFILE_SCOPE(RHI, "FrameScheduler: BuildRayTracingShaderTables");

        for (auto rayTracingShaderTable : m_rayTracingShaderTablesToBuild)
        {
            rayTracingShaderTable->Validate();

            [[maybe_unused]] ResultCode resultCode = rayTracingShaderTable->BuildInternal();
            AZ_Assert(resultCode == ResultCode::Success, "DeviceRayTracingShaderTable build failed");

            rayTracingShaderTable->m_isQueuedForBuild = false;
        }

        // clear the list now that all RayTracingShaderTables have been built for this frame
        m_rayTracingShaderTablesToBuild.clear();
    }

    ResultCode FrameScheduler::BeginFrame()
    {
        AZ_PROFILE_SCOPE(RHI, "FrameScheduler: BeginFrame");

        if (!ValidateIsInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (Validation::IsEnabled())
        {
            if (m_isProcessing)
            {
                AZ_Error("FrameScheduler", false, "BeginFrame called while already processing a frame (EndFrame was not called).");
                return ResultCode::InvalidOperation;
            }
        }

        m_isProcessing = true;

        auto result = ResultCode::Success;

        MultiDeviceObject::IterateDevices(
            m_deviceMask,
            [&result](int deviceIndex)
            {
                Device* device = RHI::RHISystemInterface::Get()->GetDevice(deviceIndex);

                if (device->BeginFrame() != ResultCode::Success)
                {
                    result = ResultCode::Fail;
                }
                return true;
            });

        if (result == ResultCode::Success)
        {
            m_frameGraph->Begin();

            MultiDeviceObject::IterateDevices(
                m_deviceMask,
                [this](int deviceIndex)
                {
                    Device* device = RHI::RHISystemInterface::Get()->GetDevice(deviceIndex);

                    ImportScopeProducer(*(m_rootScopeProducers.at(deviceIndex)));
                    m_rootScopes[deviceIndex]->QueueResourcePoolResolves(device->GetResourcePoolDatabase());

                    // This is broadcast after beginning the frame so that the CPU and GPU are synchronized.
                    FrameEventBus::Event(device, &FrameEventBus::Events::OnFrameBegin);
                    return true;
                });

            return ResultCode::Success;
        }

        return ResultCode::Fail;
    }

    ResultCode FrameScheduler::EndFrame()
    {
        AZ_PROFILE_SCOPE(RHI, "FrameScheduler: EndFrame");

        if (Validation::IsEnabled())
        {
            if (!m_isProcessing)
            {
                AZ_Error("FrameScheduler", false, "EndFrame called without matching BeginFrame.");
                return ResultCode::InvalidOperation;
            }
        }

        m_isProcessing = false;
        m_frameGraphExecuter->End();
        m_frameGraph->Clear();

        MultiDeviceObject::IterateDevices(
            m_deviceMask,
            [this](int deviceIndex)
            {
                Device* device = RHI::RHISystemInterface::Get()->GetDevice(deviceIndex);

                device->EndFrame();

                if (CheckBitsAny(m_compileRequest.m_statisticsFlags, FrameSchedulerStatisticsFlags::GatherMemoryStatistics))
                {
                    device->CompileMemoryStatistics(m_memoryStatistics, MemoryStatisticsReportFlags::Detail);
                    m_memoryStatistics.m_detailedCapture = true;
                }
                else
                {
                    device->CompileMemoryStatistics(m_memoryStatistics, MemoryStatisticsReportFlags::Basic);
                    m_memoryStatistics.m_detailedCapture = false;
                }

                device->UpdateCpuTimingStatistics();

                {
                    AZ_PROFILE_SCOPE(RHI, "FrameScheduler: EndFrame: OnFrameEnd");
                    FrameEventBus::Event(device, &FrameEventBus::Events::OnFrameEnd);
                }
                return true;
            });

        m_scopeProducers.clear();
        m_scopeProducerLookup.clear();

        const AZStd::sys_time_t timeNowTicks = AZStd::GetTimeNowTicks();
        if (auto statsProfiler = AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get(); statsProfiler)
        {
            statsProfiler->PushSample(rhiMetricsId, frameTimeMetricId, static_cast<double>(timeNowTicks - m_lastFrameEndTime));
        }
        m_lastFrameEndTime = timeNowTicks;

        return ResultCode::Success;
    }

    void FrameScheduler::ExecuteContextInternal(FrameGraphExecuteGroup& group, uint32_t index)
    {
        FrameGraphExecuteContext* executeContext = group.BeginContext(index);

        {
            ScopeProducer* scopeProducer = FindScopeProducer(executeContext->GetScopeId());

            AZ_PROFILE_SCOPE(RHI, "ScopeProducer: %s", scopeProducer->GetScopeId().GetCStr());

            if (executeContext->GetCommandList())
            {
                // reset the submit count in preparation for the scope submits
                executeContext->GetCommandList()->ResetTotalSubmits();
            }

            scopeProducer->BuildCommandList(*executeContext);

            if (executeContext->GetCommandList())
            {
                // validate the submits that were added during BuildCommandList
                executeContext->GetCommandList()->ValidateTotalSubmits(scopeProducer);
            }
        }

        group.EndContext(index);
    }

    void FrameScheduler::ExecuteGroupInternal(AZ::Job* parentJob, uint32_t groupIndex)
    {
        AZ_PROFILE_SCOPE(RHI, "FrameScheduler: ExecuteGroupInternal");

        FrameGraphExecuteGroup* executeGroup = m_frameGraphExecuter->BeginGroup(groupIndex);
        const uint32_t contextCount = executeGroup->GetContextCount();

        // We must do serial execution if the group itself is serial, the parent job is null
        // (indicating the scheduler itself chose not to jobify), or we're just executing a single
        // context.
        const bool isSerialPolicy = executeGroup->GetJobPolicy() == JobPolicy::Serial;
        const bool isSerialExecute = parentJob == nullptr || contextCount == 1 || isSerialPolicy;

        if (isSerialExecute)
        {
            for (uint32_t i = 0; i < contextCount; ++i)
            {
                ExecuteContextInternal(*executeGroup, i);
            }
        }

        // Spawns a job for each context in the group as a child of this job.
        else
        {
            for (uint32_t i = 0; i < contextCount; ++i)
            {
                const auto jobLambda = [this, executeGroup, i]()
                {
                    ExecuteContextInternal(*executeGroup, i);
                };

                parentJob->StartAsChild(AZ::CreateJobFunction(AZStd::move(jobLambda), true, nullptr));
            }

            {
                AZ_PROFILE_SCOPE(RHI, "FrameScheduler: ExecuteGroupInternal: WaitForChildren");
                parentJob->WaitForChildren();
            }
        }

        m_frameGraphExecuter->EndGroup(groupIndex);
    }

    void FrameScheduler::Execute(JobPolicy overrideJobPolicy)
    {
        AZ_PROFILE_SCOPE(RHI, "FrameScheduler: Execute");

        const uint32_t groupCount = m_frameGraphExecuter->GetGroupCount();
        const JobPolicy platformJobPolicy = m_frameGraphExecuter->GetJobPolicy();

        // The scheduler itself can force serial execution even if the platform supports it (e.g. as a debugging flag).
        // We must run serially if the scheduler or the platform forces us to.
        if (overrideJobPolicy == JobPolicy::Serial ||
            platformJobPolicy == JobPolicy::Serial)
        {
            for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
            {
                ExecuteGroupInternal(nullptr, groupIndex);
            }
        }

        // Otherwise, fork a job for each group.
        else
        {
            AZ::JobCompletion jobCompletion;
            for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
            {
                const auto jobLambda = [this, groupIndex](AZ::Job& owner)
                {
                    ExecuteGroupInternal(&owner, groupIndex);
                };

                AZ::Job* executeGroupJob = AZ::CreateJobFunction(jobLambda, true, nullptr);
                executeGroupJob->SetDependent(&jobCompletion);
                executeGroupJob->Start();
            }

            jobCompletion.StartAndWaitForCompletion();
        }
    }

    ScopeProducer* FrameScheduler::FindScopeProducer(const ScopeId& scopeId)
    {
        auto findIt = m_scopeProducerLookup.find(scopeId);
        if (findIt != m_scopeProducerLookup.end())
        {
            return findIt->second;
        }
        return nullptr;
    }

    const MemoryStatistics* FrameScheduler::GetMemoryStatistics() const
    {
        return &m_memoryStatistics;
    }

    AZStd::unordered_map<int, TransientAttachmentStatistics> FrameScheduler::GetTransientAttachmentStatistics() const
    {
        return
            CheckBitsAny(m_compileRequest.m_statisticsFlags, FrameSchedulerStatisticsFlags::GatherTransientAttachmentStatistics)
            ? m_transientAttachmentPool->GetStatistics()
            : AZStd::unordered_map<int, TransientAttachmentStatistics>();
    }

    double FrameScheduler::GetCpuFrameTime() const
    {
        if (auto statsProfiler = AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get(); statsProfiler)
        {
            auto& rhiMetrics = statsProfiler->GetProfiler(rhiMetricsId);
            const auto* frameTimeStat = rhiMetrics.GetStatistic(frameTimeMetricId);
            return !frameTimeStat ? 0 : (frameTimeStat->GetMostRecentSample() * 1000) / aznumeric_cast<double>(AZStd::GetTimeTicksPerSecond());
        }
        return 0;
    }

    ScopeId FrameScheduler::GetRootScopeId(int deviceIndex)
    {
        auto iterator{ m_rootScopeIds.find(deviceIndex) };
        if (iterator == m_rootScopeIds.end())
        {
            auto [new_iterator, inserted]{ m_rootScopeIds.insert(
                AZStd::make_pair(deviceIndex, ScopeId{AZStd::string("Root") + AZStd::to_string(deviceIndex)})) };
            if (inserted)
            {
                return new_iterator->second;
            }
        }
        return iterator->second;
    }

    const AZStd::unordered_map<int, TransientAttachmentPoolDescriptor>* FrameScheduler::GetTransientAttachmentPoolDescriptor() const
    {
        return m_transientAttachmentPool ? &m_transientAttachmentPool->GetDescriptor() : nullptr;
    }

    void FrameScheduler::QueueRayTracingShaderTableForBuild(DeviceRayTracingShaderTable* rayTracingShaderTable)
    {
        m_rayTracingShaderTablesToBuild.push_back(rayTracingShaderTable);
    }
}
