/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#pragma once

#include <AzFramework/Spawnable/RootSpawnableInterface.h>
#include <Prefab/Benchmark/PrefabBenchmarkFixture.h>

namespace AzFramework
{
    class EntitySpawnTicket;
    class RootSpawnableDefinition;
}

namespace Benchmark
{
    class BM_Spawnable
        : public Benchmark::BM_Prefab
    {
    protected:
        void SetUp(const benchmark::State& state) override;
        void SetUp(benchmark::State& state) override;
        void SetUpHelper(const benchmark::State& state);

        void TearDown(const benchmark::State& state) override;
        void TearDown(benchmark::State& state) override;
        void TearDownHelper(const benchmark::State& state);

        void SetUpSpawnableAsset(uint64_t entityCount);

        AZ::Data::Asset<AzFramework::Spawnable> m_spawnableAsset;
        AzFramework::EntitySpawnTicket* m_spawnTicket;
        AzFramework::RootSpawnableDefinition* m_rootSpawnableInterface;
    };
} // namespace Benchmark

#endif
