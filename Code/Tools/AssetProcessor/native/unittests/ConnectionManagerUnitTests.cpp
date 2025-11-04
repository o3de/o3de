/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/unittests/AssetProcessorUnitTests.h>

#include <native/connection/connectionManager.h>
#include <native/connection/connection.h>

#include <QCoreApplication>

const QString s_connectionSettingsPrefix = QStringLiteral("AssetProcessorUnitTests_");

class ConnectionManagerUnitTest
    : public UnitTest::AssetProcessorUnitTestBase
{
public:

    virtual void SetUp() override;
    virtual void TearDown() override;

protected:
    void UpdateConnectionManager();
    ConnectionManager* m_connectionManager;
};

void ConnectionManagerUnitTest::SetUp()
{
    UnitTest::AssetProcessorUnitTestBase::SetUp();

    m_connectionManager = ConnectionManager::Get();
}

void ConnectionManagerUnitTest::TearDown()
{
    //removing all connections
    for (auto iter = m_connectionManager->getConnectionMap().begin(); iter != m_connectionManager->getConnectionMap().end(); iter++)
    {
        iter.value()->Terminate();
    }

    // Process all the pending events
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    UnitTest::AssetProcessorUnitTestBase::TearDown();
}

TEST_F(ConnectionManagerUnitTest, AddAndSaveConnections_FeedUserConnections_Success)
{
    UpdateConnectionManager();

    int count = m_connectionManager->getCount();
    EXPECT_EQ(count, 4) << "Count is Invalid";
    m_connectionManager->SaveConnections(s_connectionSettingsPrefix);
}

TEST_F(ConnectionManagerUnitTest, LoadConnection_FeedConnectionSettingsPrefix_Success)
{
    int count = m_connectionManager->getCount();
    EXPECT_EQ(count, 0) << "Count is Invalid";
    m_connectionManager->LoadConnections(s_connectionSettingsPrefix);
    count = m_connectionManager->getCount();
    EXPECT_EQ(count, 4) << "Count is Invalid";
}

TEST_F(ConnectionManagerUnitTest, GetConnection_FeedConnectionId_Success)
{
    m_connectionManager->LoadConnections(s_connectionSettingsPrefix);
    unsigned int connId = m_connectionManager->GetConnectionId("127.0.0.2", 12345);
    EXPECT_NE(connId, 0) << "Connection is not present ,which is Invalid";
    Connection* testConnection = m_connectionManager->getConnection(connId);
    EXPECT_EQ(testConnection->Identifier().compare("PC Game", Qt::CaseSensitive), 0) << "Identifier is Invalid";
    EXPECT_EQ(testConnection->IpAddress().compare("127.0.0.2", Qt::CaseInsensitive), 0) << "IpAddress is Invalid";
    EXPECT_EQ(testConnection->Port(), 12345) << "Port is Invalid";
    EXPECT_EQ(testConnection->Status(), Connection::Disconnected) << "Status is Invalid";
    EXPECT_FALSE(testConnection->AutoConnect()) << "AutoConnect status is Invalid";
}

TEST_F(ConnectionManagerUnitTest, RemoveConnection_FeedConnectionId_Success)
{
    // Add a new connection to remove
    unsigned int connId = m_connectionManager->addUserConnection();
    int count = m_connectionManager->getCount();
    EXPECT_EQ(count, 1) << "Count is Invalid";
    EXPECT_NE(connId, 0) << "Index of the connection is Invalid";
    Connection* testConnection = m_connectionManager->getConnection(connId);
    testConnection->SetIdentifier("PC Game");
    testConnection->SetIpAddress("98.45.67.89");
    testConnection->SetPort(22234);
    testConnection->SetStatus(Connection::Connecting);
    testConnection->SetAutoConnect(true);

    EXPECT_EQ(testConnection->Identifier().compare("PC Game", Qt::CaseSensitive), 0) << "Identifier is Invalid";
    EXPECT_EQ(testConnection->IpAddress().compare("98.45.67.89", Qt::CaseInsensitive), 0) << "IpAddress is Invalid";
    EXPECT_EQ(testConnection->Port(), 22234) << "Port is Invalid";
    EXPECT_EQ(testConnection->Status(), Connection::Connecting) << "Status is Invalid";
    EXPECT_TRUE(testConnection->AutoConnect()) << "Autoconnect status is Invalid";
    connId = m_connectionManager->GetConnectionId("98.45.67.89", 22234);
    EXPECT_NE(connId, 0) << "Connection is not present ,which is Invalid";

    m_connectionManager->removeConnection(connId);
    // Process all the pending events
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    count = m_connectionManager->getCount();
    EXPECT_EQ(count, 0) << "Count is Invalid";

    connId = m_connectionManager->GetConnectionId("98.45.67.89", 22234);
    EXPECT_EQ(connId, 0) << "Connection is present ,which is Invalid";
}


void ConnectionManagerUnitTest::UpdateConnectionManager()
{
    unsigned int connId1 = m_connectionManager->addUserConnection();
    unsigned int connId2 = m_connectionManager->addUserConnection();
    unsigned int connId3 = m_connectionManager->addUserConnection();
    unsigned int connId4 = m_connectionManager->addUserConnection();
    Connection* c1 = m_connectionManager->getConnection(connId1);
    Connection* c2 = m_connectionManager->getConnection(connId2);
    Connection* c3 = m_connectionManager->getConnection(connId3);
    Connection* c4 = m_connectionManager->getConnection(connId4);
    c1->SetIdentifier("Android Game");
    c1->SetStatus(Connection::Disconnected);
    c1->SetIpAddress("127.0.0.1");
    c1->SetPort(12345);
    c1->SetAutoConnect(false);

    c2->SetIdentifier("PC Game");
    c2->SetStatus(Connection::Disconnected);
    c2->SetIpAddress("127.0.0.2");
    c2->SetPort(12345);
    c2->SetAutoConnect(false);

    c3->SetIdentifier("Mac Game");
    c3->SetStatus(Connection::Disconnected);
    c3->SetIpAddress("127.0.0.3");
    c3->SetPort(12345);
    c3->SetAutoConnect(false);

    c4->SetIdentifier("Android Game");
    c4->SetStatus(Connection::Disconnected);
    c4->SetIpAddress("127.0.0.4");
    c4->SetPort(12345);
    c4->SetAutoConnect(false);
}
