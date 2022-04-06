/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/thread.h>
#include <AzCore/Time/ITime.h>
#include <AzNetworking/AzNetworkingConfiguration.h>

namespace AzNetworking
{
    //! @class TimedThread
    //! @brief A thread wrapper class that makes it easy to have a time throttled thread.
    class TimedThread
    {
    public:

        AZNETWORKING_API TimedThread(const char* name, AZ::TimeMs updateRate);
        AZNETWORKING_API virtual ~TimedThread();

        //! Starts the thread.
        AZNETWORKING_API void Start();

        //! Stops the thread.
        AZNETWORKING_API void Stop();

        //! Joins the thread.
        AZNETWORKING_API void Join();

        //! Returns true if the thread is running.
        //! @return boolean true if the thread is running, false otherwise
        AZNETWORKING_API bool IsRunning() const;

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
