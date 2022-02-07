/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <GridMate/Session/Session.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/Platform.h>

#include <GridMate/Carrier/Utils.h>

#include <GridMate/VoiceChat/VoiceChatServiceBus.h>

#include <GridMate/Session/Session_Platform.h>
#include <cinttypes>

namespace GridMate
{
    const EndianType kSessionEndian = EndianType::BigEndian;

    namespace Internal
    {
        /**
         * Called and executed from the carrier thread. Make sure everything is thread safe when you interact with this class.
         */
        class GridSessionHandshake
            : public Handshake
        {
        public:
            GM_CLASS_ALLOCATOR(GridSessionHandshake);

            struct NewConnection
            {
                NewConnection()
                    : m_id(InvalidConnectionID)
                    , m_isInvited(false)
                    , m_peerModeRequested(Mode_Undefined)
                    , m_userData(kSessionEndian, 0)
                {}

                NewConnection(NewConnection&& nc) = default;

                NewConnection& operator=(NewConnection&& nc)
                {
                    m_id = nc.m_id;
                    m_isInvited = nc.m_isInvited;
                    m_peerModeRequested = nc.m_peerModeRequested;
                    m_userData = AZStd::move(nc.m_userData);
                    return *this;
                }

                ConnectionID        m_id;
                bool                m_isInvited;
                RemotePeerMode      m_peerModeRequested;
                WriteBufferDynamic  m_userData;
            };

            typedef vector<NewConnection> NewConnectionsType;
            typedef vector<AZ::u8> UserDataBufferType;
            typedef unordered_set<AZStd::string> AddressSetType;

            GridSessionHandshake(unsigned int handshakeTimeoutMS, const VersionType& version);
            ~GridSessionHandshake() override {}

            //////////////////////////////////////////////////////////////////////////
            // Handshake

            /// Called from the system to write initial handshake data.
            void            OnInitiate(ConnectionID id, WriteBuffer& wb) override;
            /**
            * Called when a system receives a handshake initiation from another system.
            * You can write a reply in the WriteBuffer.
            * return true if you accept this connection and false if you reject it.
            */
            HandshakeErrorCode OnReceiveRequest(ConnectionID id, ReadBuffer& rb, WriteBuffer& wb) override;
            /**
            * If we already have a valid connection and we receive another connection request, the system will
            * call this function to verify the state of the connection.
            */
            bool            OnConfirmRequest(ConnectionID id, ReadBuffer& rb) override;
            /**
            * Called when we receive Ack from the other system on our initial data \ref OnInitiate.
            * return true to accept the ack or false to reject the handshake.
            */
            bool            OnReceiveAck(ConnectionID id, ReadBuffer& rb) override   { (void)id; (void)rb; return true; }  // we don't do any further filtering
            /**
            * Called when we receive Ack from the other system while we were connected. This callback is called
            * so we can just confirm that our connection is valid!
            */
            bool            OnConfirmAck(ConnectionID id, ReadBuffer& rb) override   { (void)id; (void)rb; return true; } // we don't do any further filtering
            /// Return true if you want to reject early reject a connection.
            bool            OnNewConnection(const AZStd::string& address) override;
            /// Called when we close a connection.
            void            OnDisconnect(ConnectionID id) override;
            /// Return timeout in milliseconds of the handshake procedure.
            unsigned int    GetHandshakeTimeOutMS() const override                   { return m_handshakeTimeOutMS; }
            //////////////////////////////////////////////////////////////////////////

            void                    BanAddress(AZStd::string address);
            void                    SetHost(bool isHost);
            void                    SetInvited(bool isInvited);
            void                    SetHostMigration(bool isMigrating);
            void                    SetUserData(const void* data, size_t dataSize);
            void                    SetSessionId(AZStd::string sessionId);
            bool                    IsNewConnections()                              { return !m_newConnections.empty(); }
            NewConnectionsType&     AcquireNewConnections();
            void                    ReleaseNewConnections();

            void                    SetPeerMode(RemotePeerMode mode)                { m_peerMode = mode; }
            RemotePeerMode          GetPeerMode() const                             { return m_peerMode; }

        protected:
            AZStd::mutex            m_dataLock;
            NewConnectionsType      m_newConnections;
            AddressSetType          m_banList;
            UserDataBufferType      m_userData;
            AZStd::string           m_sessionId;
            RemotePeerMode          m_peerMode;
            VersionType             m_version;

            unsigned int            m_handshakeTimeOutMS;
            bool                    m_isHost;
            bool                    m_isInvited;
            bool                    m_isMigratingHost;
        };
    }
}


using namespace GridMate;
using namespace GridMate::Internal;
using namespace AZ;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// GridSessionParam
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void GridSessionParam::SetValue(AZ::s32 value)
{
    m_type = VT_INT32;
    AZStd::to_string(m_value, value);
}
void GridSessionParam::SetValue(AZ::s64 value)
{
    m_type = VT_INT64;
    AZStd::to_string(m_value, value);
}
void GridSessionParam::SetValue(float value)
{
    m_type = VT_FLOAT;
    AZStd::to_string(m_value, value);
}
void GridSessionParam::SetValue(double value)
{
    m_type = VT_DOUBLE;
    AZStd::to_string(m_value, value);
}
void GridSessionParam::SetValue(AZ::s32* values, size_t numElements)
{
    m_type = VT_INT32_ARRAY;
    if (numElements > 0)
    {
        AZ_Assert(values != nullptr, "Invalid values pointer!");
        AZStd::string temp;
        for (size_t i = 0; i < numElements; ++i)
        {
            AZStd::to_string(temp, values[i]);
            m_value += temp;
            if (i != numElements - 1)
            {
                m_value += ",";
            }
        }
    }
}
void GridSessionParam::SetValue(AZ::s64* values, size_t numElements)
{
    m_type = VT_INT64_ARRAY;
    if (numElements > 0)
    {
        AZ_Assert(values != nullptr, "Invalid values pointer!");
        AZStd::string temp;
        for (size_t i = 0; i < numElements; ++i)
        {
            AZStd::to_string(temp, values[i]);
            m_value += temp;
            if (i != numElements - 1)
            {
                m_value += ",";
            }
        }
    }
}
void GridSessionParam::SetValue(float* values, size_t numElements)
{
    m_type = VT_FLOAT_ARRAY;
    if (numElements > 0)
    {
        AZ_Assert(values != nullptr, "Invalid values pointer!");
        AZStd::string temp;
        for (size_t i = 0; i < numElements; ++i)
        {
            AZStd::to_string(temp, values[i]);
            m_value += temp;
            if (i != numElements - 1)
            {
                m_value += ",";
            }
        }
    }
}
void GridSessionParam::SetValue(double* values, size_t numElements)
{
    m_type = VT_DOUBLE_ARRAY;
    if (numElements > 0)
    {
        AZ_Assert(values != nullptr, "Invalid values pointer!");
        AZStd::string temp;
        for (size_t i = 0; i < numElements; ++i)
        {
            AZStd::to_string(temp, values[i]);
            m_value += temp;
            if (i != numElements - 1)
            {
                m_value += ",";
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// GridSession
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// GridSession
//=========================================================================
GridSession::GridSession(SessionService* service)
    : m_carrier(nullptr)
    , m_replicaMgr(nullptr)
    , m_handshake(nullptr)
    , m_isShutdown(true)
    , m_myMember(nullptr)
    , m_state(nullptr)
    , m_hostMigrationInProcess(false)
    , m_disconnectKickedPlayersDelay(500)
{
    AZ_Assert(service, "Invalid service");
    m_service = service;
    m_gridMate = service->GetGridMate();
}

//=========================================================================
// ~GridSession
//=========================================================================
GridSession::~GridSession()
{
    while (!m_eventQueue.empty())
    {
        EventCommand& cmd = m_eventQueue.front();
        if (cmd.m_userDataSize)
        {
            azfree(cmd.event.userData, GridMateAllocatorMP);
        }
        m_eventQueue.pop();
    }
    AZ_Assert(m_isShutdown, "Shutdown has not been called!");

    // we own the local member so we manually delete it, member might not exist if initialization failed or did not happen
    delete m_myMember;
    m_myMember = nullptr;
}

//=========================================================================
// Initialize
//=========================================================================
bool GridSession::Initialize(const CarrierDesc& carrierDesc)
{
    m_carrierDesc = carrierDesc;
    m_isShutdown = false;
    m_service->AddSession(this);
    return true;
}

//=========================================================================
// Shutdown
//=========================================================================
void
GridSession::Shutdown()
{
    if (!m_isShutdown)
    {
        if (m_carrier)
        {
            m_carrier->Disconnect(AllConnections);
        }

        m_service->RemoveSession(this);

        ReplicaMgrCallbackBus::Handler::BusDisconnect(m_gridMate);

        // delete replica manager in case we do a hard delete (otherwise SE_DELETE should already have done that)
        if (m_replicaMgr && m_replicaMgr->IsInitialized())
        {
            m_replicaMgr->Shutdown();
        }

        if (m_carrier)
        {
            m_carrier->Shutdown();
        }

        EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionDelete, this);
        EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionDelete, this);

        m_state = nullptr;

        delete m_replicaMgr;
        m_replicaMgr = nullptr;

        // remove the carrier we can't send more data.
        CarrierEventBus::Handler::BusDisconnect();
        delete m_carrier;
        m_carrier = nullptr;

        delete m_handshake;
        m_handshake = nullptr;

        m_isShutdown = true;
    }
}

//=========================================================================
// SetHandshakeUserData
//=========================================================================
void GridSession::SetHandshakeUserData(const void* data, size_t size)
{
    AZ_Assert(m_handshake, "We should have created a valid handshake first!");
    m_handshake->SetUserData(data, size);
}

//=========================================================================
// Update
//=========================================================================
void
GridSession::Update()
{
    AZ_Assert(!m_isShutdown, "Update() called on session that was not initialized.");

    // check if we need to delete the session and escape
    if (m_sm.GetCurrentState() == SS_START_UP)
    {
        Shutdown();
        delete this;
        return;
    }

    // We always pass the tick event, no need to queue it.
    HSM::Event e;
    e.id = SE_UPDATE;
    m_sm.Dispatch(e); // send update event to the state machine

    // Process queued events that depend on overlapped operations.
    ProcessEvents();

    if (m_carrier)
    {
        m_carrier->Update();

        // TODO: Hook this change when we do a variable change, make sure we don't call too often.
        m_carrier->DebugEnableDisconnectDetection(m_state->m_isDisconnectDetection.Get());
    }

    //////////////////////////////////////////////////////////////////////////
    // Add new connections
    if (IsHost())
    {
        GridSessionHandshake::NewConnectionsType& nc = m_handshake->AcquireNewConnections();
        while (!nc.empty())
        {
            GridSessionHandshake::NewConnection newConnection = AZStd::move(nc.back());
            nc.pop_back();

            ConnectionID id = newConnection.m_id;
            bool isInvited = newConnection.m_isInvited;

            if (IsConnectionIdInMemberList(id))  // if we just receive the message again while we already have the member confirm it.
            {
                continue;
            }

            // a client is trying to join. Join him if possible.
            ReadBuffer rb(kSessionEndian, newConnection.m_userData.Get(), newConnection.m_userData.Size());
            GridMember* member = CreateRemoteMember(m_carrier->ConnectionToAddress(id), rb, newConnection.m_peerModeRequested, id);

            if (member)
            {
                // Set invited flag, before we call AddToMember so we can reserve the proper slot type.
                member->m_isInvited.Set(isInvited);
                // Set the peer mode in the member
                member->m_peerMode.Set(newConnection.m_peerModeRequested);
                // If we failed adding a member means that we could not find free slot, so just drop
                // that member.
                if (!AddMember(member))
                {
                    m_carrier->Disconnect(id);
                    delete member;
                }
            }
        }
        m_handshake->ReleaseNewConnections();
    }

    //////////////////////////////////////////////////////////////////////////
    // Try to bind state replicas
    for (auto memberStateIter = m_unboundMemberStates.begin(); memberStateIter != m_unboundMemberStates.end(); )
    {
        Internal::GridMemberStateReplica* memberState = *memberStateIter;

        // See if the member has arrived.
        GridMember* member = GetMemberById(memberState->m_memberId.Get());
        if (IsHost())
        {
            AZ_Assert(member, "Failed to match member to client state on the host!");
        }

        if (member)
        {
            AZ_Assert(member->m_clientState == nullptr /*|| member->m_clientState==this*/, "This member already have a pointer to a different replica state!");
            memberState->m_member = member;
            member->m_clientState = memberState;
            memberStateIter = m_unboundMemberStates.erase(memberStateIter);

            // Both member and client state are valid! send member joined message
            EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnMemberJoined, this, member);
            EBUS_EVENT_ID(GetGridMate(), SessionEventBus, OnMemberJoined, this, member);
        }
        else
        {
            ++memberStateIter;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Check connectivity
    if (IsHost() && GetTopology() == ST_PEER_TO_PEER)
    {
        TimeStamp now = AZStd::chrono::system_clock::now();

        // Check this
        if (AZStd::chrono::milliseconds(now - m_lastConnectivityUpdate).count() >= 1000)
        {
            m_lastConnectivityUpdate = now;

            // Loop through our members to find out how many p2p members are there.
            // Each peer will count themselves in, but since we are skipping the host,
            // the numbers even out.
            AZStd::size_t numPeerConnections = 0;
            for (AZStd::size_t i = 1 /*we the host are always member 0*/; i < m_members.size(); ++i)
            {
                GridMember* member = m_members[i];
                if (member->m_peerMode.Get() == Mode_Peer)
                {
                    numPeerConnections++;
                }
            }

            // in peer to peer mode every peer (member) should have the same number of connections
            for (AZStd::size_t i = 1 /*we the host are always member 0*/; i < m_members.size(); ++i)
            {
                GridMember* member = m_members[i];
                if (member->m_peerMode.Get() == Mode_Peer && (!member->m_clientState || member->m_clientState->m_numConnections.Get() != numPeerConnections))
                {
                    // this member is out of sync.
                    NotFullyConnectedMember ncm(member);
                    NotConnectedArrayType::iterator iter = AZStd::find(m_membersNotFullyConnected.begin(), m_membersNotFullyConnected.end(), ncm);
                    if (iter == m_membersNotFullyConnected.end())
                    {
                        ncm.time = now;
                        m_membersNotFullyConnected.push_back(ncm);
                    }
                }
            }

            for (AZStd::size_t i = 0; i < m_membersNotFullyConnected.size(); )
            {
                GridMember* member = m_membersNotFullyConnected[i].member;
                AZStd::chrono::milliseconds timeElapsed = now - m_membersNotFullyConnected[i].time;
                if (member->m_clientState && member->m_clientState->m_numConnections.Get() == numPeerConnections)
                {
                    // Member is back in sync with the host
                    m_membersNotFullyConnected.erase(m_membersNotFullyConnected.begin() + i);
                    continue;
                }
                else if (timeElapsed.count() > m_state->m_peerToPeerTimeout.Get())
                {
                    // Member was out of sync for too long. Kick him out.
                    AZ_TracePrintf("GridMate", "Member %s (id=%s) is disconnected due peer to peer connectivity!\n", member->GetName().c_str(), member->GetId().ToString().c_str());
                    m_carrier->Disconnect(member->GetConnectionId());

                    m_membersNotFullyConnected.clear();  // clear the array (allow time to machines to go back in sync)
                    break;
                }
                ++i;
            }
        }
    }

    if (IsHost())
    {
        //////////////////////////////////////////////////////////////////////////
        TimeStamp now = AZStd::chrono::system_clock::now();
        for (auto it = m_futureKickedPlayers.begin();
                    it != m_futureKickedPlayers.end(); )
        {
            if (AZStd::chrono::milliseconds(now - it->first) >= m_disconnectKickedPlayersDelay)
            {
                auto member = GetMemberById(it->second);
                if (member)
                {
                    m_carrier->Disconnect(member->GetConnectionId());
                }
                it = m_futureKickedPlayers.erase(it);
            }
            else
            {
                it++;
            }
        }
        //////////////////////////////////////////////////////////////////////////
    }
}

//=========================================================================
// GetTopology
//=========================================================================
SessionTopology
GridSession::GetTopology() const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return m_state->m_topology.Get();
}

//=========================================================================
// GetNumUsedPublicSlots
//=========================================================================
AZ::u8
GridSession::GetNumUsedPublicSlots() const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return m_state->m_numUsedPublicSlots.Get();
}
//=========================================================================
// GetNumUsedPrivateSlots
//=========================================================================
AZ::u8
GridSession::GetNumUsedPrivateSlots() const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return m_state->m_numUsedPrivateSlots.Get();
}
//=========================================================================
// GetNumFreePublicSlots
//=========================================================================
AZ::u8
GridSession::GetNumFreePublicSlots() const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return m_state->m_numFreePublicSlots.Get();
}
//=========================================================================
// GetNumFreePrivateSlots
//=========================================================================
AZ::u8
GridSession::GetNumFreePrivateSlots() const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return m_state->m_numFreePrivateSlots.Get();
}
//=========================================================================
// GetFlags
//=========================================================================
AZ::u8
GridSession::GetFlags() const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return m_state->m_flags.Get();
}
//=========================================================================
// GetPeerToPeerTimeout
//=========================================================================
AZ::u32
GridSession::GetPeerToPeerTimeout() const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return m_state->m_peerToPeerTimeout.Get();
}
//=========================================================================
// GetHostMigrationTimeout
//=========================================================================
AZ::u32
GridSession::GetHostMigrationTimeout() const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return m_state->m_hostMigrationTimeout.Get();
}
//=========================================================================
// GetHostMigrationTimeout
//=========================================================================
AZ::u32
GridSession::GetHostMigrationVotingTime() const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return m_state->m_hostMigrationVotingTime.Get();
}
//=========================================================================
// GetNumParams
//=========================================================================
unsigned int
GridSession::GetNumParams() const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return static_cast<unsigned int>(m_state->m_params.Get().size());
}
//=========================================================================
// GetParam
//=========================================================================
const GridSessionParam&
GridSession::GetParam(unsigned int index) const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return m_state->m_params.Get()[index];
}

//=========================================================================
// AddParam
//=========================================================================
bool
GridSession::SetParam(const GridSessionParam& param)
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");

    if (IsHost())
    {
        if (m_state->m_params.Get().size() < SessionParams::k_maxNumParams)
        {
            m_state->m_params.Modify([=](Internal::GridSessionReplica::ParamContainer& params)
                {
                    bool paramFound = false;
                    for (GridSessionParam& existingParam : params)
                    {
                        if (existingParam.m_id == param.m_id)
                        {
                            existingParam.m_type = param.m_type;
                            existingParam.m_value = param.m_value;
                            paramFound = true;
                            break;
                        }
                    }
                    if (!paramFound)
                    {
                        params.push_back();
                        params.back().m_id = param.m_id;
                        params.back().m_type = param.m_type;
                        params.back().m_value = param.m_value;
                    }
                    return true;
                }
                );
            OnSessionParamChanged(param);
            return true;
        }
    }
    return false;
}

//=========================================================================
// RemoveParam
//=========================================================================
bool
GridSession::RemoveParam(const AZStd::string& paramId)
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");

    bool isRemoved = false;
    if (IsHost())
    {
        isRemoved = m_state->m_params.Modify([=](Internal::GridSessionReplica::ParamContainer& params)
            {
                for (auto iter = params.begin(); iter != params.end(); ++iter)
                {
                    if (iter->m_id == paramId)
                    {
                        params.erase(iter);
                        return true;
                    }
                }
                return false;   // param not found
            }
            );
        if (isRemoved)
        {
            OnSessionParamRemoved(paramId);
        }
    }
    return isRemoved;
}

//=========================================================================
// RemoveParam
//=========================================================================
bool
GridSession::RemoveParam(unsigned int index)
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");

    if (IsHost())
    {
        if (m_state->m_params.Get().size() > index)
        {
            const GridSessionReplica::ParamContainer& curParams = m_state->m_params.Get();
            const GridSessionParam& foundParam = curParams.at(index);
            AZStd::string paramId = foundParam.m_id;
            m_state->m_params.Modify([=](Internal::GridSessionReplica::ParamContainer& params)
                {
                    params.erase(&params.at(index));
                    return true;
                }
                );
            OnSessionParamRemoved(paramId);
            return true;
        }
    }
    return false;
}

//=========================================================================
// GetMember
//=========================================================================
GridMember*
GridSession::GetMemberById(const MemberID& id) const
{
    for (unsigned int i = 0; i < m_members.size(); ++i)
    {
        if (m_members[i]->GetId() == id)
        {
            return m_members[i];
        }
    }
    return nullptr;
}

//=========================================================================
// GetMember
//=========================================================================
GridMember*
GridSession::GetMemberById(const MemberIDCompact& id) const
{
    for (unsigned int i = 0; i < m_members.size(); ++i)
    {
        if (m_members[i]->GetIdCompact() == id)
        {
            return m_members[i];
        }
    }
    return nullptr;
}

//=========================================================================
// GetHost
//=========================================================================
GridMember*
GridSession::GetHost() const
{
    for (unsigned int i = 0; i < m_members.size(); ++i)
    {
        if (m_members[i]->IsHost())
        {
            return m_members[i];
        }
    }
    return nullptr;
}

//=========================================================================
// KickMember
//=========================================================================
GridSession::Result
GridSession::KickMember(GridMember* member, AZ::u8 reason)
{
    if (!IsHost() || member == nullptr)
    {
        return GS_ERROR;
    }

    AZ_TracePrintf("GridMate", "Member %s (id=%s) was kicked!\n", member->GetName().c_str(), member->GetId().ToString().c_str());
    member->KickRpc(reason);

    m_futureKickedPlayers.push_back(AZStd::make_pair(AZStd::chrono::system_clock::now(), member->GetIdCompact()));

    return GS_OK;
}

//=========================================================================
// BanMember
//=========================================================================
GridSession::Result
GridSession::BanMember(GridMember* member, AZ::u8 reason)
{
    if (!IsHost() || member == nullptr)
    {
        return GS_ERROR;
    }

    m_handshake->BanAddress(member->GetId().ToAddress());
    AZ_TracePrintf("GridMate", "Member %s (id=%s) was banned!\n", member->GetName().c_str(), member->GetId().ToString().c_str());
    return KickMember(member, reason);
}

//=========================================================================
// Leave
//=========================================================================
void
GridSession::Leave(bool isMigrateHost)
{
    (void)isMigrateHost;
    // We should support Leave at any moment/state.
    if (!m_sm.IsInState(SS_DELETE))
    {
        RequestEvent(SE_DELETE);
    }
}

//=========================================================================
// GetTime
//=========================================================================
unsigned int
GridSession::GetTime() const
{
    if (m_carrier)
    {
        return m_carrier->GetTime();
    }

    return 0;
}

//=========================================================================
// ReserveSlot
//=========================================================================
int
GridSession::ReserveSlot(bool isInvited)
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");

    if (!IsHost())
    {
        return 1;              // if we are client we don't manage slots. we always agree
    }
    unsigned char numFreePrivateSlots = m_state->m_numFreePrivateSlots.Get();
    if (isInvited && numFreePrivateSlots > 0)
    {
        AZ::u8 numUsedPrivateSlots = m_state->m_numUsedPrivateSlots.Get();
        m_state->m_numUsedPrivateSlots.Set(++numUsedPrivateSlots);
        m_state->m_numFreePrivateSlots.Set(--numFreePrivateSlots);
        return 2;
    }

    unsigned char numFreePublicSlots = m_state->m_numFreePublicSlots.Get();
    if (numFreePublicSlots > 0)
    {
        AZ::u8 numUsedPublicSlots = m_state->m_numUsedPublicSlots.Get();
        m_state->m_numUsedPublicSlots.Set(++numUsedPublicSlots);
        m_state->m_numFreePublicSlots.Set(--numFreePublicSlots);
        return 1;
    }

    return 0;
}

//=========================================================================
// FreeSlot
//=========================================================================
void
GridSession::FreeSlot(int slotType)
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");

    if (!IsHost())
    {
        return;             // if we are client we don't manage slots.
    }
    //  AZ_Warning("GridMate",m_state,"GridSession::FreeSlot failed because of missing session state!");
    if (m_state == nullptr)
    {
        return;
    }

    if (slotType == 2)
    {
        AZ_Assert(m_state->m_numUsedPrivateSlots.Get() > 0, "Invalid free private slot operation!");
        AZ::u8 numUsedPrivateSlots = m_state->m_numUsedPrivateSlots.Get();
        AZ::u8 numFreePrivateSlots = m_state->m_numFreePrivateSlots.Get();
        m_state->m_numUsedPrivateSlots.Set(--numUsedPrivateSlots);
        m_state->m_numFreePrivateSlots.Set(++numFreePrivateSlots);
    }
    if (slotType == 1)
    {
        AZ_Assert(m_state->m_numUsedPublicSlots.Get() > 0, "Invalid free public slot operation!");
        AZ::u8 numUsedPublicSlots = m_state->m_numUsedPublicSlots.Get();
        AZ::u8 numFreePublicSlots = m_state->m_numFreePublicSlots.Get();
        m_state->m_numUsedPublicSlots.Set(--numUsedPublicSlots);
        m_state->m_numFreePublicSlots.Set(++numFreePublicSlots);
    }
}

//=========================================================================
// AddMember
//=========================================================================
bool
GridSession::AddMember(GridMember* member)
{
    AZ_Assert(IsAddressInMemberList(member->GetId().ToAddress()) == false, "This member is already in the member list!");

    member->m_slotType = ReserveSlot(member->IsInvited());
    if (member->m_slotType == 0)
    {
        AZ_TracePrintf("GridMate", "Failed to reserve slot for %s(%s) [%d,%d]\n", member->GetId().ToString().c_str(), member->GetId().ToAddress().c_str(), m_state->m_numFreePrivateSlots.Get(), m_state->m_numFreePublicSlots.Get());
        AZ_TracePrintf("GridMate", "Current Members:\n");
        for (unsigned int i = 0; i < m_members.size(); ++i)
        {
            GridMember* currentMember = m_members[i];
            (void)currentMember;
            AZ_TracePrintf("GridMate", "  Member: %s(%s)\n", currentMember->GetId().ToString().c_str(), currentMember->GetId().ToAddress().c_str());
        }
        return false;
    }

    m_members.push_back(member);

    if (IsHost())
    {
        ReplicaPtr replica = member->GetReplica();
        if (!replica)
        {
            char debugName[256];
            azsnprintf(debugName, AZ_ARRAY_SIZE(debugName), "MemberId(%s)", member->GetId().ToString().c_str());

            replica = Replica::CreateReplica(debugName);
            replica->AttachReplicaChunk(member);
        }

        m_replicaMgr->AddPrimary(replica);
        member->m_isHost.Set(member->IsLocal());
    }

    // Add member to voice chat if there is appropriate service
    EBUS_EVENT_ID(m_gridMate, VoiceChatServiceBus, RegisterMember, member);

    return true;
}

//=========================================================================
// IsAddressInMemberList
//=========================================================================
bool
GridSession::IsAddressInMemberList(const AZStd::string& address)
{
    for (AZStd::size_t i = 0; i < m_members.size(); ++i)
    {
        if (m_members[i]->GetId().ToAddress() == address)
        {
            return true;
        }
    }

    return false;
}

//=========================================================================
// IsConnectionIdInMemberList
//=========================================================================
bool
GridSession::IsConnectionIdInMemberList(const ConnectionID& connId)
{
    for (AZStd::size_t i = 0; i < m_members.size(); ++i)
    {
        if (m_members[i]->GetConnectionId() == connId)
        {
            return true;
        }
    }

    return false;
}

//=========================================================================
// RemoveMember
//=========================================================================
bool
GridSession::RemoveMember(const MemberID& id)
{
    AZ_Warning("GridMate", !m_sm.IsInState(SS_HOST_MIGRATE_ELECTION), "It should be impossible to remove a member while there is no active host!");
    for (unsigned int i = 0; i < m_members.size(); ++i)
    {
        GridMember* member = m_members[i];
        if (member->GetId() == id)
        {
            // Removing member from voice chat
            EBUS_EVENT_ID(m_gridMate, VoiceChatServiceBus, UnregisterMember, member);

            // The only ones that can get a remove member call
            // without having lost that connection are the non-hosts.
            // If we are the host, then either we already lost connection
            // or we will force disconnection if we are kicking the member.
            if (!IsHost())
            {
                m_carrier->Disconnect(member->GetConnectionId());
            }

            FreeSlot(member->m_slotType);

            m_members.erase(m_members.begin() + i);

            NotConnectedArrayType::iterator iter = AZStd::find(m_membersNotFullyConnected.begin(), m_membersNotFullyConnected.end(), NotFullyConnectedMember(member));
            if (iter != m_membersNotFullyConnected.end())
            {
                m_membersNotFullyConnected.erase(iter);
            }

            return true;
        }
    }

    return false;
}

//=========================================================================
// OnIncomingConnection
//=========================================================================
void
GridSession::OnIncomingConnection(Carrier* carrier, ConnectionID id)
{
    (void)carrier;
    (void)id;
}

//=========================================================================
// OnFailedToConnect
//=========================================================================
void
GridSession::OnFailedToConnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason)
{
    if (carrier != m_carrier)
    {
        return; // not for us
    }
    (void)reason;
    AZ_TracePrintf("GridMate", "FailedToConnect %s => (%s,%s)\n", m_myMember ? m_myMember->GetId().ToAddress().c_str() : "Local", m_carrier->ConnectionToAddress(id).c_str(), ReasonToString(reason).c_str());

    // Remove the member on the host or set ConnectionId to invalid for a peer.
    GridMember* removedMember = nullptr;
    for (unsigned int i = 0; i < m_members.size(); ++i)
    {
        GridMember* member = m_members[i];
        if (member->m_connectionId == id)
        {
            member->m_connectionId = InvalidConnectionID;
            removedMember = member;
            break;
        }
    }
    if (IsHost())
    {
        if (removedMember) // if we have a failed handshake, we will not have a member to remove.
        {
            removedMember->GetReplica()->Destroy();
        }
    }
    else
    {
        // if we are client make sure we did not failed to connected to the host. If so we failed to join.
        if (m_sm.IsInState(SS_JOIN) && m_carrier->ConnectionToAddress(id) == m_hostAddress)
        {
            AZ_TracePrintf("GridMate", "Failed to join session %s\n", m_sessionId.c_str());
            RequestEvent(SE_DELETE);
        }
    }
}

//=========================================================================
// OnConnectionEstablished
//=========================================================================
void
GridSession::OnConnectionEstablished(Carrier* carrier, ConnectionID id)
{
    if (carrier != m_carrier)
    {
        return; // not for us
    }
    m_connections.insert(id);

    RemotePeerMode peerMode = Mode_Peer;
    if (IsHost())
    {
        AZ_Assert(GetTopology() != ST_INVALID, "Invalid sesson topology! Did session replica arrive yet?");

        // Check which peer mode does the client prefer
        GridSessionHandshake::NewConnectionsType& nc = m_handshake->AcquireNewConnections();
        GridSessionHandshake::NewConnectionsType::iterator it = nc.begin();
        while (it != nc.end())
        {
            if (it->m_id == id)
            {
                break;
            }
            ++it;
        }
        AZ_Assert(it != nc.end(), "New connection is not in the handshake new connections list!");
        if (it->m_peerModeRequested != Mode_Undefined)
        {
            peerMode = it->m_peerModeRequested;
        }

        // If we are not peer to peer, then we can only support client mode.
        if (GetTopology() == ST_CLIENT_SERVER_DISTRIBUTED || GetTopology() == ST_CLIENT_SERVER)
        {
            peerMode = Mode_Client;
        }

        // write the final peer mode back into the connection record, we will use it when
        // we create the remote member.
        it->m_peerModeRequested = peerMode;

        m_handshake->ReleaseNewConnections();
    }

    AZ_Assert(m_myMember, "We should always have a valid local member!");
    AZ_Assert(m_myMember->m_clientState, "My member should always have a client state active!");
    m_myMember->m_clientState->m_numConnections.Set(static_cast<unsigned char>(m_connections.size()));

    AZ_TracePrintf("GridMate", "Connection %s => %s (%s) (Connections=%d)!\n", m_myMember->GetId().ToAddress().c_str(), m_carrier->ConnectionToAddress(id).c_str(), peerMode == Mode_Client ? "Client" : "Peer", m_connections.size());

    m_replicaMgr->AddPeer(id, peerMode);
}

//=========================================================================
// OnDisconnect
//=========================================================================
void
GridSession::OnDisconnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason)
{
    if (carrier != m_carrier)
    {
        return; // not for us
    }
    (void)reason;
    AZ_TracePrintf("GridMate", "Disconnect %s => (%s,%s)\n", m_myMember ? m_myMember->GetId().ToAddress().c_str() : "Local", m_carrier->ConnectionToAddress(id).c_str(), ReasonToString(reason).c_str());

    // Remove the member on the host or set ConnectionId to invalid for a peer.
    GridMember* removedMember = nullptr;
    MemberIDCompact removedMemberId = 0;
    for (unsigned int i = 0; i < m_members.size(); ++i)
    {
        GridMember* member = m_members[i];
        if (member->m_connectionId == id)
        {
            member->m_connectionId = InvalidConnectionID;
            removedMember = member;
            removedMemberId = member->GetId().Compact();
            break;
        }
    }
    if (IsHost())
    {
        if (removedMember)
        {
            removedMember->GetReplica()->Destroy();
        }
    }
    else
    {
        // if we are client make sure we did not failed to connected to the host. If so we failed to join.
        if (m_sm.IsInState(SS_JOIN) && m_carrier->ConnectionToAddress(id) == m_hostAddress)
        {
            AZ_TracePrintf("GridMate", "Failed to join session %s\n", m_sessionId.c_str());
            RequestEvent(SE_DELETE);
        }
    }

    m_replicaMgr->RemovePeer(id);
    m_connections.erase(id);

    if (m_myMember)
    {
        AZ_Assert(m_myMember->m_clientState, "My member should always have a client state active!");
        m_myMember->m_clientState->m_numConnections.Set(static_cast<unsigned char>(m_connections.size()));
    }

    if (removedMemberId != 0)
    {
        RequestEventParam(SE_CONNECTION_LOST, removedMemberId);
    }
}

//=========================================================================
// OnDriverError
//=========================================================================
void
GridSession::OnDriverError(Carrier* carrier, ConnectionID id, const DriverError& error)
{
    if (carrier != m_carrier)
    {
        return; // not for us
    }
    uintptr_t idInt = reinterpret_cast<uintptr_t>(static_cast<void*>(id));
    AZStd::string errorMsg = AZStd::string::format("Carrier driver error ConnectionID: %" PRIuPTR "ErrorCode: 0x%08x", idInt, error.m_errorCode);
    EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionError, this, errorMsg);
    EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionError, this, errorMsg);

    if (id != InvalidConnectionID)
    {
        // we had connection related error, carrier will close it.
        m_replicaMgr->RemovePeer(id);
        m_connections.erase(id);
    }
    else
    {
        // we had a global error, so this was it. close the session.
        Leave(false);
    }
}

//=========================================================================
// OnSecurityError
//=========================================================================
void
GridSession::OnSecurityError(Carrier* carrier, ConnectionID id, const SecurityError& error)
{
    if (carrier != m_carrier)
    {
        return; // not for us
    }
    uintptr_t idInt = reinterpret_cast<uintptr_t>(static_cast<void*>(id));
    AZStd::string errorMsg = AZStd::string::format("Carrier security error ConnectionID: %" PRIuPTR " ErrorCode: 0x%08x", idInt, error.m_errorCode);
    EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionError, this, errorMsg);
}

//=========================================================================
// OnNewHost
//=========================================================================
void
GridSession::OnNewHost(bool isHost, ReplicaManager* manager)
{
    (void)isHost;
    /// Called from the replica manager when HostMigration has been completed.
    if (manager == m_replicaMgr)
    {
        RequestEvent(SE_HM_REPLICAS_MIGRATED);
    }
}

//=========================================================================
// ElectNewHost
//=========================================================================
void
GridSession::ElectNewHost()
{
    AZ_Assert(m_myMember, "We should always have local member!");
    //  AZ_Assert(m_sm.IsInState(SS_HOST_MIGRATE_ELECTION),"We should be in host migrate election state to call this function!");
    GridMember* newHost = nullptr; // Allow the user to choose

    EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnMigrationElectHost, this, newHost);
    EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnMigrationElectHost, this, newHost);

    if (newHost == nullptr || newHost->GetConnectionId() == InvalidConnectionID || newHost->IsHost())
    {
        // Elect a new host automatically...
        for (MemberArrayType::iterator iMember = m_members.begin(); iMember != m_members.end(); ++iMember)
        {
            GridMember* member = (*iMember);
            if (!member->IsHost() && (member->GetConnectionId() != InvalidConnectionID || member == m_myMember)) // find the oldest non host member we are connected to.
            {
                newHost = member;
                break;
            }
        }
    }

    if (newHost == nullptr) // if we can't select a new host we should quit the host migration
    {
        AZ_TracePrintf("GridMate", "Host migration: can't select a new host!");
        Leave(false);
        return;
    }

    CastNewHostVote(newHost);
}

//=========================================================================
// CastNewHostVote
//=========================================================================
void
GridSession::CastNewHostVote(GridMember* newHost)
{
    AZ_Assert(m_myMember && m_myMember->m_clientState, "We should always have local member and state!");
    if (newHost)
    {
        m_myMember->m_clientState->m_newHostVote.Set(newHost->GetId().Compact());
        AZ_TracePrintf("GridMate", "Host migration: %s(%s) voted %s(%s) for new host!\n", m_myMember->GetName().c_str(), m_myMember->GetId().ToAddress().c_str(), newHost->GetName().c_str(), newHost->GetId().ToAddress().c_str());
    }
}

//=========================================================================
// EventToQueue
//=========================================================================
void
GridSession::EventToQueue(const HSM::Event& event, unsigned int userDataSize, bool isProcessRequired, EventCommand::DataCloner cloner, EventCommand::DataDeleter deleter)
{
    EventCommand cmd;
    cmd.event = event;
    cmd.m_userDataSize = userDataSize;
    cmd.m_isProcessRequired = isProcessRequired;
    cmd.m_dataDeleter = deleter;

    // copy the event data if needed
    if (userDataSize)
    {
        AZ_Assert(event.userData != nullptr, "If you provide user data size, you must have a valid data pointer!");
        if (cloner)
        {
            cmd.event.userData = cloner(event.userData, userDataSize);
        }
        else
        {
            cmd.event.userData = azmalloc(userDataSize, 1, GridMateAllocatorMP);
            memcpy(cmd.event.userData, event.userData, userDataSize);
        }
    }

    m_eventQueue.push(cmd);
}

//=========================================================================
// ProcessEventOnQueue
//=========================================================================
void
GridSession::ProcessEventOnQueue()
{
    AZ_Assert(!m_eventQueue.empty(), "You need to have an event to process it!");
    EventCommand cmd = m_eventQueue.front();
    m_eventQueue.pop();
    bool isProcessed = m_sm.Dispatch(cmd.event);
    (void)isProcessed;
    AZ_Assert(!cmd.m_isProcessRequired || isProcessed, "We require that event %d is processed by the state machine (in state %d), but it failed. Check the state machine logic!", cmd.event.id, m_sm.GetCurrentState());

    if (cmd.m_userDataSize)
    {
        if (cmd.m_dataDeleter)
        {
            cmd.m_dataDeleter(cmd.event.userData, cmd.m_userDataSize);
        }
        else
        {
            azfree(cmd.event.userData, GridMateAllocatorMP);
        }
    }
}

//=========================================================================
// RequestEvent
//=========================================================================
void
GridSession::RequestEvent(int id, const void* userData, unsigned int userDataSize, bool isProcessRequired, EventCommand::DataCloner cloner, EventCommand::DataDeleter deleter)
{
    HSM::Event event;
    event.id = id;
    event.userData = const_cast<void*>(userData);
    if (!m_eventQueue.empty() || m_sm.IsDispatching())
    {
        EventToQueue(event, userDataSize, isProcessRequired, cloner, deleter);
    }
    else
    {
        // execute on spot
        bool isProcessed = m_sm.Dispatch(event);
        (void)isProcessed;
        AZ_Assert(!isProcessRequired || isProcessed, "We require that event %d is processed by the state machine (in state %d), but it failed. Check the state machine logic!", id, m_sm.GetCurrentState());
    }
}

//=========================================================================
// ProcessEvents
//=========================================================================
void
GridSession::ProcessEvents()
{
    while (!m_eventQueue.empty())
    {
        ProcessEventOnQueue();
    }
}

//=========================================================================
// DebugEnableDisconnectDetection
//=========================================================================
void
GridSession::DebugEnableDisconnectDetection(bool isEnable)
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");

    if (IsHost())
    {
        m_state->m_isDisconnectDetection.Set(isEnable);
    }
}

//=========================================================================
// DebugIsEnableDisconnectDetection
//=========================================================================
bool
GridSession::DebugIsEnableDisconnectDetection() const
{
    AZ_Assert(m_state, "Invalid session state replica. Session is not initialized.");
    return m_state->m_isDisconnectDetection.Get();
}

//////////////////////////////////////////////////////////////////////////
// State machine
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Session state machine
//=========================================================================
// SetUpStateMachine
//=========================================================================
void
GridSession::SetUpStateMachine()
{
    // Set up the state machine
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_TOP), HSM::StateHandler(this, &GridSession::OnStateNoSession), HSM::InvalidStateId, SS_NO_SESSION);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_NO_SESSION), HSM::StateHandler(this, &GridSession::OnStateNoSession), SS_TOP, SS_START_UP);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_START_UP), HSM::StateHandler(this, &GridSession::OnStateStartup), SS_NO_SESSION);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_CREATE), HSM::StateHandler(this, &GridSession::OnStateCreate), SS_NO_SESSION);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_IN_SESSION), HSM::StateHandler(this, &GridSession::OnStateInSession), SS_TOP, SS_IDLE);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_JOIN), HSM::StateHandler(this, &GridSession::OnStateJoin), SS_IN_SESSION);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_IDLE), HSM::StateHandler(this, &GridSession::OnStateIdle), SS_IN_SESSION);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_START_GAME), HSM::StateHandler(this, &GridSession::OnStateStart), SS_IN_SESSION);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_IN_GAME), HSM::StateHandler(this, &GridSession::OnStateInGame), SS_IN_SESSION);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_END_GAME), HSM::StateHandler(this, &GridSession::OnStateEnd), SS_IN_SESSION);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_DELETE), HSM::StateHandler(this, &GridSession::OnStateDelete), SS_IN_SESSION);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_HOST_MIGRATE_ELECTION), HSM::StateHandler(this, &GridSession::OnStateHostMigrateElection), SS_IN_SESSION);
    m_sm.SetStateHandler(AZ_HSM_STATE_NAME(SS_HOST_MIGRATE_SESSION), HSM::StateHandler(this, &GridSession::OnStateHostMigrateSession), SS_IN_SESSION);

    m_sm.Start();
}

bool
GridSession::OnStateNoSession(HSM& sm, const HSM::Event& e)
{
    switch (e.id)
    {
    case SE_DELETE:
    {
        if (!sm.IsInState(SS_START_UP) && !sm.IsInState(SS_DELETE))
        {
            sm.Transition(SS_DELETE);
        }
    } return true;
    }
    return false;
}

bool
GridSession::OnStateStartup(HSM& sm, const HSM::Event& e)
{
    switch (e.id)
    {
    case SE_JOIN:
    case SE_HOST:
    {
        sm.Transition(SS_CREATE);
    } return true;
    }
    return false;
}
bool
GridSession::OnStateJoin(HSM& sm, const HSM::Event& e)
{
    switch (e.id)
    {
    case SE_JOINED:
    {
        EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionJoined, this);
        EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionJoined, this);
        sm.Transition(SS_IDLE);
    } return true;
    }
    return false;
}

bool
GridSession::OnStateInSession(HSM& sm, const HSM::Event& e)
{
    switch (e.id)
    {
    case SE_CONNECTION_LOST:
    {
        // Allow host migration only if we are in a suitable state (we have operational session)
        if (sm.IsInState(SS_IDLE) || sm.IsInState(SS_IN_GAME) || sm.IsInState(SS_START_GAME) || sm.IsInState(SS_END_GAME))
        {
            MemberIDCompact id = static_cast<MemberIDCompact>(reinterpret_cast<size_t>(e.userData));
            GridMember* member = GetMemberById(id);
            GridMember* host = GetHost();
            // Check if we lost connection to the host
            if (host == nullptr || host == member)
            {
                if (m_state && (m_state->m_flags.Get() & (SessionParams::SF_HOST_MIGRATION | SessionParams::SF_HOST_MIGRATION_NO_EMPTY_SESSIONS)))
                {
                    sm.Transition(SS_HOST_MIGRATE_ELECTION);
                }
                else
                {
                    Leave(false);            // Leave because the host left
                }
            }
        }
    } return true;
    case SE_HM_REPLICAS_MIGRATED:
    {
        AZ_Assert(false, "We should not receive replica migrated callback if we are NOT in host migration state!!!");
    } return false;
    }
    return false;
}

bool
GridSession::OnStateCreate(HSM& sm, const HSM::Event& e)
{
    switch (e.id)
    {
    case HSM::EnterEventId:
    {
        AZ_Assert(m_carrierDesc.m_handshake == nullptr, "You cannot override the default carrier handshake provider if you are using the default multiplayer service!");
        m_handshake = aznew GridSessionHandshake(m_carrierDesc.m_connectionTimeoutMS, m_carrierDesc.m_version);
        m_carrierDesc.m_handshake = m_handshake;
        m_carrier = DefaultCarrier::Create(m_carrierDesc, m_gridMate);
        CarrierEventBus::Handler::BusConnect(m_gridMate);
        m_replicaMgr = aznew ReplicaManager();
        AZ_Assert(m_replicaMgr, "Failed to create ReplicaManager!");
        ReplicaMgrCallbackBus::Handler::BusConnect(m_gridMate);
    } return true;
    case SE_CREATED:
    {
        AZ_Assert(m_state && !m_state->GetReplica(), "We must have a valid state unbound replica at this stage!");

        m_handshake->SetHost(IsHost());
        m_handshake->SetInvited(m_myMember->IsInvited());
        m_handshake->SetSessionId(m_sessionId);

        if (IsHost())
        {
            AZ_TracePrintf("GridMate", "Session %s created by %s\n", m_sessionId.c_str(), m_myMember->GetId().ToString().c_str());

            //we are the host start broadcasting the system clock
            m_carrier->StartClockSync(1000, true);

            // Init replica manager
            ReplicaMgrDesc rmDesc(m_myMember->GetId().Compact(), m_carrier, CC_REPLICA_DATA, ReplicaMgrDesc::Role_SyncHost);
            m_replicaMgr->Init(rmDesc);
            m_replicaMgr->RegisterUserContext(AZ::Crc32("GridSession"), this);

            // Bind session replica
            Replica* stateReplica = Replica::CreateReplica("SessionStateInfo");
            stateReplica->AttachReplicaChunk(m_state);
            m_replicaMgr->AddPrimary(stateReplica);

            // Bind member replica
            bool isAdded = AddMember(m_myMember);
            AZ_Error("GridMate", isAdded, "Failed to add my replica, check the number of open slots!");
            if (!isAdded)
            {
                sm.Transition(SS_DELETE);
                return true;
            }

            sm.Transition(SS_IDLE);
            EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionCreated, this);
            EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionCreated, this);
            EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionHosted, this);
        }
        else
        {
            AZ_TracePrintf("GridMate", "Joining session %s created by %s\n", m_sessionId.c_str(), m_hostAddress.c_str());

            // Init replica manager
            ReplicaMgrDesc rmDesc(m_myMember->GetId().Compact(), m_carrier, CC_REPLICA_DATA);
            m_replicaMgr->Init(rmDesc);
            m_replicaMgr->RegisterUserContext(AZ::Crc32("GridSession"), this);

            if (m_hostAddress.empty())
            {
                sm.Transition(SS_DELETE);
                return true;
            }

            sm.Transition(SS_JOIN);

            m_handshake->SetPeerMode(m_myMember->m_peerMode.Get());

            m_carrier->Connect(m_hostAddress);

            EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionCreated, this);
            EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionCreated, this);
        }

    } return true;
    };
    return false;
}
bool
GridSession::OnStateIdle(HSM& sm, const HSM::Event& e)
{
    (void)sm;
    (void)e;
    return false;
}
bool
GridSession::OnStateStart(HSM& sm, const HSM::Event& e)
{
    (void)sm;
    switch (e.id)
    {
    case HSM::EnterEventId:
    {
        EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionStart, this);
        EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionStart, this);
    } return true;
    }
    return false;
}
bool
GridSession::OnStateInGame(HSM& sm, const HSM::Event& e)
{
    (void)sm;
    (void)e;
    return false;
}
bool
GridSession::OnStateEnd(HSM& sm, const HSM::Event& e)
{
    (void)sm;
    switch (e.id)
    {
    case HSM::EnterEventId:
    {
        EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionEnd, this);
        EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionEnd, this);
    } return true;
    }
    return false;
}
bool
GridSession::OnStateDelete(HSM& sm, const HSM::Event& e)
{
    switch (e.id)
    {
    case HSM::EnterEventId:
    {
        // close all connections and give it some time to make a clean exit (while the session is being deleted)
        m_carrier->Disconnect(AllConnections);

        // remove all members
        while (!m_members.empty())
        {
            RemoveMember(m_members.front()->GetId());
        }
    } return true;
    case SE_DELETED:
    {
        // we should wait a little more to make sure send messages are send... or we can wait will all connection are gone
        // m_carrier->GetNumConnections() == 0
        sm.Transition(SS_NO_SESSION);
    } return true;
    }

    return false;
}
bool
GridSession::OnStateHostMigrateElection(AZ::HSM& sm, const AZ::HSM::Event& e)
{
    switch (e.id)
    {
    case HSM::EnterEventId:
    {
        if (m_members.size() < 2)
        {
            Leave(false);
            return true;
        }

        if (!m_hostMigrationInProcess)
        {
            m_hostMigrationInProcess = true;
            m_handshake->SetHostMigration(true);

            EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnMigrationStart, this);
            EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnMigrationStart, this);

            m_hostMigrationTimeOut = m_state ? m_state->m_hostMigrationTimeout.Get() : 0;
            m_hostMigrationVotingTime = m_state ? m_state->m_hostMigrationVotingTime.Get() : 0;
            m_hostMigrationStart = AZStd::chrono::system_clock::now();
        }
        m_hostMigrationSessionMigrated = false;
        m_hostMigrationReplicaMigrated = false;
    } // we don't return just execute the host election code.
    case SE_CONNECTION_LOST:
    {
        // If we have not voted or the host we voted for disconnected.
        if (m_myMember->m_clientState->m_newHostVote.Get() == 0 || m_myMember->m_clientState->m_newHostVote.Get() == static_cast<MemberIDCompact>(reinterpret_cast<size_t>(e.userData)))
        {
            ElectNewHost();
        }
    } return true;
    case SE_UPDATE:
    {
        TimeStamp defaultTime;
        if (m_hostMigrationStart == defaultTime)
        {
            return true;                                           // we have already queued a Leave Command, and waiting for the queue to be executed!
        }
        TimeStamp now = AZStd::chrono::system_clock::now();
        unsigned int votingTime = static_cast<unsigned int>(AZStd::chrono::milliseconds(now - m_hostMigrationStart).count());
        if (votingTime >= m_hostMigrationTimeOut)
        {
            AZ_TracePrintf("GridMate", "Host migration: election did not finish within %d ms!\n", m_hostMigrationTimeOut);
            Leave(false);
            m_hostMigrationStart = defaultTime;
            return true;
        }

        // check if we got 50 or more% of the votes if so start the session migration
        MemberIDCompact myId = m_myMember->GetId().Compact();
        int myVotes = 0;
        int hasVoted = 0;
        int maxVoters = 0;
        for (MemberArrayType::iterator iMember = m_members.begin(); iMember != m_members.end(); ++iMember)
        {
            GridMember* member = *iMember;
            if (member->m_clientState)
            {
                ++maxVoters;
                MemberIDCompact electedId = member->m_clientState->m_newHostVote.Get();
                if (electedId != 0)
                {
                    ++hasVoted;
                    if (electedId == myId)
                    {
                        ++myVotes;
                    }
                }
            }
        }
        AZ_Assert(m_state, "We should have a valid session state!");
        if (maxVoters == 1 && (m_state->m_flags.Get() & SessionParams::SF_HOST_MIGRATION_NO_EMPTY_SESSIONS))
        {
            // we are by ourself just quit the host migration.
            AZ_TracePrintf("GridMate", "Host migration: we lost all connection and SF_HOST_MIGRATION_NO_EMPTY_SESSIONS flag is set! Leaving the session!\n");
            Leave(false);
            m_hostMigrationStart = defaultTime;
            return true;
        }

        if (maxVoters == hasVoted || votingTime >= m_hostMigrationVotingTime)
        {
            // if all people voted or the voting time expired, check if we have the majority.
            float votes = static_cast<float>(myVotes);
            float majority = static_cast<float>(maxVoters)*0.5f;
            if (votes >= majority)  // if we are == we could wait 0.5s before we conclude the voting
            {
                // we have majority vote, we are the new host
                m_sessionId.clear(); // clear the current session ID. (means we are the host and we need to assign a new one)
                sm.Transition(SS_HOST_MIGRATE_SESSION);
            }
        }
    } return true;
    case SE_HM_MIGRATE_CLIENT:
    {
        // we received a command to migrate to a new host, set the target session and migrate to it
        m_sessionId = *reinterpret_cast<const SessionID*>(e.userData);
        sm.Transition(SS_HOST_MIGRATE_SESSION);
    } return true;
    }

    return false;
}
bool
GridSession::OnStateHostMigrateSession(AZ::HSM& sm, const AZ::HSM::Event& e)
{
    (void)sm;
    switch (e.id)
    {
    case HSM::EnterEventId:
    {
        // Reset host migration vote value.
        AZ_Assert(m_myMember && m_myMember->m_clientState, "We should always have local member and state!");
    } return true;
    case SE_HM_SESSION_MIGRATED:
    {
        // Send command to all clients to migrate to the new session. Using my member replica.
        m_myMember->m_clientState->OnNewHostRpc(m_sessionId);

        AZ_TracePrintf("GridMate", "New host elected %s(%s)\n", m_myMember->GetName().c_str(), m_myMember->GetId().ToAddress().c_str());

        // Tell replica manager about the new host
        m_replicaMgr->Promote();

        m_hostMigrationSessionMigrated = true;
    } return true;
    case SE_HM_CLIENT_SESSION_MIGRATED:
    {
        AZ_TracePrintf("GridMate", "Client %s(%s) migrated session %s!\n", m_myMember->GetName().c_str(), m_myMember->GetId().ToAddress().c_str(), m_sessionId.c_str());
        m_hostMigrationSessionMigrated = true;
    } return true;
    case SE_HM_REPLICAS_MIGRATED:
    {
        // Update the room, make sure all members and added/removed where needed.
        if (IsHost())
        {
            // Reset and recompute all the slots
            m_state->m_numFreePrivateSlots.Set(m_state->m_numFreePrivateSlots.Get() + m_state->m_numUsedPrivateSlots.Get());
            m_state->m_numFreePublicSlots.Set(m_state->m_numFreePublicSlots.Get() + m_state->m_numUsedPublicSlots.Get());
            m_state->m_numUsedPrivateSlots.Set(0);
            m_state->m_numUsedPublicSlots.Set(0);

            // Re-add the members
            for (AZStd::size_t i = 0; i < m_members.size(); ++i)
            {
                GridMember* member = m_members[i];
                member->m_slotType = ReserveSlot(member->IsInvited());
                AZ_Assert(member->m_slotType != 0, "Somehow during migration the config changed and we can't reserved slot for existing member!");
            }

            // Remove all unconnected members
            for (AZStd::size_t i = 0; i < m_members.size(); )
            {
                GridMember* member = m_members[i];
                if (member != m_myMember && member->GetConnectionId() == InvalidConnectionID)
                {
                    member->GetReplica()->Destroy();
                }
                else
                {
                    ++i;
                }
            }
            // at this moment all orphaned/missing member should be destroyed and deleted!

            // All clocks should be synced on US now.
            m_carrier->StartClockSync();
        }
        m_hostMigrationReplicaMigrated = true;
    } return true;
    case SE_UPDATE:
    {
        TimeStamp defaultTime;
        if (m_hostMigrationStart == defaultTime)
        {
            return true;                                           // we have already queued a Leave Command, and waiting for the queue to be executed!
        }
        TimeStamp now = AZStd::chrono::system_clock::now();
        if (AZStd::chrono::milliseconds(now - m_hostMigrationStart).count() >= m_hostMigrationTimeOut)
        {
            AZ_TracePrintf("GridMate", "Host migration: session migration did not finish within %d ms!\n", m_hostMigrationTimeOut);
            Leave(false);
            m_hostMigrationStart = defaultTime;
            return true;
        }
        // Wait for both session and replicas to migrate before we complete the session
        if (m_hostMigrationSessionMigrated && m_hostMigrationReplicaMigrated)
        {
            m_hostMigrationSessionMigrated = false;
            m_hostMigrationReplicaMigrated = false;

            GridMember* host = GetHost();

            EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnMigrationEnd, this, host);
            EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnMigrationEnd, this, host);

            m_myMember->m_clientState->m_newHostVote.Set(0);

            if (m_members.size() == 1 && (m_state->m_flags.Get() & SessionParams::SF_HOST_MIGRATION_NO_EMPTY_SESSIONS))
            {
                Leave(false);
                AZ_TracePrintf("GridMate", "Host migration: we lost all connection and SF_HOST_MIGRATION_NO_EMPTY_SESSIONS flag is set! Leaving the session!\n");
            }
            else
            {
                sm.Transition(SS_IDLE);
                m_hostMigrationInProcess = false;

                m_handshake->SetHostMigration(false);
                m_handshake->SetHost(IsHost());
                m_handshake->SetSessionId(m_sessionId);
            }

            m_hostMigrationStart = defaultTime;
        }
    } return true;
    case SE_CONNECTION_LOST:
    {
        // if the current host left
        if (m_myMember->m_clientState->m_newHostVote.Get() == static_cast<MemberIDCompact>(reinterpret_cast<size_t>(e.userData)))
        {
            AZ_TracePrintf("GridMate", "New host 0x%x disconnected while migrating the session! Going back to host election...\n", m_myMember->m_clientState->m_newHostVote.Get());

            m_myMember->m_clientState->m_newHostVote.Set(0); // reset so we revote

            // go back to election.
            sm.Transition(SS_HOST_MIGRATE_ELECTION);
        }
    } return true;
    }

    return false;
}

// End of state machine
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// GridMember
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// GridMember
//=========================================================================
GridMember::GridMember(const MemberIDCompact& memberIdCompact)
    : m_connectionId(InvalidConnectionID)
    , m_session(nullptr)
    , m_slotType(0)
    , m_memberIdCompact(memberIdCompact)
    , m_isHost("IsHost")
    , m_isInvited("IsInvited")
    , m_peerMode("PeerMode")
    , KickRpc("KickRpc")
{
    SetPriority(k_replicaPriorityRealTime);
}

//=========================================================================
// IsReplicaMigratable
//=========================================================================
bool GridMember::IsReplicaMigratable()
{
    return true;
}

//=========================================================================
// GetNatType
//=========================================================================
NatType
GridMember::GetNatType() const
{
    return m_clientState ? m_clientState->m_natType.Get() : NAT_UNKNOWN;
}

//=========================================================================
// GetName
//=========================================================================
AZStd::string
GridMember::GetName() const
{
    return m_clientState ? m_clientState->m_name.Get().c_str() : "Unknown";
}

//=========================================================================
// IsLocal
//=========================================================================
bool
GridMember::IsLocal() const
{
    return m_session->GetMyMember() == this;
}

//=========================================================================
// OnActivate
//=========================================================================
void
GridMember::OnReplicaActivate(const ReplicaContext& rc)
{
    if (IsLocal())
    {
        // if this member is me... add my state to the system.
        AZ_Assert(m_session->GetMyMember() == this, "The only local member should be myMember too!");
        rc.m_rm->AddPrimary(m_clientState->GetReplica());

        // Both member and client state are valid! send member joined message
        EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnMemberJoined, m_session, this);
        EBUS_EVENT_ID(m_session->GetGridMate(), SessionEventBus, OnMemberJoined, m_session, this);
    }
}

//=========================================================================
// OnDeactivate
//=========================================================================
void
GridMember::OnReplicaDeactivate(const ReplicaContext& rc)
{
    (void)rc;
    m_session->RemoveMember(GetId());

    if (m_clientState)
    {
        // We are deleting the member so send leave message (we are always keeping member <-> state together).
        EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnMemberLeaving, m_session, this);
        EBUS_EVENT_ID(m_session->GetGridMate(), SessionEventBus, OnMemberLeaving, m_session, this);

        m_clientState->m_member = nullptr;
    }

    if (IsLocal())
    {
        // if our member was removed the session, leave it.
        m_session->Leave(false);
    }
    else
    {
        // unbind the client state
        m_clientState = nullptr;
        m_clientStateReplica = nullptr;
    }
}

//=========================================================================
// OnChangeOwnership
//=========================================================================
void
GridMember::OnReplicaChangeOwnership(const ReplicaContext& rc)
{
    (void)rc;
    AZ_Assert(m_session->IsMigratingHost(), "This function can be called only during host migration!");
    if (IsPrimary())
    {
        // Host owns the members, if I became the owner means I am the HOST!
        if (m_session->m_myMember == this)
        {
            m_isHost.Set(true);
        }
        else
        {
            m_isHost.Set(false);
        }
    }
}

//=========================================================================
// OnKick
//=========================================================================
bool
GridMember::OnKick(AZ::u8 reason, const RpcContext& rc)
{
    //  Null check m_session and m_session->GetHost(). 
    //  2 Kick messages in quick succession can cause this to crash otherwise.
    if (m_session && m_session->GetHost() && rc.m_sourcePeer == m_session->GetHost()->GetIdCompact())    //Only the host can kick
    {
        EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnMemberKicked, m_session, this);
        EBUS_EVENT_ID(m_session->GetGridMate(), SessionEventBus, OnMemberKicked, m_session, this, reason);

        if (IsLocal())
        {
            m_session->Leave(false);
        }

        return true; // this is called only on the primary
    }
    return false;
}

//=========================================================================
// Mute
//=========================================================================
void
GridMember::Mute(const MemberIDCompact& id)
{
    if (IsLocal())
    {
        const GridMemberStateReplica::MuteListType& muteList = m_clientState->m_muteList.Get();
        GridMemberStateReplica::MuteListType::const_iterator it = AZStd::find(muteList.begin(), muteList.end(), id);
        if (it == muteList.end())
        {
            // TODO: remove the copy (should be a ref) once replica allows it (with something like GetToModify)
            GridMemberStateReplica::MuteListType muteListCopy = muteList;
            muteListCopy.push_back(id);
            m_clientState->m_muteList.Set(muteListCopy);
        }
    }
}

//=========================================================================
// Unmute
//=========================================================================
void
GridMember::Unmute(const MemberIDCompact& id)
{
    if (IsLocal())
    {
        const GridMemberStateReplica::MuteListType& muteList = m_clientState->m_muteList.Get();
        GridMemberStateReplica::MuteListType::const_iterator it = AZStd::find(muteList.begin(), muteList.end(), id);
        if (it != muteList.end())
        {
            // TODO: remove the copy (should be a ref) once replica allows it (with something like GetToModify)
            GridMemberStateReplica::MuteListType muteListCopy = muteList;
            size_t distance = AZStd::distance(it, muteList.begin());
            muteListCopy.erase(muteListCopy.begin() + distance);
            m_clientState->m_muteList.Set(muteListCopy);
        }
    }
}

//=========================================================================
// IsMuted
//=========================================================================
bool
GridMember::IsMuted(const MemberIDCompact& id) const
{
    if (m_clientState)
    {
        const GridMemberStateReplica::MuteListType& muteList = m_clientState->m_muteList.Get();
        GridMemberStateReplica::MuteListType::const_iterator it = AZStd::find(muteList.begin(), muteList.end(), id);
        return it != muteList.end();
    }
    return true;
}

//=========================================================================
// IsTalking
//=========================================================================
bool
GridMember::IsTalking() const
{
    return AZStd::chrono::milliseconds(AZStd::chrono::system_clock::now() - m_voiceDataProcessed).count() < 250;
}

void GridMember::UpdateTalking()
{
    m_voiceDataProcessed = AZStd::chrono::system_clock::now();
}

//=========================================================================
// SetHost
//=========================================================================
void
GridMember::SetHost(bool isHost)
{
    m_isHost.Set(isHost);
}

//=========================================================================
// SetInvited
//=========================================================================
void
GridMember::SetInvited(bool isInvited)
{
    m_isInvited.Set(isInvited);
}

//=========================================================================
// SendBinary
//=========================================================================
bool
GridMember::SendBinary(const void* data, unsigned int dataSize, Carrier::DataReliability reliability, Carrier::DataPriority priority)
{
    if (m_connectionId != InvalidConnectionID && m_session->m_carrier)
    {
        if (dataSize > 0)
        {
            m_session->m_carrier->Send(reinterpret_cast<const char*>(data), dataSize, m_connectionId, reliability, priority, GridSession::CC_USER_DATA);
        }
        return true;
    }
    return false;
}

//=========================================================================
// ReceiveBinary
//=========================================================================
Carrier::ReceiveResult
GridMember::ReceiveBinary(char* data, unsigned int maxDataSize)
{
    if (m_connectionId != InvalidConnectionID && m_session->m_carrier)
    {
        return m_session->m_carrier->Receive(data, maxDataSize, m_connectionId, GridSession::CC_USER_DATA);
    }
    else
    {
        Carrier::ReceiveResult result;
        result.m_state = Carrier::ReceiveResult::NO_MESSAGE_TO_RECEIVE;
        result.m_numBytes = 0;
        return result;
    }
}

//=========================================================================
// GetPlatformId
//=========================================================================
AZ::PlatformID
GridMember::GetPlatformId() const
{
    if (m_clientState)
    {
        return m_clientState->m_platformId.Get();
    }
    else
    {
        return AZ::PlatformID::PLATFORM_MAX;
    }
}

//=========================================================================
// GridMemberGetProcessId
//=========================================================================
AZ::u32
GridMember::GetProcessId() const
{
    if (m_clientState)
    {
        return m_clientState->m_processId.Get();
    }
    else
    {
        return 0;
    }
}

//=========================================================================
// GetMachineName
//=========================================================================
AZStd::string
GridMember::GetMachineName() const
{
    if (m_clientState)
    {
        return m_clientState->m_machineName.Get();
    }
    else
    {
        return AZStd::string();
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// GridMemberStateReplica
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// GridMemberStateReplica
//=========================================================================
GridMemberStateReplica::GridMemberStateReplica(GridMember* member)
    : m_member(member)
    , OnNewHostRpc("OnNewHostRpc")
    , m_numConnections("NumConnections")
    , m_natType("NatType")
    , m_name("Name")
    , m_memberId("MemberId")
    , m_newHostVote("NewHostVote")
    , m_muteList("MuteList")
    , m_platformId("PlatformId")
    , m_machineName("MachineName")
    , m_processId("ProcessId")
    , m_isInvited("IsInvited")
{
    m_platformId.Set(AZ::g_currentPlatform);
    m_machineName.Set(Utils::GetMachineAddress((m_member) ? m_member->GetSession()->GetCarrierDesc().m_familyType : 0));
    SetPriority(k_replicaPriorityRealTime);
    m_processId.Set(AZ::Platform::GetCurrentProcessId());
}

//=========================================================================
// GridMemberStateReplica::OnReplicaActivate
//=========================================================================
void
GridMemberStateReplica::OnReplicaActivate(const ReplicaContext& rc)
{
    (void)rc;
    if (m_member)
    {
        // This code should be executed only for local members/states
        AZ_Assert(m_member->IsLocal(), "The only GridMemberStateReplica that has a non NULL member pointer should be the local member/state!");
        AZ_Assert(m_member->GetReplica()->GetRepId() != 0 /*InvalidReplicaId*/, "We should always have the member replica activated!");
        m_memberId.Set(m_member->GetIdCompact());
        AZ_Assert(m_member->m_clientState == this, "This member should already point to us!");
    }
    else
    {
        GridSession* session = static_cast<GridSession*>(rc.m_rm->GetUserContext(AZ::Crc32("GridSession")));
        AZ_Assert(session, "We need to have a valid session!");
        session->m_unboundMemberStates.insert(this);
    }
}

//=========================================================================
// GridMemberStateReplica::OnReplicaDeactivate
//=========================================================================
void
GridMemberStateReplica::OnReplicaDeactivate(const ReplicaContext& rc)
{
    (void)rc;
    // for primary (this is our state) we always keep it. So don't do anything.
    if (IsPrimary())
    {
        return;
    }

    if (m_member)
    {
        // client state is gone send leave message
        EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnMemberLeaving, m_member->m_session, m_member);
        EBUS_EVENT_ID(m_member->GetSession()->GetGridMate(), SessionEventBus, OnMemberLeaving, m_member->m_session, m_member);

        m_member->m_clientState = nullptr;
        m_member->m_clientStateReplica = nullptr;
        m_member = nullptr;
    }
    else
    {
        GridSession* session = static_cast<GridSession*>(rc.m_rm->GetUserContext(AZ::Crc32("GridSession")));
        AZ_Assert(session, "We need to have a valid session!");
        session->m_unboundMemberStates.erase(this);
    }
}

//=========================================================================
// OnNewHost
//=========================================================================
bool
GridMemberStateReplica::OnNewHost(const SessionID& sessionId, const RpcContext&)
{
    // only process on proxies (remote members)
    if (IsProxy() && m_member)
    {
        // store the new host
        GridMember* myMember = m_member->m_session->GetMyMember();
        AZ_Assert(myMember->m_clientState, "We must have a valid client state!");

        // make sure we are in host migrate state
        if (m_member->m_session->m_sm.GetCurrentState() == GridSession::SS_HOST_MIGRATE_ELECTION)
        {
            // store the new host
            myMember->m_clientState->m_newHostVote.Set(m_member->GetId().Compact());
            // migrate to the new host
            m_member->m_session->RequestEventData(GridSession::SE_HM_MIGRATE_CLIENT, sessionId);
        }
        else
        {
            // Evaluate the connection to the host, if it's in trouble and over a certain threshold join the new host.
            GridMember* hostMember = m_member->m_session->GetHost();
            Carrier* carrier = m_member->m_session->GetCarrier();
            bool isHostLost = false;
            if (hostMember)
            {
                Carrier::Statistics hostStats;
                carrier->QueryStatistics(hostMember->GetConnectionId(), nullptr, &hostStats);
                float connectionEvaluationThreshold = m_member->m_session->GetCarrierDesc().m_connectionEvaluationThreshold;
                if (hostStats.m_connectionFactor >= connectionEvaluationThreshold)
                {
                    AZ_TracePrintf("GridMate", "Host migration: Host %s disconnected %.2f connection factor (max %.2f) for switching to %s!\n",
                        hostMember->GetId().ToAddress().c_str(), hostStats.m_connectionFactor, connectionEvaluationThreshold, m_member->GetId().ToAddress().c_str());

                    isHostLost = true;
                    // disconnect the host and make SURE we end up in host migration election state!
                    carrier->Disconnect(hostMember->GetConnectionId());
                }
                else
                {
                    AZ_TracePrintf("GridMate", "Host migration: Host connection factor %.2f allowed %.2f\n", hostStats.m_connectionFactor, connectionEvaluationThreshold);
                }
            }
            if (isHostLost)
            {
                // store the new host
                myMember->m_clientState->m_newHostVote.Set(m_member->GetId().Compact());
                // we should be in host migration state by now
                m_member->m_session->RequestEventData(GridSession::SE_HM_MIGRATE_CLIENT, sessionId);
            }
            else
            {
                AZ_TracePrintf("GridMate", "Host migration: %s rejected new host %s!\n", m_member->m_session->GetMyMember()->GetId().ToAddress().c_str(), m_member->GetId().ToAddress().c_str());
                // we have a good connection to our host, so we need to close the connection to the new host.
                carrier->Disconnect(m_member->GetConnectionId());
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// GridSession Manager
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// SessionService
//=========================================================================
SessionService::SessionService(const SessionServiceDesc& desc)
    : m_gridMate(nullptr)
{
    (void)desc;
}

//=========================================================================
// ~SessionService
//=========================================================================
SessionService::~SessionService()
{
    AZ_Assert(m_sessions.empty(), "We are still having active session(s)! Did you forget to call SessionService::Shutdown?");
    AZ_Assert(m_activeSearches.empty() && m_completedSearches.empty(), "We are still having searches! Did you forget to call SessionService::Shutdown?");
    AZ_Assert(m_gridMate == nullptr, "We are still registered with GridMate! Call IGridMate::StopMultiplayerService() first!");
}

void SessionService::OnGridMateUpdate(IGridMate* gridMate)
{
    (void)gridMate;
    Update();
}

void SessionService::OnServiceRegistered(IGridMate* gridMate)
{
    AZ_Assert(gridMate, "Invalid GridMate instance");
    AZ_Assert(!m_gridMate, "Already bound to GridMate instance");

    m_gridMate = gridMate;
}

void SessionService::OnServiceUnregistered(IGridMate* gridMate)
{
    (void)gridMate;

    while (!m_activeSearches.empty())
    {
        m_activeSearches.front()->Release();
    }

    while (!m_completedSearches.empty())
    {
        m_completedSearches.front()->Release();
    }

    while (!m_sessions.empty())
    {
        GridSession* session = m_sessions.front();
        session->Shutdown();
        delete session;
    }

    m_gridMate = nullptr;
}

//=========================================================================
// Update
//=========================================================================
void
SessionService::Update()
{
    //////////////////////////////////////////////////////////////////////////
    // Sessions
    // A session can shutdown itself and potentially other sessions in it's update this is why
    // we need to be very careful when we update them. List will NOT be rearranged nor new
    // sessions will be added.
    {
        AZStd::fixed_vector<GridSession*, 16> updatedSessions;
        for (int i = static_cast<int>(m_sessions.size()) - 1; i >= 0; )
        {
            GridSession* session = m_sessions[i];
            size_t preUpdateSize = m_sessions.size();
            session->Update();
            updatedSessions.push_back(session);
            if (preUpdateSize != m_sessions.size())
            {
                // we removed some sessions, find the first session to update
                i = static_cast<int>(m_sessions.size()) - 1;
                while (i >= 0)
                {
                    session = m_sessions[i];
                    if (AZStd::find(updatedSessions.begin(), updatedSessions.end(), session) == updatedSessions.end())
                    {
                        break;
                    }
                    --i;
                }
            }
            else
            {
                --i;
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Searches
    for (SearchArrayType::iterator it = m_activeSearches.begin(); it != m_activeSearches.end(); )
    {
        GridSearch* search = *it;
        search->Update();
        if (search->IsDone())
        {
            it = m_activeSearches.erase(it);
            m_completedSearches.push_back(search);

            EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnGridSearchComplete, search);
            EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnGridSearchComplete, search);
        }
        else
        {
            ++it;
        }
    }

    //////////////////////////////////////////////////////////////////////////
}

//=========================================================================
// ReleaseGridSearch
//=========================================================================
void SessionService::AddGridSeach(GridSearch* search)
{
    if (search != nullptr)
    {
        if( AZStd::find(m_activeSearches.begin(), m_activeSearches.end(), search) != m_activeSearches.end() )
        {
            AZ_Error("SessionService", false, "This search %p is already in the active searches list!", search);
        }
        else if( AZStd::find(m_completedSearches.begin(), m_completedSearches.end(), search) != m_completedSearches.end() )
        {
            AZ_Error("SessionService", false, "This search %p is already in the complete searches list!", search);
        }
        else
        {
            if (search->IsDone())
            {
                m_completedSearches.push_back(search);
            }
            else
            {
                m_activeSearches.push_back(search);
            }

            EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnGridSearchStart, search);
        }
    }
}

//=========================================================================
// ReleaseGridSearch
//=========================================================================
void SessionService::ReleaseGridSearch(GridSearch* search)
{
    if (search != nullptr)
    {
        bool released = false;

        if (search->IsDone() && !m_completedSearches.empty())
        {
            SearchArrayType::iterator it = AZStd::find(m_completedSearches.begin(), m_completedSearches.end(), search);

            if (it != m_completedSearches.end() )
            {
                m_completedSearches.erase(it);
                released = true;
            }
            else
            {
                AZ_Error("SessionService", false, "Completed search %p was NOT found in the complete list!", search);
            }
        }
        else if (!m_activeSearches.empty())
        {
            SearchArrayType::iterator it = AZStd::find(m_activeSearches.begin(), m_activeSearches.end(), search);

            if( it != m_activeSearches.end() )
            {
                m_activeSearches.erase(it);
                released = true;
            }
            else
            {
                AZ_Error("SessionService", false, "Active search %p was NOT found in the active list!", search);
            }
        }

        if (released)
        {
            EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnGridSearchRelease, search);
        }

        delete search;
    }
}

//=========================================================================
// AddSession
//=========================================================================
void SessionService::AddSession(GridSession* session)
{
    if(session != nullptr)
    {
        if( AZStd::find(m_sessions.begin(), m_sessions.end(), session) == m_sessions.end() )
        {
            m_sessions.push_back(session);
        }
        else
        {
            AZ_Error("SessionService", false, "Session %p has already been added!", session);
        }
    }
}

//=========================================================================
// RemoveSession
//=========================================================================
void SessionService::RemoveSession(GridSession* session)
{
    if (session != nullptr)
    {
        SessionArrayType::iterator it = AZStd::find(m_sessions.begin(), m_sessions.end(), session);
        if (it != m_sessions.end())
        {
            m_sessions.erase(it);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// GridSessionHandshake
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// GridSessionHandshake
//=========================================================================
GridSessionHandshake::GridSessionHandshake(unsigned int handshakeTimeoutMS, const VersionType& version)
    : m_peerMode(Mode_Undefined)
    , m_version(version)
    , m_handshakeTimeOutMS(handshakeTimeoutMS)
    , m_isHost(false)
    , m_isInvited(false)
    , m_isMigratingHost(false)
{
}

//=========================================================================
// OnInitiate
//=========================================================================
void GridSessionHandshake::OnInitiate(ConnectionID id, WriteBuffer& wb)
{
    (void)id;
    AZStd::lock_guard<AZStd::mutex> l(m_dataLock);
    AZ_Assert(!m_isHost, "A host should NOT initiate session connections, only clients (as they wish to join)!");
    AZ_Assert(!m_sessionId.empty(), "You must have a valid session ID, this is how we filter which connections are allowed and which not!");
    wb.Write(m_sessionId);
    wb.Write(m_isInvited);
    wb.Write(m_peerMode);
    wb.Write(m_version);

    if (!m_userData.empty())
    {   
        wb.WriteRaw(m_userData.data(), m_userData.size());
    }
}

//=========================================================================
// OnReceiveRequest
//=========================================================================
HandshakeErrorCode GridSessionHandshake::OnReceiveRequest(ConnectionID id, ReadBuffer& rb, WriteBuffer& wb)
{
    (void)id;
    AZStd::lock_guard<AZStd::mutex> l(m_dataLock);
    AZStd::string sessionId;
    bool isInvited = false;
    RemotePeerMode peerMode;
    VersionType version;

    bool isRead = rb.Read(sessionId);
    isRead &= rb.Read(isInvited);
    isRead &= rb.Read(peerMode);
    isRead &= rb.Read(version);

    if (version != m_version)
    {
        return HandshakeErrorCode::VERSION_MISMATCH;
    }

    bool isSaneData = isRead && (peerMode == Mode_Undefined || peerMode == Mode_Client || peerMode == Mode_Peer);
    if (!isSaneData)
    {
        return HandshakeErrorCode::REJECTED;
    }

    if (sessionId != m_sessionId)
    {
        return HandshakeErrorCode::REJECTED;
    }

    // make sure we don't accept connections during final stages of host migration...
    if (m_isMigratingHost)
    {
        return HandshakeErrorCode::REJECTED;
    }

    if (m_isHost)
    {
        for (NewConnectionsType::iterator connIt = m_newConnections.begin(); connIt != m_newConnections.end(); ++connIt)
        {
            if (connIt->m_id == id)
            {
                return HandshakeErrorCode::OK; // if we just receive the message again while we already have the member confirm it.
            }
        }

        m_newConnections.push_back();
        NewConnection& nc = m_newConnections.back();
        nc.m_id = id;
        nc.m_isInvited = isInvited;
        nc.m_peerModeRequested = peerMode;
        if (rb.Left() > 0)
        {
            nc.m_userData.WriteFromBuffer(rb, rb.Left());
        }
    }
    else
    {
        // We always need to accept connection if client, we should assume the other members knows more as long as the session ID matches.
        //if(GetTopology()==ST_PEER_TO_PEER)
        //{
        //  // check if we have the member
        //}
    }

    (void)wb; // no reply by default
    return HandshakeErrorCode::OK;
}

//=========================================================================
// OnConfirmRequest
//=========================================================================
bool GridSessionHandshake::OnConfirmRequest(ConnectionID id, ReadBuffer& rb)
{
    (void)id;
    AZStd::lock_guard<AZStd::mutex> l(m_dataLock);

    AZStd::string sessionId;
    if (!rb.Read(sessionId))
    {
        return false;
    }

    if (sessionId != m_sessionId)
    {
        return false;
    }

    if (m_isMigratingHost)
    {
        return false;
    }

    return true;
}

//=========================================================================
// OnNewConnection
//=========================================================================
bool GridSessionHandshake::OnNewConnection(const AZStd::string& address)
{
    AZStd::lock_guard<AZStd::mutex> l(m_dataLock);

    if (m_isMigratingHost)
    {
        return false;
    }

    if (m_banList.find(address) != m_banList.end())
    {
        return false;
    }
    return true;
}

//=========================================================================
// OnDisconnect
//=========================================================================
void GridSessionHandshake::OnDisconnect(ConnectionID id)
{
    AZStd::lock_guard<AZStd::mutex> l(m_dataLock);
    for (NewConnectionsType::iterator connIt = m_newConnections.begin(); connIt != m_newConnections.end(); ++connIt)
    {
        if (connIt->m_id == id)
        {
            m_newConnections.erase(connIt); // remove the connection from the list
            return;
        }
    }
}

//=========================================================================
// BanAddress
//=========================================================================
void GridSessionHandshake::BanAddress(AZStd::string address)
{
    AZStd::lock_guard<AZStd::mutex> l(m_dataLock);
    m_banList.insert(address);
}

//=========================================================================
// SetHost
//=========================================================================
void GridSessionHandshake::SetHost(bool isHost)
{
    AZStd::lock_guard<AZStd::mutex> l(m_dataLock);
    m_isHost = isHost;
}

//=========================================================================
// SetInvited
//=========================================================================
void GridSessionHandshake::SetInvited(bool isInvited)
{
    AZStd::lock_guard<AZStd::mutex> l(m_dataLock);
    m_isInvited = isInvited;
}

//=========================================================================
// SetHostMigration
//=========================================================================
void GridSessionHandshake::SetHostMigration(bool isMigrating)
{
    AZStd::lock_guard<AZStd::mutex> l(m_dataLock);
    m_isMigratingHost = isMigrating;
}

//=========================================================================
// SetUserData
//=========================================================================
void GridSessionHandshake::SetUserData(const void* data, size_t dataSize)
{
    AZStd::lock_guard<AZStd::mutex> l(m_dataLock);
    const AZ::u8* bytes = reinterpret_cast<const AZ::u8*>(data);
    m_userData.assign(bytes, bytes + dataSize);
}

//=========================================================================
// SetSessionId
//=========================================================================
void GridSessionHandshake::SetSessionId(AZStd::string sessionId)
{
    AZStd::lock_guard<AZStd::mutex> l(m_dataLock);
    m_sessionId = sessionId;
}

//=========================================================================
// AcquireNewConnections
//=========================================================================
GridSessionHandshake::NewConnectionsType& GridSessionHandshake::AcquireNewConnections()
{
    m_dataLock.lock();
    return m_newConnections;
}

//=========================================================================
// ReleaseNewConnections
//=========================================================================
void GridSessionHandshake::ReleaseNewConnections()
{
    m_dataLock.unlock();
}
