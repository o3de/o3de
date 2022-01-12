/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/Utilities/TimedThread.h>
#include <AzCore/Console/ILogger.h>

namespace AzNetworking
{
    TimedThread::TimedThread(const char* name, AZ::TimeMs updateRate)
        : m_updateRate(updateRate)
    {
        m_threadDesc.m_name = name;
    }

    TimedThread::~TimedThread()
    {
        AZ_Assert(!IsRunning(), "You must stop and join your thread before destructing it");
    }

    void TimedThread::Start()
    {
        m_running = true;
        m_joinable = true;
        m_thread = AZStd::thread(
            m_threadDesc,
            [this]()
            {
                OnStart();
                while (m_running)
                {
                    const AZ::TimeMs startTimeMs = AZ::GetElapsedTimeMs();
                    OnUpdate(m_updateRate);
                    const AZ::TimeMs updateTimeMs = AZ::GetElapsedTimeMs() - startTimeMs;

                    if (m_updateRate > updateTimeMs)
                    {
                        AZStd::chrono::milliseconds sleepTimeMs(static_cast<int64_t>(m_updateRate - updateTimeMs));
                        AZStd::this_thread::sleep_for(sleepTimeMs);
                    }
                    else if (m_updateRate < updateTimeMs)
                    {
                        AZLOG(NET_TimedThread, "TimedThread bled %d ms", aznumeric_cast<int32_t>(updateTimeMs - m_updateRate));
                    }
                }
                OnStop();
            });
    }

    void TimedThread::Stop()
    {
        m_running = false;
    }

    void TimedThread::Join()
    {
        if (m_joinable && m_thread.joinable())
        {
            m_thread.join();
            m_joinable = false;
        }
    }

    bool TimedThread::IsRunning() const
    {
        return m_running;
    }
}
