
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Network/IToolingConnection.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>
#include <AzNetworking/Utilities/TimedThread.h>
#include <ToolingConnection/ToolingConnectionBus.h>

#include "Utilities/ToolingJoinThread.h"
#include "Utilities/ToolingOutboxThread.h"

namespace ToolingConnectionPackets
{
    class ToolingConnect;
    class ToolingPacket;
} // namespace ToolingConnectionPackets

namespace ToolingConnection
{
    class ToolingConnectionSystemComponent
        : public AZ::Component
        , public AzFramework::IToolingConnection
        , public AzNetworking::IConnectionListener
        , private AZ::SystemTickBus::Handler
    {
    public:
        AZ_COMPONENT(ToolingConnectionSystemComponent, "{ca110b7c-795e-4fa5-baa9-a115d7e3d86e}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ToolingConnectionSystemComponent();
        ~ToolingConnectionSystemComponent();

        bool HandleRequest(
            AzNetworking::IConnection* connection,
            const AzNetworking::IPacketHeader& packetHeader,
            const ToolingConnectionPackets::ToolingConnect& packet);
        bool HandleRequest(
            AzNetworking::IConnection* connection,
            const AzNetworking::IPacketHeader& packetHeader,
            const ToolingConnectionPackets::ToolingPacket& packet);

        void SetTargetAsHost(bool isHost);

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
        void OnMessageParsed(AzFramework::ToolingMessage** ppMsg, void* classPtr, const AZ::Uuid& classId, const AZ::SerializeContext* sc);
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzFramework::IToolingConnection interface implementation
        void RegisterToolingMessageClientHandler(
            const char* hostname, uint16_t port, AzFramework::ToolingMessageEvent::Handler handler) override;

        void RegisterToolingMessageHostHandler(uint16_t port, AzFramework::ToolingMessageEvent::Handler handler) override;

        void EnumTargetInfos(AzFramework::ToolingEndpointContinaer& infos) override;

        void SetDesiredEndpoint(AZ::u32 desiredTargetID) override;

        void SetDesiredEndpointInfo(const AzFramework::ToolingEndpointInfo& targetInfo) override;

        virtual AzFramework::ToolingEndpointInfo GetDesiredEndpoint() override;

        AzFramework::ToolingEndpointInfo GetEndpointInfo(AZ::u32 desiredTargetID) override;

        bool IsEndpointOnline(AZ::u32 desiredTargetID) override;

        void SendToolingMessage(const AzFramework::ToolingEndpointInfo& target, const AzFramework::ToolingMessage& msg) override;

        void DispatchMessages(AZ::u64 id) override;
        ////////////////////////////////////////////////////////////////////////

        AZStd::unique_ptr<ToolingJoinThread> m_joinThread;
        AZStd::unique_ptr<ToolingOutboxThread> m_outboxThread;

        AzFramework::ToolingEndpointContinaer m_availableTargets;
        AZStd::vector<char, AZ::OSStdAllocator> m_tmpInboundBuffer;
        uint32_t m_tmpInboundBufferPos;

        AzFramework::ToolingMessageQueue m_inbox;
        AZStd::mutex m_inboxMutex;
    };

} // namespace ToolingConnection
