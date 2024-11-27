/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Network/IRemoteTools.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>
#include <AzNetworking/DataStructures/ByteBuffer.h>
#include <AzNetworking/Utilities/TimedThread.h>

#include "Utilities/RemoteToolsJoinThread.h"

namespace RemoteToolsPackets
{
    class RemoteToolsConnect;
    class RemoteToolsMessage;
} // namespace RemoteToolsPackets

namespace RemoteTools
{
    // Slightly below AzNetworking's TCP max packet size for maximum message space with room for packet headers
    static constexpr uint32_t RemoteToolsBufferSize = AzNetworking::MaxPacketSize - 384;
    using RemoteToolsMessageBuffer = AzNetworking::ByteBuffer<RemoteToolsBufferSize>;

    struct RemoteToolsRegistryEntry
    {
        bool m_isHost;
        AZ::Name m_name;
        AzNetworking::IpAddress m_ip;

        AzFramework::RemoteToolsEndpointContainer m_availableTargets;
        AzFramework::RemoteToolsEndpointInfo m_lastTarget;
        AZStd::vector<AZStd::byte> m_tmpInboundBuffer;
        uint32_t m_tmpInboundBufferPos;

        AzFramework::RemoteToolsEndpointStatusEvent m_endpointJoinedEvent;
        AzFramework::RemoteToolsEndpointStatusEvent m_endpointLeftEvent;
        AzFramework::RemoteToolsEndpointConnectedEvent m_endpointConnectedEvent;
        AzFramework::RemoteToolsEndpointChangedEvent m_endpointChangedEvent;
    };

    class RemoteToolsSystemComponent
        : public AZ::Component
        , public AZ::SystemTickBus::Handler
        , public AzFramework::IRemoteTools
        , public AzNetworking::IConnectionListener
    {
        friend class RemoteToolsJoinThread;

    public:
        AZ_COMPONENT(RemoteToolsSystemComponent, "{ca110b7c-795e-4fa5-baa9-a115d7e3d86e}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        RemoteToolsSystemComponent();
        ~RemoteToolsSystemComponent();

        bool HandleRequest(
            AzNetworking::IConnection* connection,
            const AzNetworking::IPacketHeader& packetHeader,
            const RemoteToolsPackets::RemoteToolsConnect& packet);
        bool HandleRequest(
            AzNetworking::IConnection* connection,
            const AzNetworking::IPacketHeader& packetHeader,
            const RemoteToolsPackets::RemoteToolsMessage& packet);

    protected:
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

        //////////////////////////////////////////////////////////////////////////
        // ObjectStream Load Callback
        void OnMessageParsed(AzFramework::RemoteToolsMessage** ppMsg, void* classPtr, const AZ::Uuid& classId, const AZ::SerializeContext* sc);
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::SystemTickBus::Handler overrides.
        void OnSystemTick() override;
        ////////////////////////////////////////////////////////////////////////
        
        ////////////////////////////////////////////////////////////////////////
        // AzFramework::IRemoteTools interface implementation
        void RegisterToolingServiceClient(AZ::Crc32 key, AZ::Name name, uint16_t port) override;

        void RegisterToolingServiceHost(AZ::Crc32 key, AZ::Name name, uint16_t port) override;

        const AzFramework::ReceivedRemoteToolsMessages* GetReceivedMessages(AZ::Crc32 key) const override;

        void ClearReceivedMessages(AZ::Crc32 key) override;

        void ClearReceivedMessagesForNextTick(AZ::Crc32 key) override;

        void RegisterRemoteToolsEndpointJoinedHandler(AZ::Crc32 key, AzFramework::RemoteToolsEndpointStatusEvent::Handler& handler) override;

        void RegisterRemoteToolsEndpointLeftHandler(AZ::Crc32 key, AzFramework::RemoteToolsEndpointStatusEvent::Handler& handler) override;

        void RegisterRemoteToolsEndpointConnectedHandler(AZ::Crc32 key, AzFramework::RemoteToolsEndpointConnectedEvent::Handler& handler) override;

        void RegisterRemoteToolsEndpointChangedHandler(AZ::Crc32 key, AzFramework::RemoteToolsEndpointChangedEvent::Handler& handler) override;

        void EnumTargetInfos(AZ::Crc32 key, AzFramework::RemoteToolsEndpointContainer& infos) override;

        void SetDesiredEndpoint(AZ::Crc32 key, AZ::u32 desiredTargetID) override;

        void SetDesiredEndpointInfo(AZ::Crc32 key, const AzFramework::RemoteToolsEndpointInfo& targetInfo) override;

        AzFramework::RemoteToolsEndpointInfo GetDesiredEndpoint(AZ::Crc32 key) override;

        AzFramework::RemoteToolsEndpointInfo GetEndpointInfo(AZ::Crc32 key, AZ::u32 desiredTargetID) override;

        bool IsEndpointOnline(AZ::Crc32 key, AZ::u32 desiredTargetID) override;

        void SendRemoteToolsMessage(const AzFramework::RemoteToolsEndpointInfo& target, const AzFramework::RemoteToolsMessage& msg) override;
        ////////////////////////////////////////////////////////////////////////

        AZStd::unique_ptr<RemoteToolsJoinThread> m_joinThread;

        AZStd::unordered_map<AZ::Crc32, RemoteToolsRegistryEntry> m_entryRegistry;

        AZStd::unordered_map<AZ::Crc32, AzFramework::ReceivedRemoteToolsMessages> m_inbox;
        AZStd::mutex m_inboxMutex;

        AZStd::set<AZ::Crc32> m_messageTypesToClearForNextTick;

    private:
        AzFramework::RemoteToolsMessage* DeserializeMessage(const AZ::Crc32& key, AZStd::vector<AZStd::byte>& buffer, const uint32_t& totalBufferSize);
    };
} // namespace RemoteTools
