/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/ranges/ranges_algorithm.h>


#if defined(HAVE_BENCHMARK)
namespace Benchmark
{
    class RangesAlgorithmBenchmarkFixture
        : public UnitTest::AllocatorsBenchmarkFixture
    {};

    // Benchmarks for ranges copy when using trivally copyable types with contiguous iterators
    // memcpy or memmove should be used under the hood
    BENCHMARK_F(RangesAlgorithmBenchmarkFixture, BM_RangesCopy_TriviallyCopyableType_DirectCall)(benchmark::State& state)
    {
        AZStd::array test1 = AZStd::to_array<const char>("The brown quick wolf jumped over the hyperactive cat");
        [[maybe_unused]] AZStd::array<char, test1.size()> result;
        for ([[maybe_unused]] auto _ : state)
        {
            // In profile configuration the AZStd::ranges::copy call triggers
            // the overlapping range assert, most likely due to optimizations with result not being used
            // Using DoNotOptimize resolves the issue
            benchmark::DoNotOptimize(AZStd::ranges::copy(test1, result.begin()));
        }
    }

    BENCHMARK_F(RangesAlgorithmBenchmarkFixture, BM_RangesCopyN_TriviallyCopyableType_DirectCall)(benchmark::State& state)
    {
        AZStd::array test1 = AZStd::to_array<const char>("The brown quick wolf jumped over the hyperactive cat");
        [[maybe_unused]] AZStd::array<char, test1.size()> result;
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(AZStd::ranges::copy_n(test1.begin(), test1.size(), result.begin()));
        }
    }

    BENCHMARK_F(RangesAlgorithmBenchmarkFixture, BM_RangesCopyBackward_TriviallyCopyableType_DirectCall)(benchmark::State& state)
    {
        AZStd::array test1 = AZStd::to_array<const char>("The brown quick wolf jumped over the hyperactive cat");
        [[maybe_unused]] AZStd::array<char, test1.size()> result;
        for ([[maybe_unused]] auto _ : state)
        {
            // copy_backward requires past the end of range sentinel and iterates backwards
            benchmark::DoNotOptimize(AZStd::ranges::copy_backward(test1, result.end()));
        }
    }

    BENCHMARK_F(RangesAlgorithmBenchmarkFixture, BM_RangesCopy_TriviallyCopyableType_CompileTime_DirectCall)(benchmark::State& state)
    {
        auto invokeAtCompileTime = []()
        {
            constexpr auto copyArray = []() constexpr
            {
                constexpr AZStd::array test1 = AZStd::to_array<const char>("The brown quick wolf jumped over the hyperactive cat");
                AZStd::array<char, test1.size()> resultArray{};
                AZStd::ranges::copy(test1, resultArray.begin());
                return resultArray;
            };

            [[maybe_unused]] constexpr auto result = copyArray();
            return true;
        };
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(invokeAtCompileTime());
        }
    }

    BENCHMARK_F(RangesAlgorithmBenchmarkFixture, BM_RangesCopyN_TriviallyCopyableType_CompileTime_DirectCall)(benchmark::State& state)
    {
        auto invokeAtCompileTime = []()
        {
            constexpr auto copyArray = []() constexpr
            {
                constexpr AZStd::array test1 = AZStd::to_array<const char>("The brown quick wolf jumped over the hyperactive cat");
                AZStd::array<char, test1.size()> resultArray{};
                AZStd::ranges::copy_n(test1.begin(), test1.size(), resultArray.begin());
                return resultArray;
            };
            [[maybe_unused]] constexpr auto result = copyArray();
            return true;
        };

        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(invokeAtCompileTime());
        }
    }

    BENCHMARK_F(RangesAlgorithmBenchmarkFixture, BM_RangesCopyBackward_TriviallyCopyableType_CompileTime_DirectCall)(benchmark::State& state)
    {
        auto invokeAtCompileTime = []()
        {
            constexpr auto copyArray = []() constexpr
            {
                constexpr AZStd::array test1 = AZStd::to_array<const char>("The brown quick wolf jumped over the hyperactive cat");
                AZStd::array<char, test1.size()> resultArray{};
                AZStd::ranges::copy_backward(test1, resultArray.end());
                return resultArray;
            };
            [[maybe_unused]] constexpr auto result = copyArray();
            return true;
        };

        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(invokeAtCompileTime());
        }
    }

    BENCHMARK_F(RangesAlgorithmBenchmarkFixture, BM_RangesCopy_TriviallyCopyableType_MemcpyCall)(benchmark::State& state)
    {
        AZStd::array test1 = AZStd::to_array<const char>("The brown quick wolf jumped over the hyperactive cat");
        [[maybe_unused]] AZStd::array<char, test1.size()> result;
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(memcpy(result.data(), test1.data(), test1.size() * sizeof(char)));
        }
    }

    BENCHMARK_F(RangesAlgorithmBenchmarkFixture, BM_RangesCopy_TriviallyCopyableType_MemmoveCall)(benchmark::State& state)
    {
        AZStd::array test1 = AZStd::to_array<const char>("The brown quick wolf jumped over the hyperactive cat");
        [[maybe_unused]] AZStd::array<char, test1.size()> result;
        for ([[maybe_unused]] auto _ : state)
        {
            benchmark::DoNotOptimize(memmove(result.data(), test1.data(), test1.size() * sizeof(char)));
        }
    }

    BENCHMARK_F(RangesAlgorithmBenchmarkFixture, BM_RangesCopy_TriviallyCopyableType_ForLoopImplementation)(benchmark::State& state)
    {
        AZStd::array test1 = AZStd::to_array<const char>("The brown quick wolf jumped over the hyperactive cat");
        [[maybe_unused]] AZStd::array<char, test1.size()> resultArray;
        auto forLoopCopy = [&test1, &resultArray]() constexpr
        {
            auto first = test1.begin();
            auto last = test1.end();
            auto result = resultArray.begin();
            for (; first != last; ++first, ++result)
            {
                *result = *first;
            }

            return true;
        };

        for ([[maybe_unused]] auto _ : state)
        {
            bool ranLoop = forLoopCopy();
            benchmark::DoNotOptimize(ranLoop);
        }
    }
} // Benchmark
#endif
