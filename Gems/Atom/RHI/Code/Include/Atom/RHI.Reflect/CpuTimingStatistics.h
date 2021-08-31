/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>

#include <AzCore/Debug/Timer.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ
{
    namespace RHI
    {
        //! Container and helper type for storing per frame CPU timing data.
        //! Users can queue up generic timings in Scopes or add to specific timing data.
        struct CpuTimingStatistics
        {
            struct QueueStatistics
            {
                //! The display name of the queue the statistics are for.
                Name m_queueName;

                //! Time spent executing queued work.
                AZStd::sys_time_t m_executeDuration{};
            };

            //! Statistics for each command queue.
            AZStd::vector<QueueStatistics> m_queueStatistics;

            //! The amount of time spent between two calls to EndFrame.
            AZStd::sys_time_t m_frameToFrameTime{};

            //! The amount of time spent presenting (vsync can affect this).
            AZStd::sys_time_t m_presentDuration{};

            void Reset()
            {
                m_queueStatistics.clear();
            }

            double GetFrameToFrameTimeMilliseconds() const
            {
                return (m_frameToFrameTime * 1000) / aznumeric_cast<double>(AZStd::GetTimeTicksPerSecond());
            }
        };

        //! Utility type that updates the given variable with the lifetime of the object in cycles.
        //! Useful for quick scope based timing.
        struct VariableTimer
        {
            VariableTimer() = delete;
            VariableTimer(AZStd::sys_time_t& variable)
                : m_variable(variable)
            {
                m_timer.Stamp();
            }
            ~VariableTimer()
            {
                m_variable = m_timer.GetDeltaTimeInTicks();
            }

            AZStd::sys_time_t& m_variable;
            AZ::Debug::Timer   m_timer;
        };
    }
}

//! Utility for timing a section of code and writing the timing (in cycles) to the given variable.
#define AZ_PROFILE_RHI_VARIABLE(variable) \
    AZ::RHI::VariableTimer AZ_JOIN(variableTimer, __LINE__)(variable);
