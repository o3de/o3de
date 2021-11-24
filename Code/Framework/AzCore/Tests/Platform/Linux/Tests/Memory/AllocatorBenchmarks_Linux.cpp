/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Debug/Trace.h>

#include <malloc.h>
#include <stdio.h>

namespace Benchmark
{
    namespace Platform
    {
        size_t GetProcessMemoryUsageBytes()
        {
            struct rusage rusage;
            getrusage(RUSAGE_SELF, &rusage);
            return rusage.ru_maxrss * 1024L;
        }

        size_t GetMemorySize(void* memory)
        {
            return memory ? _aligned_msize(memory, 1, 0) : 0;
        }
    }
}
