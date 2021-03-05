/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Visibility/OctreeSystemComponent.h>

#if defined(HAVE_BENCHMARK)

#include <random>
#include <benchmark/benchmark.h>

namespace Benchmark
{
    class BM_Octree
        : public benchmark::Fixture
    {
    public:
        void SetUp([[maybe_unused]] const ::benchmark::State& state) override
        {
            // Create the SystemAllocator if not available
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
                m_ownsSystemAllocator = true;
            }

            m_octreeSystemComponent = new AzFramework::OctreeSystemComponent;
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

        void TearDown([[maybe_unused]] const ::benchmark::State& state) override
        {
            delete m_octreeSystemComponent;

            m_dataArray.clear();
            m_dataArray.shrink_to_fit();

            m_queryDataArray.clear();
            m_queryDataArray.shrink_to_fit();

            // Destroy system allocator only if it was created by this environment
            if (m_ownsSystemAllocator)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
        }

        void InsertEntries(uint32_t entryCount)
        {
            AzFramework::IVisibilitySystem* visSystem = AZ::Interface<AzFramework::IVisibilitySystem>::Get();
            for (uint32_t i = 0; i < entryCount; ++i)
            {
                visSystem->InsertOrUpdateEntry(m_dataArray[i]);
            }
        }

        void RemoveEntries(uint32_t entryCount)
        {
            AzFramework::IVisibilitySystem* visSystem = AZ::Interface<AzFramework::IVisibilitySystem>::Get();
            for (uint32_t i = 0; i < entryCount; ++i)
            {
                visSystem->RemoveEntry(m_dataArray[i]);
            }
        }

        struct QueryData
        {
            AZ::Aabb aabb;
            AZ::Sphere sphere;
            AZ::Frustum frustum;
        };

        bool m_ownsSystemAllocator = false;
        AZStd::vector<AzFramework::VisibilityEntry> m_dataArray;
        AZStd::vector<QueryData> m_queryDataArray;
        AzFramework::OctreeSystemComponent* m_octreeSystemComponent;
    };

    BENCHMARK_F(BM_Octree, InsertDelete1000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000;
        for (auto _ : state)
        {
            InsertEntries(EntryCount);
            RemoveEntries(EntryCount);
        }
    }

    BENCHMARK_F(BM_Octree, InsertDelete10000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 10000;
        for (auto _ : state)
        {
            InsertEntries(EntryCount);
            RemoveEntries(EntryCount);
        }
    }

    BENCHMARK_F(BM_Octree, InsertDelete100000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 100000;
        for (auto _ : state)
        {
            InsertEntries(EntryCount);
            RemoveEntries(EntryCount);
        }
    }

    BENCHMARK_F(BM_Octree, InsertDelete1000000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000000;
        for (auto _ : state)
        {
            InsertEntries(EntryCount);
            RemoveEntries(EntryCount);
        }
    }

    BENCHMARK_F(BM_Octree, EnumerateAabb1000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.aabb, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateAabb10000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 10000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.aabb, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateAabb100000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 100000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.aabb, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateAabb1000000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.aabb, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateSphere1000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.sphere, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateSphere10000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 10000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.sphere, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateSphere100000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 100000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.sphere, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateSphere1000000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.sphere, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateFrustum1000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.frustum, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateFrustum10000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 10000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.frustum, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateFrustum100000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 100000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.frustum, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }

    BENCHMARK_F(BM_Octree, EnumerateFrustum1000000)(benchmark::State& state)
    {
        constexpr uint32_t EntryCount = 1000000;
        InsertEntries(EntryCount);
        for (auto _ : state)
        {
            for (auto& queryData : m_queryDataArray)
            {
                m_octreeSystemComponent->Enumerate(queryData.frustum, [](const AzFramework::IVisibilitySystem::NodeData&) {});
            }
        }
        RemoveEntries(EntryCount);
    }
}

#endif
