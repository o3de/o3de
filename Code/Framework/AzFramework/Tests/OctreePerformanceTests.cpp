/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Name/NameDictionary.h>
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
