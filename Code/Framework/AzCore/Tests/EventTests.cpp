/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/Event.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class EventTests
        : public LeakDetectionFixture
    {
    };

    TEST_F(EventTests, TestHasCallback)
    {
        AZ::Event<int32_t> testEvent;
        AZ::Event<int32_t>::Handler testHandler([]([[maybe_unused]] int32_t value) {});

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
        testHandler.Connect(testEvent);
        EXPECT_TRUE(testEvent.HasHandlerConnected());
    }

    TEST_F(EventTests, TestScopedConnect)
    {
        AZ::Event<int32_t> testEvent;

        {
            AZ::Event<int32_t>::Handler testHandler([]([[maybe_unused]] int32_t value) {});
            testHandler.Connect(testEvent);
            EXPECT_TRUE(testEvent.HasHandlerConnected());
        }

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
    }

    TEST_F(EventTests, TestEvent)
    {
        int32_t invokedValue = 0;

        AZ::Event<int32_t> testEvent;
        AZ::Event<int32_t>::Handler testHandler([&invokedValue](int32_t value) { invokedValue = value; });

        testHandler.Connect(testEvent);

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(1);
        EXPECT_TRUE(invokedValue == 1);
        testEvent.Signal(-1);
        EXPECT_TRUE(invokedValue == -1);
    }

    TEST_F(EventTests, TestEventRValueParam)
    {
        int32_t invokedValue = 0;

        AZ::Event<int32_t> testEvent;
        AZ::Event<int32_t>::Handler testHandler([&invokedValue](int32_t value) { invokedValue = value; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(value);
        EXPECT_TRUE(invokedValue == 1);
    }

    TEST_F(EventTests, TestEventRefParam)
    {
        int32_t invokedValue = 0;

        AZ::Event<int32_t&> testEvent;
        AZ::Event<int32_t&>::Handler testHandler([&invokedValue](int32_t& value) { invokedValue = value++; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(value);
        EXPECT_TRUE(invokedValue == 1);
        EXPECT_TRUE(value == 2);
        testEvent.Signal(value);
        EXPECT_TRUE(invokedValue == 2);
        EXPECT_TRUE(value == 3);
    }

    TEST_F(EventTests, TestEventConstRefParam)
    {
        int32_t invokedValue = 0;

        AZ::Event<const int32_t&> testEvent;
        AZ::Event<const int32_t&>::Handler testHandler([&invokedValue](const int32_t& value) { invokedValue = value; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(value);
        EXPECT_TRUE(invokedValue == 1);
    }

    TEST_F(EventTests, TestEventPointerParam)
    {
        int32_t invokedValue = 0;

        AZ::Event<int32_t*> testEvent;
        AZ::Event<int32_t*>::Handler testHandler([&invokedValue](int32_t* value) { invokedValue = (*value)++; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(&value);
        EXPECT_TRUE(invokedValue == 1);
        EXPECT_TRUE(value == 2);
        testEvent.Signal(&value);
        EXPECT_TRUE(invokedValue == 2);
        EXPECT_TRUE(value == 3);
    }

    TEST_F(EventTests, TestEventConstPointerParam)
    {
        int32_t invokedValue = 0;

        AZ::Event<const int32_t*> testEvent;
        AZ::Event<const int32_t*>::Handler testHandler([&invokedValue](const int32_t* value) { invokedValue = *value; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(&value);
        EXPECT_TRUE(invokedValue == 1);
    }

    TEST_F(EventTests, TestEventMultiParam)
    {
        int32_t invokedValue1 = 0;
        bool invokedValue2 = false;

        AZ::Event<int32_t, bool> testEvent;
        AZ::Event<int32_t, bool>::Handler testHandler([&invokedValue1, &invokedValue2](int32_t value1, bool value2) { invokedValue1 = value1; invokedValue2 = value2; });

        testHandler.Connect(testEvent);

        EXPECT_TRUE(invokedValue1 == 0);
        EXPECT_TRUE(invokedValue2 == false);
        testEvent.Signal(1, true);
        EXPECT_TRUE(invokedValue1 == 1);
        EXPECT_TRUE(invokedValue2 == true);
        testEvent.Signal(-1, false);
        EXPECT_TRUE(invokedValue1 == -1);
        EXPECT_TRUE(invokedValue2 == false);
    }

    TEST_F(EventTests, TestConnectDuringEvent)
    {
        AZ::Event<int32_t> testEvent;

        {
            int32_t testHandler2Data = 0;
            AZ::Event<int32_t>::Handler testHandler2([&testHandler2Data](int32_t value) { testHandler2Data = value; });

            AZ::Event<int32_t>::Handler testHandler([&testHandler2, &testEvent]([[maybe_unused]] int32_t value) { testHandler2.Connect(testEvent); });
            testHandler.Connect(testEvent);

            testEvent.Signal(1);
            EXPECT_TRUE(testHandler2Data == 0);

            testHandler.Disconnect();
            EXPECT_TRUE(testEvent.HasHandlerConnected());

            testEvent.Signal(2);
            EXPECT_TRUE(testHandler2Data == 2);
        }

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
    }

    TEST_F(EventTests, TestDisconnectDuringEvent)
    {
        AZ::Event<int32_t> testEvent;

        {
            int32_t testHandler2Data = 0;
            AZ::Event<int32_t>::Handler testHandler2([&testHandler2Data](int32_t value) { testHandler2Data = value; });
            AZ::Event<int32_t>::Handler testHandler([&testHandler2]([[maybe_unused]] int32_t value) { testHandler2.Disconnect(); });

            testHandler2.Connect(testEvent);
            testHandler.Connect(testEvent);

            testEvent.Signal(1);
            EXPECT_TRUE(testHandler2Data == 1);
            EXPECT_TRUE(testEvent.HasHandlerConnected());

            testEvent.Signal(2);
            EXPECT_TRUE(testHandler2Data == 1);
        }

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
    }

    TEST_F(EventTests, TestDisconnectDuringEventReversed)
    {
        AZ::Event<int32_t> testEvent;

        // Same test as above, but connected using reversed ordering
        {
            int32_t testHandler2Data = 0;
            AZ::Event<int32_t>::Handler testHandler2([&testHandler2Data](int32_t value) { testHandler2Data = value; });
            AZ::Event<int32_t>::Handler testHandler([&testHandler2]([[maybe_unused]] int32_t value) { testHandler2.Disconnect(); });

            testHandler.Connect(testEvent);
            testHandler2.Connect(testEvent);

            testEvent.Signal(1);
            EXPECT_TRUE(testHandler2Data == 0);
            EXPECT_TRUE(testEvent.HasHandlerConnected());

            testEvent.Signal(2);
            EXPECT_TRUE(testHandler2Data == 0);
        }

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
    }

    TEST_F(EventTests, CopyConstructorAndCopyAssignmentOperator_AreNotCallable)
    {
        static_assert(!AZStd::is_copy_constructible_v<AZ::Event<int32_t>>, "AZ Events should not be copy constructible");
        static_assert(!AZStd::is_copy_assignable_v<AZ::Event<int32_t>>, "AZ Events should not be copy assignable");
    }

    TEST_F(EventTests, TestClaimHandlers_TakesAllSourceHandlers)
    {
        AZ::Event<> testEvent1;
        AZ::Event<> testEvent2;

        int32_t handlerInvokeCount{};
        auto handlerCallback = [&handlerInvokeCount]()
        {
            ++handlerInvokeCount;
        };
        AZ::Event<>::Handler testHandler1(handlerCallback);
        AZ::Event<>::Handler testHandler2(handlerCallback);

        testHandler1.Connect(testEvent1);
        testHandler2.Connect(testEvent2);

        EXPECT_TRUE(testEvent1.HasHandlerConnected());
        EXPECT_TRUE(testEvent2.HasHandlerConnected());

        testEvent1.ClaimHandlers(AZStd::move(testEvent2));
        EXPECT_TRUE(testEvent1.HasHandlerConnected());
        EXPECT_FALSE(testEvent2.HasHandlerConnected());

        // testEvent1 should have both handlers
        testEvent1.Signal();
        EXPECT_EQ(2, handlerInvokeCount);
        // testEvent2 should have neither of the handlers
        testEvent2.Signal();
        EXPECT_EQ(2, handlerInvokeCount);
    }

    TEST_F(EventTests, HandlerMoveAssignment_ProperlyDisconnectsFromOldEvent)
    {
        AZ::Event<> testEvent1;
        AZ::Event<> testEvent2;

        AZ::Event<>::Handler testHandler1([]() {});
        AZ::Event<>::Handler testHandler2([]() {});

        testHandler1.Connect(testEvent1);
        testHandler2.Connect(testEvent2);

        EXPECT_TRUE(testEvent1.HasHandlerConnected());
        EXPECT_TRUE(testEvent2.HasHandlerConnected());

        testHandler1 = AZStd::move(testHandler2);
        EXPECT_FALSE(testEvent1.HasHandlerConnected());
        EXPECT_TRUE(testEvent2.HasHandlerConnected());
    }

    TEST_F(EventTests, TestHandlerCopyConstructorOperator)
    {
        int32_t invokedCounter = 0;

        AZ::Event<> testEvent;
        AZ::Event<>::Handler testHandler([&invokedCounter]() { invokedCounter++; });

        testHandler.Connect(testEvent);

        AZ::Event<>::Handler testHandler2(testHandler);

        EXPECT_TRUE(testHandler.IsConnected());
        EXPECT_TRUE(testHandler2.IsConnected());

        EXPECT_TRUE(invokedCounter == 0);
        testEvent.Signal();
        EXPECT_TRUE(invokedCounter == 2);
    }

    TEST_F(EventTests, TestHandlerCopyAssignmentOperator)
    {
        int32_t invokedCounter = 0;

        AZ::Event<> testEvent;
        AZ::Event<>::Handler testHandler([&invokedCounter]() { invokedCounter++; });

        testHandler.Connect(testEvent);

        AZ::Event<>::Handler testHandler2;

        EXPECT_TRUE(testHandler.IsConnected());
        EXPECT_FALSE(testHandler2.IsConnected());

        testHandler2 = testHandler;

        EXPECT_TRUE(testHandler2.IsConnected());

        EXPECT_TRUE(invokedCounter == 0);
        testEvent.Signal();
        EXPECT_TRUE(invokedCounter == 2);
    }
}

#if defined(HAVE_BENCHMARK)
//-------------------------------------------------------------------------
// PERF TESTS
//-------------------------------------------------------------------------

#include <benchmark/benchmark.h>

namespace Benchmark
{
    static constexpr int32_t NumHandlers = 10000;

    static void BM_EventPerf_EventEmpty(benchmark::State& state)
    {
        AZ::Event<int32_t> testEvent;
        AZ::Event<int32_t>::Handler testHandler[NumHandlers];

        for (int32_t i = 0; i < NumHandlers; ++i)
        {
            testHandler[i] = AZ::Event<int32_t>::Handler([]([[maybe_unused]] int32_t value) {});
            testHandler[i].Connect(testEvent);
        }

        while (state.KeepRunning())
        {
            testEvent.Signal(1);
        }
    }
    BENCHMARK(BM_EventPerf_EventEmpty);

    static void BM_EventPerf_EventIncrement(benchmark::State& state)
    {
        AZ::Event<int32_t> testEvent;
        AZ::Event<int32_t>::Handler testHandler[NumHandlers];

        int32_t incrementCounter = 0;

        for (int32_t i = 0; i < NumHandlers; ++i)
        {
            testHandler[i] = AZ::Event<int32_t>::Handler([&incrementCounter]([[maybe_unused]] int32_t value) { ++incrementCounter; });
            testHandler[i].Connect(testEvent);
        }

        while (state.KeepRunning())
        {
            testEvent.Signal(1);
        }
    }
    BENCHMARK(BM_EventPerf_EventIncrement);

    class EBusPerfBaseline
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnSignal(int32_t) = 0;
    };
    using EBusPerfBaselineBus = AZ::EBus<EBusPerfBaseline>;

    class EBusPerfBaselineImplEmpty
        : public EBusPerfBaselineBus::Handler
    {
    public:
        EBusPerfBaselineImplEmpty() { EBusPerfBaselineBus::Handler::BusConnect(); }
        ~EBusPerfBaselineImplEmpty() override { EBusPerfBaselineBus::Handler::BusDisconnect(); }
        void OnSignal(int32_t) override {}
    };

    static void BM_EventPerf_EBusEmpty(benchmark::State& state)
    {
        EBusPerfBaselineImplEmpty testHandler[NumHandlers];

        while (state.KeepRunning())
        {
            EBusPerfBaselineBus::Broadcast(&EBusPerfBaseline::OnSignal, 1);
        }
    }
    BENCHMARK(BM_EventPerf_EBusEmpty);

    class EBusPerfBaselineImplIncrement
        : public EBusPerfBaselineBus::Handler
    {
    public:
        EBusPerfBaselineImplIncrement() { EBusPerfBaselineBus::Handler::BusConnect(); }
        ~EBusPerfBaselineImplIncrement() override { EBusPerfBaselineBus::Handler::BusDisconnect(); }
        void SetIncrementCounter(int32_t* incrementCounter) { m_incrementCounter = incrementCounter; }
        void OnSignal(int32_t) override { ++(*m_incrementCounter); }
        int32_t* m_incrementCounter;
    };

    static void BM_EventPerf_EBusIncrement(benchmark::State& state)
    {
        int32_t incrementCounter = 0;
        EBusPerfBaselineImplIncrement testHandler[NumHandlers];

        for (int32_t i = 0; i < NumHandlers; ++i)
        {
            testHandler[i].SetIncrementCounter(&incrementCounter);
        }

        while (state.KeepRunning())
        {
            EBusPerfBaselineBus::Broadcast(&EBusPerfBaseline::OnSignal, 1);
        }
    }
    BENCHMARK(BM_EventPerf_EBusIncrement);

    static void BM_EventPerf_EBusIncrementLambda(benchmark::State& state)
    {
        int32_t incrementCounter = 0;
        EBusPerfBaselineImplEmpty testHandler[NumHandlers];

        auto invokeFunc = [&incrementCounter](EBusPerfBaseline*, [[maybe_unused]] int32_t value) { ++incrementCounter; };
        while (state.KeepRunning())
        {
            EBusPerfBaselineBus::Broadcast(invokeFunc, 1);
        }
    }
    BENCHMARK(BM_EventPerf_EBusIncrementLambda);
}
#endif // HAVE_BENCHMARK
