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
    }
    
    void DespawnNodeable::OnTick([[maybe_unused]] float delta, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        if (m_despawnedTicketIdList.empty())
        {
            return;
        }

        AZStd::vector<AzFramework::EntitySpawnTicket::Id> swappedDespawnedTicketIdList;
        {
            AZStd::lock_guard lock(m_idBatchMutex);
            swappedDespawnedTicketIdList.swap(m_despawnedTicketIdList);
        }

        for (AzFramework::EntitySpawnTicket::Id ticketId : swappedDespawnedTicketIdList)
        {
            CallOnDespawn(ticketId);
        }
    }
    
    void DespawnNodeable::RequestDespawn(SpawnTicketInstance spawnTicketInstance)
    {
        auto despawnCompleteCB = [this](AzFramework::EntitySpawnTicket::Id ticketId)
        {
            AZStd::lock_guard lock(m_idBatchMutex);
            m_despawnedTicketIdList.push_back(ticketId);
        };

        AzFramework::DespawnAllEntitiesOptionalArgs optionalArgs;
        optionalArgs.m_completionCallback = AZStd::move(despawnCompleteCB);
        AzFramework::SpawnableEntitiesInterface::Get()->DespawnAllEntities(*spawnTicketInstance.m_ticket, AZStd::move(optionalArgs));
    }
} // namespace ScriptCanvas::Nodeables::Spawning
