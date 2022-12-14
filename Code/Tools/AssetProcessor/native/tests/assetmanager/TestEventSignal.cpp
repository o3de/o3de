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
        constexpr int MaxWaitTimeMilliseconds = 100;

        auto thisThreadId = AZStd::this_thread::get_id();
        bool acquireSuccess = m_event.try_acquire_for(AZStd::chrono::milliseconds(MaxWaitTimeMilliseconds));

        EXPECT_TRUE(acquireSuccess);
        EXPECT_NE(m_threadId, AZStd::thread_id{});
        EXPECT_TRUE(m_threadId != thisThreadId);

        return acquireSuccess && m_threadId != AZStd::thread_id{} && m_threadId != thisThreadId;
    }
}
