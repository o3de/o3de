/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Debug/TraceMessagesDrillerBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/time.h>
#include <AzCore/std/chrono/clocks.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;
using namespace AZ::Debug;

namespace UnitTest
{
    TEST(StackTracer, Test)
    {
        // Test can be executed only on supported platforms
        StackFrame  frames[10];
        unsigned int numFrames = StackRecorder::Record(frames, AZ_ARRAY_SIZE(frames));
        if (numFrames)
        {
            u32 numValidFrames = 0;
            for (u32 i = 0; i < AZ_ARRAY_SIZE(frames); ++i)
            {
                if (frames[i].IsValid())
                {
                    ++numValidFrames;
                }
            }
            AZ_TEST_ASSERT(numValidFrames == numFrames);
#if AZ_TRAIT_OS_STACK_FRAMES_VALID
            // We have:
            // StackTracer::run
            // DebugSuite::run
            // main_tests.cpp : DoTests
            // main_tests.cpp : main()
            // + system calls (in release some func are inlined but still >= 5)
            AZ_TEST_ASSERT(numValidFrames >= 5);
#else
            AZ_TEST_ASSERT(numValidFrames == 0);
#endif

#if AZ_TRAIT_OS_STACK_FRAMES_TRACE  // Symbols are available only on windows.
            {
                // We should have loaded at least AzCore.Tests dynamic library (where this test is)
                int isFoundModule = 0;
                char expectedNameBuffer[AZ_ARRAY_SIZE(SymbolStorage::ModuleInfo::m_modName)];
#if defined(AZCORETEST_DLL_NAME)
                azstrncpy(expectedNameBuffer, AZ_ARRAY_SIZE(expectedNameBuffer), AZCORETEST_DLL_NAME, AZ_ARRAY_SIZE(expectedNameBuffer));
#else
                azstrncpy(expectedNameBuffer, "azcoretests.dll", AZ_ARRAY_SIZE(expectedNameBuffer));
#endif
                AZStd::to_lower(expectedNameBuffer, expectedNameBuffer + AZ_ARRAY_SIZE(expectedNameBuffer));

                for (u32 i = 0; i < SymbolStorage::GetNumLoadedModules(); ++i)
                {
                    char nameBuffer[AZ_ARRAY_SIZE(SymbolStorage::ModuleInfo::m_modName)];
                    azstrncpy(nameBuffer, AZ_ARRAY_SIZE(nameBuffer), SymbolStorage::GetModuleInfo(i)->m_fileName, AZ_ARRAY_SIZE(nameBuffer));
                    AZStd::to_lower(nameBuffer, nameBuffer + AZ_ARRAY_SIZE(nameBuffer));

                    if (strstr(nameBuffer, expectedNameBuffer))
                    {
                        isFoundModule = 1;
                        break;
                    }
                }

                AZ_TEST_ASSERT(isFoundModule);
                SymbolStorage::StackLine stackLines[AZ_ARRAY_SIZE(frames)];
                AZ_TracePrintf("StackTracer", "\n====================================\n");
                AZ_TracePrintf("StackTracer", "Callstack:\n");
                SymbolStorage::DecodeFrames(frames, numFrames, stackLines);
                for (u32 i = 0; i < numFrames; ++i)
                {
                    AZ_TEST_ASSERT(strlen(stackLines[i]) > 0); // We should some result for each stack frame
                    AZ_TracePrintf("StackTracer", "%s\n", stackLines[i]);
                }
                AZ_TracePrintf("StackTracer", "====================================\n");
            }
#elif AZ_TRAIT_OS_STACK_FRAMES_PRINT
            {
                SymbolStorage::StackLine stackLines[AZ_ARRAY_SIZE(frames)];
                printf("\n====================================\n");
                printf("Callstack:\n");
                SymbolStorage::DecodeFrames(frames, numFrames, stackLines);
                for (u32 i = 0; i < numFrames; ++i)
                {
                    AZ_TEST_ASSERT(strlen(stackLines[i]) > 0); // We should some result for each stack frame
                    printf("%s\n", stackLines[i]);
                }
                printf("====================================\n");
            }
#endif
        }
    }

    class TraceTest
        : public AZ::Debug::TraceMessageDrillerBus::Handler
        , public ::testing::Test
    {
        int m_numTracePrintfs;
    public:
        TraceTest()
            : m_numTracePrintfs(0) {}

        //////////////////////////////////////////////////////////////////////////
        // TraceMessagesDrillerBus
        void OnPrintf(const char* windowName, const char* message) override
        {
            (void)windowName;
            (void)message;
            ++m_numTracePrintfs;
        }
        //////////////////////////////////////////////////////////////////////////

        void run()
        {
            bool falseExp = false;
            int  m_data = 0;
            (void)falseExp;
            (void)m_data;

            BusConnect();

            printf("\n");

            // Assert
            AZ_Assert(true, "Constand expression - always pass!");

            // Error

            // Regular warning
            AZ_Warning("Streamer", falseExp, "Test Message");

            // Printf
            int oldNumTracePrintfs = m_numTracePrintfs;
            AZ_Printf("AI | A*", "Test Message");
            // only one traceprintf should have occurred here.
            AZ_TEST_ASSERT(m_numTracePrintfs == oldNumTracePrintfs + 1);

            // test verify
            bool result = false;
            AZ_Verify(result = true, "Expression should execute even in release!");
            AZ_TEST_ASSERT(result == true);

            result = false;
            AZ_VerifyError("Test", result = true, "Expression should execute even in release!");
            AZ_TEST_ASSERT(result == true);

            result = false;
            AZ_VerifyWarning("Test", result = true, "Expression should execute even in release!");
            AZ_TEST_ASSERT(result == true);
            (void)result;

            BusDisconnect();
        }
    };

    TEST_F(TraceTest, Test)
    {
        run();
    }

    TEST(Time, Test)
    {
        AZStd::sys_time_t ticksPerSecond = AZStd::GetTimeTicksPerSecond();
        AZ_TEST_ASSERT(ticksPerSecond != 0);

        AZStd::sys_time_t timeNowSeconds = AZStd::GetTimeNowSecond();
        AZ_TEST_ASSERT(timeNowSeconds != 0);

        AZStd::sys_time_t timeNowTicks = AZStd::GetTimeNowTicks();
        AZ_TEST_ASSERT(timeNowTicks != 0);

        AZStd::sys_time_t timeNowMs = AZStd::GetTimeNowMicroSecond();
        AZ_TEST_ASSERT(timeNowMs != 0);

        AZ::u64 timeNowUTCms = AZStd::GetTimeUTCMilliSecond();
        AZ_TEST_ASSERT(timeNowUTCms != 0);
    };
}
