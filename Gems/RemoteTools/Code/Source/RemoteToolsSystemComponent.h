
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Network/IRemoteTools.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>
#include <AzNetworking/DataStructures/ByteBuffer.h>
#include <AzNetworking/Utilities/TimedThread.h>

#include "Utilities/RemoteToolsJoinThread.h"
#include "Utilities/RemoteToolsOutboxThread.h"

namespace RemoteToolsPackets
{
    class RemoteToolsConnect;
    class RemoteToolsPacket;
} // namespace RemoteToolsPackets

namespace RemoteTools
{
    // Slightly below AzNetworking's TCP max packet size for maximum message space with room for packet headers
    static constexpr uint32_t RemoteToolsBufferSize = 16000;
    using RemoteToolsMessageBuffer = AzNetworking::ByteBuffer<RemoteToolsBufferSize>;

    struct RemoteToolsRegistryEntry
    {
        AZStd::string m_name;
        uint16_t m_port;

        AzFramework::RemoteToolsEndpointContainer m_availableTargets;
        AzFramework::RemoteToolsEndpointInfo m_lastTarget;
        AZStd::vector<char, AZ::OSStdAllocator> m_tmpInboundBuffer;
        uint32_t m_tmpInboundBufferPos;

        AzFramework::RemoteToolsEndpointStatusEvent m_endpointJoinedEvent;
        AzFramework::RemoteToolsEndpointStatusEvent m_endpointLeftEvent;
        AzFramework::RemoteToolsEndpointConnectedEvent m_endpointConnectedEvent;
        AzFramework::RemoteToolsEndpointChangedEvent m_endpointChangedEvent;
    };

    class RemoteToolsSystemComponent
        : public AZ::Component
        , public AzFramework::IRemoteTools
        , public AzNetworking::IConnectionListener
        , private AZ::SystemTickBus::Handler
    {
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
            const RemoteToolsPackets::RemoteToolsPacket& packet);

    protected:
        //////////////////////////////////////////////////////////////////////////
        // AZ::SystemTickBus
        void OnSystemTick() override;
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
        // AzFramework::IRemoteTools interface implementation

        AzFramework::RemoteToolsServiceKey RegisterToolingService(AZStd::string name, uint16_t port) override;

        const AzFramework::ReceivedRemoteToolsMessages* GetReceivedMessages(AzFramework::RemoteToolsServiceKey key) const override;

        void ClearReceivedMessages(AzFramework::RemoteToolsServiceKey key) override;

        void RegisterRemoteToolsEndpointJoinedHandler(AzFramework::RemoteToolsServiceKey key, AzFramework::RemoteToolsEndpointStatusEvent::Handler handler) override;

        void RegisterRemoteToolsEndpointLeftHandler(AzFramework::RemoteToolsServiceKey key, AzFramework::RemoteToolsEndpointStatusEvent::Handler handler) override;

        void RegisterRemoteToolsEndpointConnectedHandler(AzFramework::RemoteToolsServiceKey key, AzFramework::RemoteToolsEndpointConnectedEvent::Handler handler) override;

        void RegisterRemoteToolsEndpointChangedHandler(AzFramework::RemoteToolsServiceKey key, AzFramework::RemoteToolsEndpointChangedEvent::Handler handler) override;

        void EnumTargetInfos(AzFramework::RemoteToolsServiceKey key, AzFramework::RemoteToolsEndpointContainer& infos) override;

        void SetDesiredEndpoint(AzFramework::RemoteToolsServiceKey key, AZ::u32 desiredTargetID) override;

        void SetDesiredEndpointInfo(AzFramework::RemoteToolsServiceKey key, const AzFramework::RemoteToolsEndpointInfo& targetInfo) override;

        AzFramework::RemoteToolsEndpointInfo GetDesiredEndpoint(AzFramework::RemoteToolsServiceKey key) override;

        AzFramework::RemoteToolsEndpointInfo GetEndpointInfo(AzFramework::RemoteToolsServiceKey key, AZ::u32 desiredTargetID) override;

        bool IsEndpointOnline(AzFramework::RemoteToolsServiceKey key, AZ::u32 desiredTargetID) override;

        void SendRemoteToolsMessage(const AzFramework::RemoteToolsEndpointInfo& target, const AzFramework::RemoteToolsMessage& msg) override;
        ////////////////////////////////////////////////////////////////////////

        AZStd::unique_ptr<RemoteToolsJoinThread> m_joinThread;
        AZStd::unique_ptr<RemoteToolsOutboxThread> m_outboxThread;

        AZStd::unordered_map<AzFramework::RemoteToolsServiceKey, RemoteToolsRegistryEntry> m_entryRegistry;

        AZStd::unordered_map<AzFramework::RemoteToolsServiceKey, AzFramework::ReceivedRemoteToolsMessages> m_inbox;
        AZStd::mutex m_inboxMutex;
    };
} // namespace RemoteTools
