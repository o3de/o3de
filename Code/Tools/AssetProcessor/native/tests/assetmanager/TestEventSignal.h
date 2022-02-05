/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/parallel/binary_semaphore.h>

namespace UnitTests
{
    class TestEventPair
    {
        AZStd::atomic_bool m_signaled{ false };
        AZStd::thread_id m_threadId;
        AZStd::binary_semaphore m_event;

    public:
        void Signal();
        bool WaitAndCheck();
    };

    enum TestEvents : int
    {
        Added = 0,
        Modified,
        Deleted,
        NumEvents
    };
}
