/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/DOM/DomFixtures.h>
#include <AzCore/DOM/DomPath.h>

namespace AZ::Dom::Benchmark
{
    using DomPathBenchmark = Tests::DomBenchmarkFixture;

    BENCHMARK_DEFINE_F(DomPathBenchmark, DomPath_Concatenate_InPlace)(benchmark::State& state)
    {
        AZ::Name entry1("entry1");
        AZ::Name entry2("entry2");
        PathEntry end;
        end.SetEndOfArray();

        for (auto _ : state)
        {
            Path p;
            p /= entry1;
            p /= entry2;
            p /= 0;
            p /= end;
        }

        state.SetItemsProcessed(4 * state.iterations());
    }
    BENCHMARK_REGISTER_F(DomPathBenchmark, DomPath_Concatenate_InPlace);

    BENCHMARK_DEFINE_F(DomPathBenchmark, DomPath_Concatenate_Copy)(benchmark::State& state)
    {
        AZ::Name entry1("entry1");
        AZ::Name entry2("entry2");
        PathEntry end;
        end.SetEndOfArray();

        for (auto _ : state)
        {
            Path p = Path() / entry1 / entry2 / 0 / end;
        }

        state.SetItemsProcessed(4 * state.iterations());
    }
    BENCHMARK_REGISTER_F(DomPathBenchmark, DomPath_Concatenate_Copy);

    BENCHMARK_DEFINE_F(DomPathBenchmark, DomPath_ToString)(benchmark::State& state)
    {
        Path p("/path/with/multiple/0/different/components/65536/999/-");
        AZStd::string s;
        s.resize_no_construct(p.GetStringLength());

        for (auto _ : state)
        {
            p.GetStringLength();
            p.FormatString(s.data(), s.size());
        }

        state.SetBytesProcessed(s.size() * state.iterations());
    }
    BENCHMARK_REGISTER_F(DomPathBenchmark, DomPath_ToString);

    BENCHMARK_DEFINE_F(DomPathBenchmark, DomPath_FromString)(benchmark::State& state)
    {
        AZStd::string pathString = "/path/with/multiple/0/different/components/including-long-strings-like-this/65536/999/-";

        for (auto _ : state)
        {
            Path p(pathString);
            benchmark::DoNotOptimize(p);
        }

        state.SetBytesProcessed(pathString.size() * state.iterations());
    }
    BENCHMARK_REGISTER_F(DomPathBenchmark, DomPath_FromString);

    BENCHMARK_DEFINE_F(DomPathBenchmark, DomPathEntry_IsEndOfArray)(benchmark::State& state)
    {
        PathEntry name("name");
        PathEntry index(0);
        PathEntry endOfArray;
        endOfArray.SetEndOfArray();

        for (auto _ : state)
        {
            name.IsEndOfArray();
            index.IsEndOfArray();
            endOfArray.IsEndOfArray();
        }

        state.SetItemsProcessed(3 * state.iterations());
    }
    BENCHMARK_REGISTER_F(DomPathBenchmark, DomPathEntry_IsEndOfArray);
}
