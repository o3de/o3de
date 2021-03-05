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

#include <AzCore/std/parallel/thread.h>
#include <AzCore/Time/ITime.h>

namespace AzNetworking
{
    //! @class TimedThread
    //! @brief A thread wrapper class that makes it easy to have a time throttled thread.
    class TimedThread
    {
    public:

        TimedThread(const char* name, AZ::TimeMs updateRate);
        virtual ~TimedThread();

        //! Starts the thread.
        void Start();

        //! Stops the thread.
        void Stop();

        //! Joins the thread.
        void Join();

        //! Returns true if the thread is running.
        //! @return boolean true if the thread is running, false otherwise
        bool IsRunning() const;

    protected:

        //! Invoked on thread start.
        virtual void OnStart() = 0;

        //! Invoked on thread stop.
        virtual void OnStop() = 0;

        //! Invoked on thread update.
        //! @param updateRateMs The amount of time the thread can spend in OnUpdate in ms
        virtual void OnUpdate(AZ::TimeMs updateRateMs) = 0;

    private:

        AZ_DISABLE_COPY_MOVE(TimedThread);

        AZ::TimeMs m_updateRate;
        AZStd::thread_desc m_threadDesc;
        AZStd::thread m_thread;
        AZStd::atomic<bool> m_joinable = false;
        AZStd::atomic<bool> m_running = false;
    };
}
