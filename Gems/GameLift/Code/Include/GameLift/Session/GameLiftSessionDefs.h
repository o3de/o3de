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

#include <GridMate/Session/Session.h>

#if defined(BUILD_GAMELIFT_SERVER)

#include <aws/gamelift/server/GameLiftServerAPI.h>

namespace GridMate
{
    /*!
    * GameLift specific session parameters placeholder used for hosting the session
    */
    struct GameLiftSessionParams
        : public SessionParams
    {
        GameLiftSessionParams()
            : m_gameSession(nullptr)
        {
        }

        const Aws::GameLift::Server::Model::GameSession* m_gameSession; ///< Pointer to GameSession object returned by GameLift session service
    };
}
#endif // BUILD_GAMELIFT_SERVER

#if defined(BUILD_GAMELIFT_CLIENT)

namespace GridMate
{
    /*!
    * Session creation request parameters
    */
    struct GameLiftSessionRequestParams
        : public SessionParams
    {
        string m_fleetId; // GameLift fleet Id
        string m_aliasId; // GameLift fleet alias
        string m_queueName; // GameLift queue name
        string m_instanceName; //! Name of the game instance. Title players will see when browsing for instances.
        bool m_useFleetId;
    };

    /*!
    * Info returned from a GameLift search, required for joining the instance.
    */
    struct GameLiftSearchInfo
        : public SearchInfo
    {
        string m_fleetId; //< GameLift fleet Id
        string m_queueName; // GameLift queue name
        string m_ipAddress; // GameLift game session ip address
        string m_playerSessionId; // GameLift player session id
    };

    /*!
    * GameLift specific search parameters placeholder
    */
    struct GameLiftSearchParams
        : public SearchParams
    {
        string m_fleetId; // GameLift fleet Id
        string m_aliasId; // GameLift fleet alias
        string m_queueName; // GameLift queue name
        bool m_useFleetId;
        string m_gameInstanceId; //< if not empty, only specific session with the given instance id will be returned
    };

} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT

