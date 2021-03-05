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

#include <GameLift/Session/GameLiftServerServiceEventsBus.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace Multiplayer
{
#if defined(BUILD_GAMELIFT_SERVER)
    /**
    * GameLiftListener
    * pImpl implementation to listen for GameLift specific events. Will start hosting session once GameLift is ready.
    * Will trigger application shutdown when it is triggered by GameLift
    */
    class GameLiftListener
        : public GridMate::GameLiftServerServiceEventsBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(GameLiftListener, AZ::SystemAllocator, 0)

        GameLiftListener();
        ~GameLiftListener();

    private:
        void OnGameLiftGameSessionStarted(GridMate::GameLiftServerService* service, const Aws::GameLift::Server::Model::GameSession& gameSession) override;
        void OnGameLiftGameSessionUpdated(GridMate::GameLiftServerService* service, const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession) override;
        void OnGameLiftServerWillTerminate(GridMate::GameLiftServerService* service) override;
    };
#endif
}
