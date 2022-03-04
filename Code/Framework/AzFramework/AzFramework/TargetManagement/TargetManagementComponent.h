/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZFRAMEWORK_TARGETMANAGEMENTCOMPONENT_H
#define AZFRAMEWORK_TARGETMANAGEMENTCOMPONENT_H

#include <AzFramework/TargetManagement/TargetManagementAPI.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>

namespace AZ
{
    class SerializeContext;
}

namespace AzFrameworkPackets
{
    class Neighbor;
    class TargetManagementMessage;
}

namespace AzNetworking
{
    class INetworkInterface;
}

namespace AzFramework
{
    class TargetManagementNetworkImpl;

    class TargetManagementComponent
        : public AZ::Component
        , public AzNetworking::IConnectionListener
        , private TargetManager::Bus::Handler
        , private AZ::SystemTickBus::Handler
    {
        friend TargetManagementNetworkImpl;

    public:
        AZ_COMPONENT(TargetManagementComponent, "{39899133-42B3-4e92-A579-CDDC85A23277}")

        TargetManagementComponent();
        ~TargetManagementComponent() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        //////////////////////////////////////////////////////////////////////////

        bool HandleRequest(
            AzNetworking::IConnection* connection,
            const AzNetworking::IPacketHeader& packetHeader,
            const AzFrameworkPackets::Neighbor& packet);
        bool HandleRequest(
            AzNetworking::IConnection* connection,
            const AzNetworking::IPacketHeader& packetHeader,
            const AzFrameworkPackets::TargetManagementMessage& packet);

    protected:
        //////////////////////////////////////////////////////////////////////////
        // target management api
        void SetMyPersistentName(const char* name) override;
        const char* GetMyPersistentName() override;
        TargetInfo GetTargetInfo() const override;
        bool IsTargetOnline() const override;
        void SendTmMessage(const TargetInfo& target, const TmMsg& msg) override;
        void DispatchMessages(MsgSlotId id) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::SystemTickBus
        void OnSystemTick() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ObjectStream Load Callback
        void OnMsgParsed(TmMsg** ppMsg, void* classPtr, const AZ::Uuid& classId, const AZ::SerializeContext* sc);
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // IConnectionListener interface
        AzNetworking::ConnectResult ValidateConnect(
            const AzNetworking::IpAddress& remoteAddress,
            const AzNetworking::IPacketHeader& packetHeader,
            AzNetworking::ISerializer& serializer) override;
        void OnConnect(AzNetworking::IConnection* connection) override;
        AzNetworking::PacketDispatchResult OnPacketReceived(
            AzNetworking::IConnection* connection,
            const AzNetworking::IPacketHeader& packetHeader,
            AzNetworking::ISerializer& serializer) override;
        void OnPacketLost(AzNetworking::IConnection* connection, AzNetworking::PacketId packetId) override;
        void OnDisconnect(
            AzNetworking::IConnection* connection,
            AzNetworking::DisconnectReason reason,
            AzNetworking::TerminationEndpoint endpoint) override;
        ////////////////////////////////////////////////////////////////////////

        // All communication updates run on a separate thread to avoid being blocked by breakpoints
        void TickThread();

        TargetInfo                                      m_targetInfo;
        AZStd::chrono::system_clock::time_point         m_reconnectionTime; // time of next connection attempt

        AZStd::vector<char, AZ::OSStdAllocator> m_tmpInboundBuffer;
        uint32_t m_tmpInboundBufferPos;

        TmMsgQueue          m_inbox;
        AZStd::mutex        m_inboxMutex;

        typedef AZStd::pair<AZ::u32, AZStd::vector<char, AZ::OSStdAllocator> > OutboundBufferedTmMsg;
        typedef AZStd::deque<OutboundBufferedTmMsg, AZ::OSStdAllocator> TmMsgOutbox;
        TmMsgOutbox         m_outbox;
        AZStd::mutex        m_outboxMutex;

        AZ::SerializeContext*   m_serializeContext;

        // these are used for target communication
        AZStd::atomic_bool                  m_stopRequested;
        AZStd::thread                       m_threadHandle;
        TargetManagementNetworkImpl*        m_networkImpl;
        AzNetworking::INetworkInterface*    m_networkInterface;
    };
}   // namespace AzFramework

#endif // AZFRAMEWORK_TARGETMANAGEMENTCOMPONENT_H
#pragma once


