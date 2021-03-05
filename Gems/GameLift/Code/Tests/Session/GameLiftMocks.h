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
#include <gmock/gmock.h>
#include <GridMate/Session/Session.h>

namespace UnitTest
{
    template <typename T>
    const std::future<T> GetFuture(const T& outcome)
    {
        std::promise<T> outcomePromise;
        outcomePromise.set_value(outcome);
        return outcomePromise.get_future();
    }

    class SessionEventBusMock : public GridMate::SessionEventBus::Handler
    {
    public:
        SessionEventBusMock(GridMate::IGridMate* gridMate) : m_gridMate(gridMate)
        {
            GridMate::SessionEventBus::Handler::BusConnect(m_gridMate);
        }

        ~SessionEventBusMock()
        {
            GridMate::SessionEventBus::Handler::BusDisconnect(m_gridMate);
            m_gridMate = nullptr;
        }

        MOCK_METHOD1(OnGridSearchStart, void(GridMate::GridSearch* gridSearch));
        MOCK_METHOD1(OnGridSearchComplete, void(GridMate::GridSearch* gridSearch));
        MOCK_METHOD1(OnGridSearchRelease, void(GridMate::GridSearch* gridSearch));
        MOCK_METHOD1(OnSessionCreated, void(GridMate::GridSession* session));
        MOCK_METHOD1(OnSessionHosted, void(GridMate::GridSession* session));
        MOCK_METHOD1(OnSessionJoined, void(GridMate::GridSession* session));
        MOCK_METHOD0(OnSessionServiceReady, void());
        MOCK_METHOD2(OnMemberJoined, void(GridMate::GridSession* session, GridMate::GridMember* member));
        MOCK_METHOD2(OnMemberLeaving, void(GridMate::GridSession* session, GridMate::GridMember* member));
        MOCK_METHOD1(OnSessionDelete, void(GridMate::GridSession* session));

    private:
        GridMate::IGridMate* m_gridMate;
    };
};
