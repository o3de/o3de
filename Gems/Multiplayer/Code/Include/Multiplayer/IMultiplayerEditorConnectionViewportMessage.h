/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


namespace Multiplayer
{
    //! Interface class for displaying useful debug information in the editor viewport while the editor is making its connection to the editor-server.
    //! Note: This isn't used once the Editor has connected to the Multiplayer simulation itself.
    class IMultiplayerEditorConnectionViewportMessage
    {
    public:        
        AZ_RTTI(IMultiplayerEditorConnectionViewportMessage, "{83ca783b-4e99-46c9-a133-7d1e4bacdbb4}");

        //! Displays multiplayer editor debug messaging in the viewport.
        virtual void DisplayMessage(const char* text) = 0;

        //! Turns off viewport debug messaging.
        virtual void StopViewportDebugMessaging() = 0;
    };
} // namespace Multiplayer
