/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Spawnable/Script/SpawnableScriptMediator.h>
#include <ScriptCanvas/Libraries/Spawning/CreateSpawnTicketNodeable.h>
#include <AzCore/Console/ILogger.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    CreateSpawnTicketNodeable::CreateSpawnTicketNodeable([[maybe_unused]] const CreateSpawnTicketNodeable& rhs)
    {
        // this method is required by Script Canvas, left intentionally blank to avoid copying m_SpawnableScriptMediator
    }

    CreateSpawnTicketNodeable& CreateSpawnTicketNodeable::operator=([[maybe_unused]] const CreateSpawnTicketNodeable& rhs)
    {
        // this method is required by Script Canvas, left intentionally blank to avoid copying m_SpawnableScriptMediator
        return *this;
    }

    void CreateSpawnTicketNodeable::CreateTicket(const AzFramework::Scripts::SpawnableScriptAssetRef& prefab)
    {
        using namespace AzFramework;

        auto ticket = m_spawnableScriptMediator.CreateSpawnTicket(prefab);
        if (ticket.IsSuccess())
        {
            CallSuccess(ticket.GetValue());
        }
        else
        {
            AZLOG_ERROR("Unable to Create Spawn Ticket - A valid prefab was not provided");
            CallFailed();
        }
    }
}
