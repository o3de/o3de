/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomUtils.h>
#include <AzCore/JSON/document.h>
#include <AzCore/UnitTest/TestTypes.h>

#define DOM_REGISTER_SERIALIZATION_BENCHMARK(BaseClass, Method)                                                                            \
    BENCHMARK_REGISTER_F(BaseClass, Method)->Args({ 10, 5 })->Args({ 10, 500 })->Args({ 100, 5 })->Args({ 100, 500 })
#define DOM_REGISTER_SERIALIZATION_BENCHMARK_MS(BaseClass, Method)                                                                         \
    DOM_REGISTER_SERIALIZATION_BENCHMARK(BaseClass, Method)->Unit(benchmark::kMillisecond);
#define DOM_REGISTER_SERIALIZATION_BENCHMARK_NS(BaseClass, Method)                                                                         \
    DOM_REGISTER_SERIALIZATION_BENCHMARK(BaseClass, Method)->Unit(benchmark::kNanosecond);

namespace AZ::Dom::Tests
{
    class DomTestHarness
    {
    public:
        virtual ~DomTestHarness() = default;

        virtual void SetUpHarness();
        virtual void TearDownHarness();
    };

    class DomBenchmarkFixture
        : public DomTestHarness
        , public UnitTest::AllocatorsBenchmarkFixture
    {
    public:
        void SetUp(const ::benchmark::State& st) override;
        void SetUp(::benchmark::State& st) override;
        void TearDown(::benchmark::State& st) override;
        void TearDown(const ::benchmark::State& st) override;

        rapidjson::Document GenerateDomJsonBenchmarkDocument(int64_t entryCount, int64_t stringTemplateLength);
        AZStd::string GenerateDomJsonBenchmarkPayload(int64_t entryCount, int64_t stringTemplateLength);
        Value GenerateDomBenchmarkPayload(int64_t entryCount, int64_t stringTemplateLength);

        template<class T>
        static void TakeAndDiscardWithoutTimingDtor(T&& value, benchmark::State& state)
        {
            {
                T instance = AZStd::move(value);
                state.PauseTiming();
            }
            state.ResumeTiming();
        }
    };

    class DomTestFixture
        : public DomTestHarness
        , public UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override;
        void TearDown() override;
    };
} // namespace AZ::Dom::Tests
