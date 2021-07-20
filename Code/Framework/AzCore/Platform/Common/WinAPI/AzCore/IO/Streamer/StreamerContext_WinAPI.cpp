/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <utility>
#include <AzCore/Casting/numeric_cast.h>
#include <../Common/WinAPI/AzCore/IO/Streamer/StreamerContext_WinAPI.h>
#include <AzCore/Debug/Profiler.h>

namespace AZ::Platform
{
    StreamerContextThreadSync::StreamerContextThreadSync()
    {
        for (size_t i = 0; i < MAXIMUM_WAIT_OBJECTS; ++i)
        {
            HANDLE event = CreateEvent(nullptr, true, false, nullptr);
            if (event)
            {
                m_events[i] = event;
            }
            else
            {
                AZ_Assert(event, "Failed to create a required event for IO Scheduler (Error: %u).", ::GetLastError());
            }
        }
    }

    StreamerContextThreadSync::~StreamerContextThreadSync()
    {
        for (size_t i = 0; i < MAXIMUM_WAIT_OBJECTS; ++i)
        {
            HANDLE event = m_events[i];
            if (event)
            {
                if (!::CloseHandle(event))
                {
                    AZ_Assert(false, "Failed to close an event handle for IO Scheduler (Error: %u)", ::GetLastError());
                }
            }
        }
    }

    void StreamerContextThreadSync::Suspend()
    {
        AZ_Assert(m_events[0], "There is no synchronization event created for the main streamer thread to use to suspend.");

        DWORD result = ::WaitForMultipleObjects(m_handleCount, m_events, false, INFINITE);
        if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + m_handleCount)
        {
            DWORD index = result - WAIT_OBJECT_0;
            ::ResetEvent(m_events[index]);
        }
        else
        {
            AZ_Assert(false, "Unexpected wait result: %u.", result);
        }
    }

    void StreamerContextThreadSync::Resume()
    {
        AZ_Assert(m_events[0], "There is no synchronization event created for the main streamer thread to use to resume.");
        ::SetEvent(m_events[0]);
    }

    HANDLE StreamerContextThreadSync::CreateEventHandle()
    {
        AZ_Assert(m_handleCount < MAXIMUM_WAIT_OBJECTS, "There are no more slots available to allocate a new IO event in.");
        return m_events[m_handleCount++];
    }

    void StreamerContextThreadSync::DestroyEventHandle(HANDLE event)
    {
        AZ_Assert(m_handleCount > 1, "There are no more IO events that can be destroyed.");

        for (size_t i = 1; i < m_handleCount; ++i)
        {
            if (m_events[i] == event)
            {
                m_handleCount--;
                AZStd::swap(m_events[i], m_events[m_handleCount]);
                return;
            }
        }

        AZ_Assert(false, "IO event couldn't be destroyed as it wasn't found.");
    }

    DWORD StreamerContextThreadSync::GetEventHandleCount() const
    {
        return m_handleCount - 1;
    }

    bool StreamerContextThreadSync::AreEventHandlesAvailable() const
    {
        return m_handleCount < MAXIMUM_WAIT_OBJECTS;
    }
} // namespace AZ::Platform
