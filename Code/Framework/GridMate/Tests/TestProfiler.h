/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_TEST_PROFILER_H
#define GM_TEST_PROFILER_H

namespace GridMate
{
    class TestProfiler
    {
    public:
        static void StartProfiling();
        static void StopProfiling();

        static void PrintProfilingTotal(const char* systemId);
        static void PrintProfilingSelf(const char* systemId);
    };
}

#endif
