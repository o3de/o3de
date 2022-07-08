/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/OrderedEvent.h>
#include <AzCore/UnitTest/TestTypes.h>

#if defined(HAVE_BENCHMARK)
//-------------------------------------------------------------------------
// PERF TESTS
//-------------------------------------------------------------------------

#include <benchmark/benchmark.h>

namespace Benchmark
{
    static constexpr int32_t NumHandlers = 10000;

    static void BM_OrderedEventPerf_EventEmpty(benchmark::State& state)
    {
        AZ::OrderedEvent<int32_t> testEvent;
        AZ::OrderedEventHandler<int32_t> testHandler[NumHandlers];

        for (int32_t i = 0; i < NumHandlers; ++i)
        {
            testHandler[i] = AZ::OrderedEventHandler<int32_t>([]([[maybe_unused]] int32_t value) {});
            testHandler[i].Connect(testEvent);
        }

        while (state.KeepRunning())
        {
            testEvent.Signal(1);
        }
    }
    BENCHMARK(BM_OrderedEventPerf_EventEmpty);

    static void BM_OrderedEventPerf_EventIncrement(benchmark::State& state)
    {
        AZ::OrderedEvent<int32_t> testEvent;
        AZ::OrderedEventHandler<int32_t> testHandler[NumHandlers];

        int32_t incrementCounter = 0;

        for (int32_t i = 0; i < NumHandlers; ++i)
        {
            testHandler[i] = AZ::OrderedEventHandler<int32_t>([&incrementCounter]([[maybe_unused]] int32_t value) { ++incrementCounter; });
            testHandler[i].Connect(testEvent);
        }

        while (state.KeepRunning())
        {
            testEvent.Signal(1);
        }
    }
    BENCHMARK(BM_OrderedEventPerf_EventIncrement);

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

    static void BM_OrderedEventPerf_EBusEmpty(benchmark::State& state)
    {
        EBusPerfBaselineImplEmpty testHandler[NumHandlers];

        while (state.KeepRunning())
        {
            EBusPerfBaselineBus::Broadcast(&EBusPerfBaseline::OnSignal, 1);
        }
    }
    BENCHMARK(BM_OrderedEventPerf_EBusEmpty);

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

    static void BM_OrderedEventPerf_EBusIncrement(benchmark::State& state)
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
    BENCHMARK(BM_OrderedEventPerf_EBusIncrement);

    static void BM_OrderedEventPerf_EBusIncrementLambda(benchmark::State& state)
    {
        int32_t incrementCounter = 0;
        EBusPerfBaselineImplEmpty testHandler[NumHandlers];

        auto invokeFunc = [&incrementCounter](EBusPerfBaseline*, [[maybe_unused]] int32_t value) { ++incrementCounter; };
        while (state.KeepRunning())
        {
            EBusPerfBaselineBus::Broadcast(invokeFunc, 1);
        }
    }
    BENCHMARK(BM_OrderedEventPerf_EBusIncrementLambda);
}
#endif // HAVE_BENCHMARK
