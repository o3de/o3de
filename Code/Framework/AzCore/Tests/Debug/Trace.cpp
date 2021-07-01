/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>

using namespace AZ;

namespace UnitTest
{
    struct TraceTests
        : ScopedAllocatorSetupFixture
        , Debug::TraceMessageBus::Handler
    {
        bool OnPreAssert(const char*, int, const char*, const char*) override
        {
            m_assert = true;

            return true;
        }

        bool OnPreError(const char*, const char*, int, const char*, const char*) override
        {
            m_error = true;

            return true;
        }

        bool OnPreWarning(const char*, const char*, int, const char*, const char*) override
        {
            m_warning = true;

            return true;
        }

        bool OnPrintf(const char*, const char*) override
        {
            m_printf = true;

            return true;
        }

        void SetUp() override
        {
            BusConnect();

            if (!AZ::Interface<AZ::IConsole>::Get())
            {
                m_console = aznew AZ::Console();
                m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
                AZ::Interface<AZ::IConsole>::Register(m_console);
            }

            AZ_TEST_START_TRACE_SUPPRESSION;
        }

        void TearDown() override
        {
            if (m_console)
            {
                AZ::Interface<AZ::IConsole>::Unregister(m_console);
                delete m_console;
                m_console = nullptr;
            }
            AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
            BusDisconnect();
        }

        bool m_assert = false;
        bool m_error = false;
        bool m_warning = false;
        bool m_printf = false;
        AZ::Console* m_console{ nullptr };
    };

    TEST_F(TraceTests, Level0)
    {
        Interface<IConsole>::Get()->PerformCommand("bg_traceLogLevel 0");

        AZ_Assert(false, "test");
        AZ_Error("UnitTest", false, "test");
        AZ_Warning("UnitTest", false, "test");
        AZ_TracePrintf("UnitTest", "test");

        ASSERT_TRUE(m_assert);
        ASSERT_FALSE(m_error);
        ASSERT_FALSE(m_warning);
        ASSERT_FALSE(m_printf);
    }

    TEST_F(TraceTests, Level1)
    {
        Interface<IConsole>::Get()->PerformCommand("bg_traceLogLevel 1");

        AZ_Assert(false, "test");
        AZ_Error("UnitTest", false, "test");
        AZ_Warning("UnitTest", false, "test");
        AZ_TracePrintf("UnitTest", "test");

        ASSERT_TRUE(m_assert);
        ASSERT_TRUE(m_error);
        ASSERT_FALSE(m_warning);
        ASSERT_FALSE(m_printf);
    }

    TEST_F(TraceTests, Level2)
    {
        Interface<IConsole>::Get()->PerformCommand("bg_traceLogLevel 2");

        AZ_Assert(false, "test");
        AZ_Error("UnitTest", false, "test");
        AZ_Warning("UnitTest", false, "test");
        AZ_TracePrintf("UnitTest", "test");

        ASSERT_TRUE(m_assert);
        ASSERT_TRUE(m_error);
        ASSERT_TRUE(m_warning);
        ASSERT_FALSE(m_printf);
    }

    TEST_F(TraceTests, Level3)
    {
        Interface<IConsole>::Get()->PerformCommand("bg_traceLogLevel 3");

        AZ_Assert(false, "test");
        AZ_Error("UnitTest", false, "test");
        AZ_Warning("UnitTest", false, "test");
        AZ_TracePrintf("UnitTest", "test");

        ASSERT_TRUE(m_assert);
        ASSERT_TRUE(m_error);
        ASSERT_TRUE(m_warning);
        ASSERT_TRUE(m_printf);
    }
}
