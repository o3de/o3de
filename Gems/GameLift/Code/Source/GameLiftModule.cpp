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
#include "GameLiftModule.h"
#include <GridMate/NetworkGridMate.h>
#include <GameLift/Session/GameLiftClientSession.h>
#include <GameLift/Session/GameLiftClientService.h>
#include <GameLift/Session/GameLiftServerSession.h>
#include <GameLift/Session/GameLiftServerService.h>

#if defined(BUILD_GAMELIFT_CLIENT)
#include <AWSNativeSDKInit/AWSNativeSDKInit.h>
#endif

namespace GameLift
{
    /**
    * GameLiftModule implementation
    */

    void GameLiftModule::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
    {
        CryHooksModule::OnCrySystemInitialized(system, systemInitParams);
#if defined(BUILD_GAMELIFT_CLIENT)
        AWSNativeSDKInit::InitializationManager::InitAwsApi();
#endif
        // Can't interact with GridMate until CrySystem is fully initialized.
        GameLiftRequestBus::Handler::BusConnect();
    }

    void GameLiftModule::OnCrySystemShutdown(ISystem& system)
    {
        // Need to shut down before GridMate.
        GameLiftRequestBus::Handler::BusDisconnect();


#if defined(BUILD_GAMELIFT_CLIENT)
        StopClientService();
#endif

#if defined(BUILD_GAMELIFT_SERVER)
        StopServerService();
#endif

#if defined(BUILD_GAMELIFT_CLIENT)
        AWSNativeSDKInit::InitializationManager::Shutdown();
#endif

        CryHooksModule::OnCrySystemShutdown(system);
    }

    bool GameLiftModule::IsGameLiftServer() const
    {
#if defined(BUILD_GAMELIFT_SERVER)
        return m_serverService != nullptr;
#else
        return false;
#endif
    }

    void GameLiftModule::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_GAME_POST_INIT:
            break;

        default:
            (void)event;
        }
    }

    GameLiftModule::GameLiftModule()
    {
#if defined(BUILD_GAMELIFT_CLIENT)
        m_clientService = nullptr;
#endif

#if defined(BUILD_GAMELIFT_SERVER)
        m_serverService = nullptr;
#endif
    }

#if defined(BUILD_GAMELIFT_CLIENT)
    GridMate::GameLiftClientService* GameLiftModule::StartClientService(const GridMate::GameLiftClientServiceDesc& desc)
    {
        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();
        AZ_Assert(gridMate, "No gridMate instance");

        // Reconnection logic requires stopping prior instance
        if (m_clientService)
        {
            StopClientService();
        }

        m_clientService = GridMate::StartGridMateService<GridMate::GameLiftClientService>(gridMate, desc);
        if (!m_clientService)
        {
            CryLog("Failed to start GameLift client service.");
        }

        return m_clientService;
    }

    void GameLiftModule::StopClientService()
    {
        if (gEnv->pNetwork)
        {
            GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();
            AZ_Assert(gridMate, "No gridMate instance");

            if (m_clientService)
            {
                GridMate::StopGridMateService<GridMate::GameLiftClientService>(gridMate);
                m_clientService = nullptr;
            }
        }
    }

    GridMate::GameLiftClientService* GameLiftModule::GetClientService()
    {
        return m_clientService;
    }
#endif // BUILD_GAMELIFT_CLIENT

#if defined(BUILD_GAMELIFT_SERVER)
    GridMate::GameLiftServerService* GameLiftModule::StartServerService(const GridMate::GameLiftServerServiceDesc& desc)
    {
        GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();
        AZ_Assert(gridMate, "No gridMate instance");

        if (!m_serverService)
        {
            m_serverService = GridMate::StartGridMateService<GridMate::GameLiftServerService>(gridMate, desc);
            if (!m_serverService)
            {
                CryLog("Failed to start GameLift server service.");
            }
        }
        else
        {
            CryLog("GameLift Service is already started.");
        }

        return m_serverService;
    }

    void GameLiftModule::StopServerService()
    {
        if (gEnv->pNetwork)
        {
            GridMate::IGridMate* gridMate = gEnv->pNetwork->GetGridMate();
            AZ_Assert(gridMate, "No gridMate instance");

            if (m_serverService)
            {
                GridMate::StopGridMateService<GridMate::GameLiftServerService>(gridMate);
                m_serverService = nullptr;
            }
        }
    }

    GridMate::GameLiftServerService* GameLiftModule::GetServerService()
    {
        return m_serverService;
    }
#endif // BUILD_GAMELIFT_SERVER
} // namespace GameLift

AZ_DECLARE_MODULE_CLASS(Gem_GameLift, GameLift::GameLiftModule)
