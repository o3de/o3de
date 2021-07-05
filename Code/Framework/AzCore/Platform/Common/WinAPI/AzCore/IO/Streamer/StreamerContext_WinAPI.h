/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformIncl.h>
#include <AzCore/base.h>

namespace AZ::Platform
{
    class StreamerContextThreadSync
    {
    public:
        static constexpr size_t MaxIoEvents = MAXIMUM_WAIT_OBJECTS - 1;

        StreamerContextThreadSync();
        ~StreamerContextThreadSync();

        void Suspend();
        void Resume();

        HANDLE CreateEventHandle();
        void DestroyEventHandle(HANDLE event);
        DWORD GetEventHandleCount() const;
        bool AreEventHandlesAvailable() const;

    private:
        // Note: The first event handle is reserved for the synchronization of the
        // scheduler thread with the rest of the engine. The remaining event handles
        // can be freely used by Streamer's internals.
        HANDLE m_events[MAXIMUM_WAIT_OBJECTS]{};
        DWORD m_handleCount{ 1 }; // The first event is for external wake up calls.
    };

} // namespace AZ::Platform
