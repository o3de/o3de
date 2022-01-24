/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class GradientSignalTestHelpers
    {
    public:
        static void CompareGetValueAndGetValues(AZ::EntityId gradientEntityId, float shapeHalfBounds);

#ifdef HAVE_BENCHMARK
        // We use an enum to list out the different types of GetValue() benchmarks to run so that way we can condense our test cases
        // to just take the value in as a benchmark argument and switch on it. Otherwise, we would need to write a different benchmark
        // function for each test case for each gradient.
        enum GetValuePermutation : int64_t
        {
            EBUS_GET_VALUE,
            EBUS_GET_VALUES,
            SAMPLER_GET_VALUE,
            SAMPLER_GET_VALUES,
        };

        static void FillQueryPositions(AZStd::vector<AZ::Vector3>& positions, float height, float width);

        static void RunEBusGetValueBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange);
        static void RunEBusGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange);
        static void RunSamplerGetValueBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange);
        static void RunSamplerGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId, int64_t queryRange);
        static void RunGetValueOrGetValuesBenchmark(benchmark::State& state, const AZ::EntityId& gradientId);

// Because there's no good way to label different enums in the output results (they just appear as integer values), we work around it by
// registering one set of benchmark runs for each enum value and use ArgNames() to give it a friendly name in the results.
#ifndef GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F
#define GRADIENT_SIGNAL_GET_VALUES_BENCHMARK_REGISTER_F(Fixture, Func)                                                                    \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                   \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::EBUS_GET_VALUE, 1024 })                                                  \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::EBUS_GET_VALUE, 2048 })                                                  \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::EBUS_GET_VALUE, 4096 })                                                  \
        ->ArgNames({ "EbusGetValue", "size" })                                                                                            \
        ->Unit(::benchmark::kMillisecond);                                                                                                \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                   \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::EBUS_GET_VALUES, 1024 })                                                 \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::EBUS_GET_VALUES, 2048 })                                                 \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::EBUS_GET_VALUES, 4096 })                                                 \
        ->ArgNames({ "EbusGetValues", "size" })                                                                                           \
        ->Unit(::benchmark::kMillisecond);                                                                                                \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                   \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::SAMPLER_GET_VALUE, 1024 })                                               \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::SAMPLER_GET_VALUE, 2048 })                                               \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::SAMPLER_GET_VALUE, 4096 })                                               \
        ->ArgNames({ "SamplerGetValue", "size" })                                                                                         \
        ->Unit(::benchmark::kMillisecond);                                                                                                \
    BENCHMARK_REGISTER_F(Fixture, Func)                                                                                                   \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::SAMPLER_GET_VALUES, 1024 })                                              \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::SAMPLER_GET_VALUES, 2048 })                                              \
        ->Args({ GradientSignalTestHelpers::GetValuePermutation::SAMPLER_GET_VALUES, 4096 })                                              \
        ->ArgNames({ "SamplerGetValues", "size" })                                                                                        \
        ->Unit(::benchmark::kMillisecond);
#endif

#endif
    };


}
