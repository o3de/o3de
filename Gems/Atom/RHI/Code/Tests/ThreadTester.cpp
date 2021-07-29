/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/ThreadTester.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/containers/vector.h>

namespace UnitTest
{
    using namespace AZ;

    void ThreadTester::Dispatch(size_t threadCountMax, ThreadFunction threadFunction)
    {
        AZStd::mutex mutex;
        AZStd::vector<AZStd::thread> threads;
        AZStd::atomic<size_t> threadCount(threadCountMax);
        AZStd::condition_variable cv;

        for (size_t i = 0; i < threadCountMax; ++i)
        {
            threads.emplace_back([threadFunction, &threadCount, &cv, i]()
            {
                threadFunction(i);
                threadCount--;
                cv.notify_one();
            });
        }

        bool timedOut = false;

        // Used to detect a deadlock.  If we wait for more than 5 seconds, it's likely a deadlock has occurred
        while (threadCount > 0 && !timedOut)
        {
            AZStd::unique_lock<AZStd::mutex> lock(mutex);
            timedOut = (AZStd::cv_status::timeout == cv.wait_until(lock, AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(5)));
        }

        EXPECT_TRUE(threadCount == 0);

        for (auto& thread : threads)
        {
            thread.join();
        }
    }
}
