/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>
#include <AzTest/Platform.h>

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
#include <csignal>
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/base.h>
#include <AzCore/std/typetraits/has_member_function.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Debug/TraceMessageBus.h>

namespace UnitTest
{
    enum GTestColor
    {
        COLOR_DEFAULT,
        COLOR_RED,
        COLOR_GREEN,
        COLOR_YELLOW
    };

    extern void ColoredPrintf(GTestColor color, const char* fmt, ...);

    class TestRunner
    {
    public:
        static TestRunner& Instance()
        {
            static TestRunner runner;
            return runner;
        }

        void ProcessAssert(const char* /*expression*/, const char* /*file*/, int /*line*/, bool expressionTest)
        {
            ASSERT_TRUE(m_isAssertTest);

            if (m_isAssertTest)
            {
                if (!expressionTest)
                {
                    ++m_numAssertsFailed;
                }
            }
         }

        void StartAssertTests()
        {
            m_isAssertTest = true;
            m_numAssertsFailed = 0;
        }

        int  StopAssertTests()
        {
            m_isAssertTest = false;
            const int numAssertsFailed = m_numAssertsFailed;
            m_numAssertsFailed = 0;
            return numAssertsFailed;
        }

        bool m_isAssertTest;
        bool m_suppressErrors = true;
        bool m_suppressWarnings = true;
        bool m_suppressAsserts = true;
        bool m_suppressOutput = true;
        bool m_suppressPrintf = true;
        int  m_numAssertsFailed;
    };

    AZ_HAS_MEMBER(OperatorBool, operator bool, bool, ());

    class AssertionExpr
    {
        bool m_value;
    public:
        explicit AssertionExpr(bool b)
            : m_value(b)
        {}

        template <class T>
        explicit AssertionExpr(const T& t)
        {
            m_value = Evaluate(t);
        }

        operator bool() { return m_value; }

        template <class T>
        typename AZStd::enable_if<AZStd::is_integral<T>::value, bool>::type
        Evaluate(const T& t)
        {
            return t != 0;
        }

        template <class T>
        typename AZStd::enable_if<AZStd::is_class<T>::value, bool>::type
        Evaluate(const T& t)
        {
            return static_cast<bool>(t);
        }

        template <class T>
        typename AZStd::enable_if<AZStd::is_pointer<T>::value, bool>::type
        Evaluate(const T& t)
        {
            return t != nullptr;
        }
    };

    // utility classes that you can derive from or contain, which suppress AZ_Asserts
    // and AZ_Errors to the below macros (processAssert, etc)
    // If TraceBusHook or TraceBusRedirector have been started in your unit tests,
    //  use AZ_TEST_START_TRACE_SUPPRESSION and AZ_TEST_STOP_TRACE_SUPPRESSION(numExpectedAsserts) macros to perform AZ_Assert and AZ_Error suppression
    class TraceBusRedirector
        : public AZ::Debug::TraceMessageBus::Handler
    {
        bool OnPreAssert(const char* file, int line, const char* /* func */, const char* message) override
        {
            if (UnitTest::TestRunner::Instance().m_isAssertTest)
            {
                UnitTest::TestRunner::Instance().ProcessAssert(message, file, line, false);
                return true;
            }
            else if (UnitTest::TestRunner::Instance().m_suppressAsserts)
            {
                GTEST_MESSAGE_AT_(file, line, message, ::testing::TestPartResult::kNonFatalFailure);
                return true;
            }

            return false;
        }
        bool OnAssert(const char* /*message*/) override
        {
            return UnitTest::TestRunner::Instance().m_suppressAsserts; // stop processing
        }
        bool OnPreError(const char* /*window*/, const char* file, int line, const char* /*func*/, const char* message) override
        {
            if (UnitTest::TestRunner::Instance().m_isAssertTest)
            {
                UnitTest::TestRunner::Instance().ProcessAssert(message, file, line, false);
                return true;
            }

            return false;
        }
        bool OnError(const char* /*window*/, const char* message) override
        {
            if (UnitTest::TestRunner::Instance().m_isAssertTest)
            {
                UnitTest::TestRunner::Instance().ProcessAssert(message, __FILE__, __LINE__, UnitTest::AssertionExpr(false));
                return true;
            }
            else if (UnitTest::TestRunner::Instance().m_suppressErrors)
            {
                GTEST_MESSAGE_(message, ::testing::TestPartResult::kNonFatalFailure);
                return true;
            }

            return false;
        }
        bool OnPreWarning(const char* /*window*/, const char* /*fileName*/, int /*line*/, const char* /*func*/, const char* /*message*/) override
        {
            return false;

        }
        bool OnWarning(const char* /*window*/, const char* /*message*/) override
        {
            return UnitTest::TestRunner::Instance().m_suppressWarnings;
        }

        bool OnOutput(const char* /*window*/, const char* /*message*/) override
        {
            return UnitTest::TestRunner::Instance().m_suppressOutput;
        }

        bool OnPrintf(const char* window, const char* message) override
        {
            if (AZStd::string_view(window) == "Memory") // We want to print out the memory leak's stack traces
            {
                ColoredPrintf(COLOR_RED, "[  MEMORY  ] %s", message);
            }
            return UnitTest::TestRunner::Instance().m_suppressPrintf;
        }
    };

    class TraceBusHook
        : public AZ::Test::ITestEnvironment
        , public TraceBusRedirector
    {
    public:
        void SetupEnvironment() override
        {
#if AZ_TRAIT_UNITTEST_USE_TEST_RUNNER_ENVIRONMENT
            AZ::EnvironmentInstance inst = AZ::Test::GetPlatform().GetTestRunnerEnvironment();
            AZ::Environment::Attach(inst);
            m_createdAllocator = false;
#else
            if (!AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::OSAllocator>::Create(); // used by the bus
                m_createdAllocator = true;
            }
#endif
            BusConnect();

            m_environmentSetup = true;
        }

        void TeardownEnvironment() override
        {
            if (m_environmentSetup)
            {
                BusDisconnect();

                if (m_createdAllocator)
                {
                    AZ::AllocatorInstance<AZ::OSAllocator>::Destroy(); // used by the bus
                }

                // At this point, the AllocatorManager should not have any allocators left. If we happen to have any,
                // we exit the test with an error code (this way the test process does not return 0 and the test run
                // is considered a failure).
                AZ::AllocatorManager& allocatorManager = AZ::AllocatorManager::Instance();
                const int numAllocators = allocatorManager.GetNumAllocators();
                int invalidAllocatorCount = 0;

                for (int i = 0; i < numAllocators; ++i)
                {
                    if (!allocatorManager.GetAllocator(i)->IsLazilyCreated())
                    {
                        invalidAllocatorCount++;
                    }
                }

                if (invalidAllocatorCount && m_createdAllocator)
                {
                    // Print the name of the allocators still in the AllocatorManager
                    ColoredPrintf(COLOR_RED, "[     FAIL ] There are still %d registered non-lazy allocators:\n", invalidAllocatorCount);
                    for (int i = 0; i < numAllocators; ++i)
                    {
                        if (!allocatorManager.GetAllocator(i)->IsLazilyCreated())
                        {
                            ColoredPrintf(COLOR_RED, "\t\t%s\n", allocatorManager.GetAllocator(i)->GetName());
                        }
                    }

                    AZ::AllocatorManager::Destroy();
                    m_environmentSetup = false;

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
                    std::raise(SIGTERM);
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
                }

                AZ::AllocatorManager::Destroy();
                m_environmentSetup = false;
            }
        }

    private:
        bool m_environmentSetup = false;
        bool m_createdAllocator = false;
    };
}


#define AZ_TEST_ASSERT(exp) { \
    if (UnitTest::TestRunner::Instance().m_isAssertTest) \
        UnitTest::TestRunner::Instance().ProcessAssert(#exp, __FILE__, __LINE__, UnitTest::AssertionExpr(exp)); \
    else GTEST_TEST_BOOLEAN_(exp, #exp, false, true, GTEST_NONFATAL_FAILURE_); }

#define AZ_TEST_ASSERT_CLOSE(_exp, _value, _eps) EXPECT_NEAR((double)_exp, (double)_value, (double)_eps)
#define AZ_TEST_ASSERT_FLOAT_CLOSE(_exp, _value) EXPECT_NEAR(_exp, _value, 0.002f)

#define AZ_TEST_STATIC_ASSERT(_Exp)                         static_assert(_Exp, "Test Static Assert")
#ifdef AZ_ENABLE_TRACING
/*
 * The AZ_TEST_START_ASSERTTEST and AZ_TEST_STOP_ASSERTTEST macros have been deprecated and will be removed in a future Open 3D Engine release.
 * The AZ_TEST_START_TRACE_SUPPRESSION and AZ_TEST_STOP_TRACE_SUPPRESSION is the recommend macros
 * The reason for the deprecation is that the AZ_TEST_(START|STOP)_ASSERTTEST implies that they should be used to for writing assert unit test
 * where the asserts themselves are expected to cause the test process to terminate.
 * In reality these macros are for suppression of the AZ_Error(and AZ_Assert for now) trace messages.
 * For writing assert unit test the GTEST EXPECT/ASSERT_DEATH_TEST macro should be used instead
*/
#   define AZ_TEST_START_TRACE_SUPPRESSION                      ::UnitTest::TestRunner::Instance().StartAssertTests()
#   define AZ_TEST_STOP_TRACE_SUPPRESSION(_NumTriggeredTraceMessages) GTEST_ASSERT_EQ(_NumTriggeredTraceMessages, ::UnitTest::TestRunner::Instance().StopAssertTests())
#   define AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT              ::UnitTest::TestRunner::Instance().StopAssertTests()
#   define AZ_TEST_START_ASSERTTEST                             AZ_TEST_START_TRACE_SUPPRESSION
#   define AZ_TEST_STOP_ASSERTTEST(_NumTriggeredTraceMessages)  AZ_TEST_STOP_TRACE_SUPPRESSION(_NumTriggeredTraceMessages)
#else
// we can't test asserts, since they are not enabled for non trace-enabled builds!
#   define AZ_TEST_START_TRACE_SUPPRESSION
#   define AZ_TEST_STOP_TRACE_SUPPRESSION(_NumTriggeredTraceMessages)
#   define AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT
#   define AZ_TEST_START_ASSERTTEST
#   define AZ_TEST_STOP_ASSERTTEST(_NumTriggeredTraceMessages)
#endif

#define AZ_TEST(...)
#define AZ_TEST_SUITE(...)
#define AZ_TEST_SUITE_END

