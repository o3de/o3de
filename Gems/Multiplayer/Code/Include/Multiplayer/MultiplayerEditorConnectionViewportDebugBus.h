/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace Multiplayer
{
    //! Request bus for displaying useful debug information in the editor viewport while the editor is making its connection to the editor-server.
    //! Note: This isn't used once the Editor has connected to the Multiplayer simulation itself.
    class MultiplayerEditorConnectionViewportDebugRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        
        //! Displays multiplayer editor debug messaging in the viewport.
        virtual void DisplayMessage(const char* text) = 0;

        //! Turns off viewport debug messaging.
        virtual void StopViewportDebugMessaging() = 0;
    };
    using MultiplayerEditorConnectionViewportDebugRequestBus = AZ::EBus<MultiplayerEditorConnectionViewportDebugRequests>;
} // namespace Multiplayer
