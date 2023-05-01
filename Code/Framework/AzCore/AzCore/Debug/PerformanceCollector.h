/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Metrics/JsonTraceEventLogger.h>
#include <AzCore/Statistics/StatisticsManager.h>
#include <AzCore/std/chrono/chrono.h>

namespace AZ::Debug
{
    //! A helper class that facilitates collecting performance metrics as part of blocks
    //! of code, or measuring time lapses of periodically called functions.
    //! The metrics can be recorded as raw events (DataLogType::LogAllSamples)
    //! or aggregated as statistical summaries (DataLogType::LogStatistics).
    //! Performance is captured in batches of frames.
    class PerformanceCollector final
    {
    public:
        PerformanceCollector() = delete;

        //! Common properties found in the "args" dictionary of each Google Trace Json Row:
        static constexpr AZStd::string_view AVG = "avg";
        static constexpr AZStd::string_view MIN = "min";
        static constexpr AZStd::string_view MAX = "max";
        static constexpr AZStd::string_view SAMPLE_COUNT = "sampleCount";
        static constexpr AZStd::string_view UNITS = "units";
        static constexpr AZStd::string_view VARIANCE = "variance";
        static constexpr AZStd::string_view STDEV = "stdev";
        static constexpr AZStd::string_view MOST_RECENT_SAMPLE = "mostRecentSampleValue";

        //! Function signature for the notification callback that will be dispatched
        //! each time a batch of frames are measured.
        using OnBatchCompleteCallback = AZStd::function<void(AZ::u32 pendingBatchCount)>;

        //! All metrics that will ever be recorded must be declared at construction time
        //! of the performance collector.
        //! @param logCategory Category name that will be used in the Google Trace output json file.
        //!                    Each output file will be named Performance_<Category>_<CreationTime>.json
        //! @param m_metricNames List of all the metrics that will be recorded. All metrics will be measured in Microseconds.
        //! @param onBatchCompleteCallback See comments above in OnBatchCompleteCallback declaration.
        //! @param m_fileExtension The extension of the output file, to appear after "." Defaults to "json".
        PerformanceCollector(
            const AZStd::string_view logCategory,
            AZStd::span<const AZStd::string_view> m_metricNames,
            OnBatchCompleteCallback onBatchCompleteCallback,
            const AZStd::string_view fileExtension = "json");
        ~PerformanceCollector() = default;

        static constexpr char LogName[] = "PerformanceCollector";

        enum class DataLogType
        {
            LogStatistics, //! Aggregates each sampled data using the StatiscalProfiler API.
                           //! When done, a single record with statistical summary is dumped in the output file using
                           //! the IEventLogger API.
            LogAllSamples, //! Each sample becomes a unique record in the output file using the IEventLogger API.
        };

        //! Returns true if the user has disabled performance capture or
        //! the performance collector is waiting for certain amount of time
        //! before starting to measure performance.
        bool IsWaitingBeforeCapture();

        //! Records a measured value according to the current CaptureType.
        void RecordSample(AZStd::string_view metricName, AZStd::chrono::microseconds microSeconds);

        //! This is similar to RecordSample(). Captures the elapsed time between
        //! two consecutive calls to this function for any given @metricName.
        //! The time delta is recorded according to the current CaptureType.
        void RecordPeriodicEvent(AZStd::string_view metricName);

        //! The user of the API must call this function each frame. This is
        //! where this class performs book keeping and decides when to flush
        //! data into the output files, etc.
        void FrameTick();

        //! Updates the kind of data collection and reporting. See @DataLogType for details.
        //! This function logs a warning and does nothing if a set of performance capture batches
        //! is already in effect.
        void UpdateDataLogType(DataLogType newValue);

        //! Updates the number of frames that will be profiled per batch.
        //! This function logs a warning and does nothing if a set of performance capture batches
        //! is already in effect.
        void UpdateFrameCountPerCaptureBatch(AZ::u32 newValue);

        //! Updates the amount of time to wait, in seconds, before each batch starts.
        //! This function logs a warning and does nothing if a set of performance capture batches
        //! is already in effect.
        void UpdateWaitTimeBeforeEachBatch(AZStd::chrono::seconds seconds);

        //! Calling this one with newValue > 0 will trigger json file creation
        //! and performance capture for as many batches.
        void UpdateNumberOfCaptureBatches(AZ::u32 newValue);

        const AZ::IO::Path& GetOutputFilePath() const { return m_outputFilePath;  }

        const AZStd::string& GetOutputDataBuffer() const { return m_outputDataBuffer; }

        const AZStd::string& GetFileExtension() const { return m_fileExtension; }

    private:
        //! A helper function that loops across all statistics in @m_statisticsManager
        //! and reports each result into @m_eventLogger.
        void RecordStatistics();

        void RestartPeriodicEventStamps();

        void CreateOutputJsonFile();

        //! Main control parameters. Usually mirrored by CVARs. They can change at runtime.
        AZ::u32 m_numberOfCaptureBatches = 0; // A number greater than 0 starts performance collection.
        DataLogType m_dataLogType = DataLogType::LogStatistics; // Defines the data collection and report mode.
        AZ::u32 m_frameCountPerCaptureBatch = 50; // How many frames of data will be capture per batch.
        AZStd::chrono::seconds m_waitTimeBeforeEachBatch = AZStd::chrono::seconds(3); // How many seconds to wait before starting the next batch of data capture.

        AZStd::string m_logCategory;
        AZ::u32 m_frameCountInCurrentBatch = 0; // How many frames have been captured so far within the current batch.
        AZStd::chrono::steady_clock::time_point m_startWaitTime;
        bool m_isWaitingBeforeNextBatch = true;
        OnBatchCompleteCallback m_onBatchCompleteCallback; // A notification will be sent each time a batch of frames is performance collected.
        AZStd::string m_fileExtension = "json";

        //! Only used when @m_captureType == CaptureType::LogStatistics.
        AZ::Statistics::StatisticsManager<AZStd::string> m_statisticsManager;

        //! Only used to store the previous value when RecordPeriodicEvent() is called
        //! for any given metrics.
        AZStd::unordered_map<AZStd::string, AZStd::chrono::steady_clock::time_point> m_periodicEventStamps;

        //! In some circumstances, like running under UnitTests, an output file won't be created.
        //! Instead the output data will be streamed into this buffer.
        AZStd::string m_outputDataBuffer;

        AZ::IO::Path m_outputFilePath; // We store here the file path of the most recently created output file.
        Metrics::JsonTraceEventLogger m_eventLogger;
    }; // class PerformanceCollector

    //! A Convenience class used to measure time performance of scopes of code
    //! with constructor/destructor.
    class ScopeDuration
    {
    public:
        ScopeDuration() = delete;

        ScopeDuration(PerformanceCollector* performanceCollector, const AZStd::string_view metricName)
            : m_performanceCollector(performanceCollector)
            , m_metricName(metricName)
        {
            if (!performanceCollector)
            {
                // This is normal. No need to assert.
                return;
            }
            m_pushSample = !performanceCollector->IsWaitingBeforeCapture();
            if (m_pushSample)
            {
                m_startTime = AZStd::chrono::steady_clock::now();
            }
        }

        ~ScopeDuration()
        {
            if (!m_performanceCollector || !m_pushSample)
            {
                // This is normal. No need to assert.
                return;
            }
            AZStd::chrono::steady_clock::time_point stopTime = AZStd::chrono::steady_clock::now();
            auto duration = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(stopTime - m_startTime);
            m_performanceCollector->RecordSample(m_metricName, duration);
        }

    private:
        bool m_pushSample;
        PerformanceCollector* m_performanceCollector;
        const AZStd::string_view m_metricName;
        AZStd::chrono::steady_clock::time_point m_startTime;
    }; // class ScopeDuration

}; // namespace AZ::Debug
