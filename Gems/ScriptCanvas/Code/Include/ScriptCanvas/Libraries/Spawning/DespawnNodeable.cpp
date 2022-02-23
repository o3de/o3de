/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Libraries/Spawning/DespawnNodeable.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    DespawnNodeable::DespawnNodeable([[maybe_unused]] const DespawnNodeable& rhs)
    {}

    DespawnNodeable& DespawnNodeable::operator=([[maybe_unused]] DespawnNodeable& rhs)
    {
        return *this;
    }

    void DespawnNodeable::OnInitializeExecutionState()
    {
        if (!AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void DespawnNodeable::OnDeactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();

        m_despawnedTicketList = {};
    }
    
    void DespawnNodeable::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        if (m_despawnedTicketList.empty())
        {
            return;
        }

        AZStd::vector<SpawnTicketInstance> swappedDespawnedTicketList;
        {
            AZStd::lock_guard lock(m_idBatchMutex);
            swappedDespawnedTicketList.swap(m_despawnedTicketList);
        }

        for (auto spawnTicket : swappedDespawnedTicketList)
        {
            CallOnDespawn(spawnTicket);
        }
    }
    
    void DespawnNodeable::RequestDespawn(SpawnTicketInstance spawnTicket)
    {
        auto despawnCompleteCB = [this, spawnTicket]
        ([[maybe_unused]] AzFramework::EntitySpawnTicket::Id ticketId)
        {
            AZStd::lock_guard lock(m_idBatchMutex);
            m_despawnedTicketList.push_back(spawnTicket);
        };

        if (!spawnTicket.m_ticket)
        {
            return;
        }

        AzFramework::DespawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(despawnCompleteCB);
        AzFramework::SpawnableEntitiesInterface::Get()->DespawnAllEntities(*spawnTicket.m_ticket, AZStd::move(optionalArgs));
    }
} // namespace ScriptCanvas::Nodeables::Spawning
