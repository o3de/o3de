/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/unittests/AssetProcessorUnitTests.h>

class ConnectionForSendTest : public Connection
{
public:
    ConnectionForSendTest() : Connection() {}
    ~ConnectionForSendTest() = default;

    ConnectionForSendTest(ConnectionForSendTest&) = delete;
    ConnectionForSendTest(ConnectionForSendTest&&) = delete;
    MOCK_METHOD2(Send, size_t(unsigned int /*serial*/, const AzFramework::AssetSystem::BaseAssetProcessorMessage& /*message*/));
};

class ConnectionUnitTest
    : public UnitTest::AssetProcessorUnitTestBase
{
protected:
    ::testing::NiceMock<ConnectionForSendTest> m_testConnection;
};

TEST_F(ConnectionUnitTest, SendPerPlatform_SendMessage_Succeeds)
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
}
