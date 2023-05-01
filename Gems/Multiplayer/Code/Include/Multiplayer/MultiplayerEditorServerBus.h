/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzNetworking
{
    class IConnection;
}

namespace Multiplayer
{
    class MultiplayerEditorServerRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        
        //! Sends a packet that initializes a local server launched from the editor.
        //! The editor will package the data required for loading the current editor level on the editor-server; data includes entities and asset data.
        //! @param connection The connection to the editor-server
        virtual void SendEditorServerLevelDataPacket(AzNetworking::IConnection* connection) = 0;
    };
    using MultiplayerEditorServerRequestBus = AZ::EBus<MultiplayerEditorServerRequests>;

    class MultiplayerEditorServerNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Notification when the Editor has successfully opened the editor server.
        virtual void OnServerLaunched() {}

        //! Notification when the Editor has failed to open the editor server.
        //! Might have failed for various reasons, this is a catch-all.
        virtual void OnServerLaunchFail() {}
        
        //! Notification when the Editor attempts a TCP connection to the server.
        //! Note: It's possible multiple connection attempts are needed if the user starts and stops editor play mode repeatedly because the server port takes a few seconds to free.
        //! @param connectionAttempts The number of attempts made to connect to the editor-server
        virtual void OnEditorConnectionAttempt([[maybe_unused]]uint16_t connectionAttempts, [[maybe_unused]]uint16_t maxAttempts) {}

        //! Notification when the Editor failed all attempts to establish its TCP connection with the server.
        //! Maximum attempts are set using the cvar: editorsv_max_connection_attempts
        virtual void OnEditorConnectionAttemptsFailed([[maybe_unused]]uint16_t failedAttempts) {}

        //! Notification when the Editor starts sending the current level data (spawnable) to the server.
        virtual void OnEditorSendingLevelData([[maybe_unused]] uint32_t bytesSent, [[maybe_unused]] uint32_t bytesTotal) {}

        //! Notification when the Editor has failed to send the current level data to the server.
        virtual void OnEditorSendingLevelDataFailed() {}

        //! Notification when the Editor has successfully finished sending the current level data to the server.
        virtual void OnEditorSendingLevelDataSuccess() {}

        //! Notification when the Editor has sent all the level data successful and is now fully connected to the multiplayer simulation.
        virtual void OnConnectToSimulationSuccess() {}

        //! Notification when the Editor has sent all the level data successful but for some reason it fails to connect to normal multiplayer simulation.
        //! @param serverPort The server port. This is useful information since user might be trying to connect to the wrong port.
        virtual void OnConnectToSimulationFail([[maybe_unused]]uint16_t serverPort) {}

        //! Notification when the Editor multiplayer play mode is over; therefore ending the multiplayer simulation.
        virtual void OnPlayModeEnd() {}

        //! Notification when the server process launched by the editor has unexpectedly stopped running.
        //! This likely means the server crashed, or the user stopped the process by hand.
        virtual void OnEditorServerProcessStoppedUnexpectedly(){}
    };
    using MultiplayerEditorServerNotificationBus = AZ::EBus<MultiplayerEditorServerNotifications>;
} // namespace Multiplayer
