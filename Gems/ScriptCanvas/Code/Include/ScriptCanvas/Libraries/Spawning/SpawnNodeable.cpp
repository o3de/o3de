/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ScriptCanvas/Libraries/Spawning/SpawnNodeable.h>
#include <AzFramework/Components/TransformComponent.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    SpawnNodeable::SpawnNodeable([[maybe_unused]] const SpawnNodeable& rhs)
    {
        // this method is required by Script Canvas, left intentionally blank to avoid copying m_spawnableMediator
    }

    SpawnNodeable& SpawnNodeable::operator=([[maybe_unused]] const SpawnNodeable& rhs)
    {
        // this method is required by Script Canvas, left intentionally blank to avoid copying m_spawnableMediator
        return *this;
    }
    
    void SpawnNodeable::OnDeactivate()
    {
        m_spawnableMediator.Clear();
        AzFramework::SpawnableNotificationsBus::Handler::BusDisconnect();
    }

    void SpawnNodeable::OnSpawn(AzFramework::EntitySpawnTicket spawnTicket, AZStd::vector<AZ::EntityId> entityList)
    {
        AzFramework::SpawnableNotificationsBus::Handler::BusDisconnect(spawnTicket.GetId());
        CallOnSpawnCompleted(spawnTicket, move(entityList));
    }
    
    void SpawnNodeable::RequestSpawn(
        AzFramework::EntitySpawnTicket spawnTicket,
        AZ::EntityId parentId,
        Data::Vector3Type translation,
        Data::Vector3Type rotation,
        Data::NumberType scale)
    {
        using namespace AzFramework;

        if (m_spawnableMediator.Spawn(spawnTicket, parentId, translation, rotation, aznumeric_cast<float>(scale)))
        {
            SpawnableNotificationsBus::Handler::BusConnect(spawnTicket.GetId());
        }
    }
} // namespace ScriptCanvas::Nodeables::Spawning
