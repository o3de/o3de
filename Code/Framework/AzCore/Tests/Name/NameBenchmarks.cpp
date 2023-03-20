/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace AZ::NameBenchmarks
{
    class NameBenchmarkFixture : public UnitTest::AllocatorsBenchmarkFixture
    {
    public:
        void SetUp(const ::benchmark::State& st) override
        {
            UnitTest::AllocatorsBenchmarkFixture::SetUp(st);
            AZ::NameDictionary::Create();
        }

        void SetUp(::benchmark::State& st) override
        {
            UnitTest::AllocatorsBenchmarkFixture::SetUp(st);
            AZ::NameDictionary::Create();
        }

        void TearDown(::benchmark::State& st) override
        {
            AZ::NameDictionary::Destroy();
            UnitTest::AllocatorsBenchmarkFixture::TearDown(st);
        }

        void TearDown(const ::benchmark::State& st) override
        {
            AZ::NameDictionary::Destroy();
            UnitTest::AllocatorsBenchmarkFixture::TearDown(st);
        }

        AZ::Name NameFromCachedLiteral()
        {
            return AZ_NAME_LITERAL("test_literal");
        }

        AZ::Name NameFromUncachedLiteral()
        {
            return AZ::Name("test_literal");
        }
    };

    BENCHMARK_DEFINE_F(NameBenchmarkFixture, CreateNameCacheHit)(::benchmark::State& state)
    {
        constexpr size_t poolSize = 100;
        AZStd::vector<AZ::Name> existingNames;
        for (size_t i = 0; i < poolSize; ++i)
        {
            existingNames.emplace_back(AZStd::string::format("name%zu", i));
        }

        for ([[maybe_unused]] auto var_ : state)
        {
            for (size_t i = 0; i < poolSize; ++i)
            {
                benchmark::DoNotOptimize(AZ::Name(existingNames[i].GetStringView()));
            }
        }

        state.SetItemsProcessed(state.iterations() * poolSize);
    }
    BENCHMARK_REGISTER_F(NameBenchmarkFixture, CreateNameCacheHit);

    BENCHMARK_DEFINE_F(NameBenchmarkFixture, CreateNameCacheMiss)(::benchmark::State& state)
    {
        constexpr size_t poolSize = 100;
        AZStd::vector<AZStd::string> namesToCreate;
        for (size_t i = 0; i < poolSize; ++i)
        {
            namesToCreate.emplace_back(AZStd::string::format("name%zu", i));
        }

        for ([[maybe_unused]] auto var_ : state)
        {
            for (size_t i = 0; i < poolSize; ++i)
            {
                benchmark::DoNotOptimize(AZ::Name(namesToCreate[i]));
            }
        }

        state.SetItemsProcessed(state.iterations() * poolSize);
    }
    BENCHMARK_REGISTER_F(NameBenchmarkFixture, CreateNameCacheMiss);

    BENCHMARK_DEFINE_F(NameBenchmarkFixture, CopyName)(::benchmark::State& state)
    {
        constexpr size_t poolSize = 100;
        AZStd::vector<AZ::Name> existingNames;
        for (size_t i = 0; i < poolSize; ++i)
        {
            existingNames.emplace_back(AZStd::string::format("name%zu", i));
        }

        for ([[maybe_unused]] auto var_ : state)
        {
            for (size_t i = 0; i < poolSize; ++i)
            {
                benchmark::DoNotOptimize(AZ::Name(existingNames[i]));
            }
        }

        state.SetItemsProcessed(state.iterations() * poolSize);
    }
    BENCHMARK_REGISTER_F(NameBenchmarkFixture, CopyName);

    BENCHMARK_DEFINE_F(NameBenchmarkFixture, RetrieveName_WithNameLiteral)(::benchmark::State& state)
    {
        for ([[maybe_unused]] auto var_ : state)
        {
            benchmark::DoNotOptimize(AZ::Name(NameFromCachedLiteral()));
        }

        state.SetItemsProcessed(state.iterations());
    }
    BENCHMARK_REGISTER_F(NameBenchmarkFixture, RetrieveName_WithNameLiteral);

    BENCHMARK_DEFINE_F(NameBenchmarkFixture, RetrieveName_WithoutNameLiteral)(::benchmark::State& state)
    {
        for ([[maybe_unused]] auto var_ : state)
        {
            benchmark::DoNotOptimize(AZ::Name(NameFromUncachedLiteral()));
        }

        state.SetItemsProcessed(state.iterations());
    }
    BENCHMARK_REGISTER_F(NameBenchmarkFixture, RetrieveName_WithoutNameLiteral);

    BENCHMARK_DEFINE_F(NameBenchmarkFixture, NameCreateAndDestroy)(::benchmark::State& state)
    {
        AZStd::vector<AZ::Name> names;
        names.resize(state.range(0));

        for ([[maybe_unused]] auto var_ : state)
        {
            for (int64_t i = 0; i < state.range(0); ++i)
            {
                names[i] = (AZ::Name("not created as a literal"));
            }
            for (int64_t i = 0; i < state.range(0); ++i)
            {
                names[i] = AZ::Name();
            }
        }

        state.SetItemsProcessed(state.iterations() * state.range(0));
    }
    BENCHMARK_REGISTER_F(NameBenchmarkFixture, NameCreateAndDestroy)->Arg(10)->Arg(100)->Arg(1000);

    BENCHMARK_DEFINE_F(NameBenchmarkFixture, NameLiteralCreateAndDestroy)(::benchmark::State& state)
    {
        AZStd::vector<AZ::Name> names;
        names.resize(state.range(0));

        for ([[maybe_unused]] auto var_ : state)
        {
            for (int64_t i = 0; i < state.range(0); ++i)
            {
                names[i] = (AZ::Name::FromStringLiteral("created as a literal", AZ::Interface<AZ::NameDictionary>::Get()));
            }
            for (int64_t i = 0; i < state.range(0); ++i)
            {
                names[i] = AZ::Name();
            }
        }

        state.SetItemsProcessed(state.iterations() * state.range(0));
    }
    BENCHMARK_REGISTER_F(NameBenchmarkFixture, NameLiteralCreateAndDestroy)->Arg(10)->Arg(100)->Arg(1000);
} // namespace AZ::NameBenchmarks
