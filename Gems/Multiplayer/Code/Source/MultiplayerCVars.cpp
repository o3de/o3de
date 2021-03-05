/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "Multiplayer_precompiled.h"

#include <INetwork.h>
#include <SFunctor.h>
#include "MultiplayerCVars.h"
#include "MultiplayerGem.h"
#include "MultiplayerGameLiftClient.h"
#include "Multiplayer/MultiplayerUtils.h"

#include <GameLift/GameLiftBus.h>
#include <GameLift/Session/GameLiftSessionDefs.h>
#include <GameLift/Session/GameLiftServerService.h>
#include <CertificateManager/ICertificateManagerGem.h>
#include <CertificateManager/DataSource/FileDataSourceBus.h>

#include <GridMate/Carrier/DefaultSimulator.h>
#include <GridMate/Session/LANSession.h>
#include <Multiplayer_Traits_Platform.h>

namespace Multiplayer
{

#if defined(BUILD_GAMELIFT_SERVER)
    static void StartGameLiftServer([[maybe_unused]] IConsoleCmdArgs *args)
    {
        CRY_ASSERT(gEnv->pConsole);

        // set the sys_dump_type to 2 so error dump files don't exceed GameLift size limits
        ICVar* sys_dump_type_cvar = gEnv->pConsole->GetCVar("sys_dump_type");
        if (sys_dump_type_cvar)
        {
            sys_dump_type_cvar->Set(2);
        }

        GridMate::GameLiftServerServiceDesc serviceDesc;
        if (gEnv && gEnv->pFileIO)
        {
            const char pathToLogFolder[] = "@log@/";
            char resolvedPath[AZ_MAX_PATH_LEN];
            if (gEnv->pFileIO->ResolvePath(pathToLogFolder, resolvedPath, AZ_ARRAY_SIZE(resolvedPath)))
            {
                serviceDesc.m_logPaths.push_back(resolvedPath);
            }
        }

        if (gEnv->pConsole->GetCVar("sv_port"))
        {
            serviceDesc.m_port = gEnv->pConsole->GetCVar("sv_port")->GetIVal();
        }

        EBUS_EVENT(GameLift::GameLiftRequestBus, StartServerService, serviceDesc);
    }

    static void StopGameLiftServer([[maybe_unused]] IConsoleCmdArgs *args)
    {
        EBUS_EVENT(GameLift::GameLiftRequestBus, StopServerService);
    }
#endif
    //-----------------------------------------------------------------------------
    void CmdNetSimulator(IConsoleCmdArgs* args)
    {
        GridMate::Simulator* simulator = nullptr;
        EBUS_EVENT_RESULT(simulator,Multiplayer::MultiplayerRequestBus,GetSimulator);

        GridMate::GridSession* session = nullptr;
        EBUS_EVENT_RESULT(session,Multiplayer::MultiplayerRequestBus,GetSession);

        if (!simulator && session)
        {
            CryLogAlways("Simulator should be enabled before GridMate session starts. Use 'mpdisconnect' to destroy the session.");
            return;
        }

        if (args->GetArgCount() == 2 && CryStringUtils::ToYesNoType(args->GetArg(1)) == CryStringUtils::YesNoType::No)
        {
            EBUS_EVENT(Multiplayer::MultiplayerRequestBus,DisableSimulator);
            return;
        }

        if (args->GetArgCount() == 2 && !azstricmp(args->GetArg(1), "help"))
        {
            CryLogAlways("gm_net_simulator off      - Disable simulator");
            CryLogAlways("gm_net_simulator param1:value1 param2:value2, ...      - Enable simulator with given parameters");
            CryLogAlways("Available parameters:");
            CryLogAlways("oLatMin, oLatMax      - Outgoing latency in milliseconds");
            CryLogAlways("iLatMin, iLatMax      - Incoming latency in milliseconds");
            CryLogAlways("oBandMin, oBandMax      - Outgoing bandwidth in Kbps");
            CryLogAlways("iBandMin, iBandMax      - Incoming bandwidth in Kbps");
            CryLogAlways("oLossMin, oLossMax      - Outgoing packet loss, will lose one packet every interval");
            CryLogAlways("iLossMin, iLossMax      - Incoming packet loss, will lose one packet every interval");
            CryLogAlways("oDropMin, oDropMax, oDropPeriodMin, oDropPeriodMax      - Outgoing packet drop, will periodically lose packets for given interval");
            CryLogAlways("iDropMin, iDropMax, iDropPeriodMin, iDropPeriodMax      - Incoming packet drop, will periodically lose packets for given interval");
            CryLogAlways("oReorder      - [0|1] Outgoing packet reordering. You need to enable latency to reorder packets.");
            CryLogAlways("iReorder      - [0|1] Incoming packet reordering. You need to enable latency to reorder packets.");
            return;
        }

        if (args->GetArgCount() > 1)
        {
            EBUS_EVENT(Multiplayer::MultiplayerRequestBus,EnableSimulator);
            EBUS_EVENT_RESULT(simulator, Multiplayer::MultiplayerRequestBus, GetSimulator);
        }

        GridMate::DefaultSimulator* sim = static_cast<GridMate::DefaultSimulator*>(simulator);

        if (sim)
        {
            unsigned int oLatMin, oLatMax;
            unsigned int iLatMin, iLatMax;
            unsigned int oBandMin, oBandMax;
            unsigned int iBandMin, iBandMax;
            unsigned int oLossMin, oLossMax;
            unsigned int iLossMin, iLossMax;
            unsigned int oDropMin, oDropMax, oDropPeriodMin, oDropPeriodMax;
            unsigned int iDropMin, iDropMax, iDropPeriodMin, iDropPeriodMax;
            bool oReorder, iReorder;

            sim->GetOutgoingLatency(oLatMin, oLatMax);
            sim->GetIncomingLatency(iLatMin, iLatMax);
            sim->GetOutgoingBandwidth(oBandMin, oBandMax);
            sim->GetIncomingBandwidth(iBandMin, iBandMax);
            sim->GetOutgoingPacketLoss(oLossMin, oLossMax);
            sim->GetIncomingPacketLoss(iLossMin, iLossMax);
            sim->GetOutgoingPacketDrop(oDropMin, oDropMax, oDropPeriodMin, oDropPeriodMax);
            sim->GetIncomingPacketDrop(iDropMin, iDropMax, iDropPeriodMin, iDropPeriodMax);
            oReorder = sim->IsOutgoingReorder();
            iReorder = sim->IsIncomingReorder();

            for (int i = 1; i < args->GetArgCount(); ++i)
            {
                const char* arg = args->GetArg(i);

                unsigned int param;
                char key[64];

#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
                int numParams = sscanf_s(arg, "%64[^:]:%u", key, (unsigned int)AZ_ARRAY_SIZE(key), &param) - 1;
#else
                int numParams = sscanf(arg, "%64[^:]:%u", key, &param) - 1;
#endif

                if (numParams <= 0)
                {
                    CryLogAlways("ERROR: Invalid argument format: %s. Should be 'key:value'. Bailing out.", arg);
                    return;
                }

                if (!azstricmp(key, "oLatMin"))
                {
                    oLatMin = param;
                }
                else if (!azstricmp(key, "oLatMax"))
                {
                    oLatMax = param;
                }
                else if (!azstricmp(key, "iLatMin"))
                {
                    iLatMin = param;
                }
                else if (!azstricmp(key, "iLatMax"))
                {
                    iLatMax = param;
                }
                else if (!azstricmp(key, "oBandMin"))
                {
                    oBandMin = param;
                }
                else if (!azstricmp(key, "oBandMax"))
                {
                    oBandMax = param;
                }
                else if (!azstricmp(key, "iBandMin"))
                {
                    iBandMin = param;
                }
                else if (!azstricmp(key, "iBandMax"))
                {
                    iBandMax = param;
                }
                else if (!azstricmp(key, "oLossMin"))
                {
                    oLossMin = param;
                }
                else if (!azstricmp(key, "oLossMax"))
                {
                    oLossMax = param;
                }
                else if (!azstricmp(key, "iLossMin"))
                {
                    iLossMin = param;
                }
                else if (!azstricmp(key, "iLossMax"))
                {
                    iLossMax = param;
                }
                else if (!azstricmp(key, "oDropMin"))
                {
                    oDropMin = param;
                }
                else if (!azstricmp(key, "oDropMax"))
                {
                    oDropMax = param;
                }
                else if (!azstricmp(key, "oDropPeriodMin"))
                {
                    oDropPeriodMin = param;
                }
                else if (!azstricmp(key, "oDropPeriodMax"))
                {
                    oDropPeriodMax = param;
                }
                else if (!azstricmp(key, "iDropMin"))
                {
                    iDropMin = param;
                }
                else if (!azstricmp(key, "iDropMax"))
                {
                    iDropMax = param;
                }
                else if (!azstricmp(key, "iDropPeriodMin"))
                {
                    iDropPeriodMin = param;
                }
                else if (!azstricmp(key, "iDropPeriodMax"))
                {
                    iDropPeriodMax = param;
                }
                else if (!azstricmp(key, "oReorder"))
                {
                    oReorder = param != 0;
                }
                else if (!azstricmp(key, "iReorder"))
                {
                    iReorder = param != 0;
                }
                else
                {
                    CryLogAlways("ERROR: Invalid argument: %s. Bailing out.", key);
                    return;
                }
            }

            sim->SetOutgoingLatency(oLatMin, oLatMax);
            sim->SetIncomingLatency(iLatMin, iLatMax);
            sim->SetOutgoingBandwidth(oBandMin, oBandMax);
            sim->SetIncomingBandwidth(iBandMin, iBandMax);
            sim->SetOutgoingPacketLoss(oLossMin, oLossMax);
            sim->SetIncomingPacketLoss(iLossMin, iLossMax);
            sim->SetOutgoingPacketDrop(oDropMin, oDropMax, oDropPeriodMin, oDropPeriodMax);
            sim->SetIncomingPacketDrop(iDropMin, iDropMax, iDropPeriodMin, iDropPeriodMax);
            sim->SetOutgoingReorder(oReorder);
            sim->SetIncomingReorder(iReorder);

            CryLogAlways("Simulator settings:");
            CryLogAlways("OutgoingLatency: (%u, %u)", oLatMin, oLatMax);
            CryLogAlways("IncomingLatency: (%u, %u)", iLatMin, iLatMax);
            CryLogAlways("OutgoingBandwidth: (%u, %u)", oBandMin, oBandMax);
            CryLogAlways("IncomingBandwidth: (%u, %u)", iBandMin, iBandMax);
            CryLogAlways("OutgoingPacketLoss: (%u, %u)", oLossMin, oLossMax);
            CryLogAlways("IncomingPacketLoss: (%u, %u)", iLossMin, iLossMax);
            CryLogAlways("OutgoingPacketDrop: (%u, %u, %u, %u)", oDropMin, oDropMax, oDropPeriodMin, oDropPeriodMax);
            CryLogAlways("IncomingPacketDrop: (%u, %u, %u, %u)", iDropMin, iDropMax, iDropPeriodMin, iDropPeriodMax);
            CryLogAlways("OutgoingReorder: %s", oReorder ? "on" : "off");
            CryLogAlways("IncomingReorder: %s", iReorder ? "on" : "off");
        }
        else
        {
            CryLogAlways("Simulator is disabled.");
        }
    }

#if defined(NET_SUPPORT_SECURE_SOCKET_DRIVER)

    void MultiplayerCVars::OnPrivateKeyChanged(ICVar* filename)
    {
        if (filename && filename->GetString() && filename->GetString()[0])
        {
            CreateFileDataSource();
            EBUS_EVENT(CertificateManager::FileDataSourceConfigurationBus,ConfigurePrivateKey,filename->GetString());
        }
        else
        {
            AZ_Warning("CertificateManager", false, "Failed to load Private Key '%s'.", filename->GetString());
        }
    }

    void MultiplayerCVars::OnCertificateChanged(ICVar* filename)
    {
        if (filename && filename->GetString() && filename->GetString()[0])
        {
            CreateFileDataSource();
            EBUS_EVENT(CertificateManager::FileDataSourceConfigurationBus,ConfigureCertificate,filename->GetString());
        }
        else
        {
            AZ_Warning("CertificateManager", false, "Failed to load Certificate '%s'.", filename->GetString());
        }
    }

    void MultiplayerCVars::OnCAChanged(ICVar* filename)
    {
        if (filename && filename->GetString() && filename->GetString()[0])
        {
            CreateFileDataSource();
            EBUS_EVENT(CertificateManager::FileDataSourceConfigurationBus,ConfigureCertificateAuthority,filename->GetString());
            }
        else
        {
            AZ_Warning("CertificateManager", false, "Failed to load CA '%s'.", filename->GetString());
        }
    }

    void MultiplayerCVars::CreateFileDataSource()
    {
        if (CertificateManager::FileDataSourceConfigurationBus::FindFirstHandler() == nullptr)
        {
            EBUS_EVENT(CertificateManager::FileDataSourceCreationBus,CreateFileDataSource);

            if (CertificateManager::FileDataSourceConfigurationBus::FindFirstHandler() == nullptr)
            {
                AZ_Assert(false,"Unable to create File Data Source");
            }
        }

        AZ_Assert(CertificateManager::FileDataSourceConfigurationBus::FindFirstHandler() != nullptr,"Incorrect DataSource configured for File Based CVars");
    }
#endif

    static void OnDisconnectDetectionChanged(ICVar* cvar)
    {
        GridMate::GridSession* session = nullptr;
        EBUS_EVENT_RESULT(session,Multiplayer::MultiplayerRequestBus,GetSession);
        if (!session)
        {
            return;
        }

        if (!session->IsHost())
        {
            CryLogAlways("Will not apply to the active session, only host can control disconnect detection mode for a game in progress.");
            return;
        }

        session->DebugEnableDisconnectDetection(cvar->GetIVal() != 0);
    }

    static void OnReplicasSendTimeChanged(ICVar* cvar)
    {
        GridMate::GridSession* session = nullptr;
        EBUS_EVENT_RESULT(session,Multiplayer::MultiplayerRequestBus,GetSession);

        if (!session)
        {
            return;
        }

        session->GetReplicaMgr()->SetSendTimeInterval(cvar->GetIVal());
    }

    static void OnReplicasSendLimitChanged(ICVar* cvar)
    {
        GridMate::GridSession* session = nullptr;
        EBUS_EVENT_RESULT(session,Multiplayer::MultiplayerRequestBus,GetSession);

        if (!session)
        {
            return;
        }

        session->GetReplicaMgr()->SetSendLimit(cvar->GetIVal());
    }

    static void OnReplicasBurstRangeChanged(ICVar* cvar)
    {
        GridMate::GridSession* session = nullptr;
        EBUS_EVENT_RESULT(session,Multiplayer::MultiplayerRequestBus,GetSession);

        if (!session)
        {
            return;
        }

        session->GetReplicaMgr()->SetSendLimitBurstRange(cvar->GetFVal());
    }

    //-----------------------------------------------------------------------------
    MultiplayerCVars* MultiplayerCVars::s_instance = nullptr;

    //-----------------------------------------------------------------------------
    MultiplayerCVars::MultiplayerCVars()
        : m_autoJoin(false)
        , m_search(nullptr)
    {
        s_instance = this;
    }

    //-----------------------------------------------------------------------------
    MultiplayerCVars::~MultiplayerCVars()
    {
        if (s_instance == this)
        {
            s_instance = nullptr;
        }
    }

    void MultiplayerCVars::VerifyMaxPlayers(ICVar* pVar)
    {
        int nPlayers = pVar->GetIVal();
        if (nPlayers < 2 || nPlayers > MAXIMUM_NUMBER_OF_CONNECTIONS)
        {
            nPlayers = CLAMP(nPlayers, 2, MAXIMUM_NUMBER_OF_CONNECTIONS);
            pVar->Set(nPlayers);
        }
    }


    //------------------------------------------------------------------------
    void MultiplayerCVars::RegisterCVars()
    {
        if (gEnv && !gEnv->IsEditor())
        {
            // Adding removed cvars from CryAction 
            if (gEnv->IsDedicated())
            {
                REGISTER_STRING("sv_map", "nolevel", 0, "The map the server should load");

                REGISTER_STRING("sv_levelrotation", "levelrotation", 0, "Sequence of levels to load after each game ends");

                REGISTER_STRING("sv_requireinputdevice", "dontcare", VF_DUMPTODISK | VF_REQUIRE_LEVEL_RELOAD, "Which input devices to require at connection (dontcare, none, gamepad, keyboard)");
                REGISTER_STRING("sv_gamerulesdefault", "DummyRules", 0, "The game rules that the server default to when disconnecting");
                REGISTER_STRING("sv_gamerules", "Multiplayer", 0, "The game rules that the server should use");
                REGISTER_INT("sv_port", SERVER_DEFAULT_PORT, VF_DUMPTODISK, "Server address");
                REGISTER_STRING("sv_password", "", VF_DUMPTODISK, "Server password");
                REGISTER_INT("sv_lanonly", 0, VF_DUMPTODISK, "Set for LAN games");

                REGISTER_STRING("sv_bind", "0.0.0.0", VF_REQUIRE_LEVEL_RELOAD, "Bind the server to a specific IP address");

                REGISTER_STRING("sv_servername", "", VF_DUMPTODISK, "Server name will be displayed in server list. If empty, machine name will be used.");
                REGISTER_INT_CB("sv_maxplayers", 32, VF_DUMPTODISK, "Maximum number of players allowed to join server.", VerifyMaxPlayers);
                REGISTER_INT("sv_maxspectators", 32, VF_DUMPTODISK, "Maximum number of players allowed to be spectators during the game.");
                REGISTER_INT("ban_timeout", 30, VF_DUMPTODISK, "Ban timeout in minutes");
                REGISTER_FLOAT("sv_timeofdaylength", 1.0f, VF_DUMPTODISK, "Sets time of day changing speed.");
                REGISTER_FLOAT("sv_timeofdaystart", 12.0f, VF_DUMPTODISK, "Sets time of day start time.");
                REGISTER_INT("sv_timeofdayenable", 0, VF_DUMPTODISK, "Enables time of day simulation.");    
            }

            REGISTER_COMMAND("mphost", MPHostLANCmd, 0, "begin hosting a LAN session");
            REGISTER_COMMAND("mpjoin", MPJoinLANCmd, 0, "try to join a LAN session");
            REGISTER_COMMAND("mpsearch", MPJoinLANCmd, 0, "try to find a LAN session");
            REGISTER_COMMAND("mpdisconnect", MPDisconnectCmd, 0, "disconnect from our session");

            REGISTER_INT("gm_version", 1, VF_CONST_CVAR, "Set the gridmate version number.");

#ifdef NET_SUPPORT_SECURE_SOCKET_DRIVER
            REGISTER_CVAR2("gm_netsec_enable", &NetSec::s_NetsecEnabled, NetSec::s_NetsecEnabled, VF_NULL,
                "Enable network level encryption. Must be called before hosting or joining a session (e.g. by using mphost or mpjoin).");
            REGISTER_STRING_CB("gm_netsec_private_key", nullptr, VF_DEV_ONLY,
                "Set the private key file (PEM format) to use when establishing a secure network connection.", OnPrivateKeyChanged);
            REGISTER_STRING_CB("gm_netsec_certificate", nullptr, VF_DEV_ONLY,
                "Set the certificate file (PEM format) to use when establishing a secure network connection.", OnCertificateChanged);
            REGISTER_STRING_CB("gm_netsec_ca", nullptr, VF_DEV_ONLY,
                "Set the CA certificate file (PEM format) to use when establishing a secure network connection.", OnCAChanged);
            REGISTER_CVAR2("gm_netsec_verify_client", &NetSec::s_NetsecVerifyClient, NetSec::s_NetsecVerifyClient, VF_NULL,
                "Enable client authentication. If not set only the server will be authenticated. Only needs to be called on the server!");
#endif

            REGISTER_COMMAND("gm_net_simulator", CmdNetSimulator, VF_DEV_ONLY, "Setup network simulator. See 'gm_net_simulator help' for available options.");
            REGISTER_INT_CB("gm_disconnectDetection", 1, VF_NULL, "GridMate disconnect detection.", OnDisconnectDetectionChanged);
            REGISTER_FLOAT("gm_disconnectDetectionRttThreshold", 500.0f, VF_NULL, "Rtt threshold in milliseconds, connection will be dropped once actual rtt is bigger than this value");
            REGISTER_FLOAT("gm_disconnectDetectionPacketLossThreshold", 0.3f, VF_NULL, "Packet loss percentage threshold (0.0..1.0, 1.0 is 100%), connection will be dropped once actual packet loss exceeds this value");
            REGISTER_INT("gm_recvPacketsLimit", 0, VF_NULL, "Maximum packets per second allowed to be received from an existing connection");

            REGISTER_INT("gm_maxSearchResults", GridMate::SearchParams::s_defaultMaxSessions, VF_NULL, "Maximum number of search results to be returned from a session search.");
            REGISTER_STRING("gm_ipversion", "IPv4", 0, "IP protocol version. (Can be 'IPv4' or 'IPv6')");
            REGISTER_STRING("gm_securityData", "", 0, AZ_TRAIT_MULTIPLAYER_REGISTER_CVAR_SECURITY_DATA_DESC);
            REGISTER_INT_CB("gm_replicasSendTime", 0, VF_NULL, "Time interval between replicas sends (in milliseconds), 0 will bound sends to GridMate tick rate", OnReplicasSendTimeChanged);
            REGISTER_INT_CB("gm_replicasSendLimit", 0, VF_DEV_ONLY, "Replica data send limit in bytes per second. 0 - limiter turned off. (Dev build only)", OnReplicasSendLimitChanged);
            REGISTER_FLOAT_CB("gm_burstTimeLimit", 10.f, VF_DEV_ONLY, "Burst in bandwidth will be allowed for the given amount of time(in seconds). Burst will only be allowed if bandwidth is not capped at the time of burst. (Dev build only)", OnReplicasBurstRangeChanged);

#if AZ_TRAIT_MULTIPLAYER_USE_MATCH_MAKER_CVARS
            REGISTER_STRING(AZ_TRAIT_MULTIPLAYER_CVAR_MATCH_MAKER_SESSION_TEMPLATE, "GroupBuildingLobby", 0, AZ_TRAIT_MULTIPLAYER_CVAR_MATCH_MAKER_SESSION_TEMPLATE_DESC);
            REGISTER_STRING(AZ_TRAIT_MULTIPLAYER_CVAR_MATCH_MAKER_ID, "DefaultHopper", 0, AZ_TRAIT_MULTIPLAYER_CVAR_MATCH_MAKER_ID_DESC);
#endif

#if defined(BUILD_GAMELIFT_CLIENT)
            REGISTER_STRING("gamelift_fleet_id", "", VF_DUMPTODISK, "Id of GameLift Fleet to use with this client.");
            REGISTER_STRING("gamelift_queue_name", "", VF_DUMPTODISK, "Name of GameLift Queue to use with this client.");
            REGISTER_STRING("gamelift_aws_access_key", "", VF_DUMPTODISK, "AWS Access Key.");
            REGISTER_STRING("gamelift_aws_secret_key", "", VF_DUMPTODISK, "AWS Secret Key.");
            REGISTER_STRING("gamelift_aws_region", "us-west-2", VF_DUMPTODISK, "AWS Region to use for GameLift.");
            REGISTER_STRING("gamelift_endpoint", "gamelift.us-west-2.amazonaws.com", VF_DUMPTODISK, "GameLift service endpoint.");
            REGISTER_STRING("gamelift_alias_id", "", VF_DUMPTODISK, "Id of GameLift alias to use with the client.");
            REGISTER_STRING("gamelift_matchmaking_config_name", "", VF_DUMPTODISK, "Matchmaking config name");
            REGISTER_INT("gamelift_uselocalserver", 0, VF_DEV_ONLY, "Set to non zero to use the local GameLift Server.");

            REGISTER_COMMAND_DEV_ONLY("gamelift_host", MPHostGameLiftCmd, 0, "try to create and then join a GameLift session. gamelift_host <serverName> <mapName> <maxPlayers>");
            REGISTER_COMMAND_DEV_ONLY("gamelift_join", MPJoinGameLiftCmd, 0, "try to join a GameLift session");
            REGISTER_COMMAND_DEV_ONLY("gamelift_flexmatch", MPMatchmakingGameLiftCmd, 0, "try to matchmake a GameLift session creates or backfills matchmake game session. gamelift_flexmatch <configName>");

            // player IDs must be unique and anonymous
            bool includeBrackets = false;
            bool includeDashes = true;
            AZStd::string defaultPlayerId = AZ::Uuid::CreateRandom().ToString<AZStd::string>(includeBrackets, includeDashes);
            REGISTER_STRING("gamelift_player_id", defaultPlayerId.c_str(), VF_DUMPTODISK, "Player Id.");
            REGISTER_COMMAND("gamelift_stop_client", StopGameLiftClient, VF_NULL, "Stops GameLift session service and terminates the session if it had one.");
#endif

#if defined(BUILD_GAMELIFT_SERVER)
            REGISTER_COMMAND("gamelift_start_server", StartGameLiftServer, VF_NULL, "Start up the GameLift server. This will initialize gameLift server API.\nThe session will start after GameLift initialization");
            REGISTER_COMMAND("gamelift_stop_server", StopGameLiftServer, VF_NULL, "Stops GameLift session service and terminates the session if it had one.");
            REGISTER_INT("gamelift_flexmatch_enable", 0, VF_NULL, "Enable Custom backfill");
            REGISTER_INT("gamelift_flexmatch_onplayerremoved_enable", 0, VF_NULL, "Enables creating backfill tickets on player disconnect.");
            REGISTER_INT("gamelift_flexmatch_minimumplayersessioncount", 2, VF_NULL, "Minimum player session count in a matchmaking config. Same as min players in matchmaking rule set");
            REGISTER_FLOAT("gamlift_flexmatch_start_delay", 5.0F, VF_NULL, "initial delay for custom backfill in seconds.");
#endif
        }
    }
    //------------------------------------------------------------------------
    void MultiplayerCVars::UnregisterCVars()
    {
        if (gEnv && !gEnv->IsEditor())
        {
#if defined(BUILD_GAMELIFT_CLIENT)
            UNREGISTER_COMMAND("gamelift_start_client");
            UNREGISTER_CVAR("gamelift_player_id");
            UNREGISTER_CVAR("gamelift_alias_id");
            UNREGISTER_CVAR("gamelift_uselocalserver");
            UNREGISTER_CVAR("gamelift_endpoint");
            UNREGISTER_CVAR("gamelift_aws_region");
            UNREGISTER_CVAR("gamelift_aws_secret_key");
            UNREGISTER_CVAR("gamelift_aws_access_key");
            UNREGISTER_CVAR("gamelift_fleet_id");
            UNREGISTER_CVAR("gamelift_queue_name");
            UNREGISTER_CVAR("gamelift_matchmaking_config_name");
#endif

#if defined(BUILD_GAMELIFT_SERVER)
            UNREGISTER_COMMAND("gamelift_stop_server");
            UNREGISTER_COMMAND("gamelift_start_server");
            UNREGISTER_COMMAND("gamelift_flexmatch_enable");
            UNREGISTER_COMMAND("gamelift_flexmatch_onplayerremoved_enable");
            UNREGISTER_COMMAND("gamelift_flexmatch_minimumplayersessioncount");
            UNREGISTER_COMMAND("gamlift_flexmatch_start_delay");
#endif

#if AZ_TRAIT_MULTIPLAYER_USE_MATCH_MAKER_CVARS
            UNREGISTER_CVAR(AZ_TRAIT_MULTIPLAYER_CVAR_MATCH_MAKER_ID);
            UNREGISTER_CVAR(AZ_TRAIT_MULTIPLAYER_CVAR_MATCH_MAKER_SESSION_TEMPLATE);
#endif
            UNREGISTER_CVAR("gm_burstTimeLimit");
            UNREGISTER_CVAR("gm_replicasSendLimit");
            UNREGISTER_CVAR("gm_replicasSendTime");
            UNREGISTER_CVAR("gm_securityData");
            UNREGISTER_CVAR("gm_ipversion");
            UNREGISTER_CVAR("gm_maxSearchResults");
            UNREGISTER_CVAR("gm_disconnectDetectionPacketLossThreshold");
            UNREGISTER_CVAR("gm_disconnectDetectionRttThreshold");
            UNREGISTER_CVAR("gm_disconnectDetection");

            UNREGISTER_COMMAND("gm_net_simulator");

#ifdef NET_SUPPORT_SECURE_SOCKET_DRIVER
            UNREGISTER_CVAR("gm_netsec_ca");
            UNREGISTER_CVAR("gm_netsec_certificate");
            UNREGISTER_CVAR("gm_netsec_private_key");
            UNREGISTER_CVAR("gm_netsec_enable");
#endif
            UNREGISTER_CVAR("gm_version");

            UNREGISTER_COMMAND("mpdisconnect");
            UNREGISTER_COMMAND("mpsearch");
            UNREGISTER_COMMAND("mpjoin");
            UNREGISTER_COMMAND("mphost");
            {
                gEnv->pConsole->RemoveCommand("mpdisconnect");
                gEnv->pConsole->RemoveCommand("mpsearch");
                gEnv->pConsole->RemoveCommand("mpjoin");
                gEnv->pConsole->RemoveCommand("mphost");
            }
        }
    }

    //------------------------------------------------------------------------
    static void UpdateServerName(ICVar* serverNameCVar)
    {
        GridMate::GridSession* gridSession = nullptr;
        EBUS_EVENT_RESULT(gridSession,Multiplayer::MultiplayerRequestBus,GetSession);

        if (!(gridSession && gridSession->IsHost()))
        {
            return;
        }

        AZ_TracePrintf("MultiplayerModule", "Updating session server name to: %s", serverNameCVar->GetString());

        GridMate::GridSessionParam serverNameParam;
        serverNameParam.m_id = "sv_name";
        serverNameParam.SetValue(serverNameCVar->GetString());
        gridSession->SetParam(serverNameParam);
    }

    //------------------------------------------------------------------------
    // It would be more convenient to setup this CVar change event listener in RegisterCVars, however,
    // it is currently not possible to do so. This is due to Gem CVars being registered in response to
    // CryHooksModule::OnCrySystemInitialized, while CryAction CVars (such as sv_servername) are not registered
    // until after that event completes. Instead, this method is called during the system event
    // ESYSTEM_EVENT_GAME_POST_INIT, which does occur after CryAction's CVars have been registered.
    void MultiplayerCVars::PostInitRegistration()
    {
        ISystem* system = nullptr;
        CrySystemRequestBus::BroadcastResult(system, &CrySystemRequestBus::Events::GetCrySystem);
        if (system && system->GetIConsole())
        {
            if (ICVar* serverNameCVar = system->GetIConsole()->GetCVar("sv_servername"))
            {
                SFunctor onServerNameChange;
                onServerNameChange.Set(UpdateServerName, serverNameCVar);
                serverNameCVar->AddOnChangeFunctor(onServerNameChange);
            }
        }
    }

    //------------------------------------------------------------------------
    void MultiplayerCVars::MPHostLANCmd(IConsoleCmdArgs* args)
    {
        (void)args;

        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();
        if (!gridMate)
        {
            CryLogAlways("GridMate has not been initialized.");
            return;
        }

        GridMate::GridSession* gridSession = nullptr;
        EBUS_EVENT_RESULT(gridSession,Multiplayer::MultiplayerRequestBus,GetSession);

        if (gridSession)
        {
            CryLogAlways("You're already part of a session. Use 'mpdisconnect' first.");
            return;
        }

        if (!GridMate::HasGridMateService<GridMate::LANSessionService>(gridMate))
        {
            Multiplayer::LAN::StartSessionService(gridMate);
        }

        // Attempt to start a hosted LAN session. If we do, we're now a server and in multiplayer mode.
        GridMate::LANSessionParams sp;
        sp.m_topology = GridMate::ST_CLIENT_SERVER;
        sp.m_numPublicSlots = gEnv->pConsole->GetCVar("sv_maxplayers")->GetIVal() + (gEnv->IsDedicated() ? 1 : 0); // One slot for server member.
        sp.m_numPrivateSlots = 0;
        sp.m_port = gEnv->pConsole->GetCVar("sv_port")->GetIVal() + 1; // Listen for searches on sv_port + 1
        sp.m_peerToPeerTimeout = 60000;
        sp.m_flags = 0;
        sp.m_numParams = 0;

        ICVar* serverName = gEnv->pConsole->GetCVar("sv_servername");

        if (serverName)
        {
            sp.m_params[sp.m_numParams].m_id = "sv_name";
            sp.m_params[sp.m_numParams].SetValue(serverName->GetString());
            sp.m_numParams++;
        }

        GridMate::CarrierDesc carrierDesc;

        Multiplayer::Utils::InitCarrierDesc(carrierDesc);
        Multiplayer::NetSec::ConfigureCarrierDescForHost(carrierDesc);

        carrierDesc.m_port = static_cast<uint16>(gEnv->pConsole->GetCVar("sv_port")->GetIVal());
        carrierDesc.m_enableDisconnectDetection = !!gEnv->pConsole->GetCVar("gm_disconnectDetection")->GetIVal();
        carrierDesc.m_connectionTimeoutMS = 10000;
        carrierDesc.m_threadUpdateTimeMS = 30;
        carrierDesc.m_disconnectDetectionRttThreshold = gEnv->pConsole->GetCVar("gm_disconnectDetectionRttThreshold")->GetFVal();
        carrierDesc.m_disconnectDetectionPacketLossThreshold = gEnv->pConsole->GetCVar("gm_disconnectDetectionPacketLossThreshold")->GetFVal();
        carrierDesc.m_maxConnections = gEnv->pConsole->GetCVar("sv_maxplayers")->GetIVal();
        carrierDesc.m_recvPacketsLimit = gEnv->pConsole->GetCVar("gm_recvPacketsLimit")->GetIVal();

        GridMate::GridSession* session = nullptr;
        EBUS_EVENT_ID_RESULT(session,gridMate,GridMate::LANSessionServiceBus,HostSession,sp,carrierDesc);

        if (session)
        {
            EBUS_EVENT(Multiplayer::MultiplayerRequestBus,RegisterSession,session);
        }
    }

    //------------------------------------------------------------------------
    void MultiplayerCVars::MPJoinLANCmd(IConsoleCmdArgs* args)
    {
        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();
        if (!gridMate)
        {
            CryLogAlways("GridMate has not been initialized.");
            return;
        }

        GridMate::GridSession* gridSession = nullptr;
        EBUS_EVENT_RESULT(gridSession,Multiplayer::MultiplayerRequestBus,GetSession);

        if (gridSession)
        {
            CryLogAlways("You're already part of a session. Use 'mpdisconnect' first.");
            return;
        }

        if (GridMate::LANSessionServiceBus::FindFirstHandler(gridMate) == nullptr)
        {
            Multiplayer::LAN::StartSessionService(gridMate);
        }

        // Parse optional arguments
        if (args->GetArgCount() > 1)
        {
            gEnv->pConsole->GetCVar("cl_serveraddr")->Set(args->GetArg(1));

            // check if a port was provided
            if (args->GetArgCount()>2)
            {
                gEnv->pConsole->GetCVar("cl_serverport")->Set(args->GetArg(2));
            }
        }

        const char* serveraddr = gEnv->pConsole->GetCVar("cl_serveraddr")->GetString();
        // LANSession doesn't support names. At least handle localhost here.
        if (!serveraddr || 0 == azstricmp("localhost", serveraddr))
        {
            serveraddr = "127.0.0.1";
        }

        const bool autoJoin = (nullptr != CryStringUtils::stristr(args->GetArg(0), "join"));

        CryLogAlways("Attempting to '%s' server with search key \"%s\"...",
            args->GetArg(0), serveraddr);

        // Attempt to join the session at the specified address. Should we succeed, we're
        // now a client and in multiplayer mode.
        s_instance->BusConnect(gridMate);
        s_instance->m_autoJoin = autoJoin;

        GridMate::LANSearchParams searchParams;
        searchParams.m_serverAddress = serveraddr;
        searchParams.m_serverPort = gEnv->pConsole->GetCVar("cl_serverport")->GetIVal() + 1;
        searchParams.m_version = gEnv->pConsole->GetCVar("gm_version")->GetIVal();
        searchParams.m_listenPort = 0; // Always use ephemeral port for searches for the time being, until we change the API to allow users to customize this.

        s_instance->m_search = nullptr;

        EBUS_EVENT_ID_RESULT(s_instance->m_search,gridMate,GridMate::LANSessionServiceBus,StartGridSearch,searchParams);
    }

#if defined(BUILD_GAMELIFT_CLIENT)
    //------------------------------------------------------------------------
    void MultiplayerCVars::MPHostGameLiftCmd(IConsoleCmdArgs* args)
    {
        if (args->GetArgCount() != 4)
        {
            AZ_TracePrintf("MultiplayerModule", "gamelift_host: Invalid number of arguments.");
            return;
        }

        const char* serverName = args->GetArg(1);
        const char* mapName = args->GetArg(2);

        AZ::u32 maxPlayers = strtoul(args->GetArg(3), nullptr, 0);
        if (maxPlayers == 0 || maxPlayers == std::numeric_limits<AZ::u32>::max())
        {
            AZ_TracePrintf("MultiplayerModule", "Invalid value for maxPlayers");
            return;
        }

        MultiplayerGameLiftClientBus::Broadcast(
            &MultiplayerGameLiftClientBus::Events::HostGameLiftSession, serverName, mapName, maxPlayers);
    }

    //------------------------------------------------------------------------
    void MultiplayerCVars::MPJoinGameLiftCmd(IConsoleCmdArgs* args)
    {
        AZ_UNUSED(args);
        MultiplayerGameLiftClientBus::Broadcast(
            &MultiplayerGameLiftClientBus::Events::JoinGameLiftSession);
    }

    //------------------------------------------------------------------------
    void MultiplayerCVars::StopGameLiftClient(IConsoleCmdArgs *args)
    {
        AZ_UNUSED(args);
        MultiplayerGameLiftClientBus::Broadcast(
            &MultiplayerGameLiftClientBus::Events::StopGameLiftClientService);
    }

    //------------------------------------------------------------------------
    void MultiplayerCVars::MPMatchmakingGameLiftCmd(IConsoleCmdArgs *args)
    {
        if (args->GetArgCount() != 2)
        {
            AZ_TracePrintf("MultiplayerModule", "gamelift_flexmatch: Invalid number of arguments. Expected gamelift_flexmatch <configName>");
            return;
        }

        const char* configName = args->GetArg(1);        

        MultiplayerGameLiftClientBus::Broadcast(
            &MultiplayerGameLiftClientBus::Events::StartGameLiftMatchmaking, configName);
    }
#endif

    //------------------------------------------------------------------------
    void MultiplayerCVars::MPDisconnectCmd(IConsoleCmdArgs* args)
    {
        (void)args;

        GridMate::GridSession* gridSession = nullptr;
        EBUS_EVENT_RESULT(gridSession,Multiplayer::MultiplayerRequestBus,GetSession);

        if (!gridSession)
        {
            CryLogAlways("You're not in any MP session.");
            return;
        }

        gridSession->Leave(false);
    }

    //------------------------------------------------------------------------
    void MultiplayerCVars::OnGridSearchComplete(GridMate::GridSearch* search)
    {
        if (search == m_search)
        {
            m_search = nullptr;

            GridMate::SessionEventBus::Handler::BusDisconnect();

            if (m_autoJoin)
            {
                m_autoJoin = false;

                if (search->GetNumResults() > 0)
                {
                    const GridMate::SearchInfo* searchInfo = search->GetResult(0);

                    GridMate::GridSession* session = nullptr;

                    GridMate::CarrierDesc carrierDesc;

                    Multiplayer::Utils::InitCarrierDesc(carrierDesc);
                    Multiplayer::NetSec::ConfigureCarrierDescForJoin(carrierDesc);

                    GridMate::JoinParams joinParams;

                    const GridMate::LANSearchInfo& lanSearchInfo = static_cast<const GridMate::LANSearchInfo&>((*searchInfo));
                    EBUS_EVENT_ID_RESULT(session,gEnv->pNetwork->GetGridMate(),GridMate::LANSessionServiceBus,JoinSessionBySearchInfo,lanSearchInfo,joinParams,carrierDesc);

                    if (session != nullptr)
                    {
                        EBUS_EVENT(Multiplayer::MultiplayerRequestBus,RegisterSession,session);
                        CryLogAlways("Successfully joined game session.");
                    }
                    else
                    {
                        CryLogAlways("Found a game session, but failed to join.");
                    }
                }
                else
                {
                    CryLogAlways("No game sessions found.");
                }
            }
        }
    }

} // namespace Multiplayer
