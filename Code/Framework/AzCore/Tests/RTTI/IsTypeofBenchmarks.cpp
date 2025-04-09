/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <benchmark/benchmark.h>

namespace AZ
{
    class IsTypeofBenchmarkBaseType
    {
    public:
        virtual ~IsTypeofBenchmarkBaseType() = default;
        AZ_RTTI(IsTypeofBenchmarkBaseType, "{4699B4B3-2A44-4A02-9F70-CD9889B4B36D}")
    };

    class IsTypeOfBenchmarkDerivedType : public IsTypeofBenchmarkBaseType
    {
    public:
        AZ_RTTI(IsTypeOfBenchmarkDerivedType, "{C89B7EAC-26A0-4B37-A14E-CEC20FCAD7A8}", IsTypeofBenchmarkBaseType)
    };

    static void BenchmarkIsTypeof(benchmark::State& state)
    {
        auto derived = AZStd::make_unique<IsTypeOfBenchmarkDerivedType>();
        IsTypeofBenchmarkBaseType* base = derived.get();
        for (auto _ : state)
        {
            azrtti_istypeof<IsTypeOfBenchmarkDerivedType>(base);
        }
    }

    BENCHMARK(BenchmarkIsTypeof);
} // namespace AZ

#endif
