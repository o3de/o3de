/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RemoteToolsSystemComponent.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/ObjectStream.h>

#include <AzNetworking/Framework/INetworking.h>
#include <AzNetworking/Utilities/CidrAddress.h>

#include <Source/AutoGen/RemoteTools.AutoPackets.h>
#include <Source/AutoGen/RemoteTools.AutoPacketDispatcher.h>

namespace RemoteTools
{
    static constexpr const char* RemoteServerAddress = "127.0.0.1";
    // id for the local application
    static constexpr AZ::u32 SelfNetworkId = 0xFFFFFFFF;

    AZ_CVAR(uint16_t, remote_outbox_interval, 50, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The interval to process outbound messages.");
    AZ_CVAR(uint16_t, remote_join_interval, 1000, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The interval to attempt automatic connecitons.");
    

    void RemoteToolsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RemoteToolsSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<RemoteToolsSystemComponent>("RemoteTools", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void RemoteToolsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RemoteToolsService"));
    }

    void RemoteToolsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RemoteToolsService"));
    }

    void RemoteToolsSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        ;
    }

    void RemoteToolsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        ;
    }

    RemoteToolsSystemComponent::RemoteToolsSystemComponent()
    {
#if defined(ENABLE_REMOTE_TOOLS)
        if (AzFramework::RemoteToolsInterface::Get() == nullptr)
        {
            AzFramework::RemoteToolsInterface::Register(this);
        }
#endif
    }

    RemoteToolsSystemComponent::~RemoteToolsSystemComponent()
    {
#if defined(ENABLE_REMOTE_TOOLS)
        if (AzFramework::RemoteToolsInterface::Get() == this)
        {
            AzFramework::RemoteToolsInterface::Unregister(this);
        }
#endif
    }

    void RemoteToolsSystemComponent::Init()
    {
    }

    void RemoteToolsSystemComponent::Activate()
    {
        m_joinThread = AZStd::make_unique<RemoteToolsJoinThread>(remote_join_interval, this);
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void RemoteToolsSystemComponent::Deactivate()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        m_joinThread = nullptr;
        if (AzNetworking::INetworking* networking = AZ::Interface<AzNetworking::INetworking>::Get())
        {
            for (auto registryIt = m_entryRegistry.begin(); registryIt != m_entryRegistry.end(); ++registryIt)
            {
                networking->DestroyNetworkInterface(registryIt->second.m_name);
            }
        }
        m_entryRegistry.clear();
    }

    void RemoteToolsSystemComponent::OnSystemTick()
    {
        if (!m_messageTypesToClearForNextTick.empty())
        {
            for (const AZ::Crc32& key : m_messageTypesToClearForNextTick)
            {
                ClearReceivedMessages(key);
            }
            m_messageTypesToClearForNextTick.clear();
        }

        // Join thread can stop itself, check if it needs to join
        if (m_joinThread && !m_joinThread->IsRunning())
        {
            m_joinThread->Join();
        }
    }

    void RemoteToolsSystemComponent::RegisterToolingServiceClient(AZ::Crc32 key, AZ::Name name, uint16_t port)
    {
        if (!m_entryRegistry.contains(key))
        {
            m_entryRegistry[key] = RemoteToolsRegistryEntry();
        }
        m_entryRegistry[key].m_isHost = false;
        m_entryRegistry[key].m_name = name;
        m_entryRegistry[key].m_ip = AzNetworking::IpAddress(RemoteServerAddress, port, AzNetworking::ProtocolType::Tcp);

        if (AzNetworking::INetworking* networking = AZ::Interface<AzNetworking::INetworking>::Get())
        {
            AzNetworking::INetworkInterface* netInterface = networking->CreateNetworkInterface(
                name, AzNetworking::ProtocolType::Tcp, AzNetworking::TrustZone::ExternalClientToServer, *this);
            netInterface->SetTimeoutMs(AZ::TimeMs(0));
        }

        if (m_joinThread && !m_joinThread->IsRunning())
        {
            m_joinThread->Join();
            m_joinThread->Start();
        }
    }

    void RemoteToolsSystemComponent::RegisterToolingServiceHost(AZ::Crc32 key, AZ::Name name, uint16_t port)
    {
        if (!m_entryRegistry.contains(key))
        {
            m_entryRegistry[key] = RemoteToolsRegistryEntry();
        }
        m_entryRegistry[key].m_isHost = true;
        m_entryRegistry[key].m_name = name;

        AzFramework::RemoteToolsEndpointContainer::pair_iter_bool ret = m_entryRegistry[key].m_availableTargets.insert_key(key);
        AzFramework::RemoteToolsEndpointInfo& ti = ret.first->second;
        ti.SetInfo("Self", key, SelfNetworkId);

        if (AzNetworking::INetworking* networking = AZ::Interface<AzNetworking::INetworking>::Get())
        {
            AzNetworking::INetworkInterface* netInterface = networking->CreateNetworkInterface(
                name, AzNetworking::ProtocolType::Tcp, AzNetworking::TrustZone::ExternalClientToServer, *this);
            netInterface->SetTimeoutMs(AZ::TimeMs(0));
            netInterface->Listen(port);
        }
    }

    const AzFramework::ReceivedRemoteToolsMessages* RemoteToolsSystemComponent::GetReceivedMessages(AZ::Crc32 key) const
    {
        if (m_inbox.contains(key))
        {
            return &m_inbox.at(key);
        }
        return nullptr;
    }

    void RemoteToolsSystemComponent::ClearReceivedMessages(AZ::Crc32 key)
    {
        if (m_inbox.contains(key))
        {
            m_inbox.at(key).clear();
        }
    }

    void RemoteToolsSystemComponent::ClearReceivedMessagesForNextTick(AZ::Crc32 key)
    {
        m_messageTypesToClearForNextTick.insert(key);
    }

    void RemoteToolsSystemComponent::RegisterRemoteToolsEndpointJoinedHandler(
        AZ::Crc32 key, AzFramework::RemoteToolsEndpointStatusEvent::Handler& handler)
    {
        if (!m_entryRegistry.contains(key))
        {
            m_entryRegistry[key] = RemoteToolsRegistryEntry();
        }
        handler.Connect(m_entryRegistry[key].m_endpointJoinedEvent);
    }

    void RemoteToolsSystemComponent::RegisterRemoteToolsEndpointLeftHandler(
        AZ::Crc32 key, AzFramework::RemoteToolsEndpointStatusEvent::Handler& handler)
    {
        if (!m_entryRegistry.contains(key))
        {
            m_entryRegistry[key] = RemoteToolsRegistryEntry();
        }
        handler.Connect(m_entryRegistry[key].m_endpointLeftEvent);
    }

    void RemoteToolsSystemComponent::RegisterRemoteToolsEndpointConnectedHandler(
        AZ::Crc32 key, AzFramework::RemoteToolsEndpointConnectedEvent::Handler& handler)
    {
        if (!m_entryRegistry.contains(key))
        {
            m_entryRegistry[key] = RemoteToolsRegistryEntry();
        }
        handler.Connect(m_entryRegistry[key].m_endpointConnectedEvent);
    }

    void RemoteToolsSystemComponent::RegisterRemoteToolsEndpointChangedHandler(
        AZ::Crc32 key, AzFramework::RemoteToolsEndpointChangedEvent::Handler& handler)
    {
        if (!m_entryRegistry.contains(key))
        {
            m_entryRegistry[key] = RemoteToolsRegistryEntry();
        }
        handler.Connect(m_entryRegistry[key].m_endpointChangedEvent);
    }

    void RemoteToolsSystemComponent::EnumTargetInfos(AZ::Crc32 key, AzFramework::RemoteToolsEndpointContainer& infos)
    {
        if (m_entryRegistry.contains(key))
        {
            infos = m_entryRegistry[key].m_availableTargets;
        }
        else
        {
            infos = AzFramework::RemoteToolsEndpointContainer();
        }
    }

    void RemoteToolsSystemComponent::SetDesiredEndpoint(AZ::Crc32 key, AZ::u32 desiredTargetID)
    {
        AZ_TracePrintf("RemoteToolsSystemComponent", "Set Target - %u", desiredTargetID);
        if (m_entryRegistry.contains(key))
        {
            RemoteToolsRegistryEntry& entry = m_entryRegistry[key];
            if (desiredTargetID != entry.m_lastTarget.GetPersistentId())
            {
                AzFramework::RemoteToolsEndpointInfo ti = GetEndpointInfo(key, desiredTargetID);
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

    void RemoteToolsSystemComponent::SetDesiredEndpointInfo(AZ::Crc32 key, const AzFramework::RemoteToolsEndpointInfo& targetInfo)
    {
        SetDesiredEndpoint(key, targetInfo.GetPersistentId());
    }

    AzFramework::RemoteToolsEndpointInfo RemoteToolsSystemComponent::GetDesiredEndpoint(AZ::Crc32 key)
    {
        if (m_entryRegistry.contains(key))
        {
            return m_entryRegistry[key].m_lastTarget;
        }

        return AzFramework::RemoteToolsEndpointInfo(); // return an invalid target info.
    }

    AzFramework::RemoteToolsEndpointInfo RemoteToolsSystemComponent::GetEndpointInfo(AZ::Crc32 key, AZ::u32 desiredTargetID)
    {
        if (m_entryRegistry.contains(key))
        {
            AzFramework::RemoteToolsEndpointContainer container = m_entryRegistry[key].m_availableTargets;
            AzFramework::RemoteToolsEndpointContainer::const_iterator finder = container.find(desiredTargetID);
            if (finder != container.end())
            {
                return finder->second;
            }
        }

        return AzFramework::RemoteToolsEndpointInfo(); // return an invalid target info.
    }

    bool RemoteToolsSystemComponent::IsEndpointOnline(AZ::Crc32 key, AZ::u32 desiredTargetID)
    {
        if (m_entryRegistry.contains(key))
        {
            AzFramework::RemoteToolsEndpointContainer container = m_entryRegistry[key].m_availableTargets;
            AzFramework::RemoteToolsEndpointContainer::const_iterator finder = container.find(desiredTargetID);
            if (finder != container.end())
            {
                return finder->second.IsOnline();
            }
        }

        return false;
    }

    void RemoteToolsSystemComponent::SendRemoteToolsMessage(
        const AzFramework::RemoteToolsEndpointInfo& target, const AzFramework::RemoteToolsMessage& msg)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        AZStd::vector<char, AZ::OSStdAllocator> msgBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char, AZ::OSStdAllocator>> outMsg(&msgBuffer);

        AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&outMsg, *serializeContext, AZ::ObjectStream::ST_BINARY);
        objStream->WriteClass(&msg);
        if (!objStream->Finalize())
        {
            AZ_Assert(false, "ObjectStream failed to serialize outbound TmMsg!");
        }

        const size_t customBlobSize = msg.GetCustomBlobSize();
        if (customBlobSize > 0)
        {
            outMsg.Write(customBlobSize, msg.GetCustomBlob().data());
        }

        // Messages targeted at our own application are also serialized/deserialized then moved onto the inbox
        const size_t totalSize = msgBuffer.size();
        if (target.IsSelf())
        {
            AZStd::vector<AZStd::byte> buffer;
            buffer.reserve(static_cast<uint32_t>(totalSize));
            memcpy(buffer.begin(), msgBuffer.data(), totalSize);
            AzFramework::RemoteToolsMessage* result = DeserializeMessage(target.GetPersistentId(), buffer, static_cast<uint32_t>(totalSize));

            m_inboxMutex.lock();
            if (!m_inbox[target.GetPersistentId()].full())
            {
                m_inbox[target.GetPersistentId()].push_back(result);
            }
            else
            {
                // As no network latency and not bound to framerate with local messages, possible to overflow the inbox size
                AZ_Error("RemoteTool", false, "Inbox is full, a local message got skipped on %d channel", target.GetPersistentId());
                delete result;
            }
            m_inboxMutex.unlock();

            return;
        }

        AzNetworking::INetworkInterface* networkInterface =
            AZ::Interface<AzNetworking::INetworking>::Get()->RetrieveNetworkInterface(
                AZ::Name(m_entryRegistry[target.GetPersistentId()].m_name));

        auto connectionId = static_cast<AzNetworking::ConnectionId>(target.GetNetworkId());
        const uint8_t* outBuffer = reinterpret_cast<const uint8_t*>(msgBuffer.data());
        size_t outSize = totalSize;
        while (outSize > 0 && networkInterface != nullptr)
        {
            // Fragment the message into RemoteToolsMessageBuffer packet sized chunks and send
            size_t bufferSize = AZStd::min(outSize, aznumeric_cast<size_t>(RemoteToolsBufferSize));
            RemoteToolsPackets::RemoteToolsMessage tmPacket;
            tmPacket.SetPersistentId(target.GetPersistentId());
            tmPacket.SetSize(aznumeric_cast<uint32_t>(totalSize));
            RemoteToolsMessageBuffer encodingBuffer;
            encodingBuffer.CopyValues(outBuffer + (totalSize - outSize), bufferSize);
            tmPacket.SetMessageBuffer(encodingBuffer);

            if (!networkInterface->SendReliablePacket(connectionId, tmPacket))
            {
                AZ_Error("RemoteToolsSystemComponent", false, "SendReliablePacket failed with remaining bytes %zu of %zu.\n", outSize, totalSize);
                break;
            }

            outSize -= bufferSize;
        }

    }

    void RemoteToolsSystemComponent::OnMessageParsed(
        AzFramework::RemoteToolsMessage** ppMsg, void* classPtr, const AZ::Uuid& classId, const AZ::SerializeContext* sc)
    {
        // Check that classPtr is a RemoteToolsMessage
        AZ_Assert(*ppMsg == nullptr, "ppMsg is already set! are we deserializing multiple messages in one call?");
        *ppMsg = sc->Cast<AzFramework::RemoteToolsMessage*>(classPtr, classId);
        AZ_Assert(*ppMsg, "Failed to downcast msg pointer to a TmMsg. Is RTTI and reflection set up properly?");
    }

    bool RemoteToolsSystemComponent::HandleRequest(
        AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        const RemoteToolsPackets::RemoteToolsConnect& packet)
    {
        AZ::Crc32 key = packet.GetPersistentId();

        if (m_entryRegistry.contains(key))
        {
            const uint32_t persistentId = packet.GetPersistentId();
            AzFramework::RemoteToolsEndpointContainer::pair_iter_bool ret =
                m_entryRegistry[key].m_availableTargets.insert_key(persistentId);
            AzFramework::RemoteToolsEndpointInfo& ti = ret.first->second;
            ti.SetInfo(packet.GetDisplayName(), persistentId, static_cast<uint32_t>(connection->GetConnectionId()));
            m_entryRegistry[key].m_endpointJoinedEvent.Signal(ti);
        }
        return true;
    }

    bool RemoteToolsSystemComponent::HandleRequest(
        AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] const RemoteToolsPackets::RemoteToolsMessage& packet)
    {
        const AZ::Crc32 key = packet.GetPersistentId();

        // Receive
        if (connection->GetConnectionRole() == AzNetworking::ConnectionRole::Acceptor
            && static_cast<uint32_t>(connection->GetConnectionId()) != m_entryRegistry[key].m_lastTarget.GetNetworkId())
        {
            // Listener should route traffic based on selected target
            return true;
        }
      
        if (!m_entryRegistry.contains(key))
        {
            m_entryRegistry[key] = RemoteToolsRegistryEntry();
        }

        // If we're a client, set the host to our desired target
        if (connection->GetConnectionRole() == AzNetworking::ConnectionRole::Connector)
        {
            if (GetEndpointInfo(key, packet.GetPersistentId()).GetPersistentId() == 0)
            {
                AzFramework::RemoteToolsEndpointContainer::pair_iter_bool ret =
                    m_entryRegistry[key].m_availableTargets.insert_key(packet.GetPersistentId());

                AzFramework::RemoteToolsEndpointInfo& ti = ret.first->second;
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
        const uint32_t readSize = AZStd::min(totalBufferSize - m_entryRegistry[key].m_tmpInboundBufferPos, RemoteToolsBufferSize);
        memcpy(
            m_entryRegistry[key].m_tmpInboundBuffer.begin() + m_entryRegistry[key].m_tmpInboundBufferPos,
            packet.GetMessageBuffer().GetBuffer(), readSize);
        m_entryRegistry[key].m_tmpInboundBufferPos += readSize;
        if (m_entryRegistry[key].m_tmpInboundBufferPos == totalBufferSize)
        {
            AzFramework::RemoteToolsMessage* msg = DeserializeMessage(key, m_entryRegistry[key].m_tmpInboundBuffer, totalBufferSize);
            m_entryRegistry[key].m_tmpInboundBuffer.clear();
            m_entryRegistry[key].m_tmpInboundBufferPos = 0;

            // Append to the inbox for handling
            if (msg)
            {
                m_inboxMutex.lock();
                m_inbox[msg->GetSenderTargetId()].push_back(msg);
                m_inboxMutex.unlock();
            }
        }

        return true;
    }

    AzNetworking::ConnectResult RemoteToolsSystemComponent::ValidateConnect(
        [[maybe_unused]] const AzNetworking::IpAddress& remoteAddress,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] AzNetworking::ISerializer& serializer)
    {
        return AzNetworking::ConnectResult::Accepted;
    }

    void RemoteToolsSystemComponent::OnConnect([[maybe_unused]] AzNetworking::IConnection* connection)
    {
        // Invoked when a tooling connection is established, handshake logic is handled via ToolingConnect message
        ;
    }

    AzNetworking::PacketDispatchResult RemoteToolsSystemComponent::OnPacketReceived(
        AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] AzNetworking::ISerializer& serializer)
    {
        return RemoteToolsPackets::DispatchPacket(connection, packetHeader, serializer, *this);
    }

    void RemoteToolsSystemComponent::OnPacketLost(
        [[maybe_unused]] AzNetworking::IConnection* connection, [[maybe_unused]] AzNetworking::PacketId packetId)
    {
        ;
    }

    void RemoteToolsSystemComponent::OnDisconnect(
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] AzNetworking::DisconnectReason reason,
        [[maybe_unused]] AzNetworking::TerminationEndpoint endpoint)
    {
        // If our desired target has left the network, flag it and notify listeners
        if (reason != AzNetworking::DisconnectReason::ConnectionRejected)
        {
            for (auto registryIt = m_entryRegistry.begin(); registryIt != m_entryRegistry.end(); ++registryIt)
            {
                AzFramework::RemoteToolsEndpointContainer& container = registryIt->second.m_availableTargets;
                for (auto endpointIt = container.begin(); endpointIt != container.end(); ++endpointIt)
                {
                    if (endpointIt->second.GetNetworkId() == static_cast<AZ::u32>(connection->GetConnectionId()))
                    {
                        AzFramework::RemoteToolsEndpointInfo ti = endpointIt->second;
                        registryIt->second.m_endpointLeftEvent.Signal(ti);
                        container.extract(endpointIt);
                        break;
                    }
                }
            }
        }

        if (m_joinThread && !m_joinThread->IsRunning())
        {
            m_joinThread->Join();
            m_joinThread->Start();
        }
    }

    AzFramework::RemoteToolsMessage* RemoteToolsSystemComponent::DeserializeMessage(
        const AZ::Crc32& key, AZStd::vector<AZStd::byte>& buffer, const uint32_t& totalBufferSize)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        // Deserialize the complete buffer
        AZ::IO::MemoryStream msgBuffer(buffer.data(), totalBufferSize, totalBufferSize);
        AzFramework::RemoteToolsMessage* msg = nullptr; 
        AZ::ObjectStream::ClassReadyCB readyCB(AZStd::bind(
            &RemoteToolsSystemComponent::OnMessageParsed, this, &msg, AZStd::placeholders::_1, AZStd::placeholders::_2,
            AZStd::placeholders::_3));
        AZ::ObjectStream::LoadBlocking(
            &msgBuffer, *serializeContext, readyCB,
            AZ::ObjectStream::FilterDescriptor(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));

        if (msg)
        {
            if (msg->GetCustomBlobSize() > 0)
            {
                void* blob = azmalloc(msg->GetCustomBlobSize(), 1, AZ::OSAllocator);
                msgBuffer.Read(msg->GetCustomBlobSize(), blob);
                msg->AddCustomBlob(blob, msg->GetCustomBlobSize(), true);
            }
            msg->SetSenderTargetId(key);
        }

        return msg;
    }

} // namespace RemoteTools
