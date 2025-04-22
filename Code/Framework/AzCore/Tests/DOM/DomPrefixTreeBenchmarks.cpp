/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/DOM/DomFixtures.h>
#include <AzCore/DOM/DomPrefixTree.h>

#define REGISTER_TREE_BENCHMARK(BaseClass, Method) \
    BENCHMARK_REGISTER_F(BaseClass, Method)->Args({10, 1})->Args({1000, 1})->Args({10, 5})->Args({1000, 5})

namespace AZ::Dom::Benchmark
{
    class DomPrefixTreeBenchmark : public Tests::DomBenchmarkFixture
    {
    public:
        void SetUpHarness() override
        {
            Tests::DomBenchmarkFixture::SetUpHarness();
            m_registeredPaths = AZStd::make_unique<AZStd::vector<Path>>();
        }

        void TearDownHarness() override
        {
            m_registeredPaths.reset();
            m_tree.Clear();
            Tests::DomBenchmarkFixture::TearDownHarness();
        }

        void SetupTree(benchmark::State& state)
        {
            const size_t numPaths = aznumeric_cast<size_t>(state.range(0));
            const size_t depth = aznumeric_cast<size_t>(state.range(1)); 

            Path path("/root");
            for (size_t i = 0; i < numPaths; ++i)
            {
                for (size_t c = 0; c < depth; ++c)
                {
                    path.Push(i % 4);
                    m_tree.SetValue(path, AZStd::string::format("entry%zu", i));
                    m_registeredPaths->push_back(path);
                }

                for (size_t c = 0; c < depth; ++c)
                {
                    path.Pop();
                }
            }
        }

        DomPrefixTree<AZStd::string> m_tree;
        AZStd::unique_ptr<AZStd::vector<Path>> m_registeredPaths;
    };

    BENCHMARK_DEFINE_F(DomPrefixTreeBenchmark, FindValue_ExactPath)(benchmark::State& state)
    {
        SetupTree(state);
        for ([[maybe_unused]] auto _ : state)
        {
            for (const auto& pathToCheck : *m_registeredPaths)
            {
                benchmark::DoNotOptimize(m_tree.ValueAtPath(pathToCheck, PrefixTreeMatch::ExactPath));
            }
        }
        state.SetItemsProcessed(m_registeredPaths->size() * state.iterations());
    }
    REGISTER_TREE_BENCHMARK(DomPrefixTreeBenchmark, FindValue_ExactPath);

    BENCHMARK_DEFINE_F(DomPrefixTreeBenchmark, FindValue_InexactPath)(benchmark::State& state)
    {
        SetupTree(state);
        for ([[maybe_unused]] auto _ : state)
        {
            for (const auto& pathToCheck : *m_registeredPaths)
            {
                benchmark::DoNotOptimize(m_tree.ValueAtPath(pathToCheck, PrefixTreeMatch::PathAndParents));
            }
        }
        state.SetItemsProcessed(m_registeredPaths->size() * state.iterations());
    }
    REGISTER_TREE_BENCHMARK(DomPrefixTreeBenchmark, FindValue_InexactPath);

    BENCHMARK_DEFINE_F(DomPrefixTreeBenchmark, FindValue_VisitEntries_LeastToMostSpecific)(benchmark::State& state)
    {
        SetupTree(state);

        for ([[maybe_unused]] auto _ : state)
        {
            m_tree.VisitPath(Path(), [](const Path& path, const AZStd::string& value)
            {
                benchmark::DoNotOptimize(path);
                benchmark::DoNotOptimize(value);
                return true;
            });
        }
        state.SetItemsProcessed(m_registeredPaths->size() * state.iterations());
    }
    REGISTER_TREE_BENCHMARK(DomPrefixTreeBenchmark, FindValue_VisitEntries_LeastToMostSpecific);

    BENCHMARK_DEFINE_F(DomPrefixTreeBenchmark, FindValue_VisitEntries_MostToLeastSpecific)(benchmark::State& state)
    {
        SetupTree(state);

        for ([[maybe_unused]] auto _ : state)
        {
            m_tree.VisitPath(
                Path(),
                [](const Path& path, const AZStd::string& value)
                {
                    benchmark::DoNotOptimize(path);
                    benchmark::DoNotOptimize(value);
                    return true;
                }, PrefixTreeTraversalFlags::TraverseMostToLeastSpecific);
        }
        state.SetItemsProcessed(m_registeredPaths->size() * state.iterations());
    }
    REGISTER_TREE_BENCHMARK(DomPrefixTreeBenchmark, FindValue_VisitEntries_MostToLeastSpecific);
}

#undef REGISTER_TREE_BENCHMARK
