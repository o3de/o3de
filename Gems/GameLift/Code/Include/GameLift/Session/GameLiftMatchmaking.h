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

#include <GameLift/Session/GameLiftSearch.h>

namespace GridMate
{
    class GameLiftClientService;


    /*
    * Places a matchmaking request using a config name. Waits for matchmaker to create a new game session or find existing game sessions
    * based on auto fill mode set or error after timeout.
    */
    class GameLiftMatchmaking
        : public GameLiftSearch
    {
        friend class GameLiftClientService;

    public:

        GM_CLASS_ALLOCATOR(GameLiftMatchmaking);

        // GameLiftRequestInterface
        bool Initialize() override;

    protected:
        GameLiftMatchmaking(GameLiftClientService* service, const AZStd::shared_ptr<GameLiftRequestInterfaceContext> context, const Aws::String& matchmakingConfigName);
        // GridSearch interface
        void Update() override;

        // Call when request is finished.
        virtual void SearchDone();

        Aws::GameLift::Model::StartMatchmakingOutcomeCallable m_startMatchmakingOutcomeCallable;
        Aws::GameLift::Model::DescribeMatchmakingOutcomeCallable m_describeMatchmakingOutcomeCallable;
        Aws::GameLift::Model::MatchmakingTicket m_matchmakingTicket;
        Aws::GameLift::Model::GameSessionConnectionInfo m_gameSessionConnectionInfo;

        Aws::String m_matchmakingConfigName;

        float m_pollDescribeMatchmakingDelay = 2.0F; // 2 seconds delay between describe matchmaking calls to avoid throttling

    private:
        void WaitForStartMatchmakingResult();
        void PollWithDelayDescribeMatchmaking();
        void WaitForDescribeMatchmakingResult();
        TimeStamp m_startDescribeMatchmakingTime;
    };
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
