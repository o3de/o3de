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
    * Directly places a GameSession on a GameLift fleet via CreateGameSession using the given parameters.
    * When request is completed - ordinary session search ebus events are issued.
    */
    class GameLiftSessionRequest
        : public GameLiftSearch
    {
        friend class GameLiftClientService;

    public:
        GM_CLASS_ALLOCATOR(GameLiftSessionRequest);
        bool Initialize() override;
        void AbortSearch() override;

    protected:
        GameLiftSessionRequest(GameLiftClientService* service, const AZStd::shared_ptr<GameLiftRequestInterfaceContext> context);
        void Update() override;

    private:
        Aws::GameLift::Model::CreateGameSessionOutcomeCallable m_createGameSessionOutcomeCallable;
    };
} // namespace GridMate

#endif
