/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <aws/gamelift/server/GameLiftServerAPI.h>
#include <aws/gamelift/server/model/GameSession.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Session/ISessionHandlingRequests.h>
#include <AzFramework/Session/SessionConfig.h>
#include <Request/IAWSGameLiftServerRequests.h>

namespace AWSGameLift
{
    class GameLiftServerSDKWrapper;

    //! GameLift server process settings.
    struct GameLiftServerProcessDesc
    {
        AZStd::vector<AZStd::string> m_logPaths; //!< Log paths the servers will write to. Both relative to the game root folder and absolute paths supported.

        uint16_t m_port = 0; //!< The port the server will be listening on.
    };

    //! Manage the server process for hosting game sessions via GameLiftServerSDK.
    class AWSGameLiftServerManager
        : public AWSGameLiftServerRequestBus::Handler
        , public AzFramework::ISessionHandlingProviderRequests
    {
    public:
        static constexpr const char AWSGameLiftServerManagerName[] = "AWSGameLiftServerManager";
        static constexpr const char AWSGameLiftServerSDKNotInitErrorMessage[] =
            "Amazon GameLift Server SDK is not initialized yet.";
        static constexpr const char AWSGameLiftServerSDKAlreadyInitErrorMessage[] =
            "Amazon GameLift Server SDK has already been initialized.";
        static constexpr const char AWSGameLiftServerTempPortErrorMessage[] =
            "No server port specified, server will be listening on ephemeral port.";
        static constexpr const char AWSGameLiftServerGameInitErrorMessage[] =
            "Failed to process game dependent initialization during OnStartGameSession.";
        static constexpr const char AWSGameLiftServerGameSessionDestroyErrorMessage[] =
            "Failed to destroy game session during OnProcessTerminate.";
        static constexpr const char AWSGameLiftServerPlayerConnectionRegisteredErrorMessage[] =
            "Player connection id %d is already registered to player session id %s. Remove connected player first.";
        static constexpr const char AWSGameLiftServerPlayerConnectionMissingErrorMessage[] =
            "Player connection id %d does not exist.";

        static constexpr const char AWSGameLiftServerInitSDKErrorMessage[] =
            "Failed to initialize Amazon GameLift Server SDK. ErrorMessage: %s";
        static constexpr const char AWSGameLiftServerProcessReadyErrorMessage[] =
            "Failed to notify GameLift server process ready. ErrorMessage: %s";
        static constexpr const char AWSGameLiftServerActivateGameSessionErrorMessage[] =
            "Failed to activate GameLift game session. ErrorMessage: %s";
        static constexpr const char AWSGameLiftServerProcessEndingErrorMessage[] =
            "Failed to notify GameLift server process ending. ErrorMessage: %s";
        static constexpr const char AWSGameLiftServerAcceptPlayerSessionErrorMessage[] =
            "Failed to validate player session connection with id %s. ErrorMessage: %s";
        static constexpr const char AWSGameLiftServerInvalidConnectionConfigErrorMessage[] =
            "Invalid player connection config, player connection id: %d, player session id: %s";
        static constexpr const char AWSGameLiftServerRemovePlayerSessionErrorMessage[] =
            "Failed to notify GameLift that the player with the player session id %s has disconnected from the server process. ErrorMessage: %s";

        AWSGameLiftServerManager();
        virtual ~AWSGameLiftServerManager();

        void ActivateManager();
        void DeactivateManager();

        //! Initialize GameLift API client by calling InitSDK().
        void InitializeGameLiftServerSDK();

        // AWSGameLiftServerRequestBus interface implementation
        bool NotifyGameLiftProcessReady() override;

        // ISessionHandlingProviderRequests interface implementation
        void HandleDestroySession() override;
        bool ValidatePlayerJoinSession(const AzFramework::PlayerConnectionConfig& playerConnectionConfig) override;
        void HandlePlayerLeaveSession(const AzFramework::PlayerConnectionConfig& playerConnectionConfig) override;
        AZ::IO::Path GetExternalSessionCertificate() override;
        AZ::IO::Path GetInternalSessionCertificate() override;

    protected:
        void SetGameLiftServerSDKWrapper(AZStd::unique_ptr<GameLiftServerSDKWrapper> gameLiftServerSDKWrapper);

        //! Add connected player session id.
        bool AddConnectedPlayer(const AzFramework::PlayerConnectionConfig& playerConnectionConfig);

    private:
        //! Build the serverProcessDesc with appropriate server port number and log paths.
        GameLiftServerProcessDesc BuildGameLiftServerProcessDesc();

        //! Build session config by using AWS GameLift Server GameSession Model.
        AzFramework::SessionConfig BuildSessionConfig(const Aws::GameLift::Server::Model::GameSession& gameSession);

        //! Callback function that the GameLift service invokes to activate a new game session.
        void OnStartGameSession(const Aws::GameLift::Server::Model::GameSession& gameSession);

        //! Callback function that the GameLift service invokes to pass an updated game session object to the server process.
        void OnUpdateGameSession();

        //! Callback function that the server process or GameLift service invokes to force the server process to shut down.
        void OnProcessTerminate();

        //! Callback function that the GameLift service invokes to request a health status report from the server process.
        //! @return Whether the server process is healthy.
        bool OnHealthCheck();

        //! Remove connected player session id.
        //! @param playerConnectionId Connection id of the player to remove.
        //! @param outPlayerSessionId Session id of the removed player. Empty if the player cannot be removed.
        //! @return Whether the player is removed successfully.
        bool RemoveConnectedPlayer(uint32_t playerConnectionId, AZStd::string& outPlayerSessionId);

        AZStd::unique_ptr<GameLiftServerSDKWrapper> m_gameLiftServerSDKWrapper;
        bool m_serverSDKInitialized;

        AZStd::mutex m_gameliftMutex;
        using PlayerConnectionId = uint32_t;
        using PlayerSessionId = AZStd::string;
        AZStd::unordered_map<PlayerConnectionId, PlayerSessionId> m_connectedPlayers;
    };
} // namespace AWSGameLift
