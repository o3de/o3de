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
#include <GameLift/Session/GameLiftSessionDefs.h>

namespace GridMate
{
    class GameLiftClientService;

    /*
    * Requests a game session to be created on a GameLift queue (using queue name) via StartGameSessionPlacement API
    * On Successful placement, describes the created game session and adds it to search results to use by join session.
    */
    class GameLiftGameSessionPlacementRequest
        : public GameLiftSearch
    {
        friend class GameLiftClientService;

    public:
        GM_CLASS_ALLOCATOR(GameLiftGameSessionPlacementRequest);

        void AbortSearch() override;
        bool Initialize() override;

    protected:
        GameLiftGameSessionPlacementRequest(GameLiftClientService* service, const AZStd::shared_ptr<GameLiftRequestInterfaceContext> context);
        void Update() override;

    private:

        void StartGameSessionPlacement();
        void WaitForGameSessionPlacement();
        void AddGameSessionSearchResult(const Aws::GameLift::Model::GameSession &gameSession);
        const Aws::GameLift::Model::GameSession GetPlacedGameSession();

        // GameLiftSearch overrides
        void SearchDone() override;

        Aws::GameLift::Model::StartGameSessionPlacementOutcomeCallable  m_startGameSessionPlacementOutcomeCallable;
        Aws::GameLift::Model::DescribeGameSessionPlacementOutcomeCallable m_describeGameSessionPlacementCallable;

        Aws::String m_placementId = "";
        Aws::String m_gameSessionId = "";

        enum class GameSessionPlacementState
        {
            Unknown = -1,
            StartPlacement = 0,
            WaitForPlacement = 1,
            AddGameSessionSearchResult = 2
        };
        GameSessionPlacementState m_queueSessionState;
    };
} // namespace GridMate

#endif
