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

#include <AzCore/EBus/Event.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    struct EventInt32Info
    {
        using EventType = AZ::Event<int32_t>;
        using EventTypeRef = AZ::Event<int32_t&>;
        using EventTypeConstRef = AZ::Event<const int32_t&>;
        using EventTypePtr = AZ::Event<int32_t*>;
        using EventTypeConstPtr = AZ::Event<const int32_t*>;
    };

    struct ThreadSafeEventInt32Info
    {
        using EventType = AZ::ThreadSafeEvent<int32_t>;
        using EventTypeRef = AZ::ThreadSafeEvent<int32_t&>;
        using EventTypeConstRef = AZ::ThreadSafeEvent<const int32_t&>;
        using EventTypePtr = AZ::ThreadSafeEvent<int32_t*>;
        using EventTypeConstPtr = AZ::ThreadSafeEvent<const int32_t*>;
    };

    template<typename TestStruct>
    struct EventTests
        : public ScopedAllocatorSetupFixture
    {
        using EventType = typename TestStruct::EventType;
        using EventTypeRef = typename TestStruct::EventTypeRef;
        using EventTypeConstRef = typename TestStruct::EventTypeConstRef;
        using EventTypePtr = typename TestStruct::EventTypePtr;
        using EventTypeConstPtr = typename TestStruct::EventTypeConstPtr;
    };

    using EventTestTypes = ::testing::Types<EventInt32Info, ThreadSafeEventInt32Info>;
    TYPED_TEST_CASE(EventTests, EventTestTypes);

    TYPED_TEST(EventTests, TestHasCallback)
    {
        EventType testEvent;
        EventType::Handler testHandler([]([[maybe_unused]] int32_t value) {});

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
        testHandler.Connect(testEvent);
        EXPECT_TRUE(testEvent.HasHandlerConnected());
    }

    TYPED_TEST(EventTests, TestScopedConnect)
    {
        EventType testEvent;

        {
            EventType::Handler testHandler([]([[maybe_unused]] int32_t value) {});
            testHandler.Connect(testEvent);
            EXPECT_TRUE(testEvent.HasHandlerConnected());
        }

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
    }

    TYPED_TEST(EventTests, TestEvent)
    {
        int32_t invokedValue = 0;

        EventType testEvent;
        EventType::Handler testHandler([&invokedValue](int32_t value) { invokedValue = value; });

        testHandler.Connect(testEvent);

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(1);
        EXPECT_TRUE(invokedValue == 1);
        testEvent.Signal(-1);
        EXPECT_TRUE(invokedValue == -1);
    }

    TYPED_TEST(EventTests, TestEventRValueParam)
    {
        int32_t invokedValue = 0;

        EventType testEvent;
        EventType::Handler testHandler([&invokedValue](int32_t value) { invokedValue = value; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(value);
        EXPECT_TRUE(invokedValue == 1);
    }

    TYPED_TEST(EventTests, TestEventRefParam)
    {
        int32_t invokedValue = 0;

        EventTypeRef testEvent;
        EventTypeRef::Handler testHandler([&invokedValue](int32_t& value) { invokedValue = value++; });

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

    TYPED_TEST(EventTests, TestEventConstRefParam)
    {
        int32_t invokedValue = 0;

        EventTypeConstRef testEvent;
        EventTypeConstRef::Handler testHandler([&invokedValue](const int32_t& value) { invokedValue = value; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(value);
        EXPECT_TRUE(invokedValue == 1);
    }

    TYPED_TEST(EventTests, TestEventPointerParam)
    {
        int32_t invokedValue = 0;

        EventTypePtr testEvent;
        EventTypePtr::Handler testHandler([&invokedValue](int32_t* value) { invokedValue = (*value)++; });

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

    TYPED_TEST(EventTests, TestEventConstPointerParam)
    {
        int32_t invokedValue = 0;

        EventTypeConstPtr testEvent;
        EventTypeConstPtr::Handler testHandler([&invokedValue](const int32_t* value) { invokedValue = *value; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(&value);
        EXPECT_TRUE(invokedValue == 1);
    }

    TYPED_TEST(EventTests, TestConnectDuringEvent)
    {
        EventType testEvent;

        {
            int32_t testHandler2Data = 0;
            EventType::Handler testHandler2([&testHandler2Data](int32_t value) { testHandler2Data = value; });

            EventType::Handler testHandler([&testHandler2, &testEvent]([[maybe_unused]] int32_t value) { testHandler2.Connect(testEvent); });
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

    TYPED_TEST(EventTests, TestDisconnectDuringEvent)
    {
        EventType testEvent;

        {
            int32_t testHandler2Data = 0;
            EventType::Handler testHandler2([&testHandler2Data](int32_t value) { testHandler2Data = value; });
            EventType::Handler testHandler([&testHandler2]([[maybe_unused]] int32_t value) { testHandler2.Disconnect(); });

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

    TYPED_TEST(EventTests, TestDisconnectDuringEventReversed)
    {
        EventType testEvent;

        // Same test as above, but connected using reversed ordering
        {
            int32_t testHandler2Data = 0;
            EventType::Handler testHandler2([&testHandler2Data](int32_t value) { testHandler2Data = value; });
            EventType::Handler testHandler([&testHandler2]([[maybe_unused]] int32_t value) { testHandler2.Disconnect(); });

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

    TYPED_TEST(EventTests, CopyConstructorAndCopyAssignmentOperator_AreNotCallable)
    {
        static_assert(!AZStd::is_copy_constructible_v<EventType>, "AZ Events should not be copy constructible");
        static_assert(!AZStd::is_copy_assignable_v<EventType>, "AZ Events should not be copy assignable");
    }

    class AdditionalEventTests : public ScopedAllocatorSetupFixture
    {
    };

    TEST_F(AdditionalEventTests, TestEventMultiParam)
    {
        int32_t invokedValue1 = 0;
        bool invokedValue2 = false;

        AZ::Event<int32_t, bool> testEvent;
        AZ::Event<int32_t, bool>::Handler testHandler(
            [&invokedValue1, &invokedValue2](int32_t value1, bool value2)
            {
                invokedValue1 = value1;
                invokedValue2 = value2;
            });

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

    TEST_F(AdditionalEventTests, HandlerMoveAssignment_ProperlyDisconnectsFromOldEvent)
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
        ~EBusPerfBaselineImplEmpty() { EBusPerfBaselineBus::Handler::BusDisconnect(); }
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
        ~EBusPerfBaselineImplIncrement() { EBusPerfBaselineBus::Handler::BusDisconnect(); }
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
