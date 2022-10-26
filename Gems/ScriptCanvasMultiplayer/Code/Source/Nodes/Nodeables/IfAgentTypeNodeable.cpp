/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "IfAgentTypeNodeable.h"

#include <Multiplayer/IMultiplayer.h>

namespace ScriptCanvasMultiplayer
{
    void IfAgentTypeNodeable::In()
    {
        const auto multiplayer = Multiplayer::GetMultiplayer();
        if (!multiplayer)
        {
            CallIfSingleplayer();
            return;
        }

        const Multiplayer::MultiplayerAgentType type = multiplayer->GetAgentType();

        switch (type)
        {
        case Multiplayer::MultiplayerAgentType::Uninitialized:
            CallIfSingleplayer();
            break;
        case Multiplayer::MultiplayerAgentType::Client:
            CallIfClientType();
            break;
        case Multiplayer::MultiplayerAgentType::ClientServer:
            CallIfClientServerType();
            break;
        case Multiplayer::MultiplayerAgentType::DedicatedServer:
            CallIfDedicatedServerType();
            break;
        }
    }
} // namespace ScriptCanvasMultiplayer
