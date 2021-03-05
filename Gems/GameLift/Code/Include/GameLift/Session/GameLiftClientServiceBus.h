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

#if defined(BUILD_GAMELIFT_CLIENT)

#include <GridMate/Session/SessionServiceBus.h>

namespace GridMate
{
    class GameLiftClientSession;
    class GameLiftSearch;
    class GameLiftSessionRequest;
    struct GameLiftSearchInfo;
    struct GameLiftSessionRequestParams;
    struct GameLiftSearchParams;

    /*
    * GameLift session service interface.
    */
    class GameLiftClientServiceInterface : public SessionServiceBusTraits
    {
    public:
        // Joins GameLift session
        virtual GridSession* JoinSessionBySearchInfo(const GameLiftSearchInfo& params, const CarrierDesc& carrierDesc) = 0;

        // Asynchronous request to create and host new session using GameLift ec2 instances, listen for OnGridSearchComplete for completion event
        virtual GridSearch* RequestSession(const GameLiftSessionRequestParams& params) = 0;

        // Asynchronous request to start matchmaking using the passed in matchmaking config, listen for OnGridSearchComplete for completion event
        virtual GridSearch* StartMatchmaking(const AZStd::string& matchmakingConfig) = 0;

        // Asynchronous request to retrieve all gamelift sessions available for given search parameters, listen for OnGridSearchComplete for completion event
        virtual GameLiftSearch* StartSearch(const GameLiftSearchParams& params) = 0;

        // Retrieves GameLift specific session from base session, or nullptr if given generic session object is not GameLift session
        virtual GameLiftClientSession* QueryGameLiftSession(const GridSession* session) = 0;

        // Retrieves GameLift specific search from base search, or nullptr if given generic search object is not GameLift session
        virtual GameLiftSearch* QueryGameLiftSearch(const GridSearch* search) = 0;
    };
    typedef AZ::EBus<GameLiftClientServiceInterface> GameLiftClientServiceBus;
}

#endif
