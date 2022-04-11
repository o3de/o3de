/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzNetworking/UdpTransport/UdpNetworkInterface.h>
#include <AzNetworking/UdpTransport/UdpPacketTracker.h>
#include <AzNetworking/UdpTransport/UdpPacketIdWindow.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>
#include <AzNetworking/AutoGen/CorePackets.AutoPackets.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/Time/TimeSystem.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    using namespace AzNetworking;

    class TestUdpConnectionListener
        : public IConnectionListener
    {
    public:
        ConnectResult ValidateConnect([[maybe_unused]] const IpAddress& remoteAddress, [[maybe_unused]] const IPacketHeader& packetHeader, [[maybe_unused]] ISerializer& serializer) override
        {
            return ConnectResult::Accepted;
        }

        void OnConnect([[maybe_unused]] IConnection* connection) override
        {
            ;
        }

        PacketDispatchResult OnPacketReceived([[maybe_unused]] IConnection* connection, const IPacketHeader& packetHeader, [[maybe_unused]] ISerializer& serializer) override
        {
            EXPECT_TRUE((packetHeader.GetPacketType() == static_cast<PacketType>(CorePackets::PacketType::InitiateConnectionPacket))
                     || (packetHeader.GetPacketType() == static_cast<PacketType>(CorePackets::PacketType::HeartbeatPacket)));
            return PacketDispatchResult::Failure;
        }

        void OnPacketLost([[maybe_unused]] IConnection* connection, [[maybe_unused]] PacketId packetId) override
        {

        }

        void OnDisconnect([[maybe_unused]] IConnection* connection, [[maybe_unused]] DisconnectReason reason, [[maybe_unused]] TerminationEndpoint endpoint) override
        {

        }
    };

    TEST(UdpTransportTests, PacketIdWrap)
    {
        const uint32_t SEQUENCE_BOUNDARY = 0xFFFF;
        UdpPacketTracker tracker;

        for (uint32_t i = 0; i < SEQUENCE_BOUNDARY; ++i)
        {
            tracker.GetNextPacketId();
        }
        EXPECT_EQ(tracker.GetNextPacketId(), PacketId(SEQUENCE_BOUNDARY + 1));
    }

    TEST(UdpTransportTests, AckReplication)
    {
        static const SequenceId TestReliableSequenceId = InvalidSequenceId;
        static const PacketType TestPacketId = PacketType{ 0 };

        UdpPacketTracker send;
        UdpPacketTracker recv;

        for (uint32_t i = 0; i < 128; i++)
        {
            UdpPacketHeader sendHeader1(send, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader sendHeader2(send, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader sendHeader3(send, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader sendHeader4(send, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader sendHeader5(send, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader sendHeader6(send, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader sendHeader7(send, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader sendHeader8(send, TestPacketId, TestReliableSequenceId);

            UdpPacketHeader recvHeader1(recv, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader recvHeader2(recv, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader recvHeader3(recv, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader recvHeader4(recv, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader recvHeader5(recv, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader recvHeader6(recv, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader recvHeader7(recv, TestPacketId, TestReliableSequenceId);
            UdpPacketHeader recvHeader8(recv, TestPacketId, TestReliableSequenceId);

            send.ProcessReceived(nullptr, recvHeader3);
            recv.ProcessReceived(nullptr, sendHeader3);
            recv.ProcessReceived(nullptr, sendHeader2);
            send.ProcessReceived(nullptr, recvHeader3);
            recv.ProcessReceived(nullptr, sendHeader1);
            recv.ProcessReceived(nullptr, sendHeader5);
            recv.ProcessReceived(nullptr, sendHeader8);
            send.ProcessReceived(nullptr, recvHeader2);

            UdpPacketHeader recvHeaderTmp(recv, TestPacketId, TestReliableSequenceId);
            send.ProcessReceived(nullptr, recvHeaderTmp);

            {
                BitsetChunk sendChunk;
                BitsetChunk recvChunk;

                send.GetAcknowledgedWindow().GetMostRecentAckState(sendChunk);
                recv.GetReceivedWindow().GetMostRecentAckState(recvChunk);

                BitsetChunk testResult = 0;
                for (uint32_t bit = 0; bit < UdpPacketIdWindow::PacketAckContainer::NumBitsetChunkedBits; ++bit)
                {
                    if (send.GetAcknowledgedWindow().GetPacketAckContainer().GetBit(bit))
                    {
                        SetBitHelper(testResult, bit, true);
                    }
                }

                EXPECT_EQ(sendChunk, recvChunk); // PacketTracker: Replication of acked bits
                EXPECT_EQ(sendChunk, testResult); // Optimized ack window generation failed brute force check
            }

            UdpPacketHeader sendHeaderTmp(send, TestPacketId, TestReliableSequenceId);
            recv.ProcessReceived(nullptr, sendHeaderTmp);

            {
                BitsetChunk sendChunk;
                BitsetChunk recvChunk;

                recv.GetAcknowledgedWindow().GetMostRecentAckState(sendChunk);
                send.GetReceivedWindow().GetMostRecentAckState(recvChunk);

                BitsetChunk testResult = 0;
                for (uint32_t bit = 0; bit < UdpPacketIdWindow::PacketAckContainer::NumBitsetChunkedBits; ++bit)
                {
                    if (recv.GetAcknowledgedWindow().GetPacketAckContainer().GetBit(bit))
                    {
                        SetBitHelper(testResult, bit, true);
                    }
                }

                EXPECT_EQ(sendChunk, recvChunk); // PacketTracker: Replication of acked bits
                EXPECT_EQ(sendChunk, testResult); // Optimized ack window generation failed brute force check
            }
        }
    }

    TEST(UdpTransportTests, PacketIdWindow)
    {
        const PacketType TestPacketType{ 12212 };

        UdpPacketIdWindow packetWindow;

        UdpPacketHeader header1(TestPacketType, InvalidSequenceId, SequenceId{ 985 }, InvalidSequenceId, 0xF8000FFF, SequenceRolloverCount{ 0 });
        packetWindow.UpdateForRemoteAckStatus(nullptr, header1);

        UdpPacketHeader header2(TestPacketType, InvalidSequenceId, SequenceId{ 995 }, InvalidSequenceId, 0x3FFFFF, SequenceRolloverCount{ 0 });
        packetWindow.UpdateForRemoteAckStatus(nullptr, header2);

        UdpPacketHeader header3(TestPacketType, InvalidSequenceId, SequenceId{ 999 }, InvalidSequenceId, 0x3FFFFFF, SequenceRolloverCount{ 0 });
        packetWindow.UpdateForRemoteAckStatus(nullptr, header3);

        UdpPacketHeader header4(TestPacketType, InvalidSequenceId, SequenceId{ 1080 }, InvalidSequenceId, 0x3FF, SequenceRolloverCount{ 0 });
        packetWindow.UpdateForRemoteAckStatus(nullptr, header4);

        UdpPacketHeader header5(TestPacketType, InvalidSequenceId, SequenceId{ 1090 }, InvalidSequenceId, 0xFFFFF, SequenceRolloverCount{ 0 });
        packetWindow.UpdateForRemoteAckStatus(nullptr, header5);

        UdpPacketHeader header6(TestPacketType, InvalidSequenceId, SequenceId{ 1100 }, InvalidSequenceId, 0x3FFFFFFF, SequenceRolloverCount{ 0 });
        packetWindow.UpdateForRemoteAckStatus(nullptr, header6);

        UdpPacketHeader header7(TestPacketType, InvalidSequenceId, SequenceId{ 1102 }, InvalidSequenceId, 0xFFFFFFFF, SequenceRolloverCount{ 0 });
        packetWindow.UpdateForRemoteAckStatus(nullptr, header7);

        UdpPacketHeader header8(TestPacketType, InvalidSequenceId, SequenceId{ 1134 }, InvalidSequenceId, 0x1, SequenceRolloverCount{ 0 });
        packetWindow.UpdateForRemoteAckStatus(nullptr, header8);

        PacketAckState ackState = packetWindow.GetPacketAckStatus(PacketId(1007));
        EXPECT_EQ(ackState, PacketAckState::Nacked); // Testing that PacketId is not flagged as acked
    }
}
