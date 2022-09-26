/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FrameworkApplicationFixture.h"

#include <atomic>
#include <gtest/gtest.h>

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/AssetSystemComponent.h>

// This is the type and payload sent from A to B
const AZ::u32 ABType = 0x86;
const char* ABPayload = "Hello World";
const AZ::u32 ABPayloadSize = azlossy_caster(strlen(ABPayload));

// This is the type sent from A to B with no payload
const AZ::u32 ABNoPayloadType = 0x69;

// This is the type and payload sent from B to A
const AZ::u32 BAType = 0xffff0000;
const char* BAPayload = "When in the Course of human events it becomes necessary for one people to dissolve the political bands which have connected them with another and to assume among the powers of the earth, the separate and equal station to which the Laws of Nature and of Nature's God entitle them, a decent respect to the opinions of mankind requires that they should declare the causes which impel them to the separation.";
const AZ::u32 BAPayloadSize = azlossy_caster(strlen(BAPayload));

// how long before tests fail when expecting a connection.
// normally, connections to localhost happen immediately (microseconds), so this is just for when things
// go wrong.  In normal test runs, we'll be yielding and waiting very short
// amounts of time (milliseconds) instead of the full 15 seconds.
const int secondsMaxConnectionAttempt = 15;

// the longest time it should be conceivable for a message to take to send.
// most messages will arrive within microseconds, but if the machine is really busy it could take
// a couple orders of magnitude longer.  Nothing in these tests waits for this full duration
// unless a test is failing, so the actual runtime of the tests should be milliseconds.
const int millisecondsForSend = 5000;



class APConnectionTest
    : public UnitTest::FrameworkApplicationFixture
{
protected:

    void SetUp() override
    {
        FrameworkApplicationFixture::SetUp();
    }

    void TearDown() override
    {
        FrameworkApplicationFixture::TearDown();
    }

    bool WaitForConnectionStateToBeEqual(AzFramework::AssetSystem::AssetProcessorConnection& connectionObject, AzFramework::SocketConnection::EConnectionState desired)
    {
        // The connection state must be copied to a local variable as once the condition
        // matches in the loop condition it could later not match in the return statement
        // as the state is being updated on another thread
        AzFramework::SocketConnection::EConnectionState connectionState;
        auto started = AZStd::chrono::steady_clock::now();
        for (connectionState = connectionObject.GetConnectionState(); connectionState != desired;
            connectionState = connectionObject.GetConnectionState())
        {
            auto seconds_passed = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::steady_clock::now() - started).count();
            if (seconds_passed > secondsMaxConnectionAttempt)
            {
                break;
            }
            AZStd::this_thread::yield();
        }
        return connectionState == desired;
    }


    bool WaitForConnectionStateToNotBeEqual(AzFramework::AssetSystem::AssetProcessorConnection& connectionObject, AzFramework::SocketConnection::EConnectionState notDesired)
    {
        // The connection state must be copied to a local variable as once the condition
        // matches in the loop condition it could later not match in the return statement
        // as the state is being updated on another thread
        AzFramework::SocketConnection::EConnectionState connectionState;
        auto started = AZStd::chrono::steady_clock::now();
        for (connectionState = connectionObject.GetConnectionState(); connectionState == notDesired;
            connectionState = connectionObject.GetConnectionState())
        {
            auto seconds_passed = AZStd::chrono::duration_cast<AZStd::chrono::seconds>(AZStd::chrono::steady_clock::now() - started).count();
            if (seconds_passed > secondsMaxConnectionAttempt)
            {
                break;
            }
            AZStd::this_thread::yield();
        }
        return connectionState != notDesired;
    }

};

TEST_F(APConnectionTest, TestAddRemoveCallbacks)
{
    using namespace AzFramework;

    // This is connection A
    AssetSystem::AssetProcessorConnection apConnection;
    apConnection.m_unitTesting = true;

    // This is connection B
    AssetSystem::AssetProcessorConnection apListener;
    apListener.m_unitTesting = true;

    std::atomic_uint BAMessageCallbackCount;
    BAMessageCallbackCount = 0;
    std::atomic_uint ABMessageCallbackCount;
    ABMessageCallbackCount = 0;

    AZStd::binary_semaphore messageArrivedSemaphore;
    // once we disconnect, we'll set this atomic to ensure no message arrives after disconnection
    AZStd::atomic_bool failIfMessageArrivesAB = {false};
    AZStd::atomic_bool failIfMessageArrivesBA = {false};

    // Connection A is expecting the above type and payload from B, therefore it is B->A, BA
    auto BACallbackHandle = apConnection.AddMessageHandler(BAType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_FALSE(failIfMessageArrivesBA.load());
        EXPECT_EQ(typeId, BAType);
        EXPECT_EQ(BAPayloadSize, dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), BAPayload, dataLength));
        ++BAMessageCallbackCount;
        messageArrivedSemaphore.release();
    });

    // Connection B is expecting the above type and payload from A, therefore it is A->B, AB
    auto ABCallbackHandle = apListener.AddMessageHandler(ABType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_FALSE(failIfMessageArrivesAB.load());
        EXPECT_EQ(typeId, ABType);
        EXPECT_EQ(ABPayloadSize, dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), ABPayload, dataLength));
        ++ABMessageCallbackCount;
        messageArrivedSemaphore.release();
    });

    // Test listening
    EXPECT_EQ(apListener.GetConnectionState(), SocketConnection::EConnectionState::Disconnected);
    bool listenResult = apListener.Listen(11112);
    EXPECT_TRUE(listenResult);

    // Wait some time for the connection to start listening, since it doesn't actually call listen() immediately.
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Listening));
    EXPECT_EQ(apListener.GetConnectionState(), SocketConnection::EConnectionState::Listening);

    // Test connect success
    EXPECT_EQ(apConnection.GetConnectionState(), SocketConnection::EConnectionState::Disconnected);
    // This is blocking, should connect
    bool connectResult = apConnection.Connect("127.0.0.1", 11112);
    EXPECT_TRUE(connectResult);

    // Wait some time for the connection to negotiate, only after negotiation succeeds is it actually considered connected,,
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));

    // Check listener for success - by this time the listener should also be considered connected.
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    //
    // Send first set, ensure we got 1 each
    //

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_EQ(ABMessageCallbackCount, 1);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_EQ(BAMessageCallbackCount, 1);

    //
    // Send second set, ensure we got 2 each (didn't auto-remove or anything crazy)
    //

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_EQ(ABMessageCallbackCount, 2);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_EQ(BAMessageCallbackCount, 2);

    // Remove callbacks
    // after removing a listener, we expect no further messages to arrive.
    apConnection.RemoveMessageHandler(BAType, BACallbackHandle);
    failIfMessageArrivesBA = true;
    apListener.RemoveMessageHandler(ABType, ABCallbackHandle);
    failIfMessageArrivesAB = true;

    // the below 2 lines send a message while nobody is connected as a listener.
    // it may not fail immediately but will cause a cascade later, which is better than
    // waiting for some large timeout in the test.
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);

    // Disconnect A
    // which flushes and will cause any traps to spring.
    bool disconnectResult = apConnection.Disconnect(true);
    EXPECT_TRUE(disconnectResult);

    // Disconnect B
    disconnectResult = apListener.Disconnect(true);
    EXPECT_TRUE(disconnectResult);

    // Verify A and B are disconnected
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));
}

TEST_F(APConnectionTest, TestAddRemoveCallbacks_RemoveDuringCallback_DoesNotCrash)
{
    using namespace AzFramework;

    // This is connection A
    AssetSystem::AssetProcessorConnection apConnection;
    apConnection.m_unitTesting = true;

    // This is connection B
    AssetSystem::AssetProcessorConnection apListener;
    apListener.m_unitTesting = true;

    std::atomic_uint BAMessageCallbackCount;
    BAMessageCallbackCount = 0;
    std::atomic_uint ABMessageCallbackCount;
    ABMessageCallbackCount = 0;

    AZStd::binary_semaphore messageArrivedSemaphore;

    // establish connection
    EXPECT_TRUE(apListener.Listen(11112));
    EXPECT_TRUE(apConnection.Connect("127.0.0.1", 11112));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    //
    // Now try adding listeners that remove themselves during callback
    //
    // Connection A is expecting the above type and payload from B, therefore it is B->A, BA

    // we set a trap here - after we first get this message, we are removing the handler
    // so that it should not ever fire again, and we assert that its false.
    AZStd::atomic_bool failIfWeGetCalledAgainBA = {false};
    SocketConnection::TMessageCallbackHandle SelfRemovingBACallbackHandle = SocketConnection::s_invalidCallbackHandle;
    SelfRemovingBACallbackHandle = apConnection.AddMessageHandler(BAType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_FALSE(failIfWeGetCalledAgainBA.load());
        EXPECT_EQ(typeId, BAType);
        EXPECT_EQ(BAPayloadSize, dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), BAPayload, dataLength));
        ++BAMessageCallbackCount;
        apConnection.RemoveMessageHandler(BAType, SelfRemovingBACallbackHandle);
        failIfWeGetCalledAgainBA = true;
        messageArrivedSemaphore.release();

    });

    // Connection B is expecting the above type and payload from A, therefore it is A->B, AB
    AZStd::atomic_bool failIfWeGetCalledAgainAB = {false};
    SocketConnection::TMessageCallbackHandle SelfRemovingABCallbackHandle = SocketConnection::s_invalidCallbackHandle;
    SelfRemovingABCallbackHandle = apListener.AddMessageHandler(ABType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_FALSE(failIfWeGetCalledAgainAB.load());
        EXPECT_EQ(typeId, ABType);
        EXPECT_EQ(ABPayloadSize, dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), ABPayload, dataLength));
        ++ABMessageCallbackCount;
        apListener.RemoveMessageHandler(ABType, SelfRemovingABCallbackHandle);
        failIfWeGetCalledAgainAB = true;
        messageArrivedSemaphore.release();
    });

    // Send message, should be at 1 each

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_EQ(ABMessageCallbackCount, 1);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_EQ(BAMessageCallbackCount, 1);

    // the callback has disconnected, so sending additional messages should NOT result in the callback
    // being called.
    // we send some additional messages, as a "trap", if the callbacks fire, then the
    // above callback functions will trigger their asserts.
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);

    // disconnect fully, which flushes sender queue and reciever queue and will cause any traps to spring!
    EXPECT_TRUE(apConnection.Disconnect(true));
    EXPECT_TRUE(apListener.Disconnect(true));

    // Verify A and B are disconnected
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));
}

TEST_F(APConnectionTest, TestAddRemoveCallbacks_AddDuringCallback_DoesNotCrash)
{
    using namespace AzFramework;

    // This is connection A
    AssetSystem::AssetProcessorConnection apConnection;
    apConnection.m_unitTesting = true;

    // This is connection B
    AssetSystem::AssetProcessorConnection apListener;
    apListener.m_unitTesting = true;

    std::atomic_uint BAMessageCallbackCount;
    BAMessageCallbackCount = 0;
    std::atomic_uint ABMessageCallbackCount;
    ABMessageCallbackCount = 0;

    AZStd::binary_semaphore messageArrivedSemaphore;

    // establish connection
    EXPECT_TRUE(apListener.Listen(11112));
    EXPECT_TRUE(apConnection.Connect("127.0.0.1", 11112));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    //
    // Now try adding listeners that add more listeners during callback
    //

    // Connection A is expecting the above type and payload from B, therefore it is B->A, BA
    SocketConnection::TMessageCallbackHandle SecondAddedBACallbackHandle = SocketConnection::s_invalidCallbackHandle;
    SocketConnection::TMessageCallbackHandle AddingBACallbackHandle = SocketConnection::s_invalidCallbackHandle;

    // set some traps so that if things call more than once, its a failure:
    AZStd::atomic_bool AddingBACallbackFailIfCalledAgain = {false};
    AZStd::atomic_bool AddingBACallbackFailIfCalledAgain_inner = {false};

    AddingBACallbackHandle = apConnection.AddMessageHandler(BAType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_FALSE(AddingBACallbackFailIfCalledAgain.load());
        EXPECT_EQ(typeId, BAType);
        EXPECT_EQ(BAPayloadSize, dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), BAPayload, dataLength));
        ++BAMessageCallbackCount;
        apConnection.RemoveMessageHandler(BAType, AddingBACallbackHandle);
        AddingBACallbackFailIfCalledAgain = true;
        SecondAddedBACallbackHandle = apConnection.AddMessageHandler(BAType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
        {
            EXPECT_FALSE(AddingBACallbackFailIfCalledAgain_inner.load());
            EXPECT_EQ(typeId, BAType);
            EXPECT_EQ(BAPayloadSize, dataLength);
            EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), BAPayload, dataLength));
            ++BAMessageCallbackCount;
            apConnection.RemoveMessageHandler(BAType, SecondAddedBACallbackHandle);
            AddingBACallbackFailIfCalledAgain_inner = true;
            messageArrivedSemaphore.release();
        });
        messageArrivedSemaphore.release();
    });

// Connection B is expecting the above type and payload from A, therefore it is A->B, AB
    AZStd::atomic_bool AddingABCallbackFailIfCalledAgain = {false};
    AZStd::atomic_bool AddingABCallbackFailIfCalledAgain_inner = {false};
    SocketConnection::TMessageCallbackHandle SecondAddedABCallbackHandle = SocketConnection::s_invalidCallbackHandle;
    SocketConnection::TMessageCallbackHandle AddingABCallbackHandle = SocketConnection::s_invalidCallbackHandle;
    AddingABCallbackHandle = apListener.AddMessageHandler(ABType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_FALSE(AddingABCallbackFailIfCalledAgain.load());
        EXPECT_EQ(typeId, ABType);
        EXPECT_EQ(ABPayloadSize, dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), ABPayload, dataLength));
        ++ABMessageCallbackCount;
        apListener.RemoveMessageHandler(ABType, AddingABCallbackHandle);
        AddingABCallbackFailIfCalledAgain = true;
        messageArrivedSemaphore.release();
        SecondAddedABCallbackHandle = apListener.AddMessageHandler(ABType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
        {
            EXPECT_FALSE(AddingABCallbackFailIfCalledAgain_inner.load());
            EXPECT_EQ(typeId, ABType);
            EXPECT_EQ(ABPayloadSize, dataLength);
            EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), ABPayload, dataLength));
            ++ABMessageCallbackCount;
            apListener.RemoveMessageHandler(ABType, SecondAddedABCallbackHandle);
            AddingABCallbackFailIfCalledAgain_inner = true;
            messageArrivedSemaphore.release();
        });
    });
    // Send message, should be at 1 each

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_EQ(ABMessageCallbackCount, 1);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_EQ(BAMessageCallbackCount, 1);

    // Send message, should be at 2 each since the handlers
    // have been replaced with the ones created in the callback

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_EQ(ABMessageCallbackCount, 2);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_EQ(BAMessageCallbackCount, 2);

    // Send message, we don't wait for these as our listeners should be disconnected, but there
    // are traps set to make sure they dont call.
    // since we flush on disconnect, these traps will activate.
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);

    // Disconnect A
    // which flushes and will cause any traps to spring.
    bool disconnectResult = apConnection.Disconnect(true);
    EXPECT_TRUE(disconnectResult);

    // Disconnect B
    disconnectResult = apListener.Disconnect(true);
    EXPECT_TRUE(disconnectResult);

    // Verify A and B are disconnected
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));
}

TEST_F(APConnectionTest, TestConnection)
{
    using namespace AzFramework;

    // This is connection A
    AssetSystem::AssetProcessorConnection apConnection;
    apConnection.m_unitTesting = true;

    // This is connection B
    AssetSystem::AssetProcessorConnection apListener;
    apListener.m_unitTesting = true;

    bool ABMessageSuccess = false;
    bool ABNoPayloadMessageSuccess = false;
    bool BAMessageSuccess = false;

    AZStd::binary_semaphore messageArrivedSemaphore;
    // Connection A is expecting the above type and payload from B, therefore it is B->A, BA
    apConnection.AddMessageHandler(BAType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_EQ(typeId, BAType);
        EXPECT_EQ(BAPayloadSize, dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), BAPayload, dataLength));
        BAMessageSuccess = true;
        messageArrivedSemaphore.release();
    });

    // Connection B is expecting the above type and payload from A, therefore it is A->B, AB
    apListener.AddMessageHandler(ABType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* data, AZ::u32 dataLength) -> void
    {
        EXPECT_EQ(typeId, ABType);
        EXPECT_EQ(ABPayloadSize, dataLength);
        EXPECT_TRUE(!strncmp(reinterpret_cast<const char*>(data), ABPayload, dataLength));
        ABMessageSuccess = true;
        messageArrivedSemaphore.release();
    });

    // Connection B is expecting the above type and no payload from A, therefore it is A->B, AB
    apListener.AddMessageHandler(ABNoPayloadType, [&](AZ::u32 typeId, AZ::u32 /*serial*/, const void* /*data*/, AZ::u32 dataLength) -> void
    {
        EXPECT_EQ(typeId, ABNoPayloadType);
        EXPECT_EQ(dataLength, 0);
        ABNoPayloadMessageSuccess = true;
        messageArrivedSemaphore.release();
    });

    // Test connection coming online first
    EXPECT_TRUE(apConnection.GetConnectionState() == SocketConnection::EConnectionState::Disconnected);
    bool connectResult = apConnection.Connect("127.0.0.1", 11120);
    EXPECT_TRUE(connectResult);

    // during the connect/disconnect/reconnect loop, the status of the connection rapidly oscillates
    // between "connecting" and "disconnecting" as it tries, fails, and sets up to try again.
    // Since the connection attempt starts as disconnected (checked before the connect), here we will
    // check that it transitions to a state different than disconnected (connecting/disconnecting) and then
    // it gets back to disconnected once the attempts are exhausted
    EXPECT_TRUE(WaitForConnectionStateToNotBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));

    // Test listening on separate port.  This is NOT the port that the connecting one is trying to reach.
    EXPECT_TRUE(apListener.GetConnectionState() == SocketConnection::EConnectionState::Disconnected);
    EXPECT_TRUE(apListener.Listen(54321));

    // we should end up listening, not connected.
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Listening));
    // since we listened on the 'wrong' port, we should not see a successful connection:
    EXPECT_NE(apConnection.GetConnectionState(), SocketConnection::EConnectionState::Connected);
    // Disconnect listener from wrong port
    EXPECT_TRUE(apListener.Disconnect());
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));

    // Listen with correct port
    EXPECT_TRUE(apListener.Listen(11120));
    // Wait some time for apConnection to connect (it has to finish negotiation)
    // Also the listener needs to tick and connect
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    // Send message from A to B
    apConnection.SendMsg(ABType, ABPayload, ABPayloadSize);
    // Wait some time to allow message to send
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABMessageSuccess);

    // Send message from B to A
    apListener.SendMsg(BAType, BAPayload, BAPayloadSize);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(BAMessageSuccess);

    // Send no payload message from A to B
    apConnection.SendMsg(ABNoPayloadType, nullptr, 0);
    messageArrivedSemaphore.try_acquire_for(AZStd::chrono::milliseconds(millisecondsForSend));
    EXPECT_TRUE(ABNoPayloadMessageSuccess);

    EXPECT_TRUE(apConnection.Disconnect(true));
    EXPECT_TRUE(apListener.Disconnect(true));

    // Verify they've disconnected
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));
}

TEST_F(APConnectionTest, TestReconnect)
{
    using namespace AzFramework;

    // This is connection A
    AssetSystem::AssetProcessorConnection apConnection;
    apConnection.m_unitTesting = true;

    // This is connection B
    AssetSystem::AssetProcessorConnection apListener;
    apListener.m_unitTesting = true;

    // Test listening - listen takes a moment to actually start listening:
    EXPECT_TRUE(apListener.GetConnectionState() == SocketConnection::EConnectionState::Disconnected);
    bool listenResult = apListener.Listen(11120);
    EXPECT_TRUE(listenResult);
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Listening));

    // Test connect success
    EXPECT_TRUE(apConnection.GetConnectionState() == SocketConnection::EConnectionState::Disconnected);
    bool connectResult = apConnection.Connect("127.0.0.1", 11120);
    EXPECT_TRUE(connectResult);

    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    // Disconnect B
    bool disconnectResult = apListener.Disconnect();
    EXPECT_TRUE(disconnectResult);

    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));

    // disconncting the listener should kick out the other end:
    // note that the listener was the ONLY one we told to disconnect
    // the other end (the apConnection) is likely to be in a retry state - so it wont be connected, but it also won't necessarily
    // be disconnected, connecting, etc.
    EXPECT_TRUE(WaitForConnectionStateToNotBeEqual(apConnection, SocketConnection::EConnectionState::Connected));

    // start listening again
    listenResult = apListener.Listen(11120);
    EXPECT_TRUE(listenResult);

    // once we start listening, the ap connection should autoconnect very shortly:
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));

    // at that point, both sides should consider themselves cyonnected (the listener, too)
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    // now disconnect A without waiting for it to finish
    disconnectResult = apConnection.Disconnect();
    EXPECT_TRUE(disconnectResult);

    // wait for it to finish
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));

    // ensure that B rebinds and starts listening again
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Listening));

    // reconnect manually from A -> B
    connectResult = apConnection.Connect("127.0.0.1", 11120);
    EXPECT_TRUE(connectResult);

    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Connected));
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Connected));

    // disconnect everything, starting with B to ensure that reconnect thread exits on disconnect
    disconnectResult = apListener.Disconnect();
    EXPECT_TRUE(disconnectResult);

    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apListener, SocketConnection::EConnectionState::Disconnected));

    // note that the listener was the ONLY one we told to disconnect!
    // the other end (the apConnection) is likely to be in a retry state (ie, one of the states that is not connected)
    EXPECT_TRUE(WaitForConnectionStateToNotBeEqual(apConnection, SocketConnection::EConnectionState::Connected));

    // disconnect A
    disconnectResult = apConnection.Disconnect(true); // we're not going to wait, so we do a final disconnect here (true)
    EXPECT_TRUE(disconnectResult);
    EXPECT_TRUE(WaitForConnectionStateToBeEqual(apConnection, SocketConnection::EConnectionState::Disconnected));
}
