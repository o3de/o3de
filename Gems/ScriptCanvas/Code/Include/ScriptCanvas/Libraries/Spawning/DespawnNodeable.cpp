/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Spawnable/SpawnableMediator.h>
#include <ScriptCanvas/Libraries/Spawning/DespawnNodeable.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    DespawnNodeable::DespawnNodeable([[maybe_unused]] const DespawnNodeable& rhs)
    {
        // this method is required by Script Canvas, left intentionally blank to avoid copying m_spawnableMediator
    }

    DespawnNodeable& DespawnNodeable::operator=([[maybe_unused]] const DespawnNodeable& rhs)
    {
        // this method is required by Script Canvas, left intentionally blank to avoid copying m_spawnableMediator
        return *this;
    }
    
    void DespawnNodeable::OnDeactivate()
    {
        m_spawnableMediator.Clear();
        AzFramework::SpawnableNotificationsBus::Handler::BusDisconnect();
    }

    void DespawnNodeable::OnDespawn(AzFramework::EntitySpawnTicket spawnTicket)
    {
        AzFramework::SpawnableNotificationsBus::Handler::BusDisconnect(spawnTicket.GetId());
        CallOnDespawn(spawnTicket);
    }

    void DespawnNodeable::RequestDespawn(AzFramework::EntitySpawnTicket spawnTicket)
    {
        using namespace AzFramework;

        if (m_spawnableMediator.Despawn(spawnTicket))
        {
            SpawnableNotificationsBus::Handler::BusConnect(spawnTicket.GetId());
        }
    }
} // namespace ScriptCanvas::Nodeables::Spawning
