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

#include <GameLift/Session/GameLiftClientService.h>
#include <aws/gamelift/GameLiftClient.h>

namespace GridMate
{
    /*
    * Context object to hold request information
    */
    struct GameLiftRequestInterfaceContext
    {
        GameLiftSessionRequestParams m_requestParams;
        GameLiftSearchParams m_searchParams;
        AZStd::weak_ptr<Aws::GameLift::GameLiftClient> m_gameLiftClient;
        AZStd::string m_playerId;
    };

    /* Interface to use to implement request classes for GameLift.
    */
    class GameLiftRequestInterface
    {
    public:
        // Initializes GameLift requests and starts the call chain.
        virtual bool Initialize() = 0;

    protected:
        AZStd::shared_ptr<GameLiftRequestInterfaceContext> m_context;

        GameLiftRequestInterface(const AZStd::shared_ptr<GameLiftRequestInterfaceContext> context)
        {
            this->m_context = context;
        }

        virtual ~GameLiftRequestInterface()
        {
            m_context.reset();
        }
    };
} // namespace GridMate

#endif
