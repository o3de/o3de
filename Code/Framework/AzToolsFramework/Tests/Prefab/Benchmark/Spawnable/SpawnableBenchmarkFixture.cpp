/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>
#include <Prefab/Benchmark/Spawnable/SpawnableBenchmarkFixture.h>

namespace Benchmark
{
    void BM_Spawnable::SetUp(const benchmark::State& state)
    {
        SetUpHelper(state);
    }

    void BM_Spawnable::SetUp(benchmark::State& state)
    {
        SetUpHelper(state);
    }

    void BM_Spawnable::SetUpHelper(const benchmark::State& state)
    {
        BM_Prefab::SetUp(state);
        m_rootSpawnableInterface = AzFramework::RootSpawnableInterface::Get();
        AZ_Assert(m_rootSpawnableInterface != nullptr, "RootSpawnableInterface isn't found.");
    }

    void BM_Spawnable::TearDown(const benchmark::State& state)
    {
        TearDownHelper(state);
    }

    void BM_Spawnable::TearDown(benchmark::State& state)
    {
        TearDownHelper(state);
    }

    void BM_Spawnable::TearDownHelper(const benchmark::State& state)
    {
        m_spawnableAsset.Release();
        BM_Prefab::TearDown(state);
    }

    void BM_Spawnable::SetUpSpawnableAsset(uint64_t entityCount)
    {
        AZStd::vector<AZ::Entity*> entities;
        entities.reserve(entityCount);

        for (uint64_t i = 0; i < entityCount; i++)
        {
            entities.emplace_back(CreateEntity("Entity"));
        }

        AZStd::unique_ptr<Instance> instance = m_prefabSystemComponent->CreatePrefab(AZStd::move(entities), {}, m_pathString);
        const PrefabDom& prefabDom = m_prefabSystemComponent->FindTemplateDom(instance->GetTemplateId());

        // Lifecycle of spawnable is managed by the asset that's created using it.
        AzFramework::Spawnable* spawnable = new AzFramework::Spawnable(
            AZ::Data::AssetId::CreateString("{612F2AB1-30DF-44BB-AFBE-17A85199F09E}:0"), AZ::Data::AssetData::AssetStatus::Ready);
        AzToolsFramework::Prefab::SpawnableUtils::CreateSpawnable(*spawnable, prefabDom);
        m_spawnableAsset = AZ::Data::Asset<AzFramework::Spawnable>(spawnable, AZ::Data::AssetLoadBehavior::Default);
    }
} // namespace Benchmark

#endif
