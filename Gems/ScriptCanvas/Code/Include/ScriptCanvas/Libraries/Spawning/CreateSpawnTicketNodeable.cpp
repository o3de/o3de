/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Libraries/Spawning/CreateSpawnTicketNodeable.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    SpawnTicketInstance CreateSpawnTicketNodeable::CreateTicket(const SpawnableAsset& Prefab)
    {
        SpawnTicketInstance ticketInstance;
        ticketInstance.m_ticket = AZStd::make_shared<AzFramework::EntitySpawnTicket>(Prefab.m_asset);
        return ticketInstance;
    }
}
