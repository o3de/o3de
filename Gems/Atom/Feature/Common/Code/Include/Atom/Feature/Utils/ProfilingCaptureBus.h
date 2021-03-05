/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

            //! Dump the PipelineStatistics from passes to a json file.
            virtual bool CapturePassPipelineStatistics(const AZStd::string& outputFilePath) = 0;

            //! Dump the Cpu Profiling Statistics to a json file.
            virtual bool CaptureCpuProfilingStatistics(const AZStd::string& outputFilePath) = 0;

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

            //! Notify when the current PipelineStatistics query capture is finished
            //! @param result Set to true if it's finished successfully
            //! @param info The output file path or error information which depends on the return. 
            virtual void OnCaptureQueryPipelineStatisticsFinished(bool result, const AZStd::string& info) = 0;

            //! Notify when the current CpuProfilingStatistics capture is finished
            //! @param result Set to true if it's finished successfully
            //! @param info The output file path or error information which depends on the return. 
            virtual void OnCaptureCpuProfilingStatisticsFinished(bool result, const AZStd::string& info) = 0;
        };
        using ProfilingCaptureNotificationBus = EBus<ProfilingCaptureNotifications>;

    } // namespace Render
} // namespace AZ
