/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSGameLiftServerManager.h>
#include <AWSGameLiftSessionConstants.h>
#include <GameLiftServerSDKWrapper.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/std/bind/bind.h>
#include <AzFramework/Session/SessionNotifications.h>

namespace AWSGameLift
{
    AWSGameLiftServerManager::AWSGameLiftServerManager()
        : m_serverSDKInitialized(false)
        , m_gameLiftServerSDKWrapper(AZStd::make_unique<GameLiftServerSDKWrapper>())
        , m_connectedPlayers()
    {
    }

    AWSGameLiftServerManager::~AWSGameLiftServerManager()
    {
        m_gameLiftServerSDKWrapper.reset();
        m_connectedPlayers.clear();
    }

    void AWSGameLiftServerManager::ActivateManager()
    {
        AZ::Interface<IAWSGameLiftServerRequests>::Register(this);
        AWSGameLiftServerRequestBus::Handler::BusConnect();
    }

    void AWSGameLiftServerManager::DeactivateManager()
    {
        AWSGameLiftServerRequestBus::Handler::BusDisconnect();
        AZ::Interface<IAWSGameLiftServerRequests>::Unregister(this);
    }

    bool AWSGameLiftServerManager::AddConnectedPlayer(const AzFramework::PlayerConnectionConfig& playerConnectionConfig)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_gameliftMutex);
        if (m_connectedPlayers.contains(playerConnectionConfig.m_playerConnectionId))
        {
            if (m_connectedPlayers[playerConnectionConfig.m_playerConnectionId] != playerConnectionConfig.m_playerSessionId)
            {
                AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerPlayerConnectionRegisteredErrorMessage,
                    playerConnectionConfig.m_playerConnectionId, playerConnectionConfig.m_playerSessionId.c_str());
            }
            return false;
        }
        else
        {
            m_connectedPlayers.emplace(playerConnectionConfig.m_playerConnectionId, playerConnectionConfig.m_playerSessionId);
            return true;
        }
    }

    GameLiftServerProcessDesc AWSGameLiftServerManager::BuildGameLiftServerProcessDesc()
    {
        GameLiftServerProcessDesc serverProcessDesc;
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetDirectInstance();
        if (fileIO)
        {
            const char pathToLogFolder[] = "@log@/";
            char resolvedPath[AZ_MAX_PATH_LEN];
            if (fileIO->ResolvePath(pathToLogFolder, resolvedPath, AZ_ARRAY_SIZE(resolvedPath)))
            {
                serverProcessDesc.m_logPaths.push_back(resolvedPath);
            }
            else
            {
                AZ_Error(AWSGameLiftServerManagerName, false, "Failed to resolve the path to the log folder.");
            }
        }
        else
        {
            AZ_Error(AWSGameLiftServerManagerName, false, "Failed to get File IO.");
        }

        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            [[maybe_unused]] AZ::GetValueResult getCvarResult = console->GetCvarValue("sv_port", serverProcessDesc.m_port);
            AZ_Error(AWSGameLiftServerManagerName, getCvarResult == AZ::GetValueResult::Success,
                "Lookup of 'sv_port' console variable failed with error %s", AZ::GetEnumString(getCvarResult));
        }
        return serverProcessDesc;
    }

    AzFramework::SessionConfig AWSGameLiftServerManager::BuildSessionConfig(const Aws::GameLift::Server::Model::GameSession& gameSession)
    {
        AzFramework::SessionConfig sessionConfig;

        sessionConfig.m_dnsName = gameSession.GetDnsName().c_str();
        AZStd::string propertiesOutput = "";
        for (const auto& gameProperty : gameSession.GetGameProperties())
        {
            sessionConfig.m_sessionProperties.emplace(gameProperty.GetKey().c_str(), gameProperty.GetValue().c_str());
            propertiesOutput += AZStd::string::format("{Key=%s,Value=%s},", gameProperty.GetKey().c_str(), gameProperty.GetValue().c_str());
        }
        if (!propertiesOutput.empty())
        {
            propertiesOutput = propertiesOutput.substr(0, propertiesOutput.size() - 1); // Trim last comma to fit array format
        }
        sessionConfig.m_matchmakingData = gameSession.GetMatchmakerData().c_str();
        sessionConfig.m_sessionId = gameSession.GetGameSessionId().c_str();
        sessionConfig.m_ipAddress = gameSession.GetIpAddress().c_str();
        sessionConfig.m_maxPlayer = gameSession.GetMaximumPlayerSessionCount();
        sessionConfig.m_sessionName = gameSession.GetName().c_str();
        sessionConfig.m_port = static_cast<uint16_t>(gameSession.GetPort());
        sessionConfig.m_status = AWSGameLiftSessionStatusNames[(int)gameSession.GetStatus()];

        AZ_TracePrintf(AWSGameLiftServerManagerName,
            "Built SessionConfig with Name=%s, Id=%s, Status=%s, DnsName=%s, IpAddress=%s, Port=%d, MaxPlayer=%d and Properties=%s",
            sessionConfig.m_sessionName.c_str(),
            sessionConfig.m_sessionId.c_str(),
            sessionConfig.m_status.c_str(),
            sessionConfig.m_dnsName.c_str(),
            sessionConfig.m_ipAddress.c_str(),
            sessionConfig.m_port,
            sessionConfig.m_maxPlayer,
            AZStd::string::format("[%s]", propertiesOutput.c_str()).c_str());

        return sessionConfig;
    }

    bool AWSGameLiftServerManager::BuildServerMatchBackfillPlayer(
        const AWSGameLiftPlayer& player, Aws::GameLift::Server::Model::Player& outBackfillPlayer)
    {
        outBackfillPlayer.SetPlayerId(player.m_playerId.c_str());
        outBackfillPlayer.SetTeam(player.m_team.c_str());
        for (auto latencyPair : player.m_latencyInMs)
        {
            outBackfillPlayer.AddLatencyInMs(latencyPair.first.c_str(), latencyPair.second);
        }

        for (auto attributePair : player.m_playerAttributes)
        {
            Aws::GameLift::Server::Model::AttributeValue playerAttribute;
            rapidjson::Document attributeDocument;
            rapidjson::ParseResult parseResult = attributeDocument.Parse(attributePair.second.c_str());
            // player attribute json content should always be a single member object
            if (parseResult && attributeDocument.IsObject() && attributeDocument.MemberCount() == 1)
            {
                if ((attributeDocument.HasMember(AWSGameLiftMatchmakingPlayerAttributeSTypeName) ||
                    attributeDocument.HasMember(AWSGameLiftMatchmakingPlayerAttributeSServerTypeName)) &&
                    attributeDocument.MemberBegin()->value.IsString())
                {
                    playerAttribute = Aws::GameLift::Server::Model::AttributeValue(
                        attributeDocument.MemberBegin()->value.GetString());
                }
                else if ((attributeDocument.HasMember(AWSGameLiftMatchmakingPlayerAttributeNTypeName) ||
                    attributeDocument.HasMember(AWSGameLiftMatchmakingPlayerAttributeNServerTypeName)) &&
                    attributeDocument.MemberBegin()->value.IsNumber())
                {
                    playerAttribute = Aws::GameLift::Server::Model::AttributeValue(
                        attributeDocument.MemberBegin()->value.GetDouble());
                }
                else if ((attributeDocument.HasMember(AWSGameLiftMatchmakingPlayerAttributeSDMTypeName) ||
                    attributeDocument.HasMember(AWSGameLiftMatchmakingPlayerAttributeSDMServerTypeName)) &&
                    attributeDocument.MemberBegin()->value.IsObject())
                {
                    playerAttribute = Aws::GameLift::Server::Model::AttributeValue::ConstructStringDoubleMap();
                    for (auto iter = attributeDocument.MemberBegin()->value.MemberBegin();
                         iter != attributeDocument.MemberBegin()->value.MemberEnd(); iter++)
                    {
                        if (iter->name.IsString() && iter->value.IsNumber())
                        {
                            playerAttribute.AddStringAndDouble(iter->name.GetString(), iter->value.GetDouble());
                        }
                        else
                        {
                            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftMatchmakingPlayerAttributeInvalidErrorMessage,
                                player.m_playerId.c_str(), "String double map key must be string type and value must be number type");
                            return false;
                        }
                    }
                }
                else if ((attributeDocument.HasMember(AWSGameLiftMatchmakingPlayerAttributeSLTypeName) ||
                    attributeDocument.HasMember(AWSGameLiftMatchmakingPlayerAttributeSLServerTypeName)) &&
                    attributeDocument.MemberBegin()->value.IsArray())
                {
                    playerAttribute = Aws::GameLift::Server::Model::AttributeValue::ConstructStringList();
                    for (auto iter = attributeDocument.MemberBegin()->value.Begin();
                        iter != attributeDocument.MemberBegin()->value.End(); iter++)
                    {
                        if (iter->IsString())
                        {
                            playerAttribute.AddString(iter->GetString());
                        }
                        else
                        {
                            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftMatchmakingPlayerAttributeInvalidErrorMessage,
                                player.m_playerId.c_str(), "String list element must be string type");
                            return false;
                        }
                    }
                }
                else
                {
                    AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftMatchmakingPlayerAttributeInvalidErrorMessage,
                        player.m_playerId.c_str(), "S, N, SDM or SLM is expected as attribute type.");
                    return false;
                }
            }
            else
            {
                AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftMatchmakingPlayerAttributeInvalidErrorMessage,
                    player.m_playerId.c_str(), rapidjson::GetParseError_En(parseResult.Code()));
                return false;
            }
            outBackfillPlayer.AddPlayerAttribute(attributePair.first.c_str(), playerAttribute);
        }
        return true;
    }

    AZStd::vector<AWSGameLiftPlayer> AWSGameLiftServerManager::GetActiveServerMatchBackfillPlayers()
    {
        AZStd::vector<AWSGameLiftPlayer> activePlayers;
        // Keep processing only when game session has matchmaking data
        if (IsMatchmakingDataValid())
        {
            auto activePlayerSessions = GetActivePlayerSessions();
            for (auto playerSession : activePlayerSessions)
            {
                AWSGameLiftPlayer player;
                if (BuildActiveServerMatchBackfillPlayer(playerSession.GetPlayerId().c_str(), player))
                {
                    activePlayers.push_back(player);
                }
            }
        }
        return activePlayers;
    }

    bool AWSGameLiftServerManager::IsMatchmakingDataValid()
    {
        return m_matchmakingData.IsObject() &&
            m_matchmakingData.HasMember(AWSGameLiftMatchmakingConfigurationKeyName) &&
            m_matchmakingData.HasMember(AWSGameLiftMatchmakingTeamsKeyName);
    }

    AZStd::vector<Aws::GameLift::Server::Model::PlayerSession> AWSGameLiftServerManager::GetActivePlayerSessions()
    {
        Aws::GameLift::Server::Model::DescribePlayerSessionsRequest describeRequest;
        describeRequest.SetGameSessionId(m_gameSession.GetGameSessionId());
        describeRequest.SetPlayerSessionStatusFilter(
            Aws::GameLift::Server::Model::PlayerSessionStatusMapper::GetNameForPlayerSessionStatus(
                Aws::GameLift::Server::Model::PlayerSessionStatus::ACTIVE));
        int maxPlayerSession = m_gameSession.GetMaximumPlayerSessionCount();

        AZStd::vector<Aws::GameLift::Server::Model::PlayerSession> activePlayerSessions;
        if (maxPlayerSession <= AWSGameLiftDescribePlayerSessionsPageSize)
        {
            describeRequest.SetLimit(maxPlayerSession);
            auto outcome = m_gameLiftServerSDKWrapper->DescribePlayerSessions(describeRequest);
            if (outcome.IsSuccess())
            {
                for (auto playerSession : outcome.GetResult().GetPlayerSessions())
                {
                    activePlayerSessions.push_back(playerSession);
                }
            }
            else
            {
                AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftDescribePlayerSessionsErrorMessage,
                    outcome.GetError().GetErrorMessage().c_str());
            }
        }
        else
        {
            describeRequest.SetLimit(AWSGameLiftDescribePlayerSessionsPageSize);
            while (true)
            {
                auto outcome = m_gameLiftServerSDKWrapper->DescribePlayerSessions(describeRequest);
                if (outcome.IsSuccess())
                {
                    for (auto playerSession : outcome.GetResult().GetPlayerSessions())
                    {
                        activePlayerSessions.push_back(playerSession);
                    }
                    if (outcome.GetResult().GetNextToken().empty())
                    {
                        break;
                    }
                    else
                    {
                        describeRequest.SetNextToken(outcome.GetResult().GetNextToken());
                    }
                }
                else
                {
                    activePlayerSessions.clear();
                    AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftDescribePlayerSessionsErrorMessage,
                        outcome.GetError().GetErrorMessage().c_str());
                    break;
                }
            }
        }
        return activePlayerSessions;
    }

    bool AWSGameLiftServerManager::BuildActiveServerMatchBackfillPlayer(const AZStd::string& playerId, AWSGameLiftPlayer& outPlayer)
    {
        // As data is from GameLift service, assume it is always in correct format
        rapidjson::Value& teams = m_matchmakingData[AWSGameLiftMatchmakingTeamsKeyName];

        // Iterate through teams to find target player
        for (rapidjson::SizeType teamIndex = 0; teamIndex < teams.Size(); ++teamIndex)
        {
            rapidjson::Value& players = teams[teamIndex][AWSGameLiftMatchmakingPlayersKeyName];

            // Iterate through players under the team to find target player
            for (rapidjson::SizeType playerIndex = 0; playerIndex < players.Size(); ++playerIndex)
            {
                if (std::strcmp(players[playerIndex][AWSGameLiftMatchmakingPlayerIdKeyName].GetString(), playerId.c_str()) == 0)
                {
                    outPlayer.m_playerId = playerId;
                    outPlayer.m_team = teams[teamIndex][AWSGameLiftMatchmakingTeamNameKeyName].GetString();
                    // Get player attributes if target player has
                    if (players[playerIndex].HasMember(AWSGameLiftMatchmakingPlayerAttributesKeyName))
                    {
                        BuildServerMatchBackfillPlayerAttributes(
                            players[playerIndex][AWSGameLiftMatchmakingPlayerAttributesKeyName], outPlayer);
                    }
                    return true;
                }
            }
        }
        return false;
    }

    void AWSGameLiftServerManager::BuildServerMatchBackfillPlayerAttributes(
        const rapidjson::Value& playerAttributes, AWSGameLiftPlayer& outPlayer)
    {
        for (auto iter = playerAttributes.MemberBegin(); iter != playerAttributes.MemberEnd(); iter++)
        {
            AZStd::string attributeName = iter->name.GetString();

            rapidjson::StringBuffer jsonStringBuffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(jsonStringBuffer);
            iter->value[AWSGameLiftMatchmakingPlayerAttributeValueKeyName].Accept(writer);
            AZStd::string attributeType = iter->value[AWSGameLiftMatchmakingPlayerAttributeTypeKeyName].GetString();
            AZStd::string attributeValue = AZStd::string::format("{\"%s\": %s}",
                attributeType.c_str(), jsonStringBuffer.GetString());

            outPlayer.m_playerAttributes.emplace(attributeName, attributeValue);
        }
    }

    bool AWSGameLiftServerManager::BuildStartMatchBackfillRequest(
        const AZStd::string& ticketId,
        const AZStd::vector<AWSGameLiftPlayer>& players,
        Aws::GameLift::Server::Model::StartMatchBackfillRequest& outRequest)
    {
        outRequest.SetGameSessionArn(m_gameSession.GetGameSessionId());
        outRequest.SetMatchmakingConfigurationArn(m_matchmakingData[AWSGameLiftMatchmakingConfigurationKeyName].GetString());
        if (!ticketId.empty())
        {
            outRequest.SetTicketId(ticketId.c_str());
        }

        AZStd::vector<AWSGameLiftPlayer> requestPlayers(players);
        if (players.size() == 0)
        {
            requestPlayers = GetActiveServerMatchBackfillPlayers();
        }
        for (auto player : requestPlayers)
        {
            Aws::GameLift::Server::Model::Player backfillPlayer;
            if (BuildServerMatchBackfillPlayer(player, backfillPlayer))
            {
                outRequest.AddPlayer(backfillPlayer);
            }
            else
            {
                return false;
            }
        }
        return true;
    }

    void AWSGameLiftServerManager::BuildStopMatchBackfillRequest(
        const AZStd::string& ticketId, Aws::GameLift::Server::Model::StopMatchBackfillRequest& outRequest)
    {
        outRequest.SetGameSessionArn(m_gameSession.GetGameSessionId());
        outRequest.SetMatchmakingConfigurationArn(m_matchmakingData[AWSGameLiftMatchmakingConfigurationKeyName].GetString());
        if (!ticketId.empty())
        {
            outRequest.SetTicketId(ticketId.c_str());
        }
    }

    AZ::IO::Path AWSGameLiftServerManager::GetExternalSessionCertificate()
    {
        // TODO: Add support to get TLS cert file path
        return AZ::IO::Path();
    }

    AZ::IO::Path AWSGameLiftServerManager::GetInternalSessionCertificate()
    {
        // GameLift doesn't support it, return empty path
        return AZ::IO::Path();
    }

    void AWSGameLiftServerManager::InitializeGameLiftServerSDK()
    {
        if (m_serverSDKInitialized)
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerSDKAlreadyInitErrorMessage);
            return;
        }

        AZ_TracePrintf(AWSGameLiftServerManagerName, "Initiating Amazon GameLift Server SDK ...");
        Aws::GameLift::Server::InitSDKOutcome initOutcome = m_gameLiftServerSDKWrapper->InitSDK();
        AZ_TracePrintf(AWSGameLiftServerManagerName, "InitSDK request against Amazon GameLift service is complete.");

        m_serverSDKInitialized = initOutcome.IsSuccess();

        AZ_Error(AWSGameLiftServerManagerName, m_serverSDKInitialized,
            AWSGameLiftServerInitSDKErrorMessage, initOutcome.GetError().GetErrorMessage().c_str());
    }

    void AWSGameLiftServerManager::HandleDestroySession()
    {
        // No further request should be handled by GameLift server manager at this point
        if (AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get())
        {
            AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Unregister(this);
        }

        AZ_TracePrintf(AWSGameLiftServerManagerName, "Server process is scheduled to be shut down at %s",
            m_gameLiftServerSDKWrapper->GetTerminationTime().c_str());

        // Send notifications to handler(s) to gracefully shut down the server process.
        bool destroySessionResult = true;
        AZ::EBusReduceResult<bool&, AZStd::logical_and<bool>> result(destroySessionResult);
        AzFramework::SessionNotificationBus::BroadcastResult(result, &AzFramework::SessionNotifications::OnDestroySessionBegin);

        if (!destroySessionResult)
        {
            AZ_Error("AWSGameLift", destroySessionResult, AWSGameLiftServerGameSessionDestroyErrorMessage);
            return;
        }

        AZ_TracePrintf(AWSGameLiftServerManagerName, "Notifying GameLift server process is ending ...");
        Aws::GameLift::GenericOutcome processEndingOutcome = m_gameLiftServerSDKWrapper->ProcessEnding();
        if (processEndingOutcome.IsSuccess())
        {
            AZ_TracePrintf(AWSGameLiftServerManagerName, "ProcessEnding request against Amazon GameLift service succeeded.");
            AzFramework::SessionNotificationBus::Broadcast(&AzFramework::SessionNotifications::OnDestroySessionEnd);
        }
        else
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerProcessEndingErrorMessage,
                processEndingOutcome.GetError().GetErrorMessage().c_str());
        }
    }

    void AWSGameLiftServerManager::HandlePlayerLeaveSession(const AzFramework::PlayerConnectionConfig& playerConnectionConfig)
    {
        AZStd::string playerSessionId = "";
        RemoveConnectedPlayer(playerConnectionConfig.m_playerConnectionId, playerSessionId);
        if (playerSessionId.empty())
        {
            return;
        }

        AZ_TracePrintf(AWSGameLiftServerManagerName,
            "Removing player session %s from Amazon GameLift service ...", playerSessionId.c_str());
        Aws::GameLift::GenericOutcome disconnectOutcome = m_gameLiftServerSDKWrapper->RemovePlayerSession(playerSessionId);
        AZ_TracePrintf(AWSGameLiftServerManagerName,
            "RemovePlayerSession request for player session %s against Amazon GameLift service is complete.", playerSessionId.c_str());

        AZ_Error(AWSGameLiftServerManagerName, disconnectOutcome.IsSuccess(), AWSGameLiftServerRemovePlayerSessionErrorMessage,
            playerSessionId.c_str(), disconnectOutcome.GetError().GetErrorMessage().c_str());
    }

    bool AWSGameLiftServerManager::NotifyGameLiftProcessReady()
    {
        if (!m_serverSDKInitialized)
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerSDKNotInitErrorMessage);
            return false;
        }

        GameLiftServerProcessDesc desc = BuildGameLiftServerProcessDesc();
        AZ_Warning(AWSGameLiftServerManagerName, desc.m_port != 0, AWSGameLiftServerTempPortErrorMessage);

        AZ::JobContext* jobContext = nullptr;
        AZ::JobManagerBus::BroadcastResult(jobContext, &AZ::JobManagerEvents::GetGlobalContext);
        AZ::Job* processReadyJob = AZ::CreateJobFunction(
            [this, desc]() {
                // The GameLift ProcessParameters object expects an vector (std::vector) of standard strings (std::string) as the log paths.
                std::vector<std::string> logPaths;
                for (const AZStd::string& path : desc.m_logPaths)
                {
                    logPaths.push_back(path.c_str());
                }

                Aws::GameLift::Server::ProcessParameters processReadyParameter = Aws::GameLift::Server::ProcessParameters(
                    AZStd::bind(&AWSGameLiftServerManager::OnStartGameSession, this, AZStd::placeholders::_1),
                    AZStd::bind(&AWSGameLiftServerManager::OnUpdateGameSession, this, AZStd::placeholders::_1),
                    AZStd::bind(&AWSGameLiftServerManager::OnProcessTerminate, this),
                    AZStd::bind(&AWSGameLiftServerManager::OnHealthCheck, this), desc.m_port,
                    Aws::GameLift::Server::LogParameters(logPaths));

                AZ_TracePrintf(AWSGameLiftServerManagerName, "Notifying GameLift server process is ready ...");
                auto processReadyOutcome = m_gameLiftServerSDKWrapper->ProcessReady(processReadyParameter);
                AZ_TracePrintf(AWSGameLiftServerManagerName, "ProcessReady request against Amazon GameLift service is complete.");

                if (!processReadyOutcome.IsSuccess())
                {
                    AZ_Error(AWSGameLiftServerManagerName, false,
                        AWSGameLiftServerProcessReadyErrorMessage, processReadyOutcome.GetError().GetErrorMessage().c_str());
                    this->HandleDestroySession();
                }
        }, true, jobContext);
        processReadyJob->Start();
        return true;
    }

    void AWSGameLiftServerManager::OnStartGameSession(const Aws::GameLift::Server::Model::GameSession& gameSession)
    {
        UpdateGameSessionData(gameSession);
        AzFramework::SessionConfig sessionConfig = BuildSessionConfig(gameSession);

        bool createSessionResult = true;
        AZ::EBusReduceResult<bool&, AZStd::logical_and<bool>> result(createSessionResult);
        AzFramework::SessionNotificationBus::BroadcastResult(
            result, &AzFramework::SessionNotifications::OnCreateSessionBegin, sessionConfig);

        if (createSessionResult)
        {
            AZ_TracePrintf(AWSGameLiftServerManagerName, "Activating GameLift game session ...");
            Aws::GameLift::GenericOutcome activationOutcome = m_gameLiftServerSDKWrapper->ActivateGameSession();

            if (activationOutcome.IsSuccess())
            {
                AZ_TracePrintf(AWSGameLiftServerManagerName, "ActivateGameSession request against Amazon GameLift service succeeded.");
                // Register server manager as handler once game session has been activated
                if (!AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get())
                {
                    AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Register(this);
                }
                AzFramework::SessionNotificationBus::Broadcast(&AzFramework::SessionNotifications::OnCreateSessionEnd);
            }
            else
            {
                AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerActivateGameSessionErrorMessage,
                    activationOutcome.GetError().GetErrorMessage().c_str());
                HandleDestroySession();
            }
        }
        else
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerGameInitErrorMessage);
            HandleDestroySession();
        }
    }

    void AWSGameLiftServerManager::OnProcessTerminate()
    {
        AZ_TracePrintf(AWSGameLiftServerManagerName, "GameLift is shutting down server process ...");

        HandleDestroySession();
    }

    bool AWSGameLiftServerManager::OnHealthCheck()
    {
        bool healthCheckResult = true;
        AZ::EBusReduceResult<bool&, AZStd::logical_and<bool>> result(healthCheckResult);
        AzFramework::SessionNotificationBus::BroadcastResult(result, &AzFramework::SessionNotifications::OnSessionHealthCheck);

        return m_serverSDKInitialized && healthCheckResult;
    }

    void AWSGameLiftServerManager::OnUpdateGameSession(const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession)
    {
        AzFramework::SessionConfig sessionConfig = BuildSessionConfig(updateGameSession.GetGameSession());
        Aws::GameLift::Server::Model::UpdateReason updateReason = updateGameSession.GetUpdateReason();
        AzFramework::SessionNotificationBus::Broadcast(&AzFramework::SessionNotifications::OnUpdateSessionBegin,
            sessionConfig, Aws::GameLift::Server::Model::UpdateReasonMapper::GetNameForUpdateReason(updateReason).c_str());

        // Update game session data locally
        if (updateReason == Aws::GameLift::Server::Model::UpdateReason::MATCHMAKING_DATA_UPDATED)
        {
            UpdateGameSessionData(updateGameSession.GetGameSession());
        }

        AzFramework::SessionNotificationBus::Broadcast(&AzFramework::SessionNotifications::OnUpdateSessionEnd);
    }

    bool AWSGameLiftServerManager::RemoveConnectedPlayer(uint32_t playerConnectionId, AZStd::string& outPlayerSessionId)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_gameliftMutex);
        if (m_connectedPlayers.contains(playerConnectionId))
        {
            outPlayerSessionId = m_connectedPlayers[playerConnectionId];
            m_connectedPlayers.erase(playerConnectionId);
            return true;
        }
        else
        {
            outPlayerSessionId = "";
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerPlayerConnectionMissingErrorMessage, playerConnectionId);
            return false;
        }
    }

    void AWSGameLiftServerManager::SetGameLiftServerSDKWrapper(AZStd::unique_ptr<GameLiftServerSDKWrapper> gameLiftServerSDKWrapper)
    {
        m_gameLiftServerSDKWrapper.reset();
        m_gameLiftServerSDKWrapper = AZStd::move(gameLiftServerSDKWrapper);
    }

    bool AWSGameLiftServerManager::StartMatchBackfill(const AZStd::string& ticketId, const AZStd::vector<AWSGameLiftPlayer>& players)
    {
        if (!m_serverSDKInitialized)
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerSDKNotInitErrorMessage);
            return false;
        }

        if (!IsMatchmakingDataValid())
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftMatchmakingDataMissingErrorMessage);
            return false;
        }

        Aws::GameLift::Server::Model::StartMatchBackfillRequest request;
        if (!BuildStartMatchBackfillRequest(ticketId, players, request))
        {
            return false;
        }

        AZ_TracePrintf(AWSGameLiftServerManagerName, "Starting match backfill %s ...", ticketId.c_str());
        auto outcome = m_gameLiftServerSDKWrapper->StartMatchBackfill(request);
        if (!outcome.IsSuccess())
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftStartMatchBackfillErrorMessage,
                outcome.GetError().GetErrorMessage().c_str());
            return false;
        }
        else
        {
            AZ_TracePrintf(AWSGameLiftServerManagerName, "StartMatchBackfill request against Amazon GameLift service succeeded.");
            return true;
        }
    }

    bool AWSGameLiftServerManager::StopMatchBackfill(const AZStd::string& ticketId)
    {
        if (!m_serverSDKInitialized)
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerSDKNotInitErrorMessage);
            return false;
        }

        if (!IsMatchmakingDataValid())
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftMatchmakingDataMissingErrorMessage);
            return false;
        }

        Aws::GameLift::Server::Model::StopMatchBackfillRequest request;
        BuildStopMatchBackfillRequest(ticketId, request);

        AZ_TracePrintf(AWSGameLiftServerManagerName, "Stopping match backfill %s ...", ticketId.c_str());
        auto outcome = m_gameLiftServerSDKWrapper->StopMatchBackfill(request);
        if (!outcome.IsSuccess())
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftStopMatchBackfillErrorMessage,
                outcome.GetError().GetErrorMessage().c_str());
            return false;
        }
        else
        {
            AZ_TracePrintf(AWSGameLiftServerManagerName, "StopMatchBackfill request against Amazon GameLift service succeeded.");
            return true;
        }
    }

    void AWSGameLiftServerManager::UpdateGameSessionData(const Aws::GameLift::Server::Model::GameSession& gameSession)
    {
        AZ_TracePrintf(AWSGameLiftServerManagerName, "Lazy loading game session and matchmaking data from Amazon GameLift service ...");
        m_gameSession = Aws::GameLift::Server::Model::GameSession(gameSession);
        if (m_gameSession.GetMatchmakerData().empty())
        {
            m_matchmakingData.Parse("{}");
        }
        else
        {
            rapidjson::ParseResult parseResult = m_matchmakingData.Parse(m_gameSession.GetMatchmakerData().c_str());
            if (!parseResult)
            {
                AZ_Error(AWSGameLiftServerManagerName, false,
                    AWSGameLiftMatchmakingDataInvalidErrorMessage, rapidjson::GetParseError_En(parseResult.Code()));
            }
        }
    }

    bool AWSGameLiftServerManager::ValidatePlayerJoinSession(const AzFramework::PlayerConnectionConfig& playerConnectionConfig)
    {
        uint32_t playerConnectionId = playerConnectionConfig.m_playerConnectionId;
        AZStd::string playerSessionId = playerConnectionConfig.m_playerSessionId;
        if (playerSessionId.empty())
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerInvalidConnectionConfigErrorMessage,
                playerConnectionId, playerSessionId.c_str());
            return false;
        }

        if (!AddConnectedPlayer(playerConnectionConfig))
        {
            return false;
        }

        AZ_TracePrintf(AWSGameLiftServerManagerName,
            "Attempting to accept player session %s connection with Amazon GameLift service ...", playerSessionId.c_str());
        auto acceptPlayerSessionOutcome = m_gameLiftServerSDKWrapper->AcceptPlayerSession(playerSessionId.c_str());
        AZ_TracePrintf(AWSGameLiftServerManagerName,
            "AcceptPlayerSession request for player session %s against Amazon GameLift service is complete.", playerSessionId.c_str());

        if (!acceptPlayerSessionOutcome.IsSuccess())
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerAcceptPlayerSessionErrorMessage,
                playerSessionId.c_str(), acceptPlayerSessionOutcome.GetError().GetErrorMessage().c_str());
            RemoveConnectedPlayer(playerConnectionId, playerSessionId);
            return false;
        }
        return true;
    }
} // namespace AWSGameLift
