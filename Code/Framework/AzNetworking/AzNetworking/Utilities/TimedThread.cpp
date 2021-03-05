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
        m_thread = AZStd::thread([this]()
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
                    AZLOG_INFO("TimedThread bled %d ms", aznumeric_cast<int32_t>(updateTimeMs - m_updateRate));
                }
            }
            OnStop();
        }, &m_threadDesc);
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
