/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Execution/ExecutionBus.h>

namespace ScriptCanvas
{
    namespace Execution
    {
        class PerformanceTimer
        {
        public:
            AZ_TYPE_INFO(PerformanceTimer, "{696597CC-BA91-4A7B-9ED3-32BEA69ED728}");
            AZ_CLASS_ALLOCATOR(PerformanceTimer, AZ::SystemAllocator);

            PerformanceTimer();

            void AddTimeFrom(const PerformanceTimer& source);

            void AddExecutionTime(AZStd::sys_time_t);

            void AddLatentTime(AZStd::sys_time_t);

            void AddInitializationTime(AZStd::sys_time_t);

            PerformanceTimingReport GetReport() const;

            AZStd::sys_time_t GetInstantDurationInMicroseconds() const;

            double GetInstantDurationInMilliseconds() const;

            AZStd::sys_time_t GetLatentDurationInMicroseconds() const;

            double GetLatentDurationInMilliseconds() const;

            AZ::u32 GetLatentExecutions() const;

            AZStd::sys_time_t GetInitializationDurationInMicroseconds() const;

            double GetInitializationDurationInMilliseconds() const;

            AZStd::sys_time_t GetTotalDurationInMicroseconds() const;

            double GetTotalDurationInMilliseconds() const;

        private:
            AZStd::sys_time_t m_initializationTime;
            AZStd::sys_time_t m_instantTime;
            AZStd::atomic<AZStd::sys_time_t> m_latentTime;
            AZStd::atomic<AZ::u32> m_latentExecutions;
        };
    }
}
