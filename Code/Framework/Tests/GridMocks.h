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

#include <AzTest/AzTest.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <GridMate/Session/Session.h>

namespace UnitTest
{
    class MockSession
        : public GridMate::GridSession
    {
    public:
        MockSession(GridMate::SessionService* service)
            : GridSession(service)
        {
        }

        void SetReplicaManager(GridMate::ReplicaManager* replicaManager)
        {
            m_replicaMgr = replicaManager;
        }

        MOCK_METHOD4(CreateRemoteMember, GridMate::GridMember*(const GridMate::string&, GridMate::ReadBuffer&, GridMate::RemotePeerMode, GridMate::ConnectionID));
        MOCK_METHOD1(OnSessionParamChanged, void(const GridMate::GridSessionParam&));
        MOCK_METHOD1(OnSessionParamRemoved, void(const GridMate::string&));
    };

    class MockSessionService
        : public GridMate::SessionService
    {
    public:
        MockSessionService()
            : SessionService(GridMate::SessionServiceDesc())
        {
        }

        ~MockSessionService()
        {
            m_activeSearches.clear();
            m_gridMate = nullptr;
        }

        MOCK_CONST_METHOD0(IsReady, bool());
    };
}
