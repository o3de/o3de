/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/tests/assetmanager/TestEventSignal.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTests
{
    void TestEventPair::Signal()
    {
        m_threadId = AZStd::this_thread::get_id();
        m_event.release();
    }

    void TestEventPair::WaitAndCheck()
    {
        constexpr int MaxWaitTimeMilliseconds = 10;

        ASSERT_TRUE(m_event.try_acquire_for(AZStd::chrono::milliseconds(10)));
        ASSERT_NE(m_threadId, AZStd::this_thread::get_id());
    }
}
