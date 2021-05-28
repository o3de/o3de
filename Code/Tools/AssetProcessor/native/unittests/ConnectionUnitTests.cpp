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

#include "ConnectionUnitTests.h"

void ConnectionUnitTest::StartTest()
{
    m_testConnection.SetAssetPlatformsString("pc");
    AzFramework::AssetSystem::AssetNotificationMessage testMessage;
    EXPECT_CALL(m_testConnection, Send(testing::_, testing::_)).Times(0);
    m_testConnection.SendPerPlatform(0, testMessage, "mac");
    EXPECT_CALL(m_testConnection, Send(testing::_, testing::_)).Times(1);
    m_testConnection.SendPerPlatform(0, testMessage, "pc");
    m_testConnection.SetAssetPlatformsString("pc,android");
    EXPECT_CALL(m_testConnection, Send(testing::_, testing::_)).Times(1);
    m_testConnection.SendPerPlatform(0, testMessage, "pc");
    EXPECT_CALL(m_testConnection, Send(testing::_, testing::_)).Times(0);
    m_testConnection.SendPerPlatform(0, testMessage, "mac");
    EXPECT_CALL(m_testConnection, Send(testing::_, testing::_)).Times(1);
    m_testConnection.SendPerPlatform(0, testMessage, "android");
    EXPECT_CALL(m_testConnection, Send(testing::_, testing::_)).Times(0);
    // Intended partial string match test - shouldn't send
    m_testConnection.SendPerPlatform(0, testMessage, "es");

    Q_EMIT UnitTestPassed();
}

int ConnectionUnitTest::UnitTestPriority() const
{
    return -10;
}

ConnectionUnitTest::~ConnectionUnitTest()
{

}

// REGISTER_UNIT_TEST(ConnectionUnitTest) // disabling due to intermittent test failure - LYN-3368
