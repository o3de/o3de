/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
