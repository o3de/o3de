/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
#include <GridMate/Session/LANSession.h>
#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <time.h>

namespace AzFramework
{
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

    struct TargetManagementSettings
        : public AZ::UserSettings
    {
        AZ_CLASS_ALLOCATOR(TargetManagementSettings, AZ::OSAllocator, 0);
        AZ_RTTI(TargetManagementSettings, "{AB1B14BB-C0E3-484A-B498-98F44A58C161}", AZ::UserSettings);

        TargetManagementSettings()
            : m_reconnectionIntervalMS(1000)
            , m_targetPort(5172)
            , m_sourcePort(0)
        {
        }

        ~TargetManagementSettings() override
        {
        }

        AZStd::string   m_neighborhoodName;         // this is the neighborhood session (hub) we want to connect to
        AZStd::string   m_persistentName;           // this string is used as the persistent name for this target
        int         m_reconnectionIntervalMS;   // time between connection attempts in ms
        TargetInfo  m_lastTarget;               // this is the target we will automatically connect to
        int         m_targetPort;               // port we will send search broadcasts on
        int         m_sourcePort;               // port we will listen for search replies on
    };

    class TargetManagementNetworkImpl
        : public GridMate::SessionEventBus::Handler
        , public Neighborhood::NeighborhoodBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(TargetManagementNetworkImpl, AZ::SystemAllocator, 0);

        TargetManagementNetworkImpl(TargetManagementComponent* component)
            : m_component(component)
            , m_gridMate(nullptr)
            , m_session(nullptr)
            , m_gridSearch(nullptr)
        {
            // start gridmate
            m_gridMate = GridMate::GridMateCreate(GridMate::GridMateDesc());
            AZ_Assert(m_gridMate, "Failed to create gridmate!");

            // start the multiplayer service (session mgr, extra allocator, etc.)
            GridMate::StartGridMateService<GridMate::LANSessionService>(m_gridMate, GridMate::SessionServiceDesc());
            AZ_Assert(GridMate::HasGridMateService<GridMate::LANSessionService>(m_gridMate), "Failed to start MP service!");

            GridMate::SessionEventBus::Handler::BusConnect(m_gridMate);
            Neighborhood::NeighborhoodBus::Handler::BusConnect();
        }

        ~TargetManagementNetworkImpl() override
        {
            Neighborhood::NeighborhoodBus::Handler::BusDisconnect();
            GridMate::SessionEventBus::Handler::BusDisconnect();

            if (m_gridSearch)
            {
                m_gridSearch->Release();
            }

            GridMate::GridMateDestroy(m_gridMate);
        }

        //////////////////////////////////////////////////////////////////////////
        // gridmate session bus
        void OnGridSearchComplete(GridMate::GridSearch* gridSearch) override
        {
            if (gridSearch == m_gridSearch)
            {
                if (gridSearch->GetNumResults() > 0)
                {
                    GridMate::CarrierDesc carrierDesc;
                    carrierDesc.m_driverIsCrossPlatform = false;
                    carrierDesc.m_driverIsFullPackets = true;
                    //carrierDesc.m_threadInstantResponse = true;
                    carrierDesc.m_driverReceiveBufferSize = 1 * 1024 * 1024;

                    // Until an authentication connection can be established between peers only supporting
                    // local connections (i.e. binding to localhost).
                    carrierDesc.m_address = "127.0.0.1";

                    GridMate::JoinParams joinParams;
                    joinParams.m_desiredPeerMode = GridMate::Mode_Peer;
                    m_session = nullptr;

                    const GridMate::LANSearchInfo& lanSearchInfo = static_cast<const GridMate::LANSearchInfo&>(*gridSearch->GetResult(0));
                    EBUS_EVENT_ID_RESULT(m_session,m_gridMate,GridMate::LANSessionServiceBus,JoinSessionBySearchInfo,lanSearchInfo,joinParams,carrierDesc);
                }
                else
                {
                    m_component->m_reconnectionTime = AZStd::chrono::system_clock::now() + AZStd::chrono::milliseconds(m_component->m_settings->m_reconnectionIntervalMS);
                }
                gridSearch->Release();
                m_gridSearch = nullptr;
            }
        }

        void OnSessionJoined(GridMate::GridSession* session) override
        {
            if (session == m_session)
            {
                GridMate::ReplicaPtr replica = GridMate::Replica::CreateReplica(m_component->m_settings->m_persistentName.c_str());
                Neighborhood::NeighborReplicaPtr replicaChunk = GridMate::CreateReplicaChunk<Neighborhood::NeighborReplica>(session->GetMyMember()->GetId().Compact(), m_component->m_settings->m_persistentName.c_str(), Neighborhood::NEIGHBOR_CAP_LUA_VM | Neighborhood::NEIGHBOR_CAP_LUA_DEBUGGER);
                replicaChunk->SetDisplayName(m_component->m_settings->m_persistentName.c_str());
                replica->AttachReplicaChunk(replicaChunk);
                session->GetReplicaMgr()->AddPrimary(replica);
            }
        }

        void OnSessionDelete(GridMate::GridSession* session) override
        {
            if (session == m_session)
            {
                m_session = nullptr;
            }
        }

        void OnMemberLeaving(GridMate::GridSession* session, GridMate::GridMember* member) override
        {
            (void)member;
            if (session == m_session)
            {
                AZ_TracePrintf("TargetManager", "Member %p(%s) leaving TM session %p.\n", member, member->IsHost() ? "Host" : "Peer", session);
            }
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // neighborhood bus
        void OnNodeJoined(const Neighborhood::NeighborReplica& node) override
        {
            Neighborhood::NeighborCaps capabilities = node.GetCapabilities();
            GridMate::MemberIDCompact memberId = node.GetTargetMemberId();
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
            if (persistentId == m_component->m_settings->m_lastTarget.m_persistentId && (m_component->m_settings->m_lastTarget.m_flags & TF_ONLINE) == 0)
            {
                m_component->m_settings->m_lastTarget.m_flags |= TF_ONLINE;
                m_component->m_settings->m_lastTarget.m_networkId = memberId;
                m_component->m_settings->m_lastTarget.m_lastSeen = ti.m_lastSeen;
                EBUS_QUEUE_EVENT(TargetManagerClient::Bus, DesiredTargetConnected, true);
            }
        }

        void OnNodeLeft(const Neighborhood::NeighborReplica& node) override
        {
            AZ_TracePrintf("Neighborhood", "Lost contact with node from member %d.\n", static_cast<int>(node.GetTargetMemberId()));

            // If our desired target has left the network, flag it and notify listeners
            if (node.GetTargetMemberId() == m_component->m_settings->m_lastTarget.m_networkId)
            {
                m_component->m_settings->m_lastTarget.m_flags &= ~TF_ONLINE;
                EBUS_QUEUE_EVENT(TargetManagerClient::Bus, DesiredTargetConnected, false);
            }

            TargetContainer::iterator it = m_component->m_availableTargets.find(static_cast<AZ::u32>(node.GetTargetMemberId()));
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
        GridMate::IGridMate*                    m_gridMate;
        GridMate::GridSession*                  m_session;
        GridMate::GridSearch*                   m_gridSearch;
        Neighborhood::NeighborReplicaPtr        m_neighborReplica;
    };

    const AZ::u32 k_nullNetworkId = static_cast<AZ::u32>(-1);

    TargetManagementComponent::TargetManagementComponent()
        : m_serializeContext(nullptr)
        , m_networkImpl(nullptr)
    {
    }

    TargetManagementComponent::~TargetManagementComponent()
    {
        AZ_Assert(m_networkImpl == nullptr, "We still have networkImpl! Was Deactivate() called?");
    }

    void TargetManagementComponent::Activate()
    {
        EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        {
            AZStd::string persistentName = Platform::GetPersistentName();

            AZStd::string targetManagementSettingsKey = AZStd::string::format("TargetManagementSettings::%s", persistentName.c_str());
            m_settings = AZ::UserSettings::CreateFind<TargetManagementSettings>(AZ::Crc32(targetManagementSettingsKey.c_str()), AZ::UserSettings::CT_GLOBAL);

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

        // Add a target for the local application.
        TargetContainer::pair_iter_bool ret = m_availableTargets.insert_key(k_selfNetworkId);

        TargetInfo& ti = ret.first->second;
        ti.m_lastSeen = time(nullptr);
        ti.m_displayName = m_settings->m_persistentName;
        ti.m_persistentId = AZ::Crc32(m_settings->m_persistentName.c_str());
        ti.m_networkId = k_selfNetworkId;
        ti.m_flags |= TF_ONLINE | TF_DEBUGGABLE | TF_RUNNABLE | TF_SELF;
        EBUS_QUEUE_EVENT(TargetManagerClient::Bus, TargetJoinedNetwork, ti);

        TargetManager::Bus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();

        AZ::AllocatorInstance<GridMate::GridMateAllocator>::Create();
        AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Create();

        m_networkImpl = aznew TargetManagementNetworkImpl(this);
        m_stopRequested = false;
        AZStd::thread_desc td;
        td.m_name = "TargetManager Thread";
        td.m_cpuId = AFFINITY_MASK_USERTHREADS;
        m_threadHandle = AZStd::thread(td, AZStd::bind(&TargetManagementComponent::TickThread, this));
    }

    void TargetManagementComponent::Deactivate()
    {
        // stop gridmate
        m_stopRequested = true;
        m_threadHandle.join();

        delete m_networkImpl;
        m_networkImpl = nullptr;

        AZ::AllocatorInstance<GridMate::GridMateAllocatorMP>::Destroy();
        AZ::AllocatorInstance<GridMate::GridMateAllocator>::Destroy();

        AZ::SystemTickBus::Handler::BusDisconnect();
        TargetManager::Bus::Handler::BusDisconnect();
        TargetManagerClient::Bus::ExecuteQueuedEvents();    // This will deliver any pending messages and clear the queue.

        // delete any undelivered messages
        m_inbox.clear();
        m_outbox.clear();

        m_tmpInboundBuffer.reserve(0);

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
                ->Field("HubPort", &TargetManagementSettings::m_targetPort)
                ->Field("Name", &TargetManagementSettings::m_persistentName)
                ->Field("LocalPort", &TargetManagementSettings::m_sourcePort)
                ->Field("ReconnectIntervalMS", &TargetManagementSettings::m_reconnectionIntervalMS)
                ->Field("LastConnectedTarget", &TargetManagementSettings::m_lastTarget)
                ;

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
                    "Target Manager", "Provides target discovery and connectivity services within a GridHub network")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Profiling")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    /*
                    ->DataElement(AZ_CRC("String", 0x9ebeb2a9), &TargetManagementSettings::m_neighborhoodName, "Hub Name", "Name of GridHub that you want to connect to.")
                    ->DataElement(AZ_CRC("Int", 0x1451dab1), &TargetManagementSettings::m_targetPort, "Hub Port", "Port GridHub listens on for new connections.")
                    ->DataElement(AZ_CRC("String", 0x9ebeb2a9), &TargetManagementSettings::m_persistentName, "Persistent Name", "Name that you want to use to refer to this target in the network.")
                    ->DataElement(AZ_CRC("Int", 0x1451dab1), &TargetManagementSettings::m_sourcePort, "Local Port", "Port to use for initial connection to the hub.")
                    ->DataElement(AZ_CRC("Int", 0x1451dab1), &TargetManagementSettings::m_reconnectionIntervalMS, "Reconnection Interval", "Delay in milliseconds before attempting reconnections to GridHub.")
                        ->Attribute(AZ::Edit::Attributes::Min, 1000)
                        ->Attribute(AZ::Edit::Attributes::Max, 10000);
                    */
                    ;
            }
        }

        if (!GridMate::ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(GridMate::ReplicaChunkClassId(Neighborhood::NeighborReplica::GetChunkName())))
        {
            GridMate::ReplicaChunkDescriptorTable::Get().RegisterChunkType<Neighborhood::NeighborReplica, Neighborhood::NeighborReplica::Desc>();
        }
    }

    void TargetManagementComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TargetManagerService", 0x6d5708bc));
    }

    void TargetManagementComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("UserSettingsService", 0xa0eadff5));
    }

    void TargetManagementComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TargetManagerService", 0x6d5708bc));
    }

    void TargetManagementComponent::OnSystemTick()
    {
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

    void TargetManagementComponent::JoinNeighborhood()
    {
        // if we don't have a specified neighborhood name, don't try to connect.
        if (m_settings->m_neighborhoodName.empty())
        {
            return;
        }

        GridMate::LANSearchParams searchParams;
        searchParams.m_serverPort = m_settings->m_targetPort;
        searchParams.m_listenPort = m_settings->m_sourcePort;
        searchParams.m_numParams = 1;
        searchParams.m_params[0].m_id = "HubName";
        searchParams.m_params[0].SetValue(m_settings->m_neighborhoodName);
        searchParams.m_params[0].m_op = GridMate::GridSessionSearchOperators::SSO_OPERATOR_EQUAL;

        // Until an authentication connection can be established between peers only supporting
        // local connections (i.e. binding to localhost).
        searchParams.m_listenAddress = "127.0.0.1";
        searchParams.m_serverAddress = "127.0.0.1";

        EBUS_EVENT_ID_RESULT(m_networkImpl->m_gridSearch,m_networkImpl->m_gridMate,GridMate::LANSessionServiceBus,StartGridSearch,searchParams);
        if (m_networkImpl->m_gridSearch)
        {
            m_reconnectionTime = AZStd::chrono::system_clock::now() + AZStd::chrono::milliseconds(m_settings->m_reconnectionIntervalMS);
        }
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
        AZ_Assert(m_networkImpl->m_session == nullptr, "We cannot change our neighborhood while connected!");
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
        AZ_Assert(m_networkImpl->m_session == nullptr, "We cannot change our neighborhood while connected!");
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
            if (m_networkImpl->m_gridMate)
            {
                AZ_PROFILE_SCOPE(AzFramework, "TargetManager::Tick");
                if (!m_networkImpl->m_session && !m_networkImpl->m_gridSearch)
                {
                    if (AZStd::chrono::system_clock::now() > m_reconnectionTime)
                    {
                        JoinNeighborhood();
                    }
                }

                {
                    AZ_PROFILE_SCOPE(AzFramework, "TargetManager::Tick Gridmate");
                    m_networkImpl->m_gridMate->Update();
                    if (m_networkImpl->m_session && m_networkImpl->m_session->GetReplicaMgr())
                    {
                        m_networkImpl->m_session->GetReplicaMgr()->Unmarshal();
                        m_networkImpl->m_session->GetReplicaMgr()->UpdateFromReplicas();
                        m_networkImpl->m_session->GetReplicaMgr()->UpdateReplicas();
                        m_networkImpl->m_session->GetReplicaMgr()->Marshal();
                    }
                }

                if (m_networkImpl->m_session)
                {
                    AZ_PROFILE_SCOPE(AzFramework, "TargetManager::Tick Send/Receive TmMsgs");

                    // Receive
                    for (unsigned int i = 0; i < m_networkImpl->m_session->GetNumberOfMembers(); ++i)
                    {
                        GridMate::GridMember* member = m_networkImpl->m_session->GetMemberByIndex(i);
                        GridMate::MemberIDCompact memberId = member->GetId().Compact();
                        const TargetInfo* target = nullptr;
                        AZ::u32 targetId = 0;
                        for (TargetContainer::const_iterator targetIt = m_availableTargets.begin(); targetIt != m_availableTargets.end(); ++targetIt)
                        {
                            if (targetIt->second.m_networkId == memberId)
                            {
                                target = &targetIt->second;
                                targetId = targetIt->first;
                            }
                        }

                        if (!member->IsLocal())
                        {
                            GridMate::Carrier::ReceiveResult result = member->ReceiveBinary(m_tmpInboundBuffer.data(), static_cast<unsigned int>(m_tmpInboundBuffer.capacity()));
                            while (result.m_state != result.NO_MESSAGE_TO_RECEIVE)
                            {
                                if (result.m_state == result.UNSUFFICIENT_BUFFER_SIZE)
                                {
                                    m_tmpInboundBuffer.reserve(result.m_numBytes);
                                    result = member->ReceiveBinary(m_tmpInboundBuffer.data(), static_cast<unsigned int>(m_tmpInboundBuffer.capacity()));
                                }
                                AZ_Assert(result.m_state == result.RECEIVED, "Failed to retrieve TmMsg from the network.");

                                if (target)
                                {
                                    AZ::IO::MemoryStream msgBuffer(m_tmpInboundBuffer.data(), result.m_numBytes, result.m_numBytes);
                                    TmMsg* msg = nullptr;
                                    AZ::ObjectStream::ClassReadyCB readyCB(AZStd::bind(&TargetManagementComponent::OnMsgParsed, this, &msg, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3));
                                    AZ::ObjectStream::LoadBlocking(&msgBuffer, *m_serializeContext, readyCB, AZ::ObjectStream::FilterDescriptor(nullptr, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES));
                                    if (msg)
                                    {
                                        if (msg->GetCustomBlobSize() > 0)
                                        {
                                            void* blob = azmalloc(msg->GetCustomBlobSize(), 1, AZ::OSAllocator, "TmMsgBlob");
                                            msgBuffer.Read(msg->GetCustomBlobSize(), blob);
                                            msg->AddCustomBlob(blob, msg->GetCustomBlobSize(), true);
                                        }
                                        msg->m_senderTargetId = targetId;

                                        m_inboxMutex.lock();
                                        m_inbox.push_back(msg);
                                        m_inboxMutex.unlock();
                                    }
                                }
                                result = member->ReceiveBinary(m_tmpInboundBuffer.data(), static_cast<unsigned int>(m_tmpInboundBuffer.capacity()));
                            }
                        }
                    }

                    // Send
                    size_t maxMsgsToSend = m_outbox.size();
                    for (size_t iSend = 0; iSend < maxMsgsToSend; ++iSend)
                    {
                        GridMate::GridMember* peer = m_networkImpl->m_session->GetMemberById(m_outbox.front().first);
                        if (peer)
                        {
                            peer->SendBinary(m_outbox.front().second.data(), static_cast<unsigned int>(m_outbox.front().second.size()), GridMate::Carrier::SEND_RELIABLE, GridMate::Carrier::PRIORITY_LOW);
                        }

                        m_outboxMutex.lock();
                        m_outbox.pop_front();
                        m_outboxMutex.unlock();
                    }
                }
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(50));
        }
    }
}   // namespace AzFramework
