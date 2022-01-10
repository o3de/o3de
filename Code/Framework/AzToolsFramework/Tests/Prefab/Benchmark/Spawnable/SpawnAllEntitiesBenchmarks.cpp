/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if defined(HAVE_BENCHMARK)

#include <Prefab/Benchmark/Spawnable/SpawnableBenchmarkFixture.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzToolsFramework/Prefab/Spawnable/SpawnableUtils.h>

namespace Benchmark
{
    using BM_SpawnAllEntities = BM_Spawnable;

    BENCHMARK_DEFINE_F(BM_SpawnAllEntities, SingleEntitySpawnable_SpawnCallVariable)(::benchmark::State& state)
    {
        const uint64_t spawnAllEntitiesCallCount = aznumeric_cast<uint64_t>(state.range());
        const uint64_t entityCountInSourcePrefab = 1;

        SetUpSpawnableAsset(entityCountInSourcePrefab);

        for (auto _ : state)
        {
            state.PauseTiming();
            m_spawnTicket = new AzFramework::EntitySpawnTicket(m_spawnableAsset);
            state.ResumeTiming();

            for (uint64_t spwanableCounter = 0; spwanableCounter < spawnAllEntitiesCallCount; spwanableCounter++)
            {
                AzFramework::SpawnableEntitiesInterface::Get()->SpawnAllEntities(*m_spawnTicket);
            }

            m_rootSpawnableInterface->ProcessSpawnableQueue();

            // Destroy the ticket so that this queues a request to delete all the entities spawned with this ticket.
            state.PauseTiming();
            delete m_spawnTicket;
            m_spawnTicket = nullptr;

            // This will process the request to delete all entities spawned with the ticket
            m_rootSpawnableInterface->ProcessSpawnableQueue();
            state.ResumeTiming();
        }

        state.SetComplexityN(spawnAllEntitiesCallCount);
    }
    BENCHMARK_REGISTER_F(BM_SpawnAllEntities, SingleEntitySpawnable_SpawnCallVariable)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();

    BENCHMARK_DEFINE_F(BM_SpawnAllEntities, SingleSpawnCall_EntityCountVariable)(::benchmark::State& state)
    {
        const uint64_t entityCountInSpawnable = aznumeric_cast<uint64_t>(state.range());

        SetUpSpawnableAsset(entityCountInSpawnable);

        for (auto _ : state)
        {
            state.PauseTiming();
            m_spawnTicket = new AzFramework::EntitySpawnTicket(m_spawnableAsset);
            state.ResumeTiming();

            AzFramework::SpawnableEntitiesInterface::Get()->SpawnAllEntities(*m_spawnTicket);
            m_rootSpawnableInterface->ProcessSpawnableQueue();

            // Destroy the ticket so that this queues a request to delete all the entities spawned with this ticket.
            state.PauseTiming();
            delete m_spawnTicket;
            m_spawnTicket = nullptr;

            // This will process the request to delete all entities spawned with the ticket
            m_rootSpawnableInterface->ProcessSpawnableQueue();
            state.ResumeTiming();
        }

        state.SetComplexityN(entityCountInSpawnable);
    }
    BENCHMARK_REGISTER_F(BM_SpawnAllEntities, SingleSpawnCall_EntityCountVariable)
        ->RangeMultiplier(10)
        ->Range(100, 10000)
        ->Unit(benchmark::kMillisecond)
        ->Complexity();

    BENCHMARK_DEFINE_F(BM_SpawnAllEntities, EntityCountVariable_SpawnCallCountVariable)(::benchmark::State& state)
    {
        const uint64_t entityCountInSpawnable = aznumeric_cast<uint64_t>(state.range(0));
        const uint64_t spawnCallCount = aznumeric_cast<uint64_t>(state.range(1));

        SetUpSpawnableAsset(entityCountInSpawnable);

        for (auto _ : state)
        {
            state.PauseTiming();
            m_spawnTicket = new AzFramework::EntitySpawnTicket(m_spawnableAsset);
            state.ResumeTiming();

            for (uint64_t spawnCallCounter = 0; spawnCallCounter < spawnCallCount; spawnCallCounter++)
            {
                AzFramework::SpawnableEntitiesInterface::Get()->SpawnAllEntities(*m_spawnTicket);
            }

            m_rootSpawnableInterface->ProcessSpawnableQueue();

            state.PauseTiming();
            delete m_spawnTicket;
            m_spawnTicket = nullptr;
            m_rootSpawnableInterface->ProcessSpawnableQueue();
            state.ResumeTiming();
        }

        state.SetComplexityN(entityCountInSpawnable * spawnCallCount);
    }
    // Provide ranges here to compare times for spawning the same number of entities by altering entityCountInSpawnable and spawnCallCount.
    BENCHMARK_REGISTER_F(BM_SpawnAllEntities, EntityCountVariable_SpawnCallCountVariable)
        ->Args({ 10, 100 })
        ->Args({ 100, 10 })
        ->Args({ 10, 1000 })
        ->Args({ 1000, 10 })
        ->Args({ 100, 1000 })
        ->Args({ 1000, 100 })
        ->Unit(benchmark::kMillisecond)
        ->Complexity();
} // namespace Benchmark

#endif
