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
    void IfNetRoleNodeable::In(AZ::EntityId MultiplayerEntity)
    {
        if (!Multiplayer::GetMultiplayer())
        {
            CallIfInvalidRole();
            return;
        }

        const Multiplayer::INetworkEntityManager* networkEntityManager = Multiplayer::GetMultiplayer()->GetNetworkEntityManager();
        if (!networkEntityManager)
        {
            CallIfInvalidRole();
            return;
        }

        const Multiplayer::NetEntityId netEntityId = networkEntityManager->GetNetEntityIdById(MultiplayerEntity);
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
                    CallIfInvalidRole();
                    break;
                case Multiplayer::NetEntityRole::Client:
                    CallIfClientRole();
                    break;
                case Multiplayer::NetEntityRole::Autonomous:
                    CallIfAutonomousRole();
                    break;
                case Multiplayer::NetEntityRole::Server:
                    CallIfServerRole();
                    break;
                case Multiplayer::NetEntityRole::Authority:
                    CallIfAuthorityRole();
                    break;
                }
            }
        }
        else
        {
            CallIfInvalidRole();
        }
    }
} // namespace ScriptCanvasMultiplayer
