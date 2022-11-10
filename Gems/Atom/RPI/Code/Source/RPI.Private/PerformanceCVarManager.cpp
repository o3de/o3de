/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzCore/Debug/PerformanceCollector.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/GpuQuery/GpuPassProfiler.h>
#include "PerformanceCVarManager.h"

namespace AZ
{
    namespace RPI
    {
        AZ_CVAR(AZ::CVarFixedString, r_metricsDataLogType,
            "statistical", // (s)(S)tatistical Summary (average, min, max, stdev) / (a)(A)ll Samples. Default=s.
            nullptr, //&PerformanceCvarManager::OnDataLogTypeChanged,
            ConsoleFunctorFlags::DontReplicate,
            "Defines the kind of data collection and logging. If starts with 's' it will log statistical summaries, if starts with 'a' will log each sample of data (high verbosity).");

        AZ_CVAR(AZ::u32, r_metricsWaitTimePerCaptureBatch,
            0, // Starts at 0, which means "do not capture performance data". When this variable changes to >0 we'll start performance capture.
            nullptr, //&PerformanceCvarManager::OnWaitTimePerCaptureBatchChanged,
            ConsoleFunctorFlags::DontReplicate,
            "How many seconds to wait before each batch of performance capture.");

        AZ_CVAR(AZ::u32, r_metricsFrameCountPerCaptureBatch,
            1200, // Number of frames in which performance will be measured per batch.
            nullptr, //&PerformanceCvarManager::OnFrameCountPerCaptureBatchChanged,
            ConsoleFunctorFlags::DontReplicate,
            "Number of frames in which performance will be measured per batch.");

        // "Frame Gpu Time" is the only gated performance metric because it takes a considerable amount
        // of CPU time. For example, when NOT capturing this metric, the default level runs at 300fps on a GTX 3070,
        // if this metric is enabled, the FPS drops to 270. It is recommended that this metric is captured in isolation
        // so it doesn't affect the results of the "Engine Cpu Time" metric.
        AZ_CVAR(bool, r_metricsMeasureGpuTime,
            false,
            nullptr,
            ConsoleFunctorFlags::DontReplicate,
            "If true, The Frame Gpu Time is measured. By default it is false, as this measurement is CPU expensive.");

        AZ_CVAR(bool, r_metricsQuitUponCompletion,
            false, // If true the application will quit when Number Of Capture Batches reaches 0.
            nullptr,
            ConsoleFunctorFlags::DontReplicate,
            "If true the application will quit when Number Of Capture Batches reaches 0.");

        AZ::Debug::PerformanceCollector::DataLogType GetDataLogTypeFromCVar(const AZ::CVarFixedString& newCaptureType)
        {
            if (newCaptureType[0] == 'a' || newCaptureType[0] == 'A')
            {
                return AZ::Debug::PerformanceCollector::DataLogType::LogAllSamples;
            }
            return AZ::Debug::PerformanceCollector::DataLogType::LogStatistics;
        }

        class PerformanceCvarManager
        {
        public:
            static constexpr char LogName[] = "RPIPerformanceCvarManager";

            static void OnNumberOfCaptureBatchesChanged(const AZ::u32& newValue)
            {
                AZ_TracePrintf(LogName, "%s cvar changed to %u.\n", __FUNCTION__, newValue);
                auto performanceCollectorOwner = PerformanceCollectorOwner::Get();
                if (!performanceCollectorOwner)
                {
                    return;
                }
                auto performanceCollector = performanceCollectorOwner->GetPerformanceCollector();
                if (!performanceCollector)
                {
                    return;
                }

                // For diagnostics purposes write to the log the state of the CVars involved
                // in Graphics performance collection.
                CVarFixedString metricsDataLogType = r_metricsDataLogType;
                AZ_TracePrintf(LogName, "%s r_metricsDataLogType=%s.\n", __FUNCTION__, metricsDataLogType.c_str());
                AZ::u32 metricsWaitTimePerCaptureBatch = r_metricsWaitTimePerCaptureBatch;
                AZ_TracePrintf(LogName, "%s r_metricsWaitTimePerCaptureBatch=%u.\n", __FUNCTION__, metricsWaitTimePerCaptureBatch);
                AZ::u32 metricsFrameCountPerCaptureBatch = r_metricsFrameCountPerCaptureBatch;
                AZ_TracePrintf(LogName, "%s r_metricsFrameCountPerCaptureBatch=%u.\n", __FUNCTION__, metricsFrameCountPerCaptureBatch);
                bool metricsMeasureGpuTime = r_metricsMeasureGpuTime;
                AZ_TracePrintf(LogName, "%s value of r_metricsMeasureGpuTime=%s.\n", __FUNCTION__, metricsMeasureGpuTime ? "true" : "false");
                [[maybe_unused]] bool metricsQuitUponCompletion = r_metricsQuitUponCompletion;
                AZ_TracePrintf(LogName, "%s value of r_metricsQuitUponCompletion=%s.\n", __FUNCTION__, metricsQuitUponCompletion ? "true" : "false");

                auto gpuPassProfiler = performanceCollectorOwner->GetGpuPassProfiler();
                if (gpuPassProfiler)
                {
                    gpuPassProfiler->SetGpuTimeMeasurementEnabled(metricsMeasureGpuTime);
                }
                performanceCollector->UpdateDataLogType(GetDataLogTypeFromCVar(metricsDataLogType));
                performanceCollector->UpdateWaitTimeBeforeEachBatch(AZStd::chrono::seconds(metricsWaitTimePerCaptureBatch));
                performanceCollector->UpdateFrameCountPerCaptureBatch(metricsFrameCountPerCaptureBatch);
                performanceCollector->UpdateNumberOfCaptureBatches(newValue);
            }

        };

        AZ_CVAR(AZ::u32, r_metricsNumberOfCaptureBatches,
            0, // Starts at 0, which means "do not capture performance data". When this variable changes to >0 we'll start performance capture.
            &PerformanceCvarManager::OnNumberOfCaptureBatchesChanged,
            ConsoleFunctorFlags::DontReplicate,
            "Collects and reports graphics performance in this number of batches.");

    } // namespace RPI
} // namespace AZ
