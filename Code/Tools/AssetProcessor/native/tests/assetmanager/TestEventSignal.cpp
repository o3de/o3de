/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/assetmanager/TestEventSignal.h>
#include <AzCore/std/parallel/thread.h>
#if !defined(Q_MOC_RUN)
#include <AzCore/UnitTest/TestTypes.h>
#endif

namespace UnitTests
{
    void TestEventPair::Signal()
    {
        bool expected = false;
        ASSERT_TRUE(m_signaled.compare_exchange_strong(expected, true));
        ASSERT_EQ(m_threadId, AZStd::thread_id{});
        m_threadId = AZStd::this_thread::get_id();
        m_event.release();
    }

    bool TestEventPair::WaitAndCheck()
    {
        // usually this completes under a millisecond or two, but a slow machine or busy machine
        // can cause hiccups of anywhere between a few milliseconds to a few seconds.
        // Since this test will exit the instant it gets its signal, prefer to set a very long timeout
        // beyond what is even remotely necessary, so that if the test hits it, we know with a high degree of confidence
        // that the message is not forthcoming, not that we just didn't wait long enough for it due to environmental
        // issues.
        constexpr int MaxWaitTimeMilliseconds = 30000;

        auto thisThreadId = AZStd::this_thread::get_id();
        bool acquireSuccess = m_event.try_acquire_for(AZStd::chrono::milliseconds(MaxWaitTimeMilliseconds));

        EXPECT_TRUE(acquireSuccess);
        EXPECT_NE(m_threadId, AZStd::thread_id{});
        EXPECT_TRUE(m_threadId != thisThreadId);

        return acquireSuccess && m_threadId != AZStd::thread_id{} && m_threadId != thisThreadId;
    }
}
