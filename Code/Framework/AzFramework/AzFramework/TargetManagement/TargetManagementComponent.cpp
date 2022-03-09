/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/AutoGen/AzFramework.AutoPackets.h>
#include <AzFramework/AutoGen/AzFramework.AutoPacketDispatcher.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzFramework/TargetManagement/NeighborhoodAPI.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/Debug/Profiler.h>
#include <AzNetworking/Framework/INetworkInterface.h>
#include <AzNetworking/Framework/INetworking.h>
#include <time.h>

namespace AzFramework
{
    static constexpr const char* TargetServerAddress = "127.0.0.1";

    AZ_CVAR(bool, target_autoconnect, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Enable autoconnecting to target host.");
    AZ_CVAR(uint16_t, target_autoconnect_interval, 1000, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The interval to attempt automatic connecitons.");
    AZ_CVAR(uint16_t, target_port, DefaultTargetPort, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "The port that target management will bind to for traffic.");
    namespace Platform
    {
        AZStd::string GetPersistentName();
        AZStd::string GetNeighborhoodName();
    }

    bool TargetInfo::IsSelf() const
    {
        return m_flags & TF_SELF && m_networkId == k_selfNetworkId;
    }

    bool TargetInfo::IsValid() const
    {
        return m_lastSeen != 0;
    }

    const char* TargetInfo::GetDisplayName() const
    {
        return m_displayName.c_str();
    }

    AZ::u32 TargetInfo::GetPersistentId() const
    {
        return m_persistentId;
    }

    int TargetInfo::GetNetworkId() const
    {
        return m_networkId;
    }

    AZ::u32 TargetInfo::GetStatusFlags() const
    {
        return m_flags;
    }

    bool TargetInfo::IsIdentityEqualTo(const TargetInfo& other) const
    {
        return m_persistentId == other.m_persistentId;
    }

    struct TargetManagementSettings : public AZ::UserSettings
    {
        AZ_CLASS_ALLOCATOR(TargetManagementSettings, AZ::OSAllocator, 0);
        AZ_RTTI(TargetManagementSettings, "{AB1B14BB-C0E3-484A-B498-98F44A58C161}", AZ::UserSettings);

        TargetManagementSettings()
        {
        }

        ~TargetManagementSettings() override
        {
        }

        AZStd::string m_neighborhoodName; // this is the neighborhood session (hub) we want to connect to
        AZStd::string m_persistentName; // this string is used as the persistent name for this target
        TargetInfo m_lastTarget; // this is the target we will automatically connect to
    };

    class TargetManagementNetworkImpl
        : public Neighborhood::NeighborhoodBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(TargetManagementNetworkImpl, AZ::SystemAllocator, 0);

        TargetManagementNetworkImpl(TargetManagementComponent* component)
            : m_component(component)
        {
            Neighborhood::NeighborhoodBus::Handler::BusConnect();
        }

        ~TargetManagementNetworkImpl() override
        {
            Neighborhood::NeighborhoodBus::Handler::BusDisconnect();
        }

        // neighborhood bus
        void OnNodeJoined(const AzFrameworkPackets::Neighbor& node) override
        {
            const AZ::u32 capabilities = node.GetCapabilities();
            const AZ::u32 memberId = node.GetPersistentId();
            const AZ::u32 persistentId = AZ::Crc32(node.GetPersistentName());
            AZ_TracePrintf("Neighborhood", "Discovered node with 0x%x from member %d.\n", capabilities, static_cast<int>(memberId));

            TargetContainer::pair_iter_bool ret = m_component->m_availableTargets.insert_key(static_cast<AZ::u32>(memberId));
            TargetInfo& ti = ret.first->second;

            ti.m_lastSeen = time(nullptr);
            ti.m_displayName = node.GetDisplayName();
            ti.m_persistentId = persistentId;
            ti.m_networkId = memberId;
            ti.m_flags |= TF_ONLINE;
            if (capabilities & Neighborhood::NEIGHBOR_CAP_LUA_VM)
            {
                ti.m_flags |= TF_DEBUGGABLE | TF_RUNNABLE;
            }
            else
            {
                ti.m_flags &= ~(TF_DEBUGGABLE | TF_RUNNABLE);
            }
            EBUS_QUEUE_EVENT(TargetManagerClient::Bus, TargetJoinedNetwork, ti);

            // If our desired target has just come online, flag it and notify listeners
            if (persistentId == m_component->m_settings->m_lastTarget.m_persistentId &&
                (m_component->m_settings->m_lastTarget.m_flags & TF_ONLINE) == 0)
            {
                m_component->m_settings->m_lastTarget.m_flags |= TF_ONLINE;
                m_component->m_settings->m_lastTarget.m_networkId = memberId;
                m_component->m_settings->m_lastTarget.m_lastSeen = ti.m_lastSeen;
                EBUS_QUEUE_EVENT(TargetManagerClient::Bus, DesiredTargetConnected, true);
            }
        }

        void OnNodeLeft(const AzFrameworkPackets::Neighbor& node) override
        {
            AZ_TracePrintf("Neighborhood", "Lost contact with node from member %d.\n", static_cast<int>(node.GetPersistentId()));

            // If our desired target has left the network, flag it and notify listeners
            if (node.GetPersistentId() == m_component->m_settings->m_lastTarget.m_networkId)
            {
                m_component->m_settings->m_lastTarget.m_flags &= ~TF_ONLINE;
                EBUS_QUEUE_EVENT(TargetManagerClient::Bus, DesiredTargetConnected, false);
            }

            TargetContainer::iterator it = m_component->m_availableTargets.find(static_cast<AZ::u32>(node.GetPersistentId()));
            if (it != m_component->m_availableTargets.end())
            {
                TargetInfo ti = it->second;
                m_component->m_availableTargets.erase(it);
                ti.m_flags &= ~TF_ONLINE;
                EBUS_QUEUE_EVENT(TargetManagerClient::Bus, TargetLeftNetwork, ti);
            }
        }
        //////////////////////////////////////////////////////////////////////////

        TargetManagementComponent*              m_component;
    };

    const AZ::u32 k_nullNetworkId = static_cast<AZ::u32>(-1);

    TargetManagementComponent::TargetManagementComponent()
        : m_serializeContext(nullptr)
        , m_networkImpl(nullptr)
    {
        m_targetJoinThread = AZStd::make_unique<TargetJoinThread>(target_autoconnect_interval);
    }

    TargetManagementComponent::~TargetManagementComponent()
    {
        AZ_Assert(m_networkImpl == nullptr, "We still have networkImpl! Was Deactivate() called?");
        m_targetJoinThread = nullptr;
    }

    void TargetManagementComponent::Activate()
    {
        EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        {
            AZStd::string persistentName = Platform::GetPersistentName();

            AZStd::string targetManagementSettingsKey = AZStd::string::format("TargetManagementSettings::%s", persistentName.c_str());
            m_settings = AZ::UserSettings::CreateFind<TargetManagementSettings>(
                AZ::Crc32(targetManagementSettingsKey.c_str()), AZ::UserSettings::CT_GLOBAL);

            if (m_settings->m_persistentName.empty())
            {
                m_settings->m_persistentName = persistentName;
            }
        }

        if (m_settings->m_neighborhoodName.empty())
        {
            m_settings->m_neighborhoodName = Platform::GetNeighborhoodName();
        }

        // Always set our desired target to be initially offline
        m_settings->m_lastTarget.m_flags &= ~TF_ONLINE;

        TargetManager::Bus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();

        m_networkImpl = aznew TargetManagementNetworkImpl(this);
        m_stopRequested = false;
        AZStd::thread_desc td;
        td.m_name = "TargetManager Thread";
        td.m_cpuId = AFFINITY_MASK_USERTHREADS;
        m_threadHandle = AZStd::thread(td, AZStd::bind(&TargetManagementComponent::TickThread, this));

        m_networkInterface = AZ::Interface<AzNetworking::INetworking>::Get()->CreateNetworkInterface(
                AZ::Name("TargetManagement"), AzNetworking::ProtocolType::Tcp, AzNetworking::TrustZone::ExternalClientToServer, *this);
        m_networkInterface->SetTimeoutMs(AZ::TimeMs(0));
    }

    void TargetManagementComponent::Deactivate()
    {
        AZ::Interface<AzNetworking::INetworking>::Get()->DestroyNetworkInterface(AZ::Name("TargetManagement"));

        m_stopRequested = true;
        m_threadHandle.join();

        delete m_networkImpl;
        m_networkImpl = nullptr;

        AZ::SystemTickBus::Handler::BusDisconnect();
        TargetManager::Bus::Handler::BusDisconnect();
        TargetManagerClient::Bus::ExecuteQueuedEvents();    // This will deliver any pending messages and clear the queue.

        // delete any undelivered messages
        m_inbox.clear();
        m_outbox.clear();

        m_tmpInboundBuffer.clear();

        // Release the user settings
        m_settings = nullptr;
    }

    void TargetManagementComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TargetInfo>()
                ->Version(2)
                ->Field("m_displayName", &TargetInfo::m_displayName)
                ->Field("m_persistentId", &TargetInfo::m_persistentId)
                ->Field("m_flags", &TargetInfo::m_flags)
                ;

            serializeContext->Class<TargetManagementSettings>()
                ->Field("Hub", &TargetManagementSettings::m_neighborhoodName)
                ->Field("Name", &TargetManagementSettings::m_persistentName)
                ->Field("LastConnectedTarget", &TargetManagementSettings::m_lastTarget);

            serializeContext->Class<TmMsg>()
                ->Field("MsgId", &TmMsg::m_msgId)
                ->Field("BinaryBlobSize", &TmMsg::m_customBlobSize)
                ;

            serializeContext->Class<TargetManagementComponent, AZ::Component>()
                ->Version(2)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TargetManagementComponent>(
                    "Target Manager", "Provides socket based connectivity services to a target application")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Profiling")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }
    }

    void TargetManagementComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TargetManagerService", 0x6d5708bc));
    }

    void TargetManagementComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UserSettingsService", 0xa0eadff5));
        required.push_back(AZ_CRC("NetworkingService"));
    }

    void TargetManagementComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TargetManagerService", 0x6d5708bc));
    }

    void TargetManagementComponent::SetTargetAsHost(bool isHost)
    {
        m_isTargetHost = isHost;
    }

    void TargetManagementComponent::OnSystemTick()
    {
#if !defined(AZ_RELEASE_BUILD)
        // If we're not the host and not connected to one, attempt to connect on a fixed interval
        if (target_autoconnect && !m_isTargetHost && !m_targetJoinThread->IsRunning() &&
            m_networkInterface->GetConnectionSet().GetActiveConnectionCount() == 0)
        {
            m_targetJoinThread->Start();
        }
#endif

        size_t maxMsgsToProcess = m_inbox.size();
        for (size_t i = 0; i < maxMsgsToProcess; ++i)
        {
            TmMsgPtr msg;
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
                EBUS_EVENT_ID(msg->GetId(), TmMsgBus, OnReceivedMsg, msg);
            }
        }

        TargetManagerClient::Bus::ExecuteQueuedEvents();
    }

    bool TargetManagementComponent::HandleRequest(
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] const AzFrameworkPackets::Neighbor& packet)
    {
        if (packet.GetIsJoin())
        {
            EBUS_EVENT(Neighborhood::NeighborhoodBus, OnNodeJoined, packet);
        }
        else
        {
            EBUS_EVENT(Neighborhood::NeighborhoodBus, OnNodeLeft, packet);
        }
        return true;
    }

    bool TargetManagementComponent::HandleRequest(
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] const AzFrameworkPackets::TargetManagementMessage& packet)
    {
        // Receive
        TargetInfo ti = GetDesiredTarget();
        if (connection->GetConnectionRole() == AzNetworking::ConnectionRole::Acceptor &&
            ti.GetPersistentId() != m_settings->m_lastTarget.m_persistentId)
        {
            // Listener should route traffic based on selected target
            return true;
        }

        const uint32_t totalBufferSize = packet.GetSize();

        if(m_tmpInboundBuffer.empty())
        {
            m_tmpInboundBuffer.reserve(totalBufferSize);
            m_tmpInboundBufferPos = 0;
        }

        const uint32_t readSize = AZStd::min(totalBufferSize - m_tmpInboundBufferPos, AzNetworking::MaxPacketSize);
        memcpy(m_tmpInboundBuffer.begin() + m_tmpInboundBufferPos, packet.GetMessageBuffer().GetBuffer(), readSize);
        m_tmpInboundBufferPos += readSize;
        if (m_tmpInboundBufferPos == totalBufferSize)
        {
            AZ::IO::MemoryStream msgBuffer(m_tmpInboundBuffer.data(), totalBufferSize, totalBufferSize);
            TmMsg* msg = nullptr;
            AZ::ObjectStream::ClassReadyCB readyCB(AZStd::bind(&TargetManagementComponent::OnMsgParsed, this, &msg, AZStd::placeholders::_1,
                AZStd::placeholders::_2, AZStd::placeholders::_3));
            AZ::ObjectStream::LoadBlocking(&msgBuffer, *m_serializeContext, readyCB,
                AZ::ObjectStream::FilterDescriptor(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
            if (msg)
            {
                if (msg->GetCustomBlobSize() > 0)
                {
                    void* blob = azmalloc(msg->GetCustomBlobSize(), 1, AZ::OSAllocator, "TmMsgBlob");
                    msgBuffer.Read(msg->GetCustomBlobSize(), blob);
                    msg->AddCustomBlob(blob, msg->GetCustomBlobSize(), true);
                }
                msg->m_senderTargetId = static_cast<uint32_t>(connection->GetConnectionId());

                m_inboxMutex.lock();
                m_inbox.push_back(msg);
                m_inboxMutex.unlock();
                m_tmpInboundBuffer.clear();
            }
        }

        return true;
    }

    AzNetworking::ConnectResult TargetManagementComponent::ValidateConnect(
        [[maybe_unused]] const AzNetworking::IpAddress& remoteAddress,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] AzNetworking::ISerializer& serializer)
    {
        return AzNetworking::ConnectResult::Accepted;
    }

    void TargetManagementComponent::OnConnect([[maybe_unused]] AzNetworking::IConnection* connection)
    {
        ;
    }

    AzNetworking::PacketDispatchResult TargetManagementComponent::OnPacketReceived(
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        [[maybe_unused]] AzNetworking::ISerializer& serializer)
    {
        return AzFrameworkPackets::DispatchPacket(connection, packetHeader, serializer, *this);
    }

    void TargetManagementComponent::OnPacketLost(
        [[maybe_unused]] AzNetworking::IConnection* connection, [[maybe_unused]] AzNetworking::PacketId packetId)
    {
        ;
    }

    void TargetManagementComponent::OnDisconnect(
        [[maybe_unused]] AzNetworking::IConnection* connection,
        [[maybe_unused]] AzNetworking::DisconnectReason reason,
        [[maybe_unused]] AzNetworking::TerminationEndpoint endpoint)
    {
        ;
    }

    // call this function to retrieve the list of currently known targets - this is mainly used for GUIs
    // when they come online and attempt to enum (they won't have been listening for target coming and going)
    // you will only be shown targets that have been seen in a reasonable amount of time.
    void TargetManagementComponent::EnumTargetInfos(TargetContainer& infos)
    {
        infos = m_availableTargets;
    }

    // set the desired target, which we'll specifically keep track of.
    // the target controls who gets lua commands, tweak stuff, that kind of thing
    void TargetManagementComponent::SetDesiredTarget(AZ::u32 desiredTargetID)
    {
        AZ_TracePrintf("TargetManagementComponent", "Set Target - %u", desiredTargetID);

        if (desiredTargetID != static_cast<AZ::u32>(m_settings->m_lastTarget.m_networkId))
        {
            TargetInfo ti = GetTargetInfo(desiredTargetID);
            AZ::u32 oldTargetID = m_settings->m_lastTarget.m_networkId;
            m_settings->m_lastTarget = ti;
            m_tmpInboundBuffer.clear();
            m_tmpInboundBufferPos = 0;

            if ((ti.IsValid()) && ((ti.m_flags & (TF_ONLINE | TF_SELF)) != 0))
            {
                EBUS_EVENT(TargetManagerClient::Bus, DesiredTargetChanged, desiredTargetID, oldTargetID);
            }
            else
            {
                EBUS_QUEUE_EVENT(TargetManagerClient::Bus, DesiredTargetChanged, desiredTargetID, oldTargetID);
            }

            if ((ti.IsValid()) && ((ti.m_flags & TF_ONLINE) != 0))
            {
                if (ti.m_flags & TF_SELF)
                {
                    EBUS_EVENT(TargetManagerClient::Bus, DesiredTargetConnected, true);
                }
                else
                {
                    EBUS_QUEUE_EVENT(TargetManagerClient::Bus, DesiredTargetConnected, true);
                }
            }
            else
            {
                EBUS_QUEUE_EVENT(TargetManagerClient::Bus, DesiredTargetConnected, false);
            }
        }
    }

    void TargetManagementComponent::SetDesiredTargetInfo(const TargetInfo& targetInfo)
    {
        SetDesiredTarget(targetInfo.GetNetworkId());
    }

    // retrieve what it was set to.
    TargetInfo TargetManagementComponent::GetDesiredTarget()
    {
        return m_settings->m_lastTarget;
    }

    // given id, get info.
    TargetInfo TargetManagementComponent::GetTargetInfo(AZ::u32 desiredTargetID)
    {
        TargetContainer::const_iterator finder = m_availableTargets.find(desiredTargetID);
        if (finder != m_availableTargets.end())
        {
            return finder->second;
        }

        return TargetInfo(); // return an invalid target info.
    }

    // check if target is online
    bool TargetManagementComponent::IsTargetOnline(AZ::u32 desiredTargetID)
    {
        TargetContainer::const_iterator finder = m_availableTargets.find(desiredTargetID);
        if (finder != m_availableTargets.end())
        {
            const TargetInfo& info = finder->second;
            return !!(info.m_flags & TF_ONLINE);
        }
        return false;
    }

    bool TargetManagementComponent::IsDesiredTargetOnline()
    {
        TargetInfo desiredTarget = GetDesiredTarget();
        return IsTargetOnline(desiredTarget.GetNetworkId());
    }

    void TargetManagementComponent::SetMyPersistentName(const char* name)
    {
        m_settings->m_persistentName = name;
    }

    const char* TargetManagementComponent::GetMyPersistentName()
    {
        return m_settings->m_persistentName.c_str();
    }

    TargetInfo TargetManagementComponent::GetMyTargetInfo() const
    {
        auto mapIter = m_availableTargets.find(k_nullNetworkId);

        if (mapIter != m_availableTargets.end())
        {
            return mapIter->second;
        }

        return TargetInfo();
    }

    void TargetManagementComponent::SetNeighborhood(const char* name)
    {
        m_settings->m_neighborhoodName = name;
    }

    const char* TargetManagementComponent::GetNeighborhood()
    {
        return m_settings->m_neighborhoodName.c_str();
    }

    void TargetManagementComponent::SendTmMessage(const TargetInfo& target, const TmMsg& msg)
    {
        // Messages targeted at our own application just transfer right over to the inbox.
        if (target.m_flags & TF_SELF)
        {
            TmMsg* inboxMessage = static_cast<TmMsg*>(m_serializeContext->CloneObject(static_cast<const void*>(&msg), msg.RTTI_GetType()));
            AZ_Assert(inboxMessage, "Failed to clone local loopback message.");
            inboxMessage->m_senderTargetId = target.GetNetworkId();

            if (msg.GetCustomBlobSize() > 0)
            {
                void* blob = azmalloc(msg.GetCustomBlobSize(), 16, AZ::OSAllocator);
                memcpy(blob, msg.GetCustomBlob(), msg.GetCustomBlobSize());
                inboxMessage->AddCustomBlob(blob, msg.GetCustomBlobSize(), true);
            }

            if (msg.IsImmediateSelfDispatchEnabled())
            {
                TmMsgPtr receivedMessage(inboxMessage);
                EBUS_EVENT_ID(inboxMessage->GetId(), TmMsgBus, OnReceivedMsg, receivedMessage);
            }
            else
            {
                m_inboxMutex.lock();
                m_inbox.push_back(inboxMessage);
                m_inboxMutex.unlock();
            }
            
            return;
        }

        AZ_PROFILE_SCOPE(AzFramework, "TargetManager::SendTmMessage");
        AZStd::vector<char, AZ::OSStdAllocator> msgBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char, AZ::OSStdAllocator> > outMsg(&msgBuffer);

        AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&outMsg, *m_serializeContext, AZ::ObjectStream::ST_BINARY);
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

        m_outboxMutex.lock();
        m_outbox.push_back();
        m_outbox.back().first = target.m_networkId;
        m_outbox.back().second.swap(msgBuffer);
        m_outboxMutex.unlock();
    }

    void TargetManagementComponent::DispatchMessages(MsgSlotId id)
    {
        AZ_PROFILE_SCOPE(AzFramework, "TargetManager::DispatchMessages");
        AZStd::lock_guard<AZStd::mutex> lock(m_inboxMutex);
        size_t maxMsgsToProcess = m_inbox.size();
        TmMsgQueue::iterator itMsg = m_inbox.begin();
        for (size_t i = 0; i < maxMsgsToProcess; ++i)
        {
            TmMsgPtr msg = *itMsg;
            if (msg->GetId() == id)
            {
                EBUS_EVENT_ID(msg->GetId(), TmMsgBus, OnReceivedMsg, msg);
                itMsg = m_inbox.erase(itMsg);
            }
            else
            {
                ++itMsg;
            }
        }
    }

    void TargetManagementComponent::OnMsgParsed(TmMsg** ppMsg, void* classPtr, const AZ::Uuid& classId, const AZ::SerializeContext* sc)
    {
        // Check that classPtr is a TmMsg
        AZ_Assert(*ppMsg == nullptr, "ppMsg is already set! are we deserializing multiple messages in one call?");
        *ppMsg = sc->Cast<TmMsg*>(classPtr, classId);
        AZ_Assert(*ppMsg, "Failed to downcast msg pointer to a TmMsg. Is RTTI and reflection set up properly?");
    }

    void TargetManagementComponent::TickThread()
    {
        while (!m_stopRequested)
        {
            AZ_PROFILE_SCOPE(AzFramework, "TargetManager::Tick");

            // Send
            size_t maxMsgsToSend = m_outbox.size();
            for (size_t iSend = 0; iSend < maxMsgsToSend; ++iSend)
            {
                // Send outbound data
                uint8_t* outBuffer = (uint8_t*)m_outbox.front().second.data();
                const size_t totalSize = m_outbox.front().second.size();
                size_t outSize = m_outbox.front().second.size();
                while (outSize > 0)
                {
                    size_t bufferSize = AZStd::min(outSize, aznumeric_cast<size_t>(AzNetworking::MaxPacketSize));
                    AzFrameworkPackets::TargetManagementMessage tmPacket;
                    tmPacket.SetSize(aznumeric_cast<uint32_t>(totalSize));
                    AzNetworking::TcpPacketEncodingBuffer encodingBuffer;
                    encodingBuffer.CopyValues(outBuffer, bufferSize);
                    tmPacket.SetMessageBuffer(encodingBuffer);
                    outSize -= bufferSize;

                    auto visitor = [this, tmPacket](AzNetworking::IConnection& connection)
                    {
                        m_networkInterface->SendReliablePacket(connection.GetConnectionId(), tmPacket);
                    };
                    m_networkInterface->GetConnectionSet().VisitConnections(visitor);               
                }

                m_outboxMutex.lock();
                m_outbox.pop_front();
                m_outboxMutex.unlock();
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(50));
        }
    }

    void connect_target([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        for (auto& networkInterface : AZ::Interface<AzNetworking::INetworking>::Get()->GetNetworkInterfaces())
        {
            if (networkInterface.first == AZ::Name("TargetManagement"))
            {
                const uint16_t serverPort = target_port;

                AzNetworking::ConnectionId connId = networkInterface.second->Connect(
                    AzNetworking::IpAddress(TargetServerAddress, serverPort, AzNetworking::ProtocolType::Tcp));
                if (connId != AzNetworking::InvalidConnectionId)
                {
                    AzFrameworkPackets::Neighbor initPacket;
                    initPacket.SetPersistentId(AZ::Crc32(Platform::GetPersistentName()));
                    initPacket.SetCapabilities(Neighborhood::NEIGHBOR_CAP_LUA_VM | Neighborhood::NEIGHBOR_CAP_LUA_DEBUGGER);
                    initPacket.SetPersistentName(Platform::GetPersistentName());
                    initPacket.SetDisplayName(Platform::GetPersistentName());
                    initPacket.SetIsJoin(true);
                    networkInterface.second->SendReliablePacket(connId, initPacket);
                }
            }
        }
    }
    AZ_CONSOLEFREEFUNC(connect_target, AZ::ConsoleFunctorFlags::DontReplicate, "Opens a target management connection to a host");

    TargetJoinThread::TargetJoinThread(int updateRate)
        : TimedThread("AzFramework::TargetJoinThread", AZ::TimeMs(updateRate))
    {
        ;
    }

    void TargetJoinThread::OnUpdate([[maybe_unused]] AZ::TimeMs updateRateMs)
    {
        connect_target(AZ::ConsoleCommandContainer());
        for (auto& networkInterface : AZ::Interface<AzNetworking::INetworking>::Get()->GetNetworkInterfaces())
        {
            if (networkInterface.first == AZ::Name("TargetManagement") &&
                networkInterface.second->GetConnectionSet().GetActiveConnectionCount() > 0)
            {
                Stop();
            }
        }
    }
}   // namespace AzFramework
