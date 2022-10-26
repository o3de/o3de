/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "IfNetRoleNodeable.h"

#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetBindComponent.h>

namespace ScriptCanvasMultiplayer
{
    void IfNetRoleNodeable::IsNetRole(AZ::EntityId entityId)
    {
        if (!Multiplayer::GetMultiplayer())
        {
            ExecutionOut(4);
            return;
        }

        const Multiplayer::INetworkEntityManager* networkEntityManager = Multiplayer::GetMultiplayer()->GetNetworkEntityManager();
        if (!networkEntityManager)
        {
            ExecutionOut(4);
            return;
        }

        const Multiplayer::NetEntityId netEntityId = networkEntityManager->GetNetEntityIdById(entityId);
        const Multiplayer::ConstNetworkEntityHandle handle = networkEntityManager->GetEntity(netEntityId);
        
        if (handle.Exists())
        {
            const Multiplayer::NetBindComponent* netBind = handle.GetNetBindComponent();
            if (netBind)
            {
                const Multiplayer::NetEntityRole role = netBind->GetNetEntityRole();
                switch (role)
                {
                case Multiplayer::NetEntityRole::InvalidRole:
                    ExecutionOut(4);
                    break;
                case Multiplayer::NetEntityRole::Client:
                    ExecutionOut(0);
                    break;
                case Multiplayer::NetEntityRole::Autonomous:
                    ExecutionOut(1);
                    break;
                case Multiplayer::NetEntityRole::Server:
                    ExecutionOut(2);
                    break;
                case Multiplayer::NetEntityRole::Authority:
                    ExecutionOut(3);
                    break;
                }
            }
        }
        else
        {
            ExecutionOut(4);
        }
    }
} // namespace ScriptCanvasMultiplayer
