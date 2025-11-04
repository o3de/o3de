/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Threading/ThreadUtils.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/Math/MathUtils.h>

namespace AZ::Threading
{
    uint32_t CalcNumWorkerThreads(float workerThreadsRatio, uint32_t minNumWorkerThreads, uint32_t maxNumWorkerThreads, uint32_t reservedNumThreads)
    {
        const uint32_t maxHardwareThreads = AZStd::thread::hardware_concurrency();
        const uint32_t numReservedThreads = AZ::GetMin<uint32_t>(reservedNumThreads, maxHardwareThreads); // protect against num reserved being bigger than the number of hw threads
        const uint32_t maxWorkerThreads = maxNumWorkerThreads > 0 ? maxNumWorkerThreads : maxHardwareThreads - numReservedThreads;
        const float requestedWorkerThreads = AZ::GetClamp<float>(workerThreadsRatio, 0.0f, 1.0f) * static_cast<float>(maxWorkerThreads);
        const uint32_t requestedWorkerThreadsRounded = static_cast<uint32_t>(AZStd::lround(requestedWorkerThreads));
        const uint32_t numWorkerThreads = AZ::GetMax<uint32_t>(minNumWorkerThreads, requestedWorkerThreadsRounded);
        return numWorkerThreads;
    }
};
