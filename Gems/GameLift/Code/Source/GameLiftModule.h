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
#pragma once

#include "GameLift/GameLiftBus.h"
#include <IGem.h>

namespace GameLift
{
    class GameLiftModule
        : public CryHooksModule
        , public GameLiftRequestBus::Handler
    {
    public:
        AZ_RTTI(GameLiftModule, "{6C3B90F6-93EB-4BE8-9B7F-E4CD94E4B93C}", CryHooksModule);

        GameLiftModule();
        ~GameLiftModule() override = default;

    private:
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

        bool IsGameLiftServer() const override;

#if defined(BUILD_GAMELIFT_CLIENT)
        GridMate::GameLiftClientService* StartClientService(const GridMate::GameLiftClientServiceDesc& desc) override;
        void StopClientService() override;
        GridMate::GameLiftClientService* GetClientService() override;

        GridMate::GameLiftClientService* m_clientService;
#endif

#if defined(BUILD_GAMELIFT_SERVER)
        GridMate::GameLiftServerService* StartServerService(const GridMate::GameLiftServerServiceDesc& desc) override;
        void StopServerService() override;
        GridMate::GameLiftServerService* GetServerService() override;

        GridMate::GameLiftServerService* m_serverService;
#endif
    };
} // namespace GameLift
