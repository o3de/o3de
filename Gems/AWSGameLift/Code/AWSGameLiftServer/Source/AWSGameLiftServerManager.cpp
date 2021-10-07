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
        AZ_TracePrintf(AWSGameLiftServerManagerName, "ProcessEnding request against Amazon GameLift service is complete.");

        [[maybe_unused]] bool processEndingIsSuccess = processEndingOutcome.IsSuccess();

        AZ_Error(AWSGameLiftServerManagerName, processEndingIsSuccess, AWSGameLiftServerProcessEndingErrorMessage,
            processEndingOutcome.GetError().GetErrorMessage().c_str());
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
                    AZStd::bind(&AWSGameLiftServerManager::OnUpdateGameSession, this),
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
        AzFramework::SessionConfig sessionConfig = BuildSessionConfig(gameSession);

        bool createSessionResult = true;
        AZ::EBusReduceResult<bool&, AZStd::logical_and<bool>> result(createSessionResult);
        AzFramework::SessionNotificationBus::BroadcastResult(
            result, &AzFramework::SessionNotifications::OnCreateSessionBegin, sessionConfig);

        if (createSessionResult)
        {
            AZ_TracePrintf(AWSGameLiftServerManagerName, "Activating GameLift game session ...");
            Aws::GameLift::GenericOutcome activationOutcome = m_gameLiftServerSDKWrapper->ActivateGameSession();
            AZ_TracePrintf(AWSGameLiftServerManagerName, "ActivateGameSession request against Amazon GameLift service is complete.");

            if (activationOutcome.IsSuccess())
            {
                // Register server manager as handler once game session has been activated
                if (!AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Get())
                {
                    AZ::Interface<AzFramework::ISessionHandlingProviderRequests>::Register(this);
                }
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

    void AWSGameLiftServerManager::OnUpdateGameSession()
    {
        // TODO: Perform game-specific tasks to prep for newly matched players
        return;
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
