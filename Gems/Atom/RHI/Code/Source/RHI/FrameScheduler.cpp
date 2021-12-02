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
#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <Atom/RHI/TransientAttachmentPool.h>
#include <Atom/RHI/ResourcePoolDatabase.h>
#include <Atom/RHI/RayTracingShaderTable.h>

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Task/TaskGraph.h>

namespace AZ
{
    namespace RHI
    {
        static constexpr const char* frameTimeMetricName = "Frame to Frame Time";
        static constexpr AZ::Crc32 frameTimeMetricId = AZ_CRC_CE(frameTimeMetricName);

        ResultCode FrameScheduler::Init(Device& device, const FrameSchedulerDescriptor& descriptor)
        {
            ResultCode resultCode = ResultCode::Success;

            m_frameGraph.reset(aznew FrameGraph());
            m_frameGraphAttachmentDatabase = &m_frameGraph->GetAttachmentDatabase();

            m_frameGraphCompiler = Factory::Get().CreateFrameGraphCompiler();
            resultCode = m_frameGraphCompiler->Init(device);

            if (resultCode != ResultCode::Success)
            {
                Shutdown();
                return resultCode;
            }

            m_frameGraphExecuter = Factory::Get().CreateFrameGraphExecuter();
            FrameGraphExecuterDescriptor executerDesc;
            executerDesc.m_device = &device;
            executerDesc.m_platformLimitsDescriptor = descriptor.m_platformLimitsDescriptor;
            resultCode = m_frameGraphExecuter->Init(executerDesc);

            if (resultCode != ResultCode::Success)
            {
                Shutdown();
                return resultCode;
            }

            if (TransientAttachmentPool::NeedsTransientAttachmentPool(descriptor.m_transientAttachmentPoolDescriptor))
            {
                m_transientAttachmentPool = Factory::Get().CreateTransientAttachmentPool();
                resultCode = m_transientAttachmentPool->Init(device, descriptor.m_transientAttachmentPoolDescriptor);

                if (resultCode != ResultCode::Success)
                {
                    Shutdown();
                    return resultCode;
                }
            }

            m_rootScopeProducer.reset(aznew ScopeProducerEmpty(GetRootScopeId()));
            m_rootScope = m_rootScopeProducer->GetScope();
            m_device = &device;

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
            m_device = nullptr;
            m_taskGraphActive = nullptr;
            m_rootScopeProducer = nullptr;
            m_rootScope = nullptr;
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
            return m_device != nullptr;
        }

        FrameGraphAttachmentInterface FrameScheduler::GetAttachmentDatabase()
        {
            return FrameGraphAttachmentInterface(*m_frameGraphAttachmentDatabase);
        }

        ResultCode FrameScheduler::ImportScopeProducer(ScopeProducer& scopeProducer)
        {
            AZ_PROFILE_SCOPE(RHI, "FrameScheduler: ImportScopeProducer");

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
                m_frameGraph->BeginScope(*scopeProducer->GetScope());
                scopeProducer->SetupFrameGraphDependencies(*m_frameGraph);

                // All scopes depend on the root scope.
                if (scopeProducer->GetScopeId() != m_rootScopeId)
                {
                    m_frameGraph->ExecuteAfter(m_rootScopeId);
                }

                m_frameGraph->EndScope();
            }
            m_frameGraph->End();
        }

        void FrameScheduler::CompileProducers()
        {
            AZ_PROFILE_SCOPE(RHI, "FrameScheduler: CompileProducers");

            for (ScopeProducer* scopeProducer : m_scopeProducers)
            {
                const FrameGraphCompileContext context(scopeProducer->GetScopeId(), m_frameGraph->GetAttachmentDatabase());
                scopeProducer->CompileResources(context);
            }
        }

        void FrameScheduler::CompileShaderResourceGroups()
        {
            AZ_PROFILE_SCOPE(RHI, "FrameScheduler: CompileShaderResourceGroups");

            // Execute all queued resource invalidations, which will mark SRG's for compilation.
            {
                AZ_PROFILE_SCOPE(RHI, "Invalidate Resources");
                ResourceInvalidateBus::ExecuteQueuedEvents();
            }

            const ResourcePoolDatabase& resourcePoolDatabase = m_device->GetResourcePoolDatabase();

            if (m_compileRequest.m_jobPolicy == JobPolicy::Parallel)
            {
                // Iterate over each SRG pool and fork jobs to compile SRGs.
                const uint32_t compilesPerJob = m_compileRequest.m_shaderResourceGroupCompilesPerJob;
                if (m_taskGraphActive && m_taskGraphActive->IsTaskGraphActive())
                {
                    AZ::TaskGraph taskGraph;

                    const auto compileIntervalsFunction = [compilesPerJob, &taskGraph](ShaderResourceGroupPool* srgPool)
                    {
                        srgPool->CompileGroupsBegin();
                        const uint32_t compilesInPool = srgPool->GetGroupsToCompileCount();
                        const uint32_t jobCount = DivideByMultiple(compilesInPool, compilesPerJob);
                        AZ::TaskDescriptor srgCompileDesc{"SrgCompile", "Graphics"};
                        AZ::TaskDescriptor srgCompileEndDesc{"SrgCompileEnd", "Graphics"};

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

                    resourcePoolDatabase.ForEachShaderResourceGroupPool<decltype(compileIntervalsFunction)>(AZStd::move(compileIntervalsFunction));
                    if (!taskGraph.IsEmpty())
                    {
                        AZ::TaskGraphEvent finishedEvent;
                        taskGraph.Submit(&finishedEvent);
                        finishedEvent.Wait();
                    }
                }
                else // use Job system
                {
                    const auto compileGroupsBeginFunction = [](ShaderResourceGroupPool* srgPool)
                    {
                        srgPool->CompileGroupsBegin();
                    };

                    resourcePoolDatabase.ForEachShaderResourceGroupPool<decltype(compileGroupsBeginFunction)>(compileGroupsBeginFunction);

                    // Iterate over each SRG pool and fork jobs to compile SRGs.
                    AZ::JobCompletion jobCompletion;

                    const auto compileIntervalsFunction = [compilesPerJob, &jobCompletion](ShaderResourceGroupPool* srgPool)
                    {
                        const uint32_t compilesInPool = srgPool->GetGroupsToCompileCount();
                        const uint32_t jobCount = DivideByMultiple(compilesInPool, compilesPerJob);

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

                            AZ::Job* executeGroupJob = AZ::CreateJobFunction(AZStd::move(compileGroupsForIntervalLambda), true, nullptr);
                            executeGroupJob->SetDependent(&jobCompletion);
                            executeGroupJob->Start();
                        }
                    };

                    resourcePoolDatabase.ForEachShaderResourceGroupPool<decltype(compileIntervalsFunction)>(AZStd::move(compileIntervalsFunction));

                    jobCompletion.StartAndWaitForCompletion();

                    const auto compileGroupsEndFunction = [](ShaderResourceGroupPool* srgPool)
                    {
                        srgPool->CompileGroupsEnd();
                    };

                    resourcePoolDatabase.ForEachShaderResourceGroupPool<decltype(compileGroupsEndFunction)>(compileGroupsEndFunction);
                }
            }
            else
            {
                const auto compileAllLambda = [](ShaderResourceGroupPool* srgPool)
                {
                    srgPool->CompileGroupsBegin();
                    srgPool->CompileGroupsForInterval(Interval(0, srgPool->GetGroupsToCompileCount()));
                    srgPool->CompileGroupsEnd();
                };

                resourcePoolDatabase.ForEachShaderResourceGroupPool<decltype(compileAllLambda)>(compileAllLambda);
            }

            //It is possible for certain back ends to run out of SRG memory (due to fragmentation) in which case
            //we try to compact and re-compile SRGs.
            [[maybe_unused]] RHI::ResultCode resultCode = m_device->CompactSRGMemory();
            AZ_Assert(resultCode == RHI::ResultCode::Success, "SRG compaction failed and this can lead to a gpu crash.");
        }

        void FrameScheduler::BuildRayTracingShaderTables()
        {
            AZ_PROFILE_SCOPE(RHI, "FrameScheduler: BuildRayTracingShaderTables");

            for (auto rayTracingShaderTable : m_rayTracingShaderTablesToBuild)
            {
                rayTracingShaderTable->Validate();

                [[maybe_unused]] ResultCode resultCode = rayTracingShaderTable->BuildInternal();
                AZ_Assert(resultCode == ResultCode::Success, "RayTracingShaderTable build failed");

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

            m_device->BeginFrame();
            m_frameGraph->Begin();

            ImportScopeProducer(*m_rootScopeProducer);

            // Queue resource pool resolves onto the root scope.
            m_rootScope->QueueResourcePoolResolves(m_device->GetResourcePoolDatabase());

            // This is broadcast after beginning the frame so that the CPU and GPU are synchronized.
            FrameEventBus::Event(m_device, &FrameEventBus::Events::OnFrameBegin);

            return ResultCode::Success;
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
            m_device->EndFrame();

            if (CheckBitsAny(m_compileRequest.m_statisticsFlags, FrameSchedulerStatisticsFlags::GatherMemoryStatistics))
            {
                m_device->CompileMemoryStatistics(m_memoryStatistics, MemoryStatisticsReportFlags::Detail);
            }

            m_device->UpdateCpuTimingStatistics();

            m_scopeProducers.clear();
            m_scopeProducerLookup.clear();

            {
                AZ_PROFILE_SCOPE(RHI, "FrameScheduler: EndFrame: OnFrameEnd");
                FrameEventBus::Event(m_device, &FrameEventBus::Events::OnFrameEnd);
            }

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
                scopeProducer->BuildCommandList(*executeContext);
            }

            group.EndContext(index);
        }

        void FrameScheduler::ExecuteGroupInternal(AZ::Job* parentJob, uint32_t groupIndex)
        {
            AZ_PROFILE_SCOPE(RHI, "FrameScheduler: ExecuteGroupInternal");

            FrameGraphExecuteGroup* executeGroup = m_frameGraphExecuter->BeginGroup(groupIndex);
            const uint32_t contextCount = executeGroup->GetContextCount();

            /**
             * We must do serial execution if the group itself is serial, the parent job is null
             * (indicating the scheduler itself chose not to jobify), or we're just executing a single
             * context.
             */
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

                parentJob->WaitForChildren();
            }

            m_frameGraphExecuter->EndGroup(groupIndex);
        }

        void FrameScheduler::Execute(JobPolicy overrideJobPolicy)
        {
            AZ_PROFILE_SCOPE(RHI, "FrameScheduler: Execute");

            const uint32_t groupCount = m_frameGraphExecuter->GetGroupCount();
            const JobPolicy platformJobPolicy = m_frameGraphExecuter->GetJobPolicy();

            /**
             * The scheduler itself can force serial execution even if the platform supports it (e.g. as a debugging flag).
             * We must run serially if the scheduler or the platform forces us to.
             */
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
            return
                CheckBitsAny(m_compileRequest.m_statisticsFlags, FrameSchedulerStatisticsFlags::GatherMemoryStatistics)
                ? &m_memoryStatistics
                : nullptr;
        }

        const TransientAttachmentStatistics* FrameScheduler::GetTransientAttachmentStatistics() const
        {
            return
                CheckBitsAny(m_compileRequest.m_statisticsFlags, FrameSchedulerStatisticsFlags::GatherTransientAttachmentStatistics)
                ? &m_transientAttachmentPool->GetStatistics()
                : nullptr;
        }

        double FrameScheduler::GetCpuFrameTime() const
        {
            if (auto statsProfiler = AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get(); statsProfiler)
            {
                auto& rhiMetrics = statsProfiler->GetProfiler(rhiMetricsId);
                const auto* frameTimeStat = rhiMetrics.GetStatistic(frameTimeMetricId);
                return (frameTimeStat->GetMostRecentSample() * 1000) / aznumeric_cast<double>(AZStd::GetTimeTicksPerSecond());
            }
            return 0;
        }

        ScopeId FrameScheduler::GetRootScopeId() const
        {
            return m_rootScopeId;
        }

        const TransientAttachmentPoolDescriptor* FrameScheduler::GetTransientAttachmentPoolDescriptor() const
        {
            return m_transientAttachmentPool ? &m_transientAttachmentPool->GetDescriptor() : nullptr;
        }

        void FrameScheduler::QueueRayTracingShaderTableForBuild(RayTracingShaderTable* rayTracingShaderTable)
        {
            m_rayTracingShaderTablesToBuild.push_back(rayTracingShaderTable);
        }
    }
}
