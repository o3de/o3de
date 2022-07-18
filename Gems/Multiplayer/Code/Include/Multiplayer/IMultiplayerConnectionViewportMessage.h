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
    //! Interface class for displaying useful Multiplayer debug information to the center of the viewport.
    //! Example: it can show status as the editor is making its connection to the editor-server.
    class IMultiplayerConnectionViewportMessage
    {
    public:        
        AZ_RTTI(IMultiplayerConnectionViewportMessage, "{83ca783b-4e99-46c9-a133-7d1e4bacdbb4}");

        //! Displays multiplayer debug messaging in the viewport.
        //! This will appear in the center of the screen.
        virtual void DisplayCenterViewportMessage(const char* text) = 0;
        
        //! Turns off viewport debug messaging.
        virtual void StopCenterViewportDebugMessaging() = 0;
    };
} // namespace Multiplayer
