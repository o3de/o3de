/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    namespace Render
    {
        class ProfilingCaptureRequests
            : public EBusTraits
        {
        public:
            virtual ~ProfilingCaptureRequests() = default;

            //! Dump the Timestamp from passes to a json file.
            virtual bool CapturePassTimestamp(const AZStd::string& outputFilePath) = 0;

            //! Dump the Cpu frame time statistics to a json file.
            virtual bool CaptureCpuFrameTime(const AZStd::string& outputFilePath) = 0;

            //! Dump the PipelineStatistics from passes to a json file.
            virtual bool CapturePassPipelineStatistics(const AZStd::string& outputFilePath) = 0;

            //! Dump the benchmark metadata to a json file.
            virtual bool CaptureBenchmarkMetadata(const AZStd::string& benchmarkName, const AZStd::string& outputFilePath) = 0;
        };
        using ProfilingCaptureRequestBus = EBus<ProfilingCaptureRequests>;

        class ProfilingCaptureNotifications
            : public EBusTraits
        {
        public:
            virtual ~ProfilingCaptureNotifications() = default;

            //! Notify when the current Timestamp query capture is finished
            //! @param result Set to true if it's finished successfully
            //! @param info The output file path or error information which depends on the return.
            virtual void OnCaptureQueryTimestampFinished(bool result, const AZStd::string& info) = 0;

            //! Notify when the current CpuFrameTimeStatistics capture is finished
            //! @param result Set to true if it's finished successfully
            //! @param info The output file path or error information which depends on the return.
            virtual void OnCaptureCpuFrameTimeFinished(bool result, const AZStd::string& info) = 0;

            //! Notify when the current PipelineStatistics query capture is finished
            //! @param result Set to true if it's finished successfully
            //! @param info The output file path or error information which depends on the return.
            virtual void OnCaptureQueryPipelineStatisticsFinished(bool result, const AZStd::string& info) = 0;

            //! Notify when the current BenchmarkMetadata capture is finished
            //! @param result Set to true if it's finished successfully
            //! @param info The output file path or error information which depends on the return.
            virtual void OnCaptureBenchmarkMetadataFinished(bool result, const AZStd::string& info) = 0;
        };
        using ProfilingCaptureNotificationBus = EBus<ProfilingCaptureNotifications>;

    } // namespace Render
} // namespace AZ
