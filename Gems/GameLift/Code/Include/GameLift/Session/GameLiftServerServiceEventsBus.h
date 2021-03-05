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

#if defined(BUILD_GAMELIFT_SERVER)

#include <AzCore/EBus/EBus.h>

#include <aws/gamelift/server/GameLiftServerAPI.h>

namespace GridMate
{
    class IGridMate;
    class GameLiftServerService;

    class GameLiftServerServiceEvents
        : public AZ::EBusTraits
    {
    public:
        typedef IGridMate* BusIdType;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        virtual ~GameLiftServerServiceEvents() {}

        /*!
        * Called when new GameLift session is initiated on a local , you can start hosting new session at this point
        */
        virtual void OnGameLiftGameSessionStarted(GameLiftServerService*, const Aws::GameLift::Server::Model::GameSession&) {}

        /*!
        * Called when GameLift session is updated with matchmaker event
        */
        virtual void OnGameLiftGameSessionUpdated(GameLiftServerService*, const Aws::GameLift::Server::Model::UpdateGameSession&) {}

        /*!
        * Called when GameLift server forced to terminate (via web dashboard or other admin tools)
        * After this is called - there will be a grace period for the server to clean up, save state and quit gracefully. Then ec2 instance will be torn down.
        */
        virtual void OnGameLiftServerWillTerminate(GameLiftServerService*) {}

        /*!
        * Called when GameLift service is ready to use
        */
        virtual void OnGameLiftSessionServiceReady(GameLiftServerService*) {}

        /*!
        * Called when GameLift service is failed to initialize
        */
        virtual void OnGameLiftSessionServiceFailed(GameLiftServerService*) {}
    };
    typedef AZ::EBus<GameLiftServerServiceEvents> GameLiftServerServiceEventsBus;
} // namespace GridMate

#endif // BUILD_GAMELIFT_SERVER
