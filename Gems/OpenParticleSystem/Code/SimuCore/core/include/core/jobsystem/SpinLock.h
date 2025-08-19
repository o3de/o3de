/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef SIMU_CORE_SPIN_LOCK_H
#define SIMU_CORE_SPIN_LOCK_H

#include <thread>
#include <atomic>

namespace SimuCore {
constexpr uint32_t SPIN_MAX = 10;
class SpinLock {
public:
    inline bool try_lock()
    {
        return !lck.test_and_set(std::memory_order_acquire);
    }

    inline void lock()
    {
        while (!try_lock()) {
            std::this_thread::yield();
        }
    }

    inline void unlock()
    {
        lck.clear(std::memory_order_release);
    }

private:
    std::atomic_flag lck = ATOMIC_FLAG_INIT;
};
}
#endif // SIMU_CORE_SPIN_LOCK_H
