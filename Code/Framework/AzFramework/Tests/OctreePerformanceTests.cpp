/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzFramework/Visibility/OctreeSystemComponent.h>

#if defined(HAVE_BENCHMARK)

#include <random>
#include <benchmark/benchmark.h>

namespace Benchmark
{
    class BM_Octree
        : public benchmark::Fixture
    {
        void internalSetUp()
        {
            m_console = new AZ::Console;
            AZ::Interface<AZ::IConsole>::Register(m_console);
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
            m_console->GetCvarValue("bg_octreeUseQuadtree", m_savedUseQuadtree);
            m_console->GetCvarValue("bg_octreeMaxWorldExtents", m_savedBounds);
            m_console->GetCvarValue("bg_octreeNodeMaxEntries", m_savedMaxEntries);
            m_console->GetCvarValue("bg_octreeNodeMinEntries", m_savedMinEntries);
            m_console->PerformCommand("bg_octreeUseQuadtree false", AZ::ConsoleSilentMode::Silent, AZ::ConsoleInvokedFrom::AzConsole, AZ::ConsoleFunctorFlags::Null, AZ::ConsoleFunctorFlags::Null);
            m_console->PerformCommand("bg_octreeMaxWorldExtents 16384");
            m_console->PerformCommand("bg_octreeNodeMaxEntries 64");
            m_console->PerformCommand("bg_octreeNodeMinEntries 32");
            if (!AZ::NameDictionary::IsReady())
            {
                AZ::NameDictionary::Create();
            }
            m_octreeSystemComponent = new AzFramework::OctreeSystemComponent;
            m_visScene = m_octreeSystemComponent->CreateVisibilityScene(AZ::Name("OctreeBenchmarkVisibilityScene"));
            m_dataArray.resize(1000000);
            m_queryDataArray.resize(1000);

            const unsigned int seed = 1;
            std::mt19937_64 rng(seed);
            std::uniform_real_distribution<float> unif;

            std::generate(m_dataArray.begin(), m_dataArray.end(), [&unif, &rng]()
            {
                AzFramework::VisibilityEntry data;
                AZ::Vector3 aabbMin = AZ::Vector3(unif(rng), unif(rng), unif(rng)) * 8000.0f;
                AZ::Vector3 aabbMax = AZ::Vector3(unif(rng), unif(rng), unif(rng)).GetAbs() * 50.0f + aabbMin;
                data.m_internalNode = nullptr;
                data.m_internalNodeIndex = 0;
                data.m_boundingVolume = AZ::Aabb::CreateFromMinMax(aabbMin, aabbMax);
                data.m_userData = nullptr;
                data.m_typeFlags = AzFramework::VisibilityEntry::TYPE_None;
                return data;
            });

            std::generate(m_queryDataArray.begin(), m_queryDataArray.end(), [&unif, &rng]()
            {
                QueryData data;
                AZ::Vector3 aabbMin = AZ::Vector3(unif(rng), unif(rng), unif(rng)) * 8000.0f;
                AZ::Vector3 aabbMax = AZ::Vector3(unif(rng), unif(rng), unif(rng)).GetAbs() * 250.0f + aabbMin;
                AZ::Vector3 frustumCenter = AZ::Vector3(unif(rng), unif(rng), unif(rng)) * 8000.0f;
                AZ::Quaternion quaternion = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3(unif(rng), unif(rng), unif(rng)).GetNormalized(), unif(rng));
                data.aabb = AZ::Aabb::CreateFromMinMax(aabbMin, aabbMax);
                data.sphere = AZ::Sphere(AZ::Vector3(unif(rng), unif(rng), unif(rng)) * 8000.0f, unif(rng) * 250.0f);
                data.frustum = AZ::Frustum(AZ::ViewFrustumAttributes(
                    AZ::Transform::CreateFromQuaternionAndTranslation(quaternion, frustumCenter), 1.0f,
                    2.0f * atanf(0.5f), unif(rng) * 10.0f, unif(rng) * 1000.0f));
                return data;
            });
        }

        void internalTearDown()
        {
            m_octreeSystemComponent->DestroyVisibilityScene(m_visScene);
            delete m_octreeSystemComponent;
            AZ::NameDictionary::Destroy();

            AZStd::string command = "bg_octreeUseQuadtree false";
            if (m_savedUseQuadtree)
            {
                command = "bg_octreeUseQuadtree true";
            }
            m_console->PerformCommand(command.c_str(), AZ::ConsoleSilentMode::Silent, AZ::ConsoleInvokedFrom::AzConsole, AZ::ConsoleFunctorFlags::Null, AZ::ConsoleFunctorFlags::Null);
            m_console->PerformCommand(AZStd::string::format("bg_octreeMaxWorldExtents %f", m_savedBounds).c_str());
            m_console->PerformCommand(AZStd::string::format("bg_octreeNodeMaxEntries %u", m_savedMaxEntries).c_str());
            m_console->PerformCommand(AZStd::string::format("bg_octreeNodeMinEntries %u", m_savedMinEntries).c_str());
            AZ::Interface<AZ::IConsole>::Unregister(m_console);
            delete m_console;

            m_dataArray.clear();
            m_dataArray.shrink_to_fit();

            m_queryDataArray.clear();
            m_queryDataArray.shrink_to_fit();
        }

    public:
        void SetUp(const benchmark::State&) override
        {
            internalSetUp();
        }
        void SetUp(benchmark::State&) override
        {
            internalSetUp();
        }

        void TearDown(const benchmark::State&) override
        {
            internalTearDown();
        }
        void TearDown(benchmark::State&) override
        {
            internalTearDown();
        }

        void InsertEntries(uint32_t entryCount)
        {
            for (uint32_t i = 0; i < entryCount; ++i)
            {
                m_visScene->InsertOrUpdateEntry(m_dataArray[i]);
            }
        }

        void RemoveEntries(uint32_t entryCount)
        {
            for (uint32_t i = 0; i < entryCount; ++i)
            {
                m_visScene->RemoveEntry(m_dataArray[i]);
            }
        }

        void SetRootStraddlingBounds(uint32_t entryCount)
        {
            const AZ::Aabb rootStraddlingBounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1.0f), AZ::Vector3(1.0f));
            for (uint32_t i = 0; i < entryCount; ++i)
            {
                m_dataArray[i].m_boundingVolume = rootStraddlingBounds;
            }
        }

        void VerifyGrowth(benchmark::State& state, AzFramework::VisibilityEntry& entry)
        {
            bool contained = false;
            m_visScene->Enumerate(entry.m_boundingVolume,
                [&entry, &contained](const AzFramework::IVisibilityScene::NodeData& nodeData)
                {
                    for (const auto* candidate : nodeData.m_entries)
                    {
                        if (candidate == &entry)
                        {
                            contained = AZ::ShapeIntersection::Contains(nodeData.m_bounds, entry.m_boundingVolume);
                        }
                    }
                });
            if (!contained)
            {
                state.SkipWithError("The entry beyond the initial root was not contained after growth");
            }
        }

        struct QueryData
        {
            AZ::Aabb aabb;
            AZ::Sphere sphere;
            AZ::Frustum frustum;
        };

        AZStd::vector<AzFramework::VisibilityEntry> m_dataArray;
        AZStd::vector<QueryData> m_queryDataArray;
        AzFramework::OctreeSystemComponent* m_octreeSystemComponent = nullptr;
        AzFramework::IVisibilityScene* m_visScene = nullptr;
        AZ::Console* m_console = nullptr;
        bool m_savedUseQuadtree = false;
        float m_savedBounds = 0.0f;
        uint32_t m_savedMaxEntries = 0;
        uint32_t m_savedMinEntries = 0;
    };

    BENCHMARK_F(BM_Octree, InsertDelete1000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000;
        for ([[maybe_unused]] auto _ : state)
        {
            InsertEntries(EntryCount);
            RemoveEntries(EntryCount);
        }
    }

    BENCHMARK_F(BM_Octree, InsertDelete10000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 10000;
        for ([[maybe_unused]] auto _ : state)
        {
            InsertEntries(EntryCount);
            RemoveEntries(EntryCount);
        }
    }

    BENCHMARK_F(BM_Octree, InsertDelete100000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 100000;
        for ([[maybe_unused]] auto _ : state)
        {
            InsertEntries(EntryCount);
            RemoveEntries(EntryCount);
        }
    }

    BENCHMARK_F(BM_Octree, InsertDelete1000000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000000;
        for ([[maybe_unused]] auto _ : state)
        {
            InsertEntries(EntryCount);
            RemoveEntries(EntryCount);
        }
    }

    BENCHMARK_DEFINE_F(BM_Octree, GrowToContainDistributed)(benchmark::State& state)
    {
        const uint32_t entryCount = aznumeric_cast<uint32_t>(state.range(0));
        for ([[maybe_unused]] auto _ : state)
        {
            state.PauseTiming();
            InsertEntries(entryCount);
            const AZ::Aabb originalBounds = m_dataArray[0].m_boundingVolume;
            constexpr float FarPosition = 35423.84375f;
            m_dataArray[0].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(FarPosition, 0.0f, 0.0f), AZ::Vector3(FarPosition + 0.25f, 0.25f, 0.25f));
            state.ResumeTiming();

            m_visScene->InsertOrUpdateEntry(m_dataArray[0]);

            state.PauseTiming();
            VerifyGrowth(state, m_dataArray[0]);
            RemoveEntries(entryCount);
            m_dataArray[0].m_boundingVolume = originalBounds;
            state.ResumeTiming();
        }
    }

    // Measure one fresh expansion per invocation. Use --benchmark_repetitions for statistical sampling.
    BENCHMARK_REGISTER_F(BM_Octree, GrowToContainDistributed)
        ->Arg(1000)
        ->Arg(10000)
        ->Arg(100000)
        ->Arg(1000000)
        ->Iterations(1)
        ->UseRealTime();

    BENCHMARK_DEFINE_F(BM_Octree, GrowToContainRootStraddlers)(benchmark::State& state)
    {
        const uint32_t entryCount = aznumeric_cast<uint32_t>(state.range(0));
        for ([[maybe_unused]] auto _ : state)
        {
            state.PauseTiming();
            SetRootStraddlingBounds(entryCount);
            InsertEntries(entryCount);
            constexpr float FarPosition = 35423.84375f;
            m_dataArray[0].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(FarPosition, 0.0f, 0.0f), AZ::Vector3(FarPosition + 0.25f, 0.25f, 0.25f));
            state.ResumeTiming();

            m_visScene->InsertOrUpdateEntry(m_dataArray[0]);

            state.PauseTiming();
            VerifyGrowth(state, m_dataArray[0]);
            RemoveEntries(entryCount);
            state.ResumeTiming();
        }
    }

    BENCHMARK_REGISTER_F(BM_Octree, GrowToContainRootStraddlers)
        ->Arg(1000)
        ->Arg(10000)
        ->Arg(100000)
        ->Arg(1000000)
        ->Iterations(1)
        ->UseRealTime();

    BENCHMARK_DEFINE_F(BM_Octree, EnumerateAabbAfterGrowth)(benchmark::State& state)
    {
        const uint32_t entryCount = aznumeric_cast<uint32_t>(state.range(0));
        constexpr uint32_t LocalEntryCount = 64;
        constexpr float FarOffset = 35423.84375f;
        InsertEntries(entryCount);
        for (uint32_t entryIndex = LocalEntryCount; entryIndex < entryCount; ++entryIndex)
        {
            m_dataArray[entryIndex].m_boundingVolume = m_dataArray[entryIndex].m_boundingVolume.GetTranslated(AZ::Vector3(FarOffset, 0.0f, 0.0f));
            m_visScene->InsertOrUpdateEntry(m_dataArray[entryIndex]);
        }
        VerifyGrowth(state, m_dataArray[LocalEntryCount]);

        const AZ::Aabb localQuery = AZ::Aabb::CreateFromMinMax(AZ::Vector3::CreateZero(), AZ::Vector3(8000.0f));
        size_t candidateCount = 0;
        for ([[maybe_unused]] auto _ : state)
        {
            candidateCount = 0;
            m_visScene->Enumerate(
                localQuery,
                [&candidateCount](const AzFramework::IVisibilityScene::NodeData& nodeData)
                {
                    candidateCount += nodeData.m_entries.size();
                    for (AzFramework::VisibilityEntry* entry : nodeData.m_entries)
                    {
                        benchmark::DoNotOptimize(entry);
                    }
                });
        }
        state.counters["Candidates"] = aznumeric_cast<double>(candidateCount);
        if (candidateCount != LocalEntryCount)
        {
            state.SkipWithError("The local query returned entries from the distant population");
        }
        RemoveEntries(entryCount);
    }

    BENCHMARK_REGISTER_F(BM_Octree, EnumerateAabbAfterGrowth)
        ->Arg(1000)
        ->Arg(10000)
        ->Arg(100000)
        ->Arg(1000000);

    BENCHMARK_DEFINE_F(BM_Octree, UpdateStationaryEntries)(benchmark::State& state)
    {
        const uint32_t entryCount = aznumeric_cast<uint32_t>(state.range(0));
        InsertEntries(entryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (uint32_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
            {
                m_visScene->InsertOrUpdateEntry(m_dataArray[entryIndex]);
            }
        }
        state.SetItemsProcessed(state.iterations() * entryCount);
        RemoveEntries(entryCount);
    }

    BENCHMARK_REGISTER_F(BM_Octree, UpdateStationaryEntries)
        ->Arg(1000)
        ->Arg(10000)
        ->Arg(100000)
        ->Arg(1000000)
        ->UseRealTime();

    BENCHMARK_DEFINE_F(BM_Octree, UpdateMovingEntries)(benchmark::State& state)
    {
        const uint32_t entryCount = aznumeric_cast<uint32_t>(state.range(0));
        InsertEntries(entryCount);
        AZ::Vector3 offset(0.125f, 0.0f, 0.0f);
        for ([[maybe_unused]] auto _ : state)
        {
            for (uint32_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
            {
                auto& entry = m_dataArray[entryIndex];
                entry.m_boundingVolume = entry.m_boundingVolume.GetTranslated(offset);
                m_visScene->InsertOrUpdateEntry(entry);
            }
            offset = -offset;
        }
        state.SetItemsProcessed(state.iterations() * entryCount);
        RemoveEntries(entryCount);
    }

    BENCHMARK_REGISTER_F(BM_Octree, UpdateMovingEntries)
        ->Arg(1000)
        ->Arg(10000)
        ->Arg(100000)
        ->Arg(1000000)
        ->UseRealTime();

    BENCHMARK_F(BM_Octree, EnumerateAabb1000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.aabb, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateAabb10000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 10000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.aabb, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateAabb100000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 100000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.aabb, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateAabb1000000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.aabb, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateSphere1000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.sphere, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateSphere10000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 10000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.sphere, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateSphere100000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 100000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.sphere, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateSphere1000000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.sphere, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateFrustum1000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.frustum, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateFrustum10000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 10000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.frustum, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateFrustum100000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 100000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.frustum, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateFrustum1000000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000000;
        InsertEntries(EntryCount);
        for ([[maybe_unused]] auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_visScene->Enumerate(queryData.frustum, [](const AzFramework::IVisibilityScene::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }
}

#endif
