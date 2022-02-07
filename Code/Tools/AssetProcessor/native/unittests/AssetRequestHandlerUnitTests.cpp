/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AssetRequestHandlerUnitTests.h"

#include "native/unittests/MockConnectionHandler.h"

#include "native/AssetManager/AssetRequestHandler.h"
#include <QCoreApplication>

using namespace AssetProcessor;
using namespace AzFramework::AssetSystem;
using namespace UnitTestUtils;

namespace
{
    //! Internal class to unit test AssetRequestHandler
    class AssetRequestHandletUnitTest : public AssetRequestHandler 
    {
    public:
        void Reset()
        {
            m_numTimesCreateFenceFileCalled = 0;
            m_numTimesDeleteFenceFileCalled = 0;
            m_requestReadyCount = 0;
            m_fencingFailed = false;
            m_fenceId = 0;
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
        QString m_fenceFileName = QString();
        bool m_deleteFenceFileResult = false;

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

REGISTER_UNIT_TEST(AssetRequestHandlerUnitTests)

AssetRequestHandlerUnitTests::AssetRequestHandlerUnitTests()
{
}

void AssetRequestHandlerUnitTests::StartTest()
{
    AssetRequestHandletUnitTest requestHandler;

    bool requestedCompileGroup = false;
    bool requestedAssetExists = false;

    QString platformSet;
    NetworkRequestID requestIdSet;
    QString searchTermSet;

    QObject::connect(&requestHandler, &AssetRequestHandler::RequestCompileGroup, this, [&](NetworkRequestID groupID, QString platform, QString searchTerm)
        {
            requestedCompileGroup = true;
            platformSet = platform;
            requestIdSet = groupID;
            searchTermSet = searchTerm;
        });

    QObject::connect(&requestHandler, &AssetRequestHandler::RequestAssetExists, this, [&](NetworkRequestID groupID, QString platform, QString searchTerm)
        {
            requestedAssetExists = true;
            platformSet = platform;
            requestIdSet = groupID;
            searchTermSet = searchTerm;
        });

    AssetProcessor::MockConnectionHandler connection;
    connection.BusConnect(1);

    // ------- FIRST TEST ----------------- create a request to process an asset that does not exist in db and also does not exist in queue:
    QByteArray buffer;
    NetworkRequestID requestId(1, 1234);
    AzFramework::AssetSystem::RequestAssetStatus request("test.dds", true, true);
    AssetProcessor::PackMessage(request, buffer);
    requestHandler.m_fenceFileName = QString();
    requestHandler.OnNewIncomingRequest(requestId.first, requestId.second, buffer, "pc");
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_requestReadyCount == 1);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_fencingFailed);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_numTimesCreateFenceFileCalled == g_RetriesForFenceFile);
    UNIT_TEST_EXPECT_FALSE(requestHandler.m_numTimesDeleteFenceFileCalled);

    requestHandler.Reset();
    requestHandler.m_fenceFileName = "foo.fence";
    requestHandler.m_deleteFenceFileResult = false;
    QMetaObject::invokeMethod(&requestHandler, "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, requestId.first), Q_ARG(unsigned int, requestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, "pc"));
    // we will have to call processEvents here equal to the number of retries, because only after that will the requestReady event be added to the event queue  
    while (requestHandler.m_numTimesDeleteFenceFileCalled <= g_RetriesForFenceFile && !requestHandler.m_requestReadyCount)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_requestReadyCount == 1);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_fencingFailed);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_numTimesCreateFenceFileCalled == 1);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_numTimesDeleteFenceFileCalled == g_RetriesForFenceFile);

    requestHandler.Reset();
    QMetaObject::invokeMethod(&requestHandler, "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, requestId.first), Q_ARG(unsigned int, requestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, "pc"));
    requestHandler.OnFenceFileDetected(requestHandler.m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    UNIT_TEST_EXPECT_TRUE(requestedCompileGroup);
    UNIT_TEST_EXPECT_TRUE(!requestedAssetExists);
    UNIT_TEST_EXPECT_TRUE(!connection.m_sent);
    UNIT_TEST_EXPECT_TRUE(platformSet == "pc");
    UNIT_TEST_EXPECT_TRUE(requestIdSet == NetworkRequestID(1, 1234));
    UNIT_TEST_EXPECT_TRUE(searchTermSet == "test.dds");
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 1);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_numTimesCreateFenceFileCalled == 1);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_numTimesDeleteFenceFileCalled == 1);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_requestReadyCount == 1);
    UNIT_TEST_EXPECT_FALSE(requestHandler.m_fencingFailed);

    requestedCompileGroup = false;
    requestedAssetExists = false;
    connection.m_sent = false;

    // it worked so far, now synthesize a response:
    // it should result in it asking for asset exists
    requestHandler.OnCompileGroupCreated(requestIdSet, AssetStatus_Unknown);

    UNIT_TEST_EXPECT_TRUE(!requestedCompileGroup);
    UNIT_TEST_EXPECT_TRUE(requestedAssetExists);
    UNIT_TEST_EXPECT_TRUE(!connection.m_sent);
    UNIT_TEST_EXPECT_TRUE(platformSet == "pc");
    UNIT_TEST_EXPECT_TRUE(requestIdSet == NetworkRequestID(1, 1234));
    UNIT_TEST_EXPECT_TRUE(searchTermSet == "test.dds");
    requestedCompileGroup = false;
    requestedAssetExists = false;
    connection.m_sent = false;
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 1); // it should still be alive!

    // it worked so far, now synthesize a response:
    // it should result in it asking for asset exists
    requestHandler.OnRequestAssetExistsResponse(requestIdSet,  false);

    // this should result in it sending:
    UNIT_TEST_EXPECT_TRUE(!requestedCompileGroup);
    UNIT_TEST_EXPECT_TRUE(!requestedAssetExists);
    UNIT_TEST_EXPECT_TRUE(connection.m_sent);
    UNIT_TEST_EXPECT_TRUE(NetworkRequestID(1, connection.m_serial) == NetworkRequestID(1, 1234));
    UNIT_TEST_EXPECT_TRUE(connection.m_type == RequestAssetStatus::MessageType);
    requestedCompileGroup = false;
    requestedAssetExists = false;
    connection.m_sent = false;
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 0); // it should be gone now

    // decode the buffer.
    ResponseAssetStatus resp;

    UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(connection.m_payload.data(), connection.m_payload.size(), resp));
    UNIT_TEST_EXPECT_TRUE(resp.m_assetStatus == AssetStatus_Missing);

    // ------ TEST ---------- Create a request for a real compile group.
    // we will mock the response as saying 'yes, I made a compile group':
    requestHandler.Reset();
    QMetaObject::invokeMethod(&requestHandler, "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, requestId.first), Q_ARG(unsigned int, requestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, "pc"));
    requestHandler.OnFenceFileDetected(requestHandler.m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    UNIT_TEST_EXPECT_TRUE(requestedCompileGroup);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 1);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_numTimesCreateFenceFileCalled == 1);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_numTimesDeleteFenceFileCalled == 1);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_requestReadyCount == 1);
    UNIT_TEST_EXPECT_FALSE(requestHandler.m_fencingFailed);
    // it worked so far, now synthesize a response:
    // it should result in it asking for asset exists
    requestHandler.OnCompileGroupCreated(requestIdSet, AssetStatus_Queued);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 0); // for a STATUS request, its enough to know that its queued

    // no callbacks should be set:
    requestedCompileGroup = false;
    requestedAssetExists = false;
    connection.m_sent = false;

    UNIT_TEST_EXPECT_TRUE(!requestedCompileGroup);
    UNIT_TEST_EXPECT_TRUE(!requestedAssetExists);
    UNIT_TEST_EXPECT_TRUE(!connection.m_sent);

    // test invalid group:
    requestHandler.OnCompileGroupFinished(NetworkRequestID(0, 0), AssetStatus_Queued);

    UNIT_TEST_EXPECT_TRUE(!requestedCompileGroup);
    UNIT_TEST_EXPECT_TRUE(!requestedAssetExists);
    UNIT_TEST_EXPECT_TRUE(!connection.m_sent);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 0);

    requestHandler.OnCompileGroupFinished(requestIdSet, AssetStatus_Failed);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 0);
    UNIT_TEST_EXPECT_TRUE(!connection.m_sent);

    // ------------ Test the success case, where its waiting for the actual compilation to be done. ---

    request.m_isStatusRequest = false;
    buffer.clear();
    AssetProcessor::PackMessage(request, buffer);
    requestHandler.Reset();
    QMetaObject::invokeMethod(&requestHandler, "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, requestId.first), Q_ARG(unsigned int, requestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, "pc"));
    requestHandler.OnFenceFileDetected(requestHandler.m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_numTimesCreateFenceFileCalled == 1);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_numTimesDeleteFenceFileCalled == 1);
    UNIT_TEST_EXPECT_TRUE(requestHandler.m_requestReadyCount == 1);
    UNIT_TEST_EXPECT_FALSE(requestHandler.m_fencingFailed);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 1);
    requestHandler.OnCompileGroupCreated(requestIdSet, AssetStatus_Queued);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 1);
    // no callbacks should be set:

    requestedCompileGroup = false;
    requestedAssetExists = false;
    connection.m_sent = false;

    requestHandler.OnCompileGroupFinished(requestIdSet, AssetStatus_Compiled);
    // no callbacks should be set:
    UNIT_TEST_EXPECT_TRUE(!requestedCompileGroup);
    UNIT_TEST_EXPECT_TRUE(!requestedAssetExists);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 0);
    UNIT_TEST_EXPECT_TRUE(connection.m_sent);
    UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(connection.m_payload.data(), connection.m_payload.size(), resp));
    UNIT_TEST_EXPECT_TRUE(resp.m_assetStatus == AssetStatus_Compiled);

    // -------------- Test the case where the file reports as being on disk just not in the queue ------------
    request.m_isStatusRequest = true;
    buffer.clear();
    AssetProcessor::PackMessage(request, buffer);
    requestHandler.Reset();
    QMetaObject::invokeMethod(&requestHandler, "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, requestId.first), Q_ARG(unsigned int, requestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, "pc"));
    requestHandler.OnFenceFileDetected(requestHandler.m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 1);

    requestedCompileGroup = false;
    requestedAssetExists = false;
    connection.m_sent = false;

    requestHandler.OnCompileGroupCreated(requestIdSet, AssetStatus_Unknown);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 1);
    UNIT_TEST_EXPECT_TRUE(!requestedCompileGroup);
    UNIT_TEST_EXPECT_TRUE(requestedAssetExists);
    UNIT_TEST_EXPECT_TRUE(!connection.m_sent);

    requestedCompileGroup = false;
    requestedAssetExists = false;
    connection.m_sent = false;
    requestHandler.OnRequestAssetExistsResponse(requestIdSet, true);
    UNIT_TEST_EXPECT_TRUE(!requestedCompileGroup);
    UNIT_TEST_EXPECT_TRUE(!requestedAssetExists);
    UNIT_TEST_EXPECT_TRUE(connection.m_sent);
    UNIT_TEST_EXPECT_TRUE(AZ::Utils::LoadObjectFromBufferInPlace(connection.m_payload.data(), connection.m_payload.size(), resp));
    UNIT_TEST_EXPECT_TRUE(resp.m_assetStatus == AssetStatus_Compiled);

    // ------------------ TEST MULTIPLE IN-FLIGHT REQUESTS ------------------
    requestHandler.Reset();
    QMetaObject::invokeMethod(&requestHandler, "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, requestId.first), Q_ARG(unsigned int, requestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, "pc"));
    requestHandler.OnFenceFileDetected(requestHandler.m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    requestId = NetworkRequestID(1, 1235);
    requestHandler.Reset();
    QMetaObject::invokeMethod(&requestHandler, "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, requestId.first), Q_ARG(unsigned int, requestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, "pc"));
    // note, this last one is for a compile.
    requestHandler.OnFenceFileDetected(requestHandler.m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    request.m_isStatusRequest = false;
    buffer.clear();
    AssetProcessor::PackMessage(request, buffer);
    requestId = NetworkRequestID(1, 1236);
    requestHandler.Reset();
    QMetaObject::invokeMethod(&requestHandler, "OnNewIncomingRequest", Qt::DirectConnection, Q_ARG(unsigned int, requestId.first), Q_ARG(unsigned int, requestId.second), Q_ARG(QByteArray, buffer), Q_ARG(QString, "pc"));
    requestHandler.OnFenceFileDetected(requestHandler.m_fenceId);
    QCoreApplication::processEvents(QEventLoop::AllEvents);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 3);

    requestHandler.OnCompileGroupCreated(NetworkRequestID(1, 1234), AssetStatus_Queued);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 2);
    requestHandler.OnCompileGroupCreated(NetworkRequestID(1, 1235), AssetStatus_Queued);
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 1);
    requestHandler.OnCompileGroupCreated(NetworkRequestID(1, 1236), AssetStatus_Queued); // this one doesn't go away yet
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 1);

    requestHandler.OnCompileGroupFinished(NetworkRequestID(1, 1236), AssetStatus_Compiled); // this one doesn't go away yet
    UNIT_TEST_EXPECT_TRUE(requestHandler.GetNumOutstandingAssetRequests() == 0);

    connection.BusDisconnect(1);

    Q_EMIT UnitTestPassed();
}

