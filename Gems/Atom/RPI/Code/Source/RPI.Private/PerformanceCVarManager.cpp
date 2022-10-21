/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzCore/Debug/PerformanceCollector.h>

#include "PerformanceCVarManager.h"

namespace AZ
{
    namespace RPI
    {

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

            static void OnDataLogTypeChanged(const AZ::CVarFixedString& newCaptureType)
            {
                AZ_TracePrintf(LogName, "%s cvar changed to %s.\n", __FUNCTION__, newCaptureType.c_str());
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
                performanceCollector->UpdateDataLogType(GetDataLogTypeFromCVar(newCaptureType));
            }

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
                performanceCollector->UpdateNumberOfCaptureBatches(newValue);
            }

            static void OnWaitTimePerCaptureBatchChanged(const AZ::u32& newValue)
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
                performanceCollector->UpdateWaitTimeBeforeEachBatch(AZStd::chrono::seconds(newValue));
            }

            static void OnFrameCountPerCaptureBatchChanged(const AZ::u32& newValue)
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
                performanceCollector->UpdateFrameCountPerCaptureBatch(newValue);
            }
        };

        AZ_CVAR(AZ::u32, r_metricsNumberOfCaptureBatches,
            0, // Starts at 0, which means "do not capture performance data". When this variable changes to >0 we'll start performance capture.
            &PerformanceCvarManager::OnNumberOfCaptureBatchesChanged,
            ConsoleFunctorFlags::DontReplicate,
            "Collects and reports graphics performance in this number of batches.");

        AZ_CVAR(AZ::CVarFixedString, r_metricsDataLogType,
            "statistical", // (s)(S)tatistical Summary (average, min, max, stdev) / (a)(A)ll Samples. Default=s.
            &PerformanceCvarManager::OnDataLogTypeChanged,
            ConsoleFunctorFlags::DontReplicate,
            "Defines the kind of data collection and logging. If starts with 's' it will log statistical summaries, if starts with 'a' will log each sample of data (high verbosity).");

        AZ_CVAR(AZ::u32, r_metricsWaitTimePerCaptureBatch,
            0, // Starts at 0, which means "do not capture performance data". When this variable changes to >0 we'll start performance capture.
            &PerformanceCvarManager::OnWaitTimePerCaptureBatchChanged,
            ConsoleFunctorFlags::DontReplicate,
            "How many seconds to wait before each batch of performance capture.");

        AZ_CVAR(AZ::u32, r_metricsFrameCountPerCaptureBatch,
            1200, // Number of frames in which performance will be measured per batch.
            &PerformanceCvarManager::OnFrameCountPerCaptureBatchChanged,
            ConsoleFunctorFlags::DontReplicate,
            "Number of frames in which performance will be measured per batch.");

        AZ_CVAR(bool, r_metricsQuitUponCompletion,
            false, // If true the application will quit when Number Of Capture Batches reaches 0.
            nullptr,
            ConsoleFunctorFlags::DontReplicate,
            "If true the application will quit when Number Of Capture Batches reaches 0.");

    } // namespace RPI
} // namespace AZ
