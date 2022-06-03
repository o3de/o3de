
#include <ToolingConnectionSystemComponent.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/ObjectStream.h>

#include <AzNetworking/Framework/INetworking.h>
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
        if (AzFramework::ToolsConnectionInterface::Get() == nullptr)
        {
            AzFramework::ToolsConnectionInterface::Register(this);
        }
    }

    ToolingConnectionSystemComponent::~ToolingConnectionSystemComponent()
    {
        if (AzFramework::ToolsConnectionInterface::Get() == this)
        {
            AzFramework::ToolsConnectionInterface::Unregister(this);
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
    }

    AzFramework::ToolingServiceKey ToolingConnectionSystemComponent::RegisterToolingService(AZStd::string name, uint16_t port)
    {
        AzFramework::ToolingServiceKey key = AZ::Crc32(name + AZStd::string::format("%d", port));
        m_entryRegistry[key] = ToolingRegistryEntry();
        m_entryRegistry[key].m_name = name;
        m_entryRegistry[key].m_port = port;

        return key;
    }

    const AzFramework::ReceivedToolingMessages* ToolingConnectionSystemComponent::GetReceivedMessages(AzFramework::ToolingServiceKey key) const
    {
        if (m_inbox.contains(key))
        {
            return &m_inbox.at(key);
        }
        return nullptr;
    }

    void ToolingConnectionSystemComponent::ClearReceivedMessages(AzFramework::ToolingServiceKey key)
    {
        if (m_inbox.contains(key))
        {
            m_inbox.at(key).clear();
        }
    }

    void ToolingConnectionSystemComponent::RegisterToolingEndpointJoinedHandler(
        AzFramework::ToolingServiceKey key, AzFramework::ToolingEndpointStatusEvent::Handler handler)
    {
        handler.Connect(m_entryRegistry[key].m_endpointJoinedEvent);
    }

    void ToolingConnectionSystemComponent::RegisterToolingEndpointLeftHandler(
        AzFramework::ToolingServiceKey key, AzFramework::ToolingEndpointStatusEvent::Handler handler)
    {
        handler.Connect(m_entryRegistry[key].m_endpointLeftEvent);
    }

    void ToolingConnectionSystemComponent::RegisterToolingEndpointConnectedHandler(
        AzFramework::ToolingServiceKey key, AzFramework::ToolingEndpointConnectedEvent::Handler handler)
    {
        handler.Connect(m_entryRegistry[key].m_endpointConnectedEvent);
    }

    void ToolingConnectionSystemComponent::RegisterToolingEndpointChangedHandler(
        AzFramework::ToolingServiceKey key, AzFramework::ToolingEndpointChangedEvent::Handler handler)
    {
        handler.Connect(m_entryRegistry[key].m_endpointChangedEvent);
    }

    void ToolingConnectionSystemComponent::EnumTargetInfos(AzFramework::ToolingServiceKey key, AzFramework::ToolingEndpointContainer& infos)
    {
        if (m_entryRegistry.contains(key))
        {
            infos = m_entryRegistry[key].m_availableTargets;
        }
        else
        {
            infos = AzFramework::ToolingEndpointContainer();
        }
    }

    void ToolingConnectionSystemComponent::SetDesiredEndpoint(AzFramework::ToolingServiceKey key, AZ::u32 desiredTargetID)
    {
        AZ_TracePrintf("ToolingConnectionSystemComponent", "Set Target - %u", desiredTargetID);
        if (m_entryRegistry.contains(key))
        {
            ToolingRegistryEntry& entry = m_entryRegistry[key];
            if (desiredTargetID != entry.m_lastTarget.GetPersistentId())
            {
                AzFramework::ToolingEndpointInfo ti = GetEndpointInfo(key, desiredTargetID);
                AZ::u32 oldTargetID = entry.m_lastTarget.GetPersistentId();
                entry.m_lastTarget = ti;
                entry.m_tmpInboundBuffer.clear();
                entry.m_tmpInboundBufferPos = 0;

                 m_entryRegistry[key].m_endpointChangedEvent.Signal(desiredTargetID, oldTargetID);


                if (ti.IsValid() && ti.IsOnline())
                {
                    m_entryRegistry[key].m_endpointConnectedEvent.Signal(true);
                }
                else
                {
                    m_entryRegistry[key].m_endpointConnectedEvent.Signal(false);
                }
            }
        }
    }

    void ToolingConnectionSystemComponent::SetDesiredEndpointInfo(AzFramework::ToolingServiceKey key, const AzFramework::ToolingEndpointInfo& targetInfo)
    {
        SetDesiredEndpoint(key, targetInfo.GetPersistentId());
    }

    AzFramework::ToolingEndpointInfo ToolingConnectionSystemComponent::GetDesiredEndpoint(AzFramework::ToolingServiceKey key)
    {
        if (m_entryRegistry.contains(key))
        {
            return m_entryRegistry[key].m_lastTarget;
        }

        return AzFramework::ToolingEndpointInfo(); // return an invalid target info.
    }

    AzFramework::ToolingEndpointInfo ToolingConnectionSystemComponent::GetEndpointInfo(AzFramework::ToolingServiceKey key, AZ::u32 desiredTargetID)
    {
        if (m_entryRegistry.contains(key))
        {
            AzFramework::ToolingEndpointContainer container = m_entryRegistry[key].m_availableTargets;
            AzFramework::ToolingEndpointContainer::const_iterator finder = container.find(desiredTargetID);
            if (finder != container.end())
            {
                return finder->second;
            }
        }

        return AzFramework::ToolingEndpointInfo(); // return an invalid target info.
    }

    bool ToolingConnectionSystemComponent::IsEndpointOnline(AzFramework::ToolingServiceKey key, AZ::u32 desiredTargetID)
    {
        if (m_entryRegistry.contains(key))
        {
            AzFramework::ToolingEndpointContainer container = m_entryRegistry[key].m_availableTargets;
            AzFramework::ToolingEndpointContainer::const_iterator finder = container.find(desiredTargetID);
            if (finder != container.end())
            {
                return finder->second.IsOnline();
            }
        }

        return false;
    }

    void ToolingConnectionSystemComponent::SendToolingMessage(
        const AzFramework::ToolingEndpointInfo& target, const AzFramework::ToolingMessage& msg)
    {
        AZ::SerializeContext* serializeContext;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);

        // Messages targeted at our own application just transfer right over to the inbox.
        if (target.IsSelf())
        {
            AzFramework::ToolingMessage* inboxMessage = static_cast<AzFramework::ToolingMessage*>(
                serializeContext->CloneObject(static_cast<const void*>(&msg), msg.RTTI_GetType()));
            AZ_Assert(inboxMessage, "Failed to clone local loopback message.");
            inboxMessage->SetSenderTargetId(target.GetPersistentId());

            if (msg.GetCustomBlobSize() > 0)
            {
                void* blob = azmalloc(msg.GetCustomBlobSize(), 16, AZ::OSAllocator);
                memcpy(blob, msg.GetCustomBlob(), msg.GetCustomBlobSize());
                inboxMessage->AddCustomBlob(blob, msg.GetCustomBlobSize(), true);
            }

            m_inboxMutex.lock();
            m_inbox[msg.GetSenderTargetId()].push_back(inboxMessage);
            m_inboxMutex.unlock();

            return;
        }

        AZStd::vector<char, AZ::OSStdAllocator> msgBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char, AZ::OSStdAllocator>> outMsg(&msgBuffer);

        AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&outMsg, *serializeContext, AZ::ObjectStream::ST_BINARY);
        objStream->WriteClass(&msg);
        if (!objStream->Finalize())
        {
            AZ_Assert(false, "ObjectStream failed to serialize outbound TmMsg!");
        }

        size_t customBlobSize = msg.GetCustomBlobSize();
        if (msg.GetCustomBlobSize() > 0)
        {
            outMsg.Write(customBlobSize, msg.GetCustomBlob());
        }

        AzNetworking::INetworkInterface* networkInterface =
            AZ::Interface<AzNetworking::INetworking>::Get()->RetrieveNetworkInterface(
                AZ::Name(AZStd::string::format("%d", target.GetPersistentId())));

        OutboundToolingDatum datum;
        datum.first = target.GetPersistentId();
        datum.second.swap(msgBuffer);
        m_outboxThread->PushOutboxMessage(
            networkInterface, static_cast<AzNetworking::ConnectionId>(target.GetNetworkId()), AZStd::move(datum));
    }

    void ToolingConnectionSystemComponent::OnMessageParsed(
        AzFramework::ToolingMessage** ppMsg, void* classPtr, const AZ::Uuid& classId, const AZ::SerializeContext* sc)
    {
        // Check that classPtr is a ToolingMessage
        AZ_Assert(*ppMsg == nullptr, "ppMsg is already set! are we deserializing multiple messages in one call?");
        *ppMsg = sc->Cast<AzFramework::ToolingMessage*>(classPtr, classId);
        AZ_Assert(*ppMsg, "Failed to downcast msg pointer to a TmMsg. Is RTTI and reflection set up properly?");
    }

    bool ToolingConnectionSystemComponent::HandleRequest(
        AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        const ToolingConnectionPackets::ToolingConnect& packet)
    {
        AzNetworking::ByteOrder byteOrder = connection->GetConnectionRole() == AzNetworking::ConnectionRole::Acceptor
            ? AzNetworking::ByteOrder::Host
            : AzNetworking::ByteOrder::Network;
        AzFramework::ToolingServiceKey key = connection->GetRemoteAddress().GetPort(byteOrder);

        if (m_entryRegistry.contains(key))
        {
            const uint32_t persistentId = packet.GetPersistentId();
            AzFramework::ToolingEndpointContainer::pair_iter_bool ret =
                m_entryRegistry[key].m_availableTargets.insert_key(persistentId);
            AzFramework::ToolingEndpointInfo& ti = ret.first->second;
            ti.SetInfo(packet.GetDisplayName(), persistentId, static_cast<uint32_t>(connection->GetConnectionId()));
            m_entryRegistry[key].m_endpointJoinedEvent.Signal(ti);
        }
        return true;
    }

    bool ToolingConnectionSystemComponent::HandleRequest(
        AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] const ToolingConnectionPackets::ToolingPacket& packet)
    {
        AzFramework::ToolingServiceKey key = packet.GetPersistentId();

        // Receive
        if (connection->GetConnectionRole() == AzNetworking::ConnectionRole::Acceptor
            && static_cast<uint32_t>(connection->GetConnectionId()) != m_entryRegistry[key].m_lastTarget.GetNetworkId())
        {
            // Listener should route traffic based on selected target
            return true;
        }
      
        if (!m_entryRegistry.contains(key))
        {
            m_entryRegistry[key] = ToolingRegistryEntry();
        }

        // If we're a client, set the host to our desired target
        if (connection->GetConnectionRole() == AzNetworking::ConnectionRole::Connector)
        {
            if (GetEndpointInfo(key, packet.GetPersistentId()).GetPersistentId() == 0)
            {
                AzFramework::ToolingEndpointContainer::pair_iter_bool ret =
                    m_entryRegistry[key].m_availableTargets.insert_key(packet.GetPersistentId());

                AzFramework::ToolingEndpointInfo& ti = ret.first->second;
                ti.SetInfo("Host", packet.GetPersistentId(), static_cast<uint32_t>(connection->GetConnectionId()));
                m_entryRegistry[key].m_endpointJoinedEvent.Signal(ti);
            }

            if (GetDesiredEndpoint(key).GetPersistentId() != packet.GetPersistentId())
            {
                SetDesiredEndpoint(key, packet.GetPersistentId());
            }
        }

        const uint32_t totalBufferSize = packet.GetSize();

        // Messages can be larger than the size of a packet so reserve a buffer for the total message size
        if (m_entryRegistry[key].m_tmpInboundBufferPos == 0)
        {
            m_entryRegistry[key].m_tmpInboundBuffer.reserve(totalBufferSize);
        }

        // Read as much data as the packet can include and append it to the buffer
        const uint32_t readSize =
            AZStd::min(totalBufferSize - m_entryRegistry[key].m_tmpInboundBufferPos, Neighborhood::NeighborBufferSize);
        memcpy(
            m_entryRegistry[key].m_tmpInboundBuffer.begin() + m_entryRegistry[key].m_tmpInboundBufferPos,
            packet.GetMessageBuffer().GetBuffer(), readSize);
        m_entryRegistry[key].m_tmpInboundBufferPos += readSize;
        if (m_entryRegistry[key].m_tmpInboundBufferPos == totalBufferSize)
        {
            AZ::SerializeContext* serializeContext;
            EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);

            // Deserialize the complete buffer
            AZ::IO::MemoryStream msgBuffer(m_entryRegistry[key].m_tmpInboundBuffer.data(), totalBufferSize, totalBufferSize);
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
                m_inbox[msg->GetSenderTargetId()].push_back(msg);
                m_inboxMutex.unlock();
                m_entryRegistry[key].m_tmpInboundBuffer.clear();
                m_entryRegistry[key].m_tmpInboundBufferPos = 0;
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
        // Invoked when a tooling connection is established, handshake logic is handled via ToolingConnect message
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
            for (auto registryIt = m_entryRegistry.begin(); registryIt != m_entryRegistry.end(); ++registryIt)
            {
                AzFramework::ToolingEndpointContainer& container = registryIt->second.m_availableTargets;
                for (auto endpointIt = container.begin(); endpointIt != container.end(); ++endpointIt)
                {
                    if (endpointIt->second.GetNetworkId() == static_cast<AZ::u32>(connection->GetConnectionId()))
                    {
                        AzFramework::ToolingEndpointInfo ti = endpointIt->second;
                        container.erase(endpointIt);
                        registryIt->second.m_endpointLeftEvent.Signal(ti);
                    }
                }
            }
        }
    }

} // namespace ToolingConnection
