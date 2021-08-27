/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/atomic.h>

namespace AWSMetrics
{
    constexpr char AwsMetricsLocalFileDir[] = "@user@/AWSMetrics/";
    constexpr char AwsMetricsLocalFileName[] = "metrics.json";

    //! ClientConfiguration is used to retrieve and store client settings from a local configuration JSON file.
    class ClientConfiguration
    {
    public:
        static constexpr const char AWSMetricsMaxQueueSizeInMbKey[] = "/Gems/AWSMetrics/MaxQueueSizeInMb";
        static constexpr const char AWSMetricsQueueFlushPeriodInSecondsKey[] = "/Gems/AWSMetrics/QueueFlushPeriodInSeconds";
        static constexpr const char AWSMetricsOfflineRecordingEnabledKey[] = "/Gems/AWSMetrics/OfflineRecording";
        static constexpr const char AWSMetricsMaxNumRetriesKey[] = "/Gems/AWSMetrics/MaxNumRetries";
        
        ClientConfiguration();

        //! Initialize the client settings based on the global setting registry.
        //! @return whether the operation is successful
        bool InitClientConfiguration();

        //! Retrieve the max queue size setting.
        //! @return Max queue size in bytes.
        AZ::s64 GetMaxQueueSizeInBytes() const;

        //! Retrieve the flush period setting.
        //! @return Flush period in seconds.
        AZ::s64 GetQueueFlushPeriodInSeconds() const;

        //! Status of the offline recording. Metrics will be sent to a local file instead of the backend if the offline recording is enabled.
        //! @return Whether the offline recording is enabled.
        bool OfflineRecordingEnabled() const;

        //! Retrieve the settings for the maximum number of retries.
        //! @return Maximum number of retries.
        AZ::s64 GetMaxNumRetries() const;

        //! Retrieve the directory of the local metrics file
        //! @return Directory of the local metrics file
        const char* GetMetricsFileDir() const;

        //! Retrieve the full path of the local metrics file
        //! @return Full path of the local metrics file
        const char* GetMetricsFileFullPath() const;

        //! Enable/Disable the offline recording.
        //! @param enable Whether to enable the offline recording.
        void UpdateOfflineRecordingStatus(bool enable);

    private:
        bool ResolveMetricsFilePath();

        double m_maxQueueSizeInMb; //< Default to 0.3MB on consideration of the Kinesis PutRecordBatch API limit (500 records/request).
        AZ::s64 m_queueFlushPeriodInSeconds; //< Default to 60 seconds to guarantee the near real time data input.
        AZStd::atomic_bool m_offlineRecordingEnabled; //< Default to false to disable the offline recording.
        AZ::s64 m_maxNumRetries; //< Maximum number of retries for submission.

        AZStd::string m_metricsDir;
        AZStd::string m_metricsFilePath;
    };   
}
