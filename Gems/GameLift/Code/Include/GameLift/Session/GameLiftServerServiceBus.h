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

#include <GridMate/Session/SessionServiceBus.h>

#include <aws/gamelift/server/GameLiftServerAPI.h>
#include <aws/gamelift/server/model/UpdateGameSession.h>

namespace GridMate
{
    class GameLiftServerSession;
    struct GameLiftSessionParams;

    /*
    * GameLift server service interface.
    */
    class GameLiftServerServiceInterface : public SessionServiceBusTraits
    {
    public:
        // Start hosting GameLift session, can only be called from GameLift ec2 instance
        virtual GridSession* HostSession(const GameLiftSessionParams& params, const CarrierDesc& carrierDesc) = 0;

        // Stop hosted GameLift game sessison.
        virtual void ShutdownSession(const GridSession* gridSession) = 0;

        // Retrieves GameLift specific session from base session, or nullptr if given generic session object is not GameLift session
        virtual GameLiftServerSession* QueryGameLiftSession(const GridSession* session) = 0;

        // Starts a matchmaking backfill request with player data from player sessions.
        virtual bool StartMatchmakingBackfill(const GridSession* gameSession, AZStd::string& matchmakingTicketId, bool checkForAutoBackfill) = 0;

        // Stops a matchmaking backfill request in flight.
        virtual bool StopMatchmakingBackfill(const GridSession* gameSession, const AZStd::string& matchmakingTicketId) = 0;
    };
    typedef AZ::EBus<GameLiftServerServiceInterface> GameLiftServerServiceBus;

    namespace Internal
    {
        /*!
        * Internal bus for server service events
        */
        class GameLiftServerSystemEvents
            : public AZ::EBusTraits
        {
        public:
            typedef AZStd::recursive_mutex MutexType;
            typedef AZStd::recursive_mutex EventQueueMutexType;
            static const bool EnableEventQueue = true;
            static const bool EnableQueuedReferences = true;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            virtual ~GameLiftServerSystemEvents() {}

            virtual void OnGameLiftGameSessionStarted(const Aws::GameLift::Server::Model::GameSession&) {}
            virtual void OnGameLiftServerWillTerminate() {}

            // Called when backfilling players via matchmaking 
            virtual void OnGameLiftGameSessionUpdated([[maybe_unused]] const Aws::GameLift::Server::Model::UpdateGameSession& updateGameSession) {}
        };

        typedef AZ::EBus<GameLiftServerSystemEvents> GameLiftServerSystemEventsBus;
    }
} // namespace GridMate

#endif // BUILD_GAMELIFT_SERVER

