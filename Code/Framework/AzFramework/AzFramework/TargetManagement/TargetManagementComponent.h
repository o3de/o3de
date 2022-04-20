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
#include <AzNetworking/Utilities/TimedThread.h>

namespace AZ
{
    class SerializeContext;
}

namespace AzFrameworkPackets
{
    class TargetConnect;
    class TargetMessage;
}

namespace AzNetworking
{
    class INetworkInterface;
}

namespace AzFramework
{
    constexpr uint16_t DefaultTargetPort = 6777;

    class TargetManagementNetworkImpl;

    struct TargetManagementSettings
    {
        AZStd::string m_neighborhoodName; // this is the neighborhood session (hub) we want to connect to
        AZStd::string m_persistentName; // this string is used as the persistent name for this target
        TargetInfo m_lastTarget; // this is the target we will automatically connect to
    };

    //! @class TargetJoinThread
    //! @brief A class for polling a connection to the host target
    class TargetJoinThread final : public AzNetworking::TimedThread
    {
    public:
        TargetJoinThread(int updateRate);
        ~TargetJoinThread() override;
    private:
        AZ_DISABLE_COPY_MOVE(TargetJoinThread);

        //! Invoked on thread start
        void OnStart() override {};

        //! Invoked on thread stop
        void OnStop() override {};

        //! Invoked on thread update to poll for a Target host to join
        //! @param updateRateMs The amount of time the thread can spend in OnUpdate in ms
        void OnUpdate(AZ::TimeMs updateRateMs) override;
    };

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
            const AzFrameworkPackets::TargetConnect& packet);
        bool HandleRequest(
            AzNetworking::IConnection* connection,
            const AzNetworking::IPacketHeader& packetHeader,
            const AzFrameworkPackets::TargetMessage& packet);

        void SetTargetAsHost(bool isHost);

    protected:
        //////////////////////////////////////////////////////////////////////////
        // target management api
        void EnumTargetInfos(TargetContainer& infos) override;
        void SetDesiredTarget(AZ::u32 desiredTargetID) override;
        void SetDesiredTargetInfo(const TargetInfo& targetInfo) override;
        TargetInfo GetDesiredTarget() override;
        TargetInfo GetTargetInfo(AZ::u32 desiredTargetID) override;
        bool IsTargetOnline(AZ::u32 desiredTargetID) override;
        bool IsDesiredTargetOnline() override;
        void SetMyPersistentName(const char* name) override;
        const char* GetMyPersistentName() override;
        TargetInfo GetMyTargetInfo() const override;
        void SetNeighborhood(const char* name) override;
        const char* GetNeighborhood() override;
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
        TargetManagementSettings                        m_settings;
        TargetContainer                                 m_availableTargets;
        AZStd::unique_ptr<TargetJoinThread>             m_targetJoinThread;
        bool m_isTargetHost = false;

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
        AZ::Name                            m_networkInterfaceName;
    };
}   // namespace AzFramework

#endif // AZFRAMEWORK_TARGETMANAGEMENTCOMPONENT_H
#pragma once

