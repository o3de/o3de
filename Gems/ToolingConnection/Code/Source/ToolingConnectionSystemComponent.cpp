
#include <ToolingConnectionSystemComponent.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/ObjectStream.h>

#include <AzNetworking/Utilities/CidrAddress.h>

#include <Source/AutoGen/ToolingConnection.AutoPackets.h>
#include <Source/AutoGen/ToolingConnection.AutoPacketDispatcher.h>

namespace ToolingConnection
{
    static const AzNetworking::CidrAddress ToolingCidrFilter = AzNetworking::CidrAddress();

    void ToolingConnectionSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ToolingConnectionSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ToolingConnectionSystemComponent>("ToolingConnection", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void ToolingConnectionSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ToolingConnectionService"));
    }

    void ToolingConnectionSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ToolingConnectionService"));
    }

    void ToolingConnectionSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NetworkingService"));
    }

    void ToolingConnectionSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        ;
    }

    ToolingConnectionSystemComponent::ToolingConnectionSystemComponent()
    {
        if (AZ::Interface<AzFramework::IToolingConnection>::Get() == nullptr)
        {
            AZ::Interface<AzFramework::IToolingConnection>::Register(this);
        }
    }

    ToolingConnectionSystemComponent::~ToolingConnectionSystemComponent()
    {
        if (AZ::Interface<AzFramework::IToolingConnection>::Get() == this)
        {
            AZ::Interface<AzFramework::IToolingConnection>::Unregister(this);
        }
    }

    void ToolingConnectionSystemComponent::Init()
    {
    }

    void ToolingConnectionSystemComponent::Activate()
    {
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void ToolingConnectionSystemComponent::Deactivate()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void ToolingConnectionSystemComponent::OnSystemTick()
    {
#if !defined(AZ_RELEASE_BUILD)
        // If we're not the host and not connected to one, attempt to connect on a fixed interval

#endif

        size_t maxMsgsToProcess = m_inbox.size();
        for (size_t i = 0; i < maxMsgsToProcess; ++i)
        {
            AzFramework::ToolingMessagePointer msg;
            if (!m_inbox.empty())
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_inboxMutex);
                msg = m_inbox.front();
                m_inbox.pop_front();
            }
            else
            {
                break;
            }

            if (msg)
            {
                // EBUS_EVENT_ID(msg->GetId(), TmMsgBus, OnReceivedMsg, msg);
            }
        }
    }

    bool ToolingConnectionSystemComponent::HandleRequest(
        AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        const ToolingConnectionPackets::ToolingConnect& packet)
    {
        const uint32_t persistentId = packet.GetPersistentId();
        AzFramework::ToolingEndpointContinaer::pair_iter_bool ret = m_availableTargets.insert_key(persistentId);
        AzFramework::ToolingEndpointInfo& ti = ret.first->second;
        ti.SetInfo(packet.GetDisplayName(), persistentId, static_cast<uint32_t>(connection->GetConnectionId()));
        //TODO EBUS_QUEUE_EVENT(TargetManagerClient::Bus, TargetJoinedNetwork, ti);
        return true;
    }

    bool ToolingConnectionSystemComponent::HandleRequest(
        AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] const ToolingConnectionPackets::ToolingPacket& packet)
    {
        // Receive
        if (connection->GetConnectionRole() == AzNetworking::ConnectionRole::Acceptor
            /*TODO && static_cast<uint32_t>(connection->GetConnectionId()) != m_settings.m_lastTarget.m_networkId*/)
        {
            // Listener should route traffic based on selected target
            return true;
        }

        // If we're a client, set the host to our desired target
        if (connection->GetConnectionRole() == AzNetworking::ConnectionRole::Connector)
        {
            if (GetEndpointInfo(packet.GetPersistentId()).GetPersistentId() == 0)
            {
                AzFramework::ToolingEndpointContinaer::pair_iter_bool ret = m_availableTargets.insert_key(packet.GetPersistentId());

                AzFramework::ToolingEndpointInfo& ti = ret.first->second;
                ti.SetInfo("Host", packet.GetPersistentId(), static_cast<uint32_t>(connection->GetConnectionId()));

                //TODO EBUS_QUEUE_EVENT(TargetManagerClient::Bus, TargetJoinedNetwork, ti);
            }

            if (GetDesiredEndpoint().GetPersistentId() != packet.GetPersistentId())
            {
                SetDesiredEndpoint(packet.GetPersistentId());
            }
        }

        const uint32_t totalBufferSize = packet.GetSize();

        // Messages can be larger than the size of a packet so reserve a buffer for the total message size
        if (m_tmpInboundBufferPos == 0)
        {
            m_tmpInboundBuffer.reserve(totalBufferSize);
        }

        // Read as much data as the packet can include and append it to the buffer
        const uint32_t readSize = AZStd::min(totalBufferSize - m_tmpInboundBufferPos, Neighborhood::NeighborBufferSize);
        memcpy(m_tmpInboundBuffer.begin() + m_tmpInboundBufferPos, packet.GetMessageBuffer().GetBuffer(), readSize);
        m_tmpInboundBufferPos += readSize;
        if (m_tmpInboundBufferPos == totalBufferSize)
        {
            AZ::SerializeContext* serializeContext;
            EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);

            // Deserialize the complete buffer
            AZ::IO::MemoryStream msgBuffer(m_tmpInboundBuffer.data(), totalBufferSize, totalBufferSize);
            AzFramework::ToolingMessage* msg = nullptr;
            AZ::ObjectStream::ClassReadyCB readyCB(AZStd::bind(
                &ToolingConnectionSystemComponent::OnMessageParsed, this, &msg, AZStd::placeholders::_1, AZStd::placeholders::_2,
                AZStd::placeholders::_3));
            AZ::ObjectStream::LoadBlocking(
                &msgBuffer, *serializeContext, readyCB,
                AZ::ObjectStream::FilterDescriptor(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));

            // Append to the inbox for handling
            if (msg)
            {
                if (msg->GetCustomBlobSize() > 0)
                {
                    void* blob = azmalloc(msg->GetCustomBlobSize(), 1, AZ::OSAllocator, "TmMsgBlob");
                    msgBuffer.Read(msg->GetCustomBlobSize(), blob);
                    msg->AddCustomBlob(blob, msg->GetCustomBlobSize(), true);
                }
                msg->SetSenderTargetId(packet.GetPersistentId());

                m_inboxMutex.lock();
                m_inbox.push_back(msg);
                m_inboxMutex.unlock();
                m_tmpInboundBuffer.clear();
                m_tmpInboundBufferPos = 0;
            }
        }

        return true;
    }

    AzNetworking::ConnectResult ToolingConnectionSystemComponent::ValidateConnect(
        [[maybe_unused]] const AzNetworking::IpAddress& remoteAddress,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] AzNetworking::ISerializer& serializer)
    {
        return AzNetworking::ConnectResult::Accepted;
    }

    void ToolingConnectionSystemComponent::OnConnect([[maybe_unused]] AzNetworking::IConnection* connection)
    {
        // Invoked when a tooling connection is established, handshake logic is handled via TargetConnect message
        ;
    }

    AzNetworking::PacketDispatchResult ToolingConnectionSystemComponent::OnPacketReceived(
        AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] AzNetworking::ISerializer& serializer)
    {
        if (!ToolingCidrFilter.IsMatch(connection->GetRemoteAddress()))
        {
            // Only IPs within the Cidr filter is valid
            return AzNetworking::PacketDispatchResult::Skipped;
        }

        return ToolingConnectionPackets::DispatchPacket(connection, packetHeader, serializer, *this);
    }

    void ToolingConnectionSystemComponent::OnPacketLost(
        [[maybe_unused]] AzNetworking::IConnection* connection, [[maybe_unused]] AzNetworking::PacketId packetId)
    {
        ;
    }

    void ToolingConnectionSystemComponent::OnDisconnect(
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] AzNetworking::DisconnectReason reason,
        [[maybe_unused]] AzNetworking::TerminationEndpoint endpoint)
    {
        // If our desired target has left the network, flag it and notify listeners
        if (reason != AzNetworking::DisconnectReason::ConnectionRejected)
        {
            AzFramework::ToolingEndpointContinaer::iterator it = m_availableTargets.find(static_cast<AZ::u32>(connection->GetConnectionId()));
            if (it != m_availableTargets.end())
            {
                AzFramework::ToolingEndpointInfo ti = it->second;
                m_availableTargets.erase(it);
                //TODO EBUS_QUEUE_EVENT(TargetManagerClient::Bus, TargetLeftNetwork, ti);
            }
        }
    }

} // namespace ToolingConnection
