/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace AZ::Threading
{
    //! Calculates the number of worker threads a system should use based on the number of hardware threads a device has. 
    //!         result = max (minNumWorkerThreads, workerThreadsRatio * (num_hardware_threads - reservedNumThreads))
    //! @param workerThreadsRatio scale applied to the calculated maximum number of threads available after reserved threads have been accounted for. Clamped between 0 and 1.
    //! @param minNumWorkerThreads minimum value that will be returned. Value is unclamped and can be more than num_hardware_threads.
    //! @param reservedNumThreads number of hardware threads to reserve for O3DE system threads. Value clamped to num_hardware_threads.
    //! @return number of worker threads for the calling system to allocate
    uint32_t CalcNumWorkerThreads(float workerThreadsRatio, uint32_t minNumWorkerThreads, uint32_t reservedNumThreads);
};
