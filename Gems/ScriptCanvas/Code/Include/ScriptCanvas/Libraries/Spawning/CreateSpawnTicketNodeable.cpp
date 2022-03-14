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
    AzFramework::EntitySpawnTicket CreateSpawnTicketNodeable::CreateTicket(const SpawnableAsset& Prefab)
    {
        return AzFramework::EntitySpawnTicket(Prefab.m_asset);
    }
}
