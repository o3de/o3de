/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Multiplayer
{
    /**
    * This bus can be used to send commands to the editor.
    */
    class MultiplayerEditorLayerPythonRequests
        : public AZ::ComponentBus
    {
    public:
        /*
        * Enters the editor game mode and launches/connects to the server launcher.
        */
        virtual void EnterGameMode() = 0;

        /*
        * Queries if the Editor is in game mode, the editor-server has finished connecting, and the default network player has spawned.
        */
        virtual bool IsInGameMode() = 0;
    };
    using MultiplayerEditorLayerPythonRequestBus = AZ::EBus<MultiplayerEditorLayerPythonRequests>;
}
