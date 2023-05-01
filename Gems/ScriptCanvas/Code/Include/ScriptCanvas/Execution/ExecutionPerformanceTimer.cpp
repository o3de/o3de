/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/SystemComponent.h>
#include <ScriptCanvas/PerformanceTracker.h>

#include "ExecutionPerformanceTimer.h"

namespace ScriptCanvas
{
    namespace Execution
    {
        PerformanceTracker& ModPerformanceTracker()
        {
            auto tracker = SystemComponent::ModPerformanceTracker();
            AZ_Assert(tracker, "no performance tracker avaialable");
            return *tracker;
        }

        void FinalizePerformanceReport(PerformanceKey key)
        {
            ModPerformanceTracker().FinalizeReport(key);
        }

        PerformanceTimingReport& PerformanceTimingReport::operator+=(const PerformanceTimingReport& rhs)
        {
            initializationTime += rhs.initializationTime;
            executionTime += rhs.executionTime;
            latentTime += rhs.latentTime;
            latentExecutions += rhs.latentExecutions;
            totalTime += rhs.totalTime;
            return *this;
        }

        PerformanceTrackingReport& PerformanceTrackingReport::operator+=(const PerformanceTrackingReport& rhs)
        {
            timing += rhs.timing;
            activationCount += rhs.activationCount;
            return *this;
        }

        PerformanceScope::PerformanceScope(const PerformanceKey& key)
            : m_key(key)
            , m_startTime(AZStd::chrono::steady_clock::now())
        {}

        PerformanceScopeExecution::PerformanceScopeExecution(const PerformanceKey& key)
            : PerformanceScope(key)
        {}

        PerformanceScopeExecution::~PerformanceScopeExecution()
        {
            ModPerformanceTracker().ReportExecutionTime(m_key, AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - m_startTime).count());
        }

        PerformanceScopeInitialization::PerformanceScopeInitialization(const PerformanceKey& key)
            : PerformanceScope(key)
        {}

        PerformanceScopeInitialization::~PerformanceScopeInitialization()
        {
            ModPerformanceTracker().ReportInitializationTime(m_key, AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - m_startTime).count());
        }

        PerformanceScopeLatent::PerformanceScopeLatent(const PerformanceKey& key)
            : PerformanceScope(key)
        {}

        PerformanceScopeLatent::~PerformanceScopeLatent()
        {
            ModPerformanceTracker().ReportLatentTime(m_key, AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - m_startTime).count());
        }

        PerformanceTimer::PerformanceTimer()
        {
            m_initializationTime = m_instantTime = m_latentTime = 0;
        }

        void PerformanceTimer::AddTimeFrom(const PerformanceTimer& source)
        {
            m_initializationTime += source.GetInitializationDurationInMicroseconds();
            m_instantTime += source.GetInstantDurationInMicroseconds();
            m_latentTime += source.GetLatentDurationInMicroseconds();
        }

        void PerformanceTimer::AddExecutionTime(AZStd::sys_time_t time)
        {
            m_instantTime += time;
        }

        void PerformanceTimer::AddLatentTime(AZStd::sys_time_t time)
        {
            ++m_latentExecutions;
            m_latentTime += time;
        }

        void PerformanceTimer::AddInitializationTime(AZStd::sys_time_t time)
        {
            m_initializationTime += time;
        }

        PerformanceTimingReport PerformanceTimer::GetReport() const
        {
            return { m_initializationTime, m_instantTime, m_latentTime, m_latentExecutions, GetTotalDurationInMicroseconds() };
        }

        AZStd::sys_time_t PerformanceTimer::GetInstantDurationInMicroseconds() const
        {
            return m_instantTime;
        }

        double PerformanceTimer::GetInstantDurationInMilliseconds() const
        {
            return m_instantTime / 1000.0;
        }

        AZStd::sys_time_t PerformanceTimer::GetLatentDurationInMicroseconds() const
        {
            return m_latentTime;
        }

        double PerformanceTimer::GetLatentDurationInMilliseconds() const
        {
            return m_latentTime / 1000.0;
        }

        AZ::u32 PerformanceTimer::GetLatentExecutions() const
        {
            return m_latentExecutions;
        }

        AZStd::sys_time_t PerformanceTimer::GetInitializationDurationInMicroseconds() const
        {
            return m_initializationTime;
        }

        double PerformanceTimer::GetInitializationDurationInMilliseconds() const
        {
            return m_initializationTime / 1000.0;
        }

        AZStd::sys_time_t PerformanceTimer::GetTotalDurationInMicroseconds() const
        {
            return m_initializationTime + m_instantTime + m_latentTime;
        }

        double PerformanceTimer::GetTotalDurationInMilliseconds() const
        {
            return GetTotalDurationInMicroseconds() / 1000.0;
        }
}
}
