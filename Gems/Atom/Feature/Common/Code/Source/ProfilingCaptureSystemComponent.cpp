/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProfilingCaptureSystemComponent.h"

#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RHI/RHIUtils.h>

#include <Atom/RPI.Public/GpuQuery/GpuQueryTypes.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>

#include <AtomCore/Serialization/Json/JsonUtils.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Render
    {
        class ProfilingCaptureNotificationBusHandler final
            : public ProfilingCaptureNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(ProfilingCaptureNotificationBusHandler, "{E45E4F37-EC1F-4010-994B-4F80998BEF15}", AZ::SystemAllocator,
                OnCaptureQueryTimestampFinished,
                OnCaptureQueryPipelineStatisticsFinished,
                OnCaptureCpuProfilingStatisticsFinished,
                OnCaptureBenchmarkMetadataFinished
            );

            void OnCaptureQueryTimestampFinished(bool result, const AZStd::string& info) override
            {
                Call(FN_OnCaptureQueryTimestampFinished, result, info);
            }

            void OnCaptureQueryPipelineStatisticsFinished(bool result, const AZStd::string& info) override
            {
                Call(FN_OnCaptureQueryPipelineStatisticsFinished, result, info);
            }

            void OnCaptureCpuProfilingStatisticsFinished(bool result, const AZStd::string& info) override
            {
                Call(FN_OnCaptureCpuProfilingStatisticsFinished, result, info);
            }

            void OnCaptureBenchmarkMetadataFinished(bool result, const AZStd::string& info) override
            {
                Call(FN_OnCaptureBenchmarkMetadataFinished, result, info);
            }

            static void Reflect(AZ::ReflectContext* context)
            {
                if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
                {
                    behaviorContext->EBus<ProfilingCaptureNotificationBus>("ProfilingCaptureNotificationBus")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Module, "atom")
                        ->Handler<ProfilingCaptureNotificationBusHandler>()
                        ;
                }
            }
        };

        // Intermediate class to serialize pass' Timestamp data.
        class TimestampSerializer
        {
        public:
            class TimestampSerializerEntry
            {
            public:
                AZ_TYPE_INFO(TimestampSerializer::TimestampSerializerEntry, "{34C90068-954C-4A07-A265-DB21462A7F9B}");
                static void Reflect(AZ::ReflectContext* context);

                Name m_passName;
                uint64_t m_timestampResultInNanoseconds;
            };

            AZ_TYPE_INFO(TimestampSerializer, "{FAAD85C2-5948-4D81-B54A-53502D69CBC0}");
            static void Reflect(AZ::ReflectContext* context);

            TimestampSerializer() = default;
            TimestampSerializer(AZStd::vector<const RPI::Pass*>&& pass);

            AZStd::vector<TimestampSerializerEntry> m_timestampEntries;
        };

        // Intermediate class to serialize pass' PipelineStatistics data.
        class PipelineStatisticsSerializer
        {
        public:
            class PipelineStatisticsSerializerEntry
            {
            public:
                AZ_TYPE_INFO(PipelineStatisticsSerializer::PipelineStatisticsSerializerEntry, "{7CEF130F-555F-4BC0-9A57-E6912F92599F}");
                static void Reflect(AZ::ReflectContext* context);

                Name m_passName;
                RPI::PipelineStatisticsResult m_pipelineStatisticsResult;
            };

            AZ_TYPE_INFO(PipelineStatisticsSerializer, "{4972BAB6-98FB-4D3B-9EAC-50FF418E77C0}");
            static void Reflect(AZ::ReflectContext* context);

            PipelineStatisticsSerializer() = default;
            PipelineStatisticsSerializer(AZStd::vector<const RPI::Pass*>&& passes);

            AZStd::vector<PipelineStatisticsSerializerEntry> m_pipelineStatisticsEntries;
        };

        // Intermediate class to serialize Cpu TimedRegion data.
        class CpuProfilingStatisticsSerializer
        {
        public:
            class CpuProfilingStatisticsSerializerEntry
            {
            public:
                AZ_TYPE_INFO(CpuProfilingStatisticsSerializer::CpuProfilingStatisticsSerializerEntry, "{26B78F65-EB96-46E2-BE7E-A1233880B225}");
                static void Reflect(AZ::ReflectContext* context);

                CpuProfilingStatisticsSerializerEntry() = default;
                CpuProfilingStatisticsSerializerEntry(const RHI::CachedTimeRegion& cachedTimeRegion);

            private:
                Name m_groupName;
                Name m_regionName;
                uint16_t m_stackDepth;
                AZStd::sys_time_t m_elapsedInNanoseconds;
            };

            AZ_TYPE_INFO(CpuProfilingStatisticsSerializer, "{D5B02946-0D27-474F-9A44-364C2706DD41}");
            static void Reflect(AZ::ReflectContext* context);

            CpuProfilingStatisticsSerializer() = default;
            CpuProfilingStatisticsSerializer(const RHI::CpuProfiler::TimeRegionMap& timeRegionMap);

            AZStd::vector<CpuProfilingStatisticsSerializerEntry> m_cpuProfilingStatisticsSerializerEntries;
        };

        // Intermediate class to serialize benchmark metadata.
        class BenchmarkMetadataSerializer
        {
        public:
            class GpuEntry
            {
            public:
                AZ_TYPE_INFO(Render::BenchmarkMetadataSerializer::GpuEntry, "{3D5C2DDE-59FB-4E28-9605-D2A083E34505}");
                static void Reflect(AZ::ReflectContext* context);

                GpuEntry() = default;
                GpuEntry(const RHI::PhysicalDeviceDescriptor& descriptor);

            private:
                AZStd::string m_description;
                uint32_t m_driverVersion;
            };

            AZ_TYPE_INFO(Render::BenchmarkMetadataSerializer, "{2BC41B6F-528F-4E59-AEDA-3B9D74E323EC}");
            static void Reflect(AZ::ReflectContext* context);

            BenchmarkMetadataSerializer() = default;
            BenchmarkMetadataSerializer(const AZStd::string& benchmarkName, const RHI::PhysicalDeviceDescriptor& gpuDescriptor);

            AZStd::string m_benchmarkName;
            GpuEntry m_gpuEntry;
        };

        // --- DelayedQueryCaptureHelper ---

        bool DelayedQueryCaptureHelper::StartCapture(CaptureCallback&& captureCallback)
        {
            if (m_state != DelayedCaptureState::Idle)
            {
                AZ_Warning("DelayedQueryCaptureHelper", false, "State is not set to idle, another process is in a pending state.");
                return false;
            }

            m_state = DelayedCaptureState::Pending;
            m_captureCallback = captureCallback;
            m_frameThreshold = FrameThreshold;

            return true;
        }

        void DelayedQueryCaptureHelper::UpdateCapture()
        {
            if (m_state == DelayedCaptureState::Pending)
            {
                m_frameThreshold--;

                if (m_frameThreshold == 0u)
                {
                    m_captureCallback();
                    m_state = DelayedCaptureState::Idle;
                }
            }
        }

        bool DelayedQueryCaptureHelper::IsIdle() const
        {
            return m_state == DelayedCaptureState::Idle;
        }

        // --- TimestampSerializer ---

        TimestampSerializer::TimestampSerializer(AZStd::vector<const RPI::Pass*>&& passes)
        {
            for (const RPI::Pass* pass : passes)
            {
                m_timestampEntries.push_back({pass->GetName(), pass->GetLatestTimestampResult().GetDurationInNanoseconds()});
            }
        }

        void TimestampSerializer::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TimestampSerializer>()
                    ->Version(1)
                    ->Field("timestampEntries", &TimestampSerializer::m_timestampEntries)
                    ;
            }

            TimestampSerializerEntry::Reflect(context);
        }

        // --- TimestampSerializerEntry ---

        void TimestampSerializer::TimestampSerializerEntry::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TimestampSerializerEntry>()
                    ->Version(1)
                    ->Field("passName", &TimestampSerializerEntry::m_passName)
                    ->Field("timestampResultInNanoseconds", &TimestampSerializerEntry::m_timestampResultInNanoseconds)
                    ;
            }
        }

        // --- PipelineStatisticsSerializer ---

        PipelineStatisticsSerializer::PipelineStatisticsSerializer(AZStd::vector<const RPI::Pass*>&& passes)
        {
            for (const RPI::Pass* pass : passes)
            {
                m_pipelineStatisticsEntries.push_back({pass->GetName(), pass->GetLatestPipelineStatisticsResult()});
            }
        }

        void PipelineStatisticsSerializer::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PipelineStatisticsSerializer>()
                    ->Version(1)
                    ->Field("pipelineStatisticsEntries", &PipelineStatisticsSerializer::m_pipelineStatisticsEntries)
                    ;
            }

            PipelineStatisticsSerializerEntry::Reflect(context);
        }

        // --- PipelineStatisticsSerializerEntry ---

        void PipelineStatisticsSerializer::PipelineStatisticsSerializerEntry::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PipelineStatisticsSerializerEntry>()
                    ->Version(1)
                    ->Field("passName", &PipelineStatisticsSerializerEntry::m_passName)
                    ->Field("pipelineStatisticsResult", &PipelineStatisticsSerializerEntry::m_pipelineStatisticsResult)
                    ;
            }
        }

        // --- CpuProfilingStatisticsSerializer ---

        CpuProfilingStatisticsSerializer::CpuProfilingStatisticsSerializer(const RHI::CpuProfiler::TimeRegionMap& timeRegionMap)
        {
            // Create serializable entries
            for (auto& threadEntry : timeRegionMap)
            {
                for (auto& cachedRegionEntry : threadEntry.second)
                {
                    m_cpuProfilingStatisticsSerializerEntries.insert(
                        m_cpuProfilingStatisticsSerializerEntries.end(),
                        cachedRegionEntry.second.begin(),
                        cachedRegionEntry.second.end());
                }
            }
        }

        void CpuProfilingStatisticsSerializer::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<CpuProfilingStatisticsSerializer>()
                    ->Version(1)
                    ->Field("cpuProfilingStatisticsSerializerEntry", &CpuProfilingStatisticsSerializer::m_cpuProfilingStatisticsSerializerEntries)
                    ;
            }

            CpuProfilingStatisticsSerializerEntry::Reflect(context);
        }

        // --- CpuProfilingStatisticsSerializerEntry ---

        CpuProfilingStatisticsSerializer::CpuProfilingStatisticsSerializerEntry::CpuProfilingStatisticsSerializerEntry(const RHI::CachedTimeRegion& cachedTimeRegion)
        {
            // Converts ticks to Nanoseconds
            static const auto ticksToNanoSeconds = [](AZStd::sys_time_t elapsedInTicks) -> AZStd::sys_time_t
            {
                const AZStd::sys_time_t ticksPerSecond = AZStd::GetTimeTicksPerSecond();

                const AZStd::sys_time_t timeInNanoseconds = (elapsedInTicks * 1000000) / (ticksPerSecond / 1000);
                return timeInNanoseconds;
            };

            m_groupName = cachedTimeRegion.m_groupRegionName->m_groupName;
            m_regionName = cachedTimeRegion.m_groupRegionName->m_regionName;
            m_stackDepth = cachedTimeRegion.m_stackDepth;
            m_elapsedInNanoseconds = ticksToNanoSeconds(cachedTimeRegion.m_endTick - cachedTimeRegion.m_startTick);
        }

        void CpuProfilingStatisticsSerializer::CpuProfilingStatisticsSerializerEntry::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<CpuProfilingStatisticsSerializerEntry>()
                    ->Version(1)
                    ->Field("groupName", &CpuProfilingStatisticsSerializerEntry::m_groupName)
                    ->Field("regionName", &CpuProfilingStatisticsSerializerEntry::m_regionName)
                    ->Field("stackDepth", &CpuProfilingStatisticsSerializerEntry::m_stackDepth)
                    ->Field("elapsedInNanoseconds", &CpuProfilingStatisticsSerializerEntry::m_elapsedInNanoseconds)
                    ;
            }
        }

        // --- BenchmarkMetadataSerializer ---

        BenchmarkMetadataSerializer::BenchmarkMetadataSerializer(const AZStd::string& benchmarkName, const RHI::PhysicalDeviceDescriptor& gpuDescriptor)
        {
            m_benchmarkName = benchmarkName;
            m_gpuEntry = GpuEntry(gpuDescriptor);
        }

        void BenchmarkMetadataSerializer::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BenchmarkMetadataSerializer>()
                    ->Version(1)
                    ->Field("benchmarkName", &BenchmarkMetadataSerializer::m_benchmarkName)
                    ->Field("gpuInfo", &BenchmarkMetadataSerializer::m_gpuEntry)
                    ;
            }

            GpuEntry::Reflect(context);
        }

        // --- GpuEntry ---

        BenchmarkMetadataSerializer::GpuEntry::GpuEntry(const RHI::PhysicalDeviceDescriptor& descriptor)
        {
            m_description = descriptor.m_description;
            m_driverVersion = descriptor.m_driverVersion;
        }


        void BenchmarkMetadataSerializer::GpuEntry::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<GpuEntry>()
                    ->Version(1)
                    ->Field("description", &GpuEntry::m_description)
                    ->Field("driverVersion", &GpuEntry::m_driverVersion)
                    ;
            }
        }

        // --- ProfilingCaptureSystemComponent ---

        void ProfilingCaptureSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ProfilingCaptureSystemComponent, AZ::Component>()
                    ->Version(1)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<ProfilingCaptureRequestBus>("ProfilingCaptureRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "atom")
                    ->Event("CapturePassTimestamp", &ProfilingCaptureRequestBus::Events::CapturePassTimestamp)
                    ->Event("CapturePassPipelineStatistics", &ProfilingCaptureRequestBus::Events::CapturePassPipelineStatistics)
                    ->Event("CaptureCpuProfilingStatistics", &ProfilingCaptureRequestBus::Events::CaptureCpuProfilingStatistics)
                    ->Event("CaptureBenchmarkMetadata", &ProfilingCaptureRequestBus::Events::CaptureBenchmarkMetadata)
                    ;

                ProfilingCaptureNotificationBusHandler::Reflect(context);
            }

            TimestampSerializer::Reflect(context);
            PipelineStatisticsSerializer::Reflect(context);
            CpuProfilingStatisticsSerializer::Reflect(context);
            BenchmarkMetadataSerializer::Reflect(context);
        }

        void ProfilingCaptureSystemComponent::Activate()
        {
            ProfilingCaptureRequestBus::Handler::BusConnect();
        }

        void ProfilingCaptureSystemComponent::Deactivate()
        {
            TickBus::Handler::BusDisconnect();

            ProfilingCaptureRequestBus::Handler::BusDisconnect();
        }

        bool ProfilingCaptureSystemComponent::CapturePassTimestamp(const AZStd::string& outputFilePath)
        {
            // Find the root pass.
            AZStd::vector<RPI::Pass*> passes = FindPasses({ "Root" });
            if (passes.empty())
            {
                return false;
            }

            RPI::Pass* root = passes[0];

            // Enable all the Timestamp queries in passes.
            root->SetTimestampQueryEnabled(true);

            const bool captureStarted = m_timestampCapture.StartCapture([this, root, outputFilePath]()
            {
                JsonSerializerSettings serializationSettings;
                serializationSettings.m_keepDefaults = true;

                TimestampSerializer timestampSerializer(CollectPassesRecursively(root));
                const auto saveResult = JsonSerializationUtils::SaveObjectToFile(&timestampSerializer,
                    outputFilePath, (TimestampSerializer*)nullptr, &serializationSettings);

                AZStd::string captureInfo = outputFilePath;
                if (!saveResult.IsSuccess())
                {
                    captureInfo = AZStd::string::format("Failed to save pass' Timestamps to file '%s'. Error: %s",
                        outputFilePath.c_str(),
                        saveResult.GetError().c_str());
                    AZ_Warning("ProfilingCaptureSystemComponent", false, captureInfo.c_str());
                }

                // Disable all the Timestamp queries in passes.
                root->SetTimestampQueryEnabled(false);

                // Notify listeners that the pass' Timestamp queries capture has finished.
                ProfilingCaptureNotificationBus::Broadcast(&ProfilingCaptureNotificationBus::Events::OnCaptureQueryTimestampFinished,
                    saveResult.IsSuccess(),
                    captureInfo);
            });

            // Start the TickBus.
            if (captureStarted)
            {
                TickBus::Handler::BusConnect();
            }

            return captureStarted;
        }

        bool ProfilingCaptureSystemComponent::CapturePassPipelineStatistics(const AZStd::string& outputFilePath)
        {
            // Find the root pass.
            AZStd::vector<RPI::Pass*> passes = FindPasses({ "Root" });
            if (passes.empty())
            {
                return false;
            }

            RPI::Pass* root = passes[0];

            // Enable all the PipelineStatistics queries in passes.
            root->SetPipelineStatisticsQueryEnabled(true);

            const bool captureStarted = m_pipelineStatisticsCapture.StartCapture([this, root, outputFilePath]()
            {
                JsonSerializerSettings serializationSettings;
                serializationSettings.m_keepDefaults = true;

                PipelineStatisticsSerializer pipelineStatisticsSerializer(CollectPassesRecursively(root));
                const auto saveResult = JsonSerializationUtils::SaveObjectToFile(&pipelineStatisticsSerializer,
                    outputFilePath, (PipelineStatisticsSerializer*)nullptr, &serializationSettings);

                AZStd::string captureInfo = outputFilePath;
                if (!saveResult.IsSuccess())
                {
                    captureInfo = AZStd::string::format("Failed to save pass' PipelineStatistics to file '%s'. Error: %s",
                        outputFilePath.c_str(),
                        saveResult.GetError().c_str());
                    AZ_Warning("ProfilingCaptureSystemComponent", false, captureInfo.c_str());
                }

                // Disable all the PipelineStatistics queries in passes.
                root->SetPipelineStatisticsQueryEnabled(false);

                // Notify listeners that the pass' PipelineStatistics queries capture has finished.
                ProfilingCaptureNotificationBus::Broadcast(&ProfilingCaptureNotificationBus::Events::OnCaptureQueryPipelineStatisticsFinished,
                    saveResult.IsSuccess(),
                    captureInfo);
            });

            // Start the TickBus.
            if (captureStarted)
            {
                TickBus::Handler::BusConnect();
            }

            return captureStarted;
        }

        bool ProfilingCaptureSystemComponent::CaptureCpuProfilingStatistics(const AZStd::string& outputFilePath)
        {
            // Start the cpu profiling
            bool wasEnabled = RHI::CpuProfiler::Get()->IsProfilerEnabled();
            if (!wasEnabled)
            {
                RHI::CpuProfiler::Get()->SetProfilerEnabled(true);
            }

            const bool captureStarted = m_cpuProfilingStatisticsCapture.StartCapture([this, outputFilePath, wasEnabled]()
            {
                JsonSerializerSettings serializationSettings;
                serializationSettings.m_keepDefaults = true;

                // Get time Cpu profiled time regions
                const RHI::CpuProfiler::TimeRegionMap& timeRegionMap = RHI::CpuProfiler::Get()->GetTimeRegionMap();

                CpuProfilingStatisticsSerializer serializer(timeRegionMap);
                const auto saveResult = JsonSerializationUtils::SaveObjectToFile(&serializer,
                    outputFilePath, (CpuProfilingStatisticsSerializer*)nullptr, &serializationSettings);

                AZStd::string captureInfo = outputFilePath;
                if (!saveResult.IsSuccess())
                {
                    captureInfo = AZStd::string::format("Failed to save Cpu Profiling Statistics data to file '%s'. Error: %s",
                        outputFilePath.c_str(),
                        saveResult.GetError().c_str());
                    AZ_Warning("ProfilingCaptureSystemComponent", false, captureInfo.c_str());
                }
                else
                {
                    AZ_Printf("ProfilingCaptureSystemComponent", "Cpu profiling statistics was saved to file [%s]\n", outputFilePath.c_str());
                }

                // Disable the profiler again
                if (!wasEnabled)
                {
                    RHI::CpuProfiler::Get()->SetProfilerEnabled(false);
                }

                // Notify listeners that the pass' PipelineStatistics queries capture has finished.
                ProfilingCaptureNotificationBus::Broadcast(&ProfilingCaptureNotificationBus::Events::OnCaptureCpuProfilingStatisticsFinished,
                    saveResult.IsSuccess(),
                    captureInfo);

            });

            // Start the TickBus.
            if (captureStarted)
            {
                TickBus::Handler::BusConnect();
            }

            return captureStarted;
        }

        bool ProfilingCaptureSystemComponent::CaptureBenchmarkMetadata(const AZStd::string& benchmarkName, const AZStd::string& outputFilePath)
        {
            const bool captureStarted = m_benchmarkMetadataCapture.StartCapture([this, benchmarkName, outputFilePath]()
            {
                JsonSerializerSettings serializationSettings;
                serializationSettings.m_keepDefaults = true;

                const RHI::PhysicalDeviceDescriptor& gpuDescriptor = RHI::GetRHIDevice()->GetPhysicalDevice().GetDescriptor();

                BenchmarkMetadataSerializer serializer(benchmarkName, gpuDescriptor);
                const auto saveResult = JsonSerializationUtils::SaveObjectToFile(&serializer,
                    outputFilePath, (BenchmarkMetadataSerializer*)nullptr, &serializationSettings);

                AZStd::string captureInfo = outputFilePath;
                if (!saveResult.IsSuccess())
                {
                    captureInfo = AZStd::string::format("Failed to save benchmark metadata data to file '%s'. Error: %s",
                        outputFilePath.c_str(),
                        saveResult.GetError().c_str());
                    AZ_Warning("ProfilingCaptureSystemComponent", false, captureInfo.c_str());
                }

                // Notify listeners that the benchmark metadata capture has finished.
                ProfilingCaptureNotificationBus::Broadcast(&ProfilingCaptureNotificationBus::Events::OnCaptureBenchmarkMetadataFinished,
                    saveResult.IsSuccess(),
                    captureInfo);
            });

            // Start the TickBus.
            if (captureStarted)
            {
                TickBus::Handler::BusConnect();
            }

            return captureStarted;
        }

        AZStd::vector<const RPI::Pass*> ProfilingCaptureSystemComponent::CollectPassesRecursively(const RPI::Pass* root) const
        {
            AZStd::vector<const RPI::Pass*> passes;
            AZStd::function<void(const RPI::Pass*)> collectPass = [&](const RPI::Pass* pass)
            {
                passes.push_back(pass);

                const RPI::ParentPass* asParent = pass->AsParent();
                if (asParent)
                {
                    for (const auto& child : asParent->GetChildren())
                    {
                        collectPass(child.get());
                    }
                }
            };

            collectPass(root);

            return passes;
        }

        AZStd::vector<RPI::Pass*> ProfilingCaptureSystemComponent::FindPasses(AZStd::vector<AZStd::string>&& passHierarchy) const
        {
            // Find the pass first.
            RPI::PassHierarchyFilter passFilter(passHierarchy);
            AZStd::vector<AZ::RPI::Pass*> foundPasses = AZ::RPI::PassSystemInterface::Get()->FindPasses(passFilter);
            if (foundPasses.size() == 0)
            {
                AZ_Warning("ProfilingCaptureSystemComponent", false, "Failed to find pass from %s", passFilter.ToString().c_str());
            }

            return foundPasses;
        }

        void ProfilingCaptureSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] ScriptTimePoint time)
        {
            // Update the delayed captures
            m_timestampCapture.UpdateCapture();
            m_pipelineStatisticsCapture.UpdateCapture();
            m_cpuProfilingStatisticsCapture.UpdateCapture();
            m_benchmarkMetadataCapture.UpdateCapture();

            // Disconnect from the TickBus if all capture states are set to idle.
            if (m_timestampCapture.IsIdle() && m_pipelineStatisticsCapture.IsIdle() && m_cpuProfilingStatisticsCapture.IsIdle() && m_benchmarkMetadataCapture.IsIdle())
            {
                TickBus::Handler::BusDisconnect();
            }
        }

    }
}
