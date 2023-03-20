/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Spawnable/SpawnableEntitiesContainer.h>

namespace AzFramework
{
    SpawnableEntitiesContainer::SpawnableEntitiesContainer(AZ::Data::Asset<Spawnable> spawnable)
    {
        Connect(AZStd::move(spawnable));
    }

    SpawnableEntitiesContainer::~SpawnableEntitiesContainer()
    {
        Clear();
    }

    bool SpawnableEntitiesContainer::IsSet() const
    {
        return m_threadData != nullptr;
    }

    uint32_t SpawnableEntitiesContainer::GetCurrentGeneration() const
    {
        return m_currentGeneration;
    }

    void SpawnableEntitiesContainer::SpawnAllEntities(SpawnAllEntitiesOptionalArgs optionalArgs)
    {
        AZ_Assert(m_threadData, "Calling SpawnAllEntities on a Spawnable container that's not set.");
        SpawnableEntitiesInterface::Get()->SpawnAllEntities(m_threadData->m_spawnedEntitiesTicket, optionalArgs);
    }

    void SpawnableEntitiesContainer::SpawnEntities(AZStd::vector<uint32_t> entityIndices)
    {
        AZ_Assert(m_threadData, "Calling SpawnEntities on a Spawnable container that's not set.");
        SpawnableEntitiesInterface::Get()->SpawnEntities(m_threadData->m_spawnedEntitiesTicket, AZStd::move(entityIndices));
    }

    void SpawnableEntitiesContainer::DespawnAllEntities()
    {
        AZ_Assert(m_threadData, "Calling DespawnEntities on a Spawnable container that's not set.");
        SpawnableEntitiesInterface::Get()->DespawnAllEntities(m_threadData->m_spawnedEntitiesTicket);
    }

    void SpawnableEntitiesContainer::Reset(AZ::Data::Asset<Spawnable> spawnable)
    {
        Clear();
        Connect(AZStd::move(spawnable));
    }

    void SpawnableEntitiesContainer::Clear()
    {
        if (m_threadData != nullptr)
        {
            m_monitor.Disconnect();
            m_monitor.m_threadData.reset();

            SpawnableEntitiesInterface::Get()->Barrier(
                m_threadData->m_spawnedEntitiesTicket,
                [threadData = m_threadData](EntitySpawnTicket::Id) mutable
                {
                    threadData.reset();
                });

            m_threadData.reset();
            // The generation is incremented here instead of Connect in order to make sure that any callback that checks the
            // provided generation with the current generation is aware that the container has moved on to the next iteration
            // of the container even though it's empty and unassigned at this point.
            m_currentGeneration++;
        }
    }

    void SpawnableEntitiesContainer::Alert(AlertCallback callback, CheckIfSpawnableIsLoaded spawnableCheck)
    {
        AZ_Assert(m_threadData, "Calling DespawnEntities on a Spawnable container that's not set.");
        auto callbackWrapper = [generation = m_threadData->m_generation, callback = AZStd::move(callback)](EntitySpawnTicket::Id)
        {
            callback(generation);
        };
        if (spawnableCheck == CheckIfSpawnableIsLoaded::No)
        {
            SpawnableEntitiesInterface::Get()->Barrier(m_threadData->m_spawnedEntitiesTicket, AZStd::move(callbackWrapper));
        }
        else
        {
            SpawnableEntitiesInterface::Get()->LoadBarrier(m_threadData->m_spawnedEntitiesTicket, AZStd::move(callbackWrapper));
        }
    }

    void SpawnableEntitiesContainer::Connect(AZ::Data::Asset<Spawnable> spawnable)
    {
        AZ::Data::AssetId spawnableId = spawnable.GetId();

        AZ_Assert(m_threadData == nullptr, "Connecting a spawnable entities container that's already connected.");
        AZ_Assert(m_monitor.m_threadData == nullptr, "Connecting a spawnable entities monitor that's already connected.");

        m_threadData = AZStd::make_shared<ThreadSafeData>();
        m_threadData->m_spawnedEntitiesTicket = EntitySpawnTicket(AZStd::move(spawnable));
        m_threadData->m_generation = m_currentGeneration;
        
        m_monitor.m_threadData = m_threadData;
        m_monitor.Connect(spawnableId);
    }

    void SpawnableEntitiesContainer::Monitor::OnSpawnableReloaded(AZ::Data::Asset<Spawnable>&& replacementAsset)
    {
        AZ_Assert(m_threadData, "SpawnableEntitiesContainer is monitoring a spawnable, but doesn't have the associated data.");

        AZ_TracePrintf("Spawnables", "Reloading spawnable '%s'.\n", replacementAsset.GetHint().c_str());
        SpawnableEntitiesInterface::Get()->ReloadSpawnable(m_threadData->m_spawnedEntitiesTicket, AZStd::move(replacementAsset));
    }
} // namespace AzFramework
