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

#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Session/ISessionHandlingRequests.h>
#include <AzFramework/Session/SessionConfig.h>

#include <AWSGameLiftPlayer.h>
#include <Request/AWSGameLiftServerRequestBus.h>

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
        static constexpr const char AWSGameLiftMatchmakingDataInvalidErrorMessage[] =
            "Failed to parse GameLift matchmaking data. ErrorMessage: %s";
        static constexpr const char AWSGameLiftMatchmakingDataMissingErrorMessage[] =
            "GameLift matchmaking data is missing or invalid to parse.";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributeInvalidErrorMessage[] =
            "Failed to build player %s attributes. ErrorMessage: %s";
        static constexpr const char AWSGameLiftDescribePlayerSessionsErrorMessage[] =
            "Failed to describe player sessions. ErrorMessage: %s";
        static constexpr const char AWSGameLiftStartMatchBackfillErrorMessage[] =
            "Failed to start match backfill. ErrorMessage: %s";
        static constexpr const char AWSGameLiftStopMatchBackfillErrorMessage[] =
            "Failed to stop match backfill. ErrorMessage: %s";

        static constexpr const char AWSGameLiftMatchmakingConfigurationKeyName[] = "matchmakingConfigurationArn";
        static constexpr const char AWSGameLiftMatchmakingTeamsKeyName[] = "teams";
        static constexpr const char AWSGameLiftMatchmakingTeamNameKeyName[] = "name";
        static constexpr const char AWSGameLiftMatchmakingPlayersKeyName[] = "players";
        static constexpr const char AWSGameLiftMatchmakingPlayerIdKeyName[] = "playerId";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributesKeyName[] = "attributes";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributeTypeKeyName[] = "attributeType";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributeValueKeyName[] = "valueAttribute";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributeSTypeName[] = "S";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributeSServerTypeName[] = "STRING";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributeNTypeName[] = "N";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributeNServerTypeName[] = "DOUBLE";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributeSLTypeName[] = "SL";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributeSLServerTypeName[] = "STRING_LIST";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributeSDMTypeName[] = "SDM";
        static constexpr const char AWSGameLiftMatchmakingPlayerAttributeSDMServerTypeName[] = "STRING_DOUBLE_MAP";
        static constexpr const uint16_t AWSGameLiftDescribePlayerSessionsPageSize = 30;

        AWSGameLiftServerManager();
        virtual ~AWSGameLiftServerManager();

        void ActivateManager();
        void DeactivateManager();

        //! Initialize GameLift API client by calling InitSDK().
        void InitializeGameLiftServerSDK();

        // AWSGameLiftServerRequestBus interface implementation
        bool NotifyGameLiftProcessReady() override;
        bool StartMatchBackfill(const AZStd::string& ticketId, const AZStd::vector<AWSGameLiftPlayer>& players) override;
        bool StopMatchBackfill(const AZStd::string& ticketId) override;

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

        //! Get active server player data from lazy loaded game session for server match backfill
        AZStd::vector<AWSGameLiftPlayer> GetActiveServerMatchBackfillPlayers();

        //! Update local game session data to latest one
        void UpdateGameSessionData(const Aws::GameLift::Server::Model::GameSession& gameSession);

    private:
        //! Build the serverProcessDesc with appropriate server port number and log paths.
        GameLiftServerProcessDesc BuildGameLiftServerProcessDesc();

        //! Build active server player data from lazy loaded game session based on player id
        bool BuildActiveServerMatchBackfillPlayer(const AZStd::string& playerId, AWSGameLiftPlayer& outPlayer);

        //! Build server player attribute data from lazy load matchmaking data
        void BuildServerMatchBackfillPlayerAttributes(const rapidjson::Value& playerAttributes, AWSGameLiftPlayer& outPlayer);

        //! Build server player data for server match backfill
        bool BuildServerMatchBackfillPlayer(const AWSGameLiftPlayer& player, Aws::GameLift::Server::Model::Player& outBackfillPlayer);

        //! Build start match backfill request for StartMatchBackfill operation
        bool BuildStartMatchBackfillRequest(
            const AZStd::string& ticketId,
            const AZStd::vector<AWSGameLiftPlayer>& players,
            Aws::GameLift::Server::Model::StartMatchBackfillRequest& outRequest);

        //! Build stop match backfill request for StopMatchBackfill operation
        void BuildStopMatchBackfillRequest(const AZStd::string& ticketId, Aws::GameLift::Server::Model::StopMatchBackfillRequest& outRequest);

        //! Build session config by using AWS GameLift Server GameSession Model.
        AzFramework::SessionConfig BuildSessionConfig(const Aws::GameLift::Server::Model::GameSession& gameSession);

        //! Check whether matchmaking data is in proper format
        bool IsMatchmakingDataValid();

        //! Fetch active player sessions in game session.
        AZStd::vector<Aws::GameLift::Server::Model::PlayerSession> GetActivePlayerSessions();

        //! Callback function that the GameLift service invokes to activate a new game session.
        void OnStartGameSession(const Aws::GameLift::Server::Model::GameSession& gameSession);

        //! Callback function that the GameLift service invokes to pass an updated game session object to the server process.
        void OnUpdateGameSession(const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession);

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

        // Lazy loaded game session and matchmaking data
        Aws::GameLift::Server::Model::GameSession m_gameSession;
        // Matchmaking data contains a unique match ID, it identifies the matchmaker that created the match
        // and describes the teams, team assignments, and players.
        // Reference https://docs.aws.amazon.com/gamelift/latest/flexmatchguide/match-server.html#match-server-data
        rapidjson::Document m_matchmakingData;
    };
} // namespace AWSGameLift
