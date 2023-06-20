/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/AssetManager/AssetRequestHandler.h>

#include <native/unittests/AssetRequestHandlerUnitTests.h>
#include <native/unittests/MockConnectionHandler.h>

#include <QCoreApplication>

using namespace AssetProcessor;
using namespace AzFramework::AssetSystem;
using namespace UnitTestUtils;

namespace AssetProcessor
{
    namespace
    {
        constexpr const char* Platform = "pc";
        constexpr const char* AssetName = "test.dds";
        constexpr const char* FenceFileName = "foo.fence";
        constexpr const NetworkRequestID RequestId = NetworkRequestID(1, 1234);
    }

    //! Internal class to unit test AssetRequestHandler
    class MockAssetRequestHandler
        : public AssetRequestHandler
    {
    public:
        void Reset()
        {
            m_numTimesCreateFenceFileCalled = 0;
            m_numTimesDeleteFenceFileCalled = 0;
            m_requestReadyCount = 0;
            m_fencingFailed = false;
            m_fenceId = 0;
            m_fenceFileName = FenceFileName;
            m_deleteFenceFileResult = true;
        }

        bool InvokeHandler(MessageData<AzFramework::AssetSystem::BaseAssetProcessorMessage> message) override
        {
            m_requestReadyCount++;
            m_fencingFailed = message.m_fencingFailed;

            return AssetRequestHandler::InvokeHandler(message);
        }

        int m_numTimesCreateFenceFileCalled = 0;
        int m_numTimesDeleteFenceFileCalled = 0;
        int  m_requestReadyCount = 0;
        bool m_fencingFailed = false;
        unsigned int m_fenceId = 0;
        QString m_fenceFileName = FenceFileName;
        bool m_deleteFenceFileResult = true;

    protected:
        QString CreateFenceFile(unsigned int fenceId) override
        {
            m_numTimesCreateFenceFileCalled++;
            m_fenceId = fenceId;
            return m_fenceFileName;
        }
        bool DeleteFenceFile(QString fenceFileName) override
        {
            m_numTimesDeleteFenceFileCalled++;
            return m_deleteFenceFileResult;
        }
    };
}

AssetRequestHandlerUnitTests::~AssetRequestHandlerUnitTests()
{
}

void AssetRequestHandlerUnitTests::SetUp()
{
    UnitTest::AssetProcessorUnitTestBase::SetUp();

    m_requestHandler = AZStd::make_unique<MockAssetRequestHandler>();
    m_connection = AZStd::make_unique<MockConnectionHandler>();

    QObject::connect(m_requestHandler.get(), &AssetRequestHandler::RequestCompileGroup, this, [&](NetworkRequestID groupID, QString platform, QString searchTerm)
        {
            m_requestedCompileGroup = true;
            m_platformSet = platform;
            m_requestIdSet = groupID;
            m_searchTermSet = searchTerm;
        });

    QObject::connect(m_requestHandler.get(), &AssetRequestHandler::RequestAssetExists, this, [&](NetworkRequestID groupID, QString platform, QString searchTerm)
        {
            m_requestedAssetExists = true;
            m_platformSet = platform;
            m_requestIdSet = groupID;
            m_searchTermSet = searchTerm;
        });

    m_connection->BusConnect(1);
}

void AssetRequestHandlerUnitTests::TearDown()
{
    m_connection->BusDisconnect(1);

    m_requestHandler.reset();

    UnitTest::AssetProcessorUnitTestBase::TearDown();
}

TEST_F(AssetRequestHandlerUnitTests, RequestToProcessAssetNotExistInDatabaseOrQueue_RequestSent_RequestHandled)
{
    QByteArray buffer;
    AzFramework::AssetSystem::RequestAssetStatus request(AssetName, true, true);
    AssetProcessor::PackMessage(request, buffer);

    m_requestHandler->m_fenceFileName = QString();
    m_requestHandler->m_deleteFenceFileResult = false;
    m_requestHandler->OnNewIncomingRequest(RequestId.first, RequestId.second, buffer, Platform);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    EXPECT_EQ(m_requestHandler->m_requestReadyCount, 1);
    EXPECT_TRUE(m_requestHandler->m_fencingFailed);
    EXPECT_EQ(m_requestHandler->m_numTimesCreateFenceFileCalled, g_RetriesForFenceFile);
    EXPECT_FALSE(m_requestHandler->m_numTimesDeleteFenceFileCalled);

    m_requestHandler->Reset();
    m_requestHandler->m_deleteFenceFileResult = false;
    QMetaObject::invokeMethod(m_requestHandler.get(), "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, RequestId.first), Q_ARG(unsigned int, RequestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, Platform));
    // we will have to call processEvents here equal to the number of retries, because only after that will the requestReady event be added to the event queue  
    while (m_requestHandler->m_numTimesDeleteFenceFileCalled <= g_RetriesForFenceFile && !m_requestHandler->m_requestReadyCount)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }
    EXPECT_EQ(m_requestHandler->m_requestReadyCount, 1);
    EXPECT_TRUE(m_requestHandler->m_fencingFailed);
    EXPECT_EQ(m_requestHandler->m_numTimesCreateFenceFileCalled, 1);
    EXPECT_EQ(m_requestHandler->m_numTimesDeleteFenceFileCalled, g_RetriesForFenceFile);

    m_requestHandler->Reset();
    QMetaObject::invokeMethod(m_requestHandler.get(), "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, RequestId.first), Q_ARG(unsigned int, RequestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, Platform));
    m_requestHandler->OnFenceFileDetected(m_requestHandler->m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    EXPECT_TRUE(m_requestedCompileGroup);
    EXPECT_FALSE(m_requestedAssetExists);
    EXPECT_FALSE(m_connection->m_sent);
    EXPECT_EQ(m_platformSet, Platform);
    EXPECT_EQ(m_requestIdSet, RequestId);
    EXPECT_EQ(m_searchTermSet, AssetName);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 1);
    EXPECT_EQ(m_requestHandler->m_numTimesCreateFenceFileCalled, 1);
    EXPECT_EQ(m_requestHandler->m_numTimesDeleteFenceFileCalled, 1);
    EXPECT_EQ(m_requestHandler->m_requestReadyCount, 1);
    EXPECT_FALSE(m_requestHandler->m_fencingFailed);

    m_requestedCompileGroup = false;
    m_requestedAssetExists = false;
    m_connection->m_sent = false;

    // it worked so far, now synthesize a response:
    // it should result in it asking for asset exists
    m_requestHandler->OnCompileGroupCreated(m_requestIdSet, AssetStatus_Unknown);

    EXPECT_FALSE(m_requestedCompileGroup);
    EXPECT_TRUE(m_requestedAssetExists);
    EXPECT_FALSE(m_connection->m_sent);
    EXPECT_EQ(m_platformSet, Platform);
    EXPECT_EQ(m_requestIdSet, RequestId);
    EXPECT_EQ(m_searchTermSet, AssetName);
    m_requestedCompileGroup = false;
    m_requestedAssetExists = false;
    m_connection->m_sent = false;
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 1); // it should still be alive!

    // it worked so far, now synthesize a response:
    // it should result in it asking for asset exists
    m_requestHandler->OnRequestAssetExistsResponse(m_requestIdSet, false);

    // this should result in it sending:
    EXPECT_FALSE(m_requestedCompileGroup);
    EXPECT_FALSE(m_requestedAssetExists);
    EXPECT_TRUE(m_connection->m_sent);
    EXPECT_EQ(NetworkRequestID(1, m_connection->m_serial), RequestId);
    EXPECT_EQ(m_connection->m_type, RequestAssetStatus::MessageType);
    m_requestedCompileGroup = false;
    m_requestedAssetExists = false;
    m_connection->m_sent = false;
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 0); // it should be gone now

    // decode the buffer.
    ResponseAssetStatus resp;
    EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(m_connection->m_payload.data(), m_connection->m_payload.size(), resp));
    EXPECT_EQ(resp.m_assetStatus, AssetStatus_Missing);
}

TEST_F(AssetRequestHandlerUnitTests, RequestToCreateCompileGroup_RequestSent_RequestHandled)
{
    // Test creating a request for a real compile group.
    // We will mock the response as saying 'yes, I made a compile group':
    QByteArray buffer;
    AzFramework::AssetSystem::RequestAssetStatus request(AssetName, true, true);
    AssetProcessor::PackMessage(request, buffer);

    QMetaObject::invokeMethod(m_requestHandler.get(), "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, RequestId.first), Q_ARG(unsigned int, RequestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, Platform));
    m_requestHandler->OnFenceFileDetected(m_requestHandler->m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    EXPECT_TRUE(m_requestedCompileGroup);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 1);
    EXPECT_EQ(m_requestHandler->m_numTimesCreateFenceFileCalled, 1);
    EXPECT_EQ(m_requestHandler->m_numTimesDeleteFenceFileCalled, 1);
    EXPECT_EQ(m_requestHandler->m_requestReadyCount, 1);
    EXPECT_FALSE(m_requestHandler->m_fencingFailed);
    // it worked so far, now synthesize a response:
    // it should result in it asking for asset exists
    m_requestHandler->OnCompileGroupCreated(m_requestIdSet, AssetStatus_Queued);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 0); // for a STATUS request, its enough to know that its queued

    // no callbacks should be set:
    m_requestedCompileGroup = false;
    m_requestedAssetExists = false;
    m_connection->m_sent = false;

    EXPECT_FALSE(m_requestedCompileGroup);
    EXPECT_FALSE(m_requestedAssetExists);
    EXPECT_FALSE(m_connection->m_sent);

    // test invalid group:
    m_requestHandler->OnCompileGroupFinished(NetworkRequestID(0, 0), AssetStatus_Queued);

    EXPECT_FALSE(m_requestedCompileGroup);
    EXPECT_FALSE(m_requestedAssetExists);
    EXPECT_FALSE(m_connection->m_sent);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 0);

    m_requestHandler->OnCompileGroupFinished(m_requestIdSet, AssetStatus_Failed);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 0);
    EXPECT_FALSE(m_connection->m_sent);
}

TEST_F(AssetRequestHandlerUnitTests, RequestToCompileAsset_RequestSent_RequestHandled)
{
    // Test the success case, where its waiting for the actual compilation to be done.
    QByteArray buffer;
    AzFramework::AssetSystem::RequestAssetStatus request(AssetName, false, true);
    AssetProcessor::PackMessage(request, buffer);

    QMetaObject::invokeMethod(m_requestHandler.get(), "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, RequestId.first), Q_ARG(unsigned int, RequestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, Platform));
    m_requestHandler->OnFenceFileDetected(m_requestHandler->m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    EXPECT_EQ(m_requestHandler->m_numTimesCreateFenceFileCalled, 1);
    EXPECT_EQ(m_requestHandler->m_numTimesDeleteFenceFileCalled, 1);
    EXPECT_EQ(m_requestHandler->m_requestReadyCount, 1);
    EXPECT_FALSE(m_requestHandler->m_fencingFailed);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 1);
    m_requestHandler->OnCompileGroupCreated(m_requestIdSet, AssetStatus_Queued);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 1);
    // no callbacks should be set:

    m_requestedCompileGroup = false;
    m_requestedAssetExists = false;
    m_connection->m_sent = false;

    m_requestHandler->OnCompileGroupFinished(m_requestIdSet, AssetStatus_Compiled);

    // decode the buffer.
    ResponseAssetStatus resp;
    // no callbacks should be set:
    EXPECT_FALSE(m_requestedCompileGroup);
    EXPECT_FALSE(m_requestedAssetExists);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 0);
    EXPECT_TRUE(m_connection->m_sent);
    EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(m_connection->m_payload.data(), m_connection->m_payload.size(), resp));
    EXPECT_EQ(resp.m_assetStatus, AssetStatus_Compiled);
}

TEST_F(AssetRequestHandlerUnitTests, RequestToProcessFileOnDiskButNotInQueue_RequestSent_RequestHandled)
{
    // Test the case where the file reports as being on disk just not in the queue
    QByteArray buffer;
    AzFramework::AssetSystem::RequestAssetStatus request(AssetName, true, true);
    AssetProcessor::PackMessage(request, buffer);

    QMetaObject::invokeMethod(m_requestHandler.get(), "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, RequestId.first), Q_ARG(unsigned int, RequestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, Platform));
    m_requestHandler->OnFenceFileDetected(m_requestHandler->m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 1);

    m_requestedCompileGroup = false;
    m_requestedAssetExists = false;
    m_connection->m_sent = false;

    m_requestHandler->OnCompileGroupCreated(m_requestIdSet, AssetStatus_Unknown);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 1);
    EXPECT_FALSE(m_requestedCompileGroup);
    EXPECT_TRUE(m_requestedAssetExists);
    EXPECT_FALSE(m_connection->m_sent);

    m_requestedCompileGroup = false;
    m_requestedAssetExists = false;
    m_connection->m_sent = false;
    m_requestHandler->OnRequestAssetExistsResponse(m_requestIdSet, true);
    EXPECT_FALSE(m_requestedCompileGroup);
    EXPECT_FALSE(m_requestedAssetExists);
    EXPECT_TRUE(m_connection->m_sent);

    // decode the buffer.
    ResponseAssetStatus resp;
    EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(m_connection->m_payload.data(), m_connection->m_payload.size(), resp));
    EXPECT_EQ(resp.m_assetStatus, AssetStatus_Compiled);
}

TEST_F(AssetRequestHandlerUnitTests, TestMultipleInFlightRequests_RequestsSent_RequestsHandled)
{
    QByteArray buffer;
    NetworkRequestID requestId = RequestId;
    AzFramework::AssetSystem::RequestAssetStatus request(AssetName, true, true);
    AssetProcessor::PackMessage(request, buffer);

    QMetaObject::invokeMethod(m_requestHandler.get(), "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, requestId.first), Q_ARG(unsigned int, requestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, Platform));
    m_requestHandler->OnFenceFileDetected(m_requestHandler->m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    requestId = NetworkRequestID(1, 1235);
    m_requestHandler->Reset();
    QMetaObject::invokeMethod(m_requestHandler.get(), "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, requestId.first), Q_ARG(unsigned int, requestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, Platform));
    // note, this last one is for a compile.
    m_requestHandler->OnFenceFileDetected(m_requestHandler->m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    request.m_isStatusRequest = false;
    buffer.clear();
    AssetProcessor::PackMessage(request, buffer);
    requestId = NetworkRequestID(1, 1236);
    m_requestHandler->Reset();
    QMetaObject::invokeMethod(m_requestHandler.get(), "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, requestId.first), Q_ARG(unsigned int, requestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, Platform));
    m_requestHandler->OnFenceFileDetected(m_requestHandler->m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 3);

    m_requestHandler->OnCompileGroupCreated(RequestId, AssetStatus_Queued);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 2);
    m_requestHandler->OnCompileGroupCreated(NetworkRequestID(1, 1235), AssetStatus_Queued);
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 1);
    m_requestHandler->OnCompileGroupCreated(NetworkRequestID(1, 1236), AssetStatus_Queued); // this one doesn't go away yet
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 1);

    m_requestHandler->OnCompileGroupFinished(NetworkRequestID(1, 1236), AssetStatus_Compiled); // this one doesn't go away yet
    EXPECT_EQ(m_requestHandler->GetNumOutstandingAssetRequests(), 0);
}

