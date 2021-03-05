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

#include <GridMate/Session/Session.h>
#include <GameLift/Session/GameLiftSessionDefs.h>
#include <GameLift/Session/GameLiftRequestInterface.h>

namespace GridMate
{
    class GameLiftClientService;

    /*!
    * GameLift search object, created by session service when search is performed.
    * So far returns list of available instances for a given fleet or fleet associated with given queue name.
    */
    class GameLiftSearch
        : public GridSearch
        , public GameLiftRequestInterface
    {
        friend class GameLiftClientService;

    public:

        GM_CLASS_ALLOCATOR(GameLiftSearch);

        // GridSearch interface
        unsigned int GetNumResults() const override;
        const SearchInfo* GetResult(unsigned int index) const override;
        void AbortSearch() override;

    protected:
        // GridSearch interface
        bool Initialize() override;
        void Update() override;
        GameLiftSearch(GameLiftClientService* service, const AZStd::shared_ptr<GameLiftRequestInterfaceContext> context);

        void ProcessGameSessionResult(const Aws::GameLift::Model::GameSession& gameSession);

        // Call when request is finished.
        virtual void SearchDone();

        void StartDescribeGameSessionQueue();
        void WaitDescribeGameSessionQueue();
        void StartSearchGameSession();
        void WaitForSearchGameSession();

        Aws::GameLift::Model::SearchGameSessionsOutcomeCallable m_searchGameSessionsOutcomeCallable;
        Aws::GameLift::Model::DescribeGameSessionQueuesOutcomeCallable m_describeGameSessionQueueOutcomeCallable;

        vector<GameLiftSearchInfo> m_results;

    };
} // namespace GridMate

#endif // BUILD_GAMELIFT_CLIENT
