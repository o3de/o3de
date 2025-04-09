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

        // Used to detect a deadlock.  The longer we wait for them to finish, the more likely it is
        // that we have instead deadlocked.
        while (threadCount > 0 && !timedOut)
        {
            AZStd::unique_lock<AZStd::mutex> lock(mutex);
            // This completes in about 2 seconds in profile.   It is tempting to wait up to 5 seconds to make sure.
            // However, the user
            // * Might be running in debug (10x or more slower)   - 2s could be as long as 20s
            // * Might be running with ASAN enabled or some other deep memory checker (also 2-5x slower)  - 20s could end up being 100s
            // * Might have a slow machine with fewer actual physical cores for threads (2-5x slower)     - 100s could be as long as 500s
            // * Might have a busy machine doing updates or other tasks in the background.  (Unknown multiplier, could be huge)
            // As such, I'm going to wait for 500 seconds instead of just 5.
            // If the test passes, it will not actually wait for any longer than the base amount of time anyway
            // since this timeout unblocks as soon as all the threads are done anyway, so the only time it waits for the full 500
            // seconds is if it truly is deadlocked.
            // So if this fails you know with a very very high confidence that it is in fact deadlocked and not just running on a potato.

            timedOut = (AZStd::cv_status::timeout == cv.wait_until(lock, AZStd::chrono::steady_clock::now() + AZStd::chrono::seconds(500)));
        }

        EXPECT_TRUE(threadCount == 0);

        for (auto& thread : threads)
        {
            thread.join();
        }
    }
}
