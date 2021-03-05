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
#include "CrySystem_precompiled.h"

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/AllocatorScope.h>
#include <AzCore/std/string/string.h>
#include <BootProfiler.h>

#if defined(ENABLE_LOADING_PROFILER)

namespace UnitTests
{
    using BootProfilerTestAllocatorScope = AZ::AllocatorScope<AZ::LegacyAllocator, CryStringAllocator>;
    class BootProfilerTest :
        public ::testing::Test,
        BootProfilerTestAllocatorScope,
        UnitTest::TraceBusRedirector
    {
    public:
        BootProfilerTest()
        {
            BootProfilerTestAllocatorScope::ActivateAllocators();
            UnitTest::TraceBusRedirector::BusConnect();
        }

        ~BootProfilerTest() 
        {
            UnitTest::TraceBusRedirector::BusDisconnect();
            BootProfilerTestAllocatorScope::DeactivateAllocators();
        }

        void SetUp() override
        {

        }

        void TearDown() override
        {

        }

    };

    TEST_F(BootProfilerTest, BootProfilerTest_StartStopBlocksInThreads_Success)
    {
        CBootProfiler testProfiler;
        const char scopeName[] = "TestScope";
        const char blockArg[] = "TestArg";
        const int numAttempts = 1000;
        const int numThreads = 10;

        auto switchSessionFunc = [&]() {
            for (int sessionNum = 0; sessionNum < numAttempts; ++sessionNum)
            {
                auto sessionName = AZStd::string::format("TestSession%d", sessionNum);
                testProfiler.StartSession(sessionName.c_str());
                testProfiler.StopSession(sessionName.c_str());
            }
        };
        auto testProfileFunc = [&]() {
            for (int blockNum = 0; blockNum < numAttempts; ++blockNum)
            {
                auto someBlock = testProfiler.StartBlock(scopeName, blockArg);
                testProfiler.StopBlock(someBlock);
            }
        };
        AZStd::thread threadArray[numThreads];

        AZStd::thread sessionThread = AZStd::thread(switchSessionFunc);
        for (int i = 0; i < numThreads; ++i)
        {
            threadArray[i] = AZStd::thread(testProfileFunc);
        }
        for (int i = 0; i < numThreads; ++i)
        {
            threadArray[i].join();
        }
        sessionThread.join();
    }

    class FrameTestBootProfiler : public CBootProfiler
    {
    public:
        FrameTestBootProfiler(int frameCount) : CBootProfiler()
        {
            SetFrameCount(frameCount);
        }
    };
    TEST_F(BootProfilerTest, BootProfilerTest_FrameStartStop_Success)
    {
        const int numTestFrames = 10;
        FrameTestBootProfiler testProfiler(numTestFrames);

        for (int i = 0; i < numTestFrames; ++i)
        {
            testProfiler.StartFrame("TestFrame");

            testProfiler.StopFrame();
        }
    }
} // namespace UnitTests

#endif
