/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Timer.h>
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
                azstrncpy(expectedNameBuffer, AZCORETEST_DLL_NAME, AZ_ARRAY_SIZE(expectedNameBuffer));
#else
                azstrncpy(expectedNameBuffer, "azcoretests.dll", AZ_ARRAY_SIZE(expectedNameBuffer));
#endif
                AZStd::to_lower(expectedNameBuffer, expectedNameBuffer + AZ_ARRAY_SIZE(expectedNameBuffer));

                for (u32 i = 0; i < SymbolStorage::GetNumLoadedModules(); ++i)
                {
                    char nameBuffer[AZ_ARRAY_SIZE(SymbolStorage::ModuleInfo::m_modName)];
                    azstrncpy(nameBuffer, SymbolStorage::GetModuleInfo(i)->m_fileName, AZ_ARRAY_SIZE(nameBuffer));
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

    class ProfilerTest
        : public AllocatorsFixture
    {
    public:
        int m_numRegistersReceived;

        bool ReadRegisterCallback(const ProfilerRegister& reg, const AZStd::thread_id& id)
        {
            (void)reg;
            (void)id;
            switch (reg.m_type)
            {
            case ProfilerRegister::PRT_TIME:
            {
                AZ_TEST_ASSERT(reg.m_timeData.m_time > 0);
                AZ_TEST_ASSERT(reg.m_timeData.m_calls > 0);
            } break;
            case ProfilerRegister::PRT_VALUE:
            {
                AZ_TEST_ASSERT(reg.m_userValues.m_value1 == 1 || reg.m_userValues.m_value1 == 2);
                AZ_TEST_ASSERT(reg.m_userValues.m_value2 == 0 || reg.m_userValues.m_value2 == 2 || reg.m_userValues.m_value2 == 4);
                AZ_TEST_ASSERT(reg.m_userValues.m_value3 == 0 || reg.m_userValues.m_value3 == 3 || reg.m_userValues.m_value3 == 6);
                AZ_TEST_ASSERT(reg.m_userValues.m_value4 == 0 || reg.m_userValues.m_value4 == 4 || reg.m_userValues.m_value4 == 8);
                AZ_TEST_ASSERT(reg.m_userValues.m_value5 == 0 || reg.m_userValues.m_value5 == 5 || reg.m_userValues.m_value5 == 10);
            } break;
            }

            //AZ::u64 threadId = (AZ::u64)id.m_id;
            //AZ_TracePrintf("Profiler","[%llu] '%s' '%s'(%d) %d Ms (Child calls: %d time: %d Ms) Parent: '%s'!\n",threadId,
            //  reg.m_name,reg.m_function,reg.m_line,reg.m_time.count(),reg.m_childrenCalls,reg.m_childrenTime.count(),reg.m_lastParent ? reg.m_lastParent->m_name : "No");
            ++m_numRegistersReceived;
            return true;
        }

        int ChildFunction(int input)
        {
            AZ_PROFILE_TIMER("UnitTest");

            auto start = AZStd::chrono::system_clock::now();

            int result = 5;
            for (int i = 0; i < 30000; ++i)
            {
                result += i % (input + 3);
            }

            auto end = AZStd::chrono::system_clock::now();
            AZ_TEST_ASSERT(end >= start);
            while (end <= start)
            {
                end = AZStd::chrono::system_clock::now();
            }
            return result;
        }

        int ChildFunction1(int input)
        {
            AZ_PROFILE_TIMER("UnitTest", "Child1");

            auto start = AZStd::chrono::system_clock::now();

            int result = 5;
            for (int i = 0; i < 30000; ++i)
            {
                result += i % (input + 1);
            }


            auto end = AZStd::chrono::system_clock::now();
            AZ_TEST_ASSERT(end >= start);
            while (end <= start)
            {
                end = AZStd::chrono::system_clock::now();
            }

            return result;
        }

        int Profile1(int numIterations)
        {
            AZ_PROFILE_TIMER("UnitTest", "Custom name");
            int result = 0;
            for (int i = 0; i < numIterations; ++i)
            {
                result += ChildFunction(i);
            }

            result += ChildFunction1(numIterations / 3);
            return result;
        }

        void UserValuesSet()
        {
            AZ_PROFILE_VALUE_SET("UnitTest", "UserValues1", 1);
            AZ_PROFILE_VALUE_SET("UnitTest", "UserValues2", 1, 2);
            AZ_PROFILE_VALUE_SET("UnitTest", "UserValues3", 1, 2, 3);
            AZ::s64 v1 = 1, v2 = 2, v3 = 3, v4 = 4, v5 = 5;
            AZ_PROFILE_VALUE_SET("UnitTest", "UserValues4", v1, v2, v3, v4);
            AZ_PROFILE_VALUE_SET("UnitTest", "UserValues5", v1, v2, v3, v4, v5);

            // test named register
            AZ_PROFILE_VALUE_SET_NAMED("UnitTest", "UserValues5", userValues5, v1, v2, v3, v4, v5);
#if defined(AZ_PROFILER_MACRO_DISABLE)
            (void)v1;
            (void)v2;
            (void)v3;
            (void)v4;
            (void)v5;
#else
            AZ_TEST_ASSERT(userValues5 != nullptr);
#endif // !defined(AZ_PROFILER_MACRO_DISABLE)
        }

        void UserValuesAdd(int numAdditions)
        {
            for (int i = 0; i < numAdditions; ++i)
            {
                AZ_PROFILE_VALUE_ADD("UnitTest", "UserValues1", 1);
                AZ_PROFILE_VALUE_ADD("UnitTest", "UserValues2", 1, 2);
                AZ_PROFILE_VALUE_ADD("UnitTest", "UserValues3", 1, 2, 3);
                AZ::s64 v1 = 1, v2 = 2, v3 = 3, v4 = 4, v5 = 5;
                AZ_PROFILE_VALUE_ADD("UnitTest", "UserValues4", v1, v2, v3, v4);
                AZ_PROFILE_VALUE_ADD("UnitTest", "UserValues5", v1, v2, v3, v4, v5);

                // test named register
                AZ_PROFILE_VALUE_ADD_NAMED("UnitTest", "UserValues5", userValues5, v1, v2, v3, v4, v5);
#if defined(AZ_PROFILER_MACRO_DISABLE)
                (void)v1;
                (void)v2;
                (void)v3;
                (void)v4;
                (void)v5;
#else
                AZ_TEST_ASSERT(userValues5 != nullptr);
#endif // !defined(AZ_PROFILER_MACRO_DISABLE)
            }
        }

        void run()
        {
            AZ_TEST_ASSERT(!Profiler::IsReady());
            Profiler::Create();
            AZ_TEST_ASSERT(Profiler::IsReady());
            Profiler::Destroy();
            AZ_TEST_ASSERT(!Profiler::IsReady());

#if !defined(AZ_PROFILER_MACRO_DISABLE)
            Profiler::Create();

            //Profile1();

            //Profiler::Instance().ReadRegisterValues(AZStd::bind(&ProfilerTest::ReadRegisterCallback,this,AZStd::placeholders::_1,AZStd::placeholders::_2));

            //Profiler::Instance().ResetRegisters();

            AZStd::thread_id removeThreadId;
            AZStd::chrono::microseconds elapsed[2];
            int numIterations = 10000;
            for (int i = 0; i < 2; ++i)
            {
                // for the second run we should not record any data
                if (i == 1)
                {
                    Profiler::Instance().DeactivateSystem("UnitTest");
                }

                AZStd::chrono::system_clock::time_point start = AZStd::chrono::system_clock::now();
                AZStd::thread t1(AZStd::bind(&ProfilerTest::Profile1, this, numIterations));
                AZStd::thread t2(AZStd::bind(&ProfilerTest::Profile1, this, numIterations));
                AZStd::thread t3(AZStd::bind(&ProfilerTest::Profile1, this, numIterations));
                AZStd::thread t4(AZStd::bind(&ProfilerTest::Profile1, this, numIterations));
                AZStd::thread t5(AZStd::bind(&ProfilerTest::Profile1, this, numIterations));
                AZStd::thread t6(AZStd::bind(&ProfilerTest::Profile1, this, numIterations));
                AZStd::thread t7(AZStd::bind(&ProfilerTest::Profile1, this, numIterations));
                AZStd::thread t8(AZStd::bind(&ProfilerTest::Profile1, this, numIterations));

                removeThreadId = t4.get_id();

                t1.join();
                t2.join();
                t3.join();
                t4.join();
                t5.join();
                t6.join();
                t7.join();
                t8.join();
                elapsed[i] = AZStd::chrono::system_clock::now() - start;
                //AZ_Printf("Profiler","Elapsed time %d\n",elapsed[i].count());

                if (i == 0)
                {
                    // just as test remove all associated data and registers.
                    Profiler::Instance().RemoveThreadData(removeThreadId);
                }

                m_numRegistersReceived = 0;
                Profiler::Instance().ReadRegisterValues(AZStd::bind(&ProfilerTest::ReadRegisterCallback, this, AZStd::placeholders::_1, AZStd::placeholders::_2));
                if (i == 0)
                {
                    AZ_TEST_ASSERT(m_numRegistersReceived == 7 * 3); // 3 registers for each thread (8 threads - 1 we removed the data for 't4')
                }
                else
                {
                    AZ_TEST_ASSERT(m_numRegistersReceived == 0);
                }
            }
            Profiler::Destroy();

            // Test user value registers
            Profiler::Create();

            for (int i = 0; i < 2; ++i)
            {
                // for the second run we should not record any data
                if (i == 1)
                {
                    Profiler::Instance().DeactivateSystem("UnitTest");
                }

                AZStd::thread t1(AZStd::bind(&ProfilerTest::UserValuesSet, this));
                AZStd::thread t2(AZStd::bind(&ProfilerTest::UserValuesSet, this));
                AZStd::thread t3(AZStd::bind(&ProfilerTest::UserValuesSet, this));
                AZStd::thread t4(AZStd::bind(&ProfilerTest::UserValuesSet, this));
                AZStd::thread t5(AZStd::bind(&ProfilerTest::UserValuesAdd, this, 2));
                AZStd::thread t6(AZStd::bind(&ProfilerTest::UserValuesAdd, this, 2));
                AZStd::thread t7(AZStd::bind(&ProfilerTest::UserValuesAdd, this, 2));
                AZStd::thread t8(AZStd::bind(&ProfilerTest::UserValuesAdd, this, 2));

                removeThreadId = t4.get_id();

                t1.join();
                t2.join();
                t3.join();
                t4.join();
                t5.join();
                t6.join();
                t7.join();
                t8.join();

                if (i == 0)
                {
                    // just as test remove all associated data and registers.
                    Profiler::Instance().RemoveThreadData(removeThreadId);
                }

                m_numRegistersReceived = 0;
                Profiler::Instance().ReadRegisterValues(AZStd::bind(&ProfilerTest::ReadRegisterCallback, this, AZStd::placeholders::_1, AZStd::placeholders::_2));
                if (i == 0)
                {
                    AZ_TEST_ASSERT(m_numRegistersReceived == 7 * 6); // 6 registers for each thread (8 threads - 1 we removed the data for 't4' )
                }
                else
                {
                    AZ_TEST_ASSERT(m_numRegistersReceived == 0);
                }
            }
            Profiler::Destroy();
#endif
        }
    };
#if AZ_TRAIT_DISABLE_FAILED_PROFILER_TEST
    TEST_F(ProfilerTest, DISABLED_Test)
#else
    TEST_F(ProfilerTest, Test)
#endif // AZ_TRAIT_DISABLE_FAILED_PROFILER_TEST

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
