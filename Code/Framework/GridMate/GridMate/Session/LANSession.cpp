/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/string/conversions.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Uuid.h>

#include <GridMate/Carrier/SocketDriver.h>
#include <GridMate/Carrier/Utils.h>

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/ReplicaFunctions.h>

#include <GridMate/Serialize/DataMarshal.h>

#include <GridMate/Session/LANSession.h>

//////////////////////////////////////////////////////////////////////////
// Implementations
namespace GridMate
{
    class LANMember;
    /**
    * Member ID is unique identifier for a member in the session.
    * \note we can typedef MemberID to a string, this will be as generic as it gets!
    */
    struct LANMemberID
        : public MemberID
    {
        friend class LANMemberIDMarshaler;

        static LANMemberID Create(MemberIDCompact id, const AZStd::string& address)
        {
            LANMemberID mid;
            mid.m_id = id;
            mid.m_address = address;
            return mid;
        }

        void SetAddress(const AZStd::string& address)
        {
            m_address = address;
        }

        MemberIDCompact GetID() const { return m_id; }

        AZStd::string ToString() const override
        {
            return AZStd::string::format("%x", m_id);
        }
        AZStd::string ToAddress() const override        { return m_address; }
        MemberIDCompact Compact() const override { return m_id; }
        bool IsValid() const override            { return m_id != 0; }

    private:
        MemberIDCompact m_id;
        AZStd::string m_address;
    };

    class LANMemberIDMarshaler
    {
    public:
        void Marshal(WriteBuffer& wb, const LANMemberID& id) const
        {
            wb.Write(id.m_id);
        }
        void Unmarshal(LANMemberID& id, ReadBuffer& rb) const
        {
            rb.Read(id.m_id);
        }
    };

    class LANSessionReplica;

    struct LANSessionMsg
    {
        LANSessionMsg(const VersionType& version)
            : m_version(version)
            , m_message(0)
        {}

        bool ValidateVersion(const VersionType& version) { return m_version == version; }

        enum MessageType
        {
            MT_SEARCH = 0,  ///< Called from a LAN client searches for LAN sessions.
            MT_SEARCH_RESULT,
            // Other messages are communicated trough replicas.
        };

        VersionType m_version; ///< Session crc
        AZ::u8 m_message; ///< messsage \ref MessageType
    };

    /**
    *
    */
    class LANSession
        : public GridSession
    {
        friend class LANSessionService;
        friend class LANSessionReplica;
        friend class LANMember;
    public:
        GM_CLASS_ALLOCATOR(LANSession);

        void Update() override;

        // TEMP so the factory can have access to the function.
        bool AddMember(GridMember* member) override { return GridSession::AddMember(member); }

        /// Creates the local player.
        GridMember* CreateLocalMember(bool isHost, bool isInvited, RemotePeerMode peerMode);
        /// Creates remote player, when he wants to join.
        GridMember* CreateRemoteMember(const AZStd::string& address, ReadBuffer& data, RemotePeerMode peerMode, ConnectionID connId = InvalidConnectionID) override;

        /// Called when we receive the session replica. We create one and return the pointer.
        LANSessionReplica*  OnSessionReplicaArrived();

        /// Called when session parameters have changed.
        void OnSessionParamChanged(const GridSessionParam& param) override { (void)param; }
        void OnSessionParamRemoved(const AZStd::string& paramId) override { (void)paramId; }

    private:
        explicit LANSession(LANSessionService* service);
        ~LANSession() override;

        /// Hosting a session.
        bool Initialize(const LANSessionParams& params, const CarrierDesc& carrierDesc);

        /// Joining a session.
        bool Initialize(const LANSearchInfo& info, const JoinParams& params, const CarrierDesc& carrierDesc);

        bool    MatchMake(const LANSearchParams& sp);

        AZStd::string  MakeSessionId();

        Driver* m_driver;   ///< Driver for network searches

        //////////////////////////////////////////////////////////////////////////
        // State machine
        // just override a few state handlers, the right way would be to sub state the
        // current ones. But this is less work.
        bool OnStateCreate(AZ::HSM& sm, const AZ::HSM::Event& e) override;
        bool OnStateDelete(AZ::HSM& sm, const AZ::HSM::Event& e) override;
        bool OnStateHostMigrateSession(AZ::HSM& sm, const AZ::HSM::Event& e) override;
        //////////////////////////////////////////////////////////////////////////
    };

    /**
    * \note many features will be common for most platforms, make sure they are moved to
    * a base class, where users can reuse and enhance them.
    */
    class LANSessionReplica
        : public Internal::GridSessionReplica
    {
    public:
        class LANSessionReplicaDesc
            : public ReplicaChunkDescriptor
        {
        public:
            LANSessionReplicaDesc()
                : ReplicaChunkDescriptor(LANSessionReplica::GetChunkName(), sizeof(LANSessionReplica))
            {
            }

            ReplicaChunkBase* CreateFromStream(UnmarshalContext& mc) override
            {
                LANSession* session = static_cast<LANSession*>(static_cast<GridSession*>(mc.m_rm->GetUserContext(AZ_CRC("GridSession", 0x099df4e6))));
                AZ_Assert(session, "We need to have a valid session!");
                return session->OnSessionReplicaArrived();
            }

            void DiscardCtorStream(UnmarshalContext&) override {}
            void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override { delete chunkInstance; }
            void MarshalCtorData(ReplicaChunkBase*, WriteBuffer&) override {}
        };

        GM_CLASS_ALLOCATOR(LANSessionReplica);
        static const char* GetChunkName() { return "GridMateLANSessionReplica"; }

        LANSessionReplica(LANSession* session)
            : GridSessionReplica(session)
            , m_hostPort("HostPort")
        {}

        DataSet<AZ::u32> m_hostPort;        ///< Port on which the host should provide matchmaking services.
    };

    struct SessionMemberInfoCtorContext
        : public CtorContextBase
    {
        CtorDataSet<MemberIDCompact> m_memberId;
        CtorDataSet<AZStd::string> m_memberAddress; ///< As the server/host sees it!
        CtorDataSet<RemotePeerMode> m_peerMode;
        CtorDataSet<bool> m_isHost;
    };

    /**
    *
    */
    class LANMember
        : public GridMember
    {
        friend class LANSession;
        friend class LANMemberState;

    public:
        class LANMemberDesc
            : public ReplicaChunkDescriptor
        {
        public:
            LANMemberDesc()
                : ReplicaChunkDescriptor(LANMember::GetChunkName(), sizeof(LANMember))
            {
            }

            ReplicaChunkBase* CreateFromStream(UnmarshalContext& mc) override
            {
                SessionMemberInfoCtorContext ctorContext;
                ctorContext.Unmarshal(*mc.m_iBuf);

                LANSession* session = static_cast<LANSession*>(static_cast<GridSession*>(mc.m_rm->GetUserContext(AZ_CRC("GridSession", 0x099df4e6))));
                AZ_Assert(session, "We need to have a valid session!");
                LANMember* member;
                MemberIDCompact memberId = ctorContext.m_memberId.Get();
                AZStd::string memberAddress = ctorContext.m_memberAddress.Get();
                RemotePeerMode remotePeerMode = ctorContext.m_peerMode.Get();
                bool isMemberHost = ctorContext.m_isHost.Get();

                if (memberId != static_cast<const LANMemberID&>(session->GetMyMember()->GetId()).GetID())
                {
                    ReadBuffer rb(EndianType::IgnoreEndian, reinterpret_cast<const char*>(&memberId), sizeof(memberId));
                    member = static_cast<LANMember*>(session->CreateRemoteMember(memberAddress, rb, remotePeerMode, isMemberHost ? mc.m_peer->GetConnectionId() : InvalidConnectionID));
                }
                else
                {
                    // just bind our local member
                    member = static_cast<LANMember*>(session->GetMyMember());
                }
                bool isAdded = session->AddMember(member);
                AZ_Assert(isAdded, "Failed to add a member, there is something wrong with the member replicas!");
                if (!isAdded)
                {
                    member = nullptr;
                }
                return member;
            }

            void DiscardCtorStream(UnmarshalContext& mc) override
            {
                SessionMemberInfoCtorContext ctorContext;
                ctorContext.Unmarshal(*mc.m_iBuf);
            }

            void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override
            {
                if (!static_cast<LANMember*>(chunkInstance)->IsLocal())
                {
                    delete chunkInstance;
                }
            }

            void MarshalCtorData(ReplicaChunkBase* chunkInstance, WriteBuffer& wb) override
            {
                LANMember* member = static_cast<LANMember*>(chunkInstance);
                SessionMemberInfoCtorContext ctorContext;
                ctorContext.m_memberId.Set(member->m_memberId.GetID());
                ctorContext.m_memberAddress.Set(member->m_memberId.ToAddress());
                ctorContext.m_peerMode.Set(member->m_peerMode.Get());
                ctorContext.m_isHost.Set(member->IsHost());
                ctorContext.Marshal(wb);
            }
        };

        GM_CLASS_ALLOCATOR(LANMember);
        static const char* GetChunkName() { return "GridMateLANMember"; }

        /// return an abstracted member id. (member ID is world unique but unrelated to player ID it's related to the session).
        const MemberID& GetId() const override           { return m_memberId; }
        /// returns a base player id, it's implementation is platform dependent. (NOT supported)
        const PlayerId* GetPlayerId() const override     { return nullptr; }

        /// Remote member ctor.
        LANMember(ConnectionID connId, const LANMemberID& id, LANSession* session);
        /// Local member ctor.
        LANMember(const LANMemberID& id, LANSession* session);

        LANMemberID         m_memberId;
    };

    /**
     *
     */
    class LANMemberState
        : public Internal::GridMemberStateReplica
    {
    public:
        GM_CLASS_ALLOCATOR(LANMemberState);
        static const char* GetChunkName() { return "GridMateLANMemberState"; }

        LANMemberState(GridMember* member = nullptr)
            : GridMemberStateReplica(member)
        {}
    };

    /**
    *
    */
    class LANSearch
        : public GridSearch
    {
        friend class LANSessionService;
    public:
        GM_CLASS_ALLOCATOR(LANSearch);
        ~LANSearch() override;

        /// Return true if the search has finished, otherwise false.
        unsigned int        GetNumResults() const override                   { return static_cast<unsigned int>(m_results.size()); }
        const SearchInfo*   GetResult(unsigned int index) const override     { return &m_results[index]; }
        void                AbortSearch() override;

    private:
        LANSearch(const LANSearchParams& searchParams, SessionService* service);
        void    Update() override;
        void            SearchDone();

        Driver*         m_driver;
        TimeStamp       m_timeStart;
        TimeStamp       m_lastSearchSend;                   ///< Time when last search was broadcasted.
        unsigned int    m_broadcastInterval;                ///< How ofter to send broadcast to find sessions in milliseconds.
        unsigned int    m_timeOutMS;                        ///< Timeout in milliseconds.
        vector<LANSearchInfo> m_results;                    ///< Search results
        LANSearchParams m_searchParams;
        IGridMate*      m_gridMate;
    };
}
//////////////////////////////////////////////////////////////////////////

namespace GridMate
{
    namespace Platform
    {
        void AssignExtendedName(AZStd::string& extendedName);
    }
}

using namespace GridMate;

//=========================================================================
// LANMember
// Local member ctor.
// [2/1/2011]
//=========================================================================
LANMember::LANMember(const LANMemberID& id, LANSession* session)
    : GridMember(id.Compact())
    , m_memberId(id)
{
    AZStd::string extendedName;
    Platform::AssignExtendedName(extendedName);

    m_session = session;
    m_clientState = CreateReplicaChunk<LANMemberState>(this);
    m_clientState->m_name.Set(extendedName);
    NatType natType = NAT_OPEN; // for LAN this is the case
    m_clientState->m_natType.Set(natType);

    m_clientStateReplica = Replica::CreateReplica(extendedName.c_str());
    m_clientStateReplica->AttachReplicaChunk(m_clientState);
}

//=========================================================================
// LANMember
// Remote member ctor.
// [12/13/2010]
//=========================================================================
LANMember::LANMember(ConnectionID connId, const LANMemberID& id, LANSession* session)
    : GridMember(id.Compact())
    , m_memberId(id)
{
    m_session = session;
    m_connectionId = connId;
}

//=========================================================================
// LANSession
// [12/13/2010]
//=========================================================================
LANSession::LANSession(LANSessionService* service)
    : GridSession(service)
    , m_driver(nullptr)
{
}

bool LANSession::Initialize(const LANSessionParams& params, const CarrierDesc& carrierDesc)
{
    if (!GridSession::Initialize(carrierDesc))
    {
        Shutdown();
        return false;
    }

    // socket driver to handle session search and matchmake
    LANSessionReplica::ParamContainer sessionParams;
    Driver::ResultCode initResult = Driver::EC_OK;
    if (params.m_port != 0)
    {
        m_driver = aznew SocketDriver(false, true);
        initResult = m_driver->Initialize(m_carrierDesc.m_familyType, params.m_address.c_str(), params.m_port, true, 2048, 2048);

        if (initResult != Driver::EC_OK)
        {
            Shutdown();
            return false;
        }

        for (AZStd::size_t i = 0; i < params.m_numParams; ++i)
        {
            sessionParams.push_back(params.m_params[i]);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // start up the session state we will bind it later
    AZ_Assert(params.m_numPublicSlots < 0xff && params.m_numPrivateSlots < 0xff, "Can't have more than 255 slots!");
    AZ_Assert(params.m_numPublicSlots > 0 || params.m_numPrivateSlots > 0, "You don't have any slots open!");

    LANSessionReplica* state = CreateReplicaChunk<LANSessionReplica>(this);
    state->m_numFreePrivateSlots.Set(static_cast<unsigned char>(params.m_numPrivateSlots));
    state->m_numFreePublicSlots.Set(static_cast<unsigned char>(params.m_numPublicSlots));
    state->m_peerToPeerTimeout.Set(params.m_peerToPeerTimeout);
    state->m_hostMigrationTimeout.Set(params.m_hostMigrationTimeout);
    state->m_hostMigrationVotingTime.Set(AZStd::GetMin(params.m_hostMigrationVotingTime, params.m_hostMigrationTimeout / 2));
    state->m_flags.Set(params.m_flags);
    state->m_topology.Set(params.m_topology);
    state->m_params.Set(sessionParams);
    state->m_hostPort.Set(params.m_port);
    m_state = state;
    //////////////////////////////////////////////////////////////////////////

    m_myMember = CreateLocalMember(true, true, Mode_Peer);

    SetUpStateMachine();

    RequestEventData(SE_HOST, params);
    return true;
}

bool LANSession::Initialize(const LANSearchInfo& info, const JoinParams& params, const CarrierDesc& carrierDesc)
{
    if (!GridSession::Initialize(carrierDesc))
    {
        Shutdown();
        return false;
    }

    m_hostAddress = SocketDriverCommon::IPPortToAddressString(info.m_serverIP.c_str(), info.m_serverPort);
    m_sessionId = info.m_sessionId;

    // start up the session state we will bind it later
    m_state = CreateReplicaChunk<LANSessionReplica>(this);

    m_myMember = CreateLocalMember(false, false, params.m_desiredPeerMode);

    SetUpStateMachine();

    RequestEventData(SE_JOIN, info);    // trigger the join event
    return true;
}

//=========================================================================
// ~LANSession
// [12/13/2010]
//=========================================================================
LANSession::~LANSession()
{
    delete m_driver;
    m_driver = nullptr;
}

//=========================================================================
// Update
// [12/13/2010]
//=========================================================================
void
LANSession::Update()
{
    //////////////////////////////////////////////////////////////////////////
    // Hosting session
    if (m_driver)
    {
        // receive connection requests
        static const int maxDataSize = 2048;
        char data[maxDataSize];
        while (true)
        {
            AZStd::intrusive_ptr<DriverAddress> from;
            Driver::ResultCode result;
            unsigned int recvd = m_driver->Receive(data, maxDataSize, from, &result);
            if (result != Driver::EC_OK)
            {
                AZ_TracePrintf("GridMate", "LANSession::Receive - recvfrom failed with code 0x%08x\n", result);
                break;
            }
            if (recvd == 0)
            {
                break;
            }

            ReadBuffer rb(kSessionEndian, data, recvd);
            LANSessionMsg msg(m_carrierDesc.m_version);
            rb.Read(msg.m_version);
            if (!msg.ValidateVersion(m_carrierDesc.m_version))
            {
                continue; // wrong message, ignore
            }
            rb.Read(msg.m_message);
            if (msg.m_message == LANSessionMsg::MT_SEARCH)
            {
                //////////////////////////////////////////////////////////////////////////
                // Matchmake
                //  read the user attributes
                LANSearchParams ssp;
                rb.Read(ssp.m_numParams);
                for (unsigned int i = 0; i < ssp.m_numParams; ++i)
                {
                    rb.Read(ssp.m_params[i].m_op);
                    rb.Read(ssp.m_params[i].m_id);
                    rb.Read(ssp.m_params[i].m_value);
                    rb.Read(ssp.m_params[i].m_type);
                }

                // compare params and filter results.
                if (!MatchMake(ssp))
                {
                    continue;
                }
                //////////////////////////////////////////////////////////////////////////

                // send back the session info structure
                LANSearchInfo si;
                si.m_serverPort = static_cast<AZ::u16>(m_carrier ? m_carrier->GetPort() : 0);// send the port for the session
                si.m_numFreePrivateSlots = m_state->m_numFreePrivateSlots.Get();
                si.m_numFreePublicSlots = m_state->m_numFreePublicSlots.Get();
                si.m_numUsedPrivateSlots = m_state->m_numUsedPrivateSlots.Get();
                si.m_numUsedPublicSlots = m_state->m_numUsedPublicSlots.Get();

                const LANSessionReplica::ParamContainer& params = AZStd::static_pointer_cast<LANSessionReplica>(m_state)->m_params.Get();
                si.m_numParams = static_cast<AZ::u32>(params.size());
                for (AZ::u32 i = 0; i < si.m_numParams; ++i)
                {
                    si.m_params[i].m_id = params[i].m_id;
                    si.m_params[i].m_value = params[i].m_value;
                    si.m_params[i].m_type = params[i].m_type;
                }

                msg.m_message = LANSessionMsg::MT_SEARCH_RESULT;

                // write and send session information
                WriteBufferStatic<> wb(kSessionEndian);

                wb.Write(msg.m_version);
                wb.Write(msg.m_message);

                wb.Write(si.m_serverPort);
                wb.Write(m_sessionId);
                wb.Write(si.m_numFreePrivateSlots);
                wb.Write(si.m_numFreePublicSlots);
                wb.Write(si.m_numUsedPrivateSlots);
                wb.Write(si.m_numUsedPublicSlots);
                wb.Write(si.m_numParams);
                for (unsigned int i = 0; i < si.m_numParams; ++i)
                {
                    wb.Write(si.m_params[i].m_id);
                    wb.Write(si.m_params[i].m_value);
                    wb.Write(si.m_params[i].m_type);
                }

                // send back the information.
                result = m_driver->Send(from, wb.Get(), static_cast<unsigned int>(wb.Size()));
                if (result != Driver::EC_OK)
                {
                    AZ_TracePrintf("GridMate", "LANSession::Send - sendto failed with code 0x%08x at %s", result, from->ToString().c_str());
                }
            }
            else
            {
                AZ_Assert(false, "We don't handle %d message!", msg.m_message);
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////

    // THIS should be last in the update, it might delete the session internally (this pointer will become invalid!
    GridSession::Update();
}

//=========================================================================
// CompareParams
// [7/15/2013]
//=========================================================================
template<class Param>
inline bool CompareParams(const Param& left, const Param& right, GridSessionSearchOperators op)
{
    switch (op)
    {
    case GridSessionSearchOperators::SSO_OPERATOR_EQUAL:
        if (left != right)
        {
            return false;
        }
        break;
    case GridSessionSearchOperators::SSO_OPERATOR_NOT_EQUAL:
        if (left == right)
        {
            return false;
        }
        break;
    case GridSessionSearchOperators::SSO_OPERATOR_GREATER_EQUAL_THAN:
        if (left < right)
        {
            return false;
        }
        break;
    case GridSessionSearchOperators::SSO_OPERATOR_GREATER_THAN:
        if (left <= right)
        {
            return false;
        }
        break;
    case GridSessionSearchOperators::SSO_OPERATOR_LESS_EQUAL_THAN:
        if (left > right)
        {
            return false;
        }
        break;
    case GridSessionSearchOperators::SSO_OPERATOR_LESS_THAN:
        if (left >= right)
        {
            return false;
        }
        break;
    default:
        AZ_Warning("GridMate", false, "Invalid operator type %d", op);
        return false;
    }

    return true;
}

//=========================================================================
// MatchMake
// [12/13/2010]
//=========================================================================
bool
LANSession::MatchMake(const LANSearchParams& sp)
{
    bool isGoodMatch = true;
    for (unsigned int i = 0; i < sp.m_numParams; ++i)
    {
        // find parameter
        const LANSessionReplica::ParamContainer& params = AZStd::static_pointer_cast<LANSessionReplica>(m_state)->m_params.Get();
        AZStd::size_t j = 0;
        for (; j < params.size(); ++j)
        {
            if (sp.m_params[i].m_id == params[j].m_id && sp.m_params[i].m_type == params[j].m_type)
            {
                break;
            }
        }

        if (j == params.size())
        {
            isGoodMatch = false; // parameter was NOT found
            break;
        }

        switch (sp.m_params[i].m_type)
        {
        case GridSessionParam::VT_INT32:
        {
            AZ::s32 left = AZStd::stoi(params[j].m_value);
            AZ::s32 right = AZStd::stoi(sp.m_params[i].m_value);
            isGoodMatch = CompareParams(left, right, sp.m_params[i].m_op);
            break;
        };
        case GridSessionParam::VT_INT64:
        {
            AZ::s64 left = AZStd::stoll(params[j].m_value);
            AZ::s64 right = AZStd::stoll(sp.m_params[i].m_value);
            isGoodMatch = CompareParams(left, right, sp.m_params[i].m_op);
            break;
        };
        case GridSessionParam::VT_FLOAT:
        {
            float left = AZStd::stof(params[j].m_value);
            float right = AZStd::stof(sp.m_params[i].m_value);
            isGoodMatch = CompareParams(left, right, sp.m_params[i].m_op);
            break;
        };
        case GridSessionParam::VT_DOUBLE:
        {
            double left = AZStd::stof(params[j].m_value);
            double right = AZStd::stof(sp.m_params[i].m_value);
            isGoodMatch = CompareParams(left, right, sp.m_params[i].m_op);
            break;
        };
        case GridSessionParam::VT_STRING:
        {
            isGoodMatch = CompareParams(params[j].m_value, sp.m_params[i].m_value, sp.m_params[i].m_op);
            break;
        };
        default:
            AZ_Assert(false, "Unsupported parameter type %d", params[j].m_type);
            isGoodMatch = false;
        }

        if (!isGoodMatch)
        {
            break;
        }
    }

    return isGoodMatch;
}

//=========================================================================
// CreateLocalMember
// [2/1/2011]
//=========================================================================
GridMember*
LANSession::CreateLocalMember(bool isHost, bool isInvited, RemotePeerMode peerMode)
{
    AZ_Assert(m_myMember == nullptr, "We already have added a local member!");

    AZStd::string ip = Utils::GetMachineAddress(m_carrierDesc.m_familyType);
    AZStd::string address = SocketDriverCommon::IPPortToAddressString(ip.c_str(), m_carrierDesc.m_port);
    /////////////////////////////////////////////////////////////////////////////
    // TODO: Use the UUID as an ID, we will need to add marshalers and so on
    AZ::Uuid uuid = AZ::Uuid::CreateRandom();
    MemberIDCompact id = AZ::Crc32(&uuid.data, sizeof(uuid.data));
    /////////////////////////////////////////////////////////////////////////////
    LANMemberID myId = LANMemberID::Create(id, address);

    LANMember* member = CreateReplicaChunk<LANMember>(myId, this);
    member->SetHost(isHost);
    member->SetInvited(isInvited);
    member->m_peerMode.Set(peerMode);
    return member;
}

//=========================================================================
// CreateRemoteMember
// [2/2/2011]
//==========================================================================
GridMember*
LANSession::CreateRemoteMember(const AZStd::string& address, ReadBuffer& data, RemotePeerMode peerMode, ConnectionID connId)
{
    MemberIDCompact id;
    data.Read(id);
    LANMemberID memberId = LANMemberID::Create(id, address);
    AZ_Warning("GridMate", GetTopology() != ST_INVALID, "Invalid sesson topology! Did session replica arrive yet?");
    if (GetTopology() == ST_PEER_TO_PEER && peerMode == Mode_Peer && static_cast<LANMember*>(m_myMember)->m_peerMode.Get() == Mode_Peer && connId == InvalidConnectionID)
    {
        connId = m_carrier->Connect(memberId.ToAddress());
    }

    return CreateReplicaChunk<LANMember>(connId, memberId, this);
}

//=========================================================================
// OnSessionReplicaArrived
// [2/1/2011]
//=========================================================================
LANSessionReplica*
LANSession::OnSessionReplicaArrived()
{
    AZ_TracePrintf("GridMate", "(%s - %s) has joined session: %s\n", m_myMember->GetId().ToString().c_str(), m_myMember->GetId().ToAddress().c_str(),
        m_sessionId.c_str());
    // join is complete
    RequestEvent(LANSession::SE_JOINED);
    return static_cast<LANSessionReplica*>(m_state.get());
}

//////////////////////////////////////////////////////////////////////////
// Session state machine overrides
bool
LANSession::OnStateCreate(AZ::HSM& sm, const AZ::HSM::Event& e)
{
    bool isProcessed = GridSession::OnStateCreate(sm, e);

    switch (e.id)
    {
    case AZ::HSM::EnterEventId:
    {
        // patch the ID if we use implicit port
        AZ_Assert(m_carrier, "Carrier must be created!");
        AZStd::string ip;
        unsigned int port;
        SocketDriverCommon::AddressStringToIPPort(m_myMember->GetId().ToAddress(), ip, port);
        AZ_Assert(port == 0 || port == m_carrier->GetPort(), "Carrier port missmatch! It should either be 0 (and patched here) in the implicit bind or the port number for explicit bind!");
        if (port == 0)
        {
            static_cast<LANMember*>(m_myMember)->m_memberId.SetAddress(SocketDriverCommon::IPPortToAddressString(ip.c_str(), m_carrier->GetPort()));
        }

        // Set my ID as userdata for the handshake (so we can proparage to the host when we connect)
        MemberIDCompact id = static_cast<const LANMemberID&>(m_myMember->GetId()).GetID();
        WriteBufferStatic<> wb(kSessionEndian);
        wb.Write(id);
        SetHandshakeUserData(wb.Get(), wb.Size());

        if (IsHost())
        {
            m_sessionId = MakeSessionId();
        }

        RequestEvent(SE_CREATED);     // just send create even
    } return true;
    }
    ;
    return isProcessed;
}
bool
LANSession::OnStateDelete(AZ::HSM& sm, const AZ::HSM::Event& e)
{
    bool isProcessed = GridSession::OnStateDelete(sm, e);

    switch (e.id)
    {
    case AZ::HSM::EnterEventId:
    {
        RequestEvent(SE_DELETED);
    } return true;
    }

    return isProcessed;
}
bool
LANSession::OnStateHostMigrateSession(AZ::HSM& sm, const AZ::HSM::Event& e)
{
    bool isProcessed = GridSession::OnStateHostMigrateSession(sm, e);

    switch (e.id)
    {
    case AZ::HSM::EnterEventId:
    {
        if (m_sessionId.empty())
        {
            // we are the new host create a new session and let the rest of the clients know.
            m_sessionId = MakeSessionId();

            // possibly reset the room slots and reassign them after host migration is complete.
            RequestEvent(SE_HM_SESSION_MIGRATED);
        }
        else
        {
            // we are a client and we need to migrate the session to the new session ID
            RequestEvent(SE_HM_CLIENT_SESSION_MIGRATED);
        }
    } return true;
    case AZ::HSM::ExitEventId:
    {
        if (IsHost())
        {
            AZ_Assert(m_driver == nullptr, "If we just became the host we should not have a matchmaking services enabled!");
            unsigned int hostPort = AZStd::static_pointer_cast<LANSessionReplica>(m_state)->m_hostPort.Get();
            // socket driver to handle session search and matchmake...
            if (hostPort != 0)
            {
                m_driver = aznew SocketDriver(false, true);
                // TODO: To support Ipv6 properly we need to store the family and bind address in the LANSessionReplica
                if (m_driver->Initialize(Driver::BSD_AF_INET, nullptr, hostPort, true) != Driver::EC_OK)
                {
                    // check the output for more info
                    AZStd::string errorMsg = AZStd::string::format("Failed to initialize socket at port %d!", hostPort);
                    EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionError, this, errorMsg);
                    EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionError, this, errorMsg);
                    // We can't be a real host if we failed to provide matching services.
                    Leave(false);
                }
            }
        }
    } return true;
    }

    return isProcessed;
}

//=========================================================================
// MakeSessionId
// [3/7/2013]
//=========================================================================
AZStd::string
LANSession::MakeSessionId()
{
    char temp[64];
    AZ::Uuid::CreateRandom().ToString(temp, AZ_ARRAY_SIZE(temp), false, false);
    return temp;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// LANSearch
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// LANSearch
// [12/13/2010]
//=========================================================================
LANSearch::LANSearch(const LANSearchParams& searchParams, SessionService* service)
    : GridSearch(service)
    , m_timeOutMS(searchParams.m_timeOutMs)
    , m_searchParams(searchParams)
{
    m_gridMate = service->GetGridMate();
    AZ_Assert(m_gridMate, "SessionService is not bound to GridMate instance");
    m_results.reserve(m_searchParams.m_maxSessions);

    m_driver = aznew SocketDriver(false, true);
    if (m_driver->Initialize(searchParams.m_familyType, searchParams.m_listenAddress.c_str(), searchParams.m_listenPort, true) != Driver::EC_OK)
    {
        SearchDone();
    }

    m_timeStart = AZStd::chrono::system_clock::now();
    m_broadcastInterval = searchParams.m_broadcastFrequencyMs;

    Update(); // Call an instant update to send a broadcast
}

//=========================================================================
// ~LANSearch
// [12/13/2010]
//=========================================================================
LANSearch::~LANSearch()
{
    if (!IsDone())
    {
        AbortSearch();
    }
}


//=========================================================================
// Update
// [12/13/2010]
//=========================================================================
void
LANSearch::Update()
{
    if (m_isDone)
    {
        return;
    }

    TimeStamp now = AZStd::chrono::system_clock::now();

    // broadcast the LAN search.
    AZStd::chrono::milliseconds timeElapsed = now - m_lastSearchSend;
    if (timeElapsed.count() >= m_broadcastInterval)
    {
        m_lastSearchSend = now;

        // write and send session information
        WriteBufferStatic<> wb(kSessionEndian);
        LANSessionMsg msg(m_searchParams.m_version);
        msg.m_message = LANSessionMsg::MT_SEARCH;
        wb.Write(msg.m_version);
        wb.Write(msg.m_message);

        // Matchmake
        // don't send the broadcast and listen ports
        wb.Write(m_searchParams.m_numParams);
        for (unsigned int i = 0; i < m_searchParams.m_numParams; ++i)
        {
            wb.Write(m_searchParams.m_params[i].m_op);
            wb.Write(m_searchParams.m_params[i].m_id);
            wb.Write(m_searchParams.m_params[i].m_value);
            wb.Write(m_searchParams.m_params[i].m_type);
        }

        AZStd::string serverAddress = m_searchParams.m_serverAddress;
        if (serverAddress.empty())
        {
            serverAddress = Utils::GetBroadcastAddress(m_searchParams.m_familyType);
        }
        Driver::ResultCode result = m_driver->Send(m_driver->CreateDriverAddress(SocketDriverCommon::IPPortToAddressString(serverAddress.c_str(), m_searchParams.m_serverPort)), wb.Get(), static_cast<unsigned int>(wb.Size()));
        if (result != Driver::EC_OK)
        {
            AZ_TracePrintf("GridMate", "LANSearch::Send - to %s port %d failed with code 0x%08x\n", serverAddress.c_str(), m_searchParams.m_serverPort, result);
        }
    }

    // Receive all the
    static const int k_maxDataSize = 2048;
    char data[k_maxDataSize];
    bool haveMaxResults = false;
    while (true)
    {
        AZStd::intrusive_ptr<DriverAddress> from;
        Driver::ResultCode result;
        unsigned int recvd = m_driver->Receive(data, k_maxDataSize, from, &result);
        if (result != Driver::EC_OK)
        {
            AZ_TracePrintf("GridMate", "LANSearch::Receive - recvfrom failed with code 0x%08x\n", result);
            break;
        }
        if (recvd == 0)
        {
            break;
        }

        ReadBuffer rb(kSessionEndian, data, recvd);
        LANSessionMsg msg(m_searchParams.m_version);
        rb.Read(msg.m_version);
        if (!msg.ValidateVersion(m_searchParams.m_version))
        {
            continue; // wrong message, ignore
        }
        rb.Read(msg.m_message);

        if (msg.m_message != LANSessionMsg::MT_SEARCH_RESULT)
        {
            continue;
        }

        LANSearchInfo si;
        unsigned int port;  // we don't use that port (as this is the search port)
        SocketDriverCommon::AddressStringToIPPort(from->ToAddress(), si.m_serverIP, port);
        rb.Read(si.m_serverPort);
        rb.Read(si.m_sessionId);
        rb.Read(si.m_numFreePrivateSlots);
        rb.Read(si.m_numFreePublicSlots);
        rb.Read(si.m_numUsedPrivateSlots);
        rb.Read(si.m_numUsedPublicSlots);
        rb.Read(si.m_numParams);
        for (unsigned int i = 0; i < si.m_numParams; ++i)
        {
            rb.Read(si.m_params[i].m_id);
            rb.Read(si.m_params[i].m_value);
            rb.Read(si.m_params[i].m_type);
        }

        bool isInResults = false;
        for (AZStd::size_t i = 0; i < m_results.size(); ++i)
        {
            if (m_results[i].m_sessionId == si.m_sessionId)
            {
                isInResults = true;
                break;
            }
        }

        if (!isInResults)
        {
            m_results.push_back(si);
        }

        if (m_results.size() == m_searchParams.m_maxSessions)
        {
            haveMaxResults = true;
            break;
        }
    }

    // check the timeout
    timeElapsed = now - m_timeStart;
    if (haveMaxResults || timeElapsed.count() > m_timeOutMS)
    {
        // we are done
        SearchDone();
        return;
    }
}

//=========================================================================
// AbortSearch
// [12/13/2010]
//=========================================================================
void
LANSearch::AbortSearch()
{
    SearchDone();
}

//=========================================================================
// SearchDone
// [12/13/2010]
//=========================================================================
void
LANSearch::SearchDone()
{
    m_isDone = true;
    delete m_driver;
    m_driver = nullptr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// LANSessionService
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// LANSessionService
// [12/13/2010]
//=========================================================================
LANSessionService::LANSessionService(const SessionServiceDesc& desc)
    : SessionService(desc)
{
#if AZ_TRAIT_OS_USE_WINDOWS_SOCKETS
    WSAData wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0)
    {
        AZ_TracePrintf("GridMate", "Failed on WSAStartup with code %d\n", err);
    }
#endif
}

LANSessionService::~LANSessionService()
{
#if AZ_TRAIT_OS_USE_WINDOWS_SOCKETS
    WSACleanup();
#endif
}

void LANSessionService::OnServiceRegistered(IGridMate* gridMate)
{
    SessionService::OnServiceRegistered(gridMate);

    ReplicaChunkDescriptorTable::Get().RegisterChunkType<LANSessionReplica, LANSessionReplica::LANSessionReplicaDesc>();
    ReplicaChunkDescriptorTable::Get().RegisterChunkType<LANMember, LANMember::LANMemberDesc>();
    ReplicaChunkDescriptorTable::Get().RegisterChunkType<LANMemberState>();

    LANSessionServiceBus::Handler::BusConnect(gridMate);

    EBUS_DBG_EVENT(Debug::SessionDrillerBus, OnSessionServiceReady);
    EBUS_EVENT_ID(m_gridMate, SessionEventBus, OnSessionServiceReady);
}

void LANSessionService::OnServiceUnregistered(IGridMate* gridMate)
{
    SessionService::OnServiceUnregistered(gridMate);

    LANSessionServiceBus::Handler::BusDisconnect(gridMate);
}

GridSession* LANSessionService::HostSession(const LANSessionParams& params, const CarrierDesc& carrierDesc)
{
    LANSession* session = aznew LANSession(this);
    if (!session->Initialize(params, carrierDesc))
    {
        delete session;
        return nullptr;
    }

    return session;
}

GridSession* LANSessionService::JoinSessionBySearchInfo(const LANSearchInfo& searchInfo, const JoinParams& params, const CarrierDesc& carrierDesc)
{
    LANSession* session = aznew LANSession(this);
    if (!session->Initialize(searchInfo, params, carrierDesc))
    {
        delete session;
        return nullptr;
    }

    return session;
}

GridSession* LANSessionService::JoinSessionBySessionIdInfo(const SessionIdInfo* info, const JoinParams& params, const CarrierDesc& carrierDesc)
{
    LANSearchInfo si;
    si.m_sessionId = info->m_sessionId;
    LANSession* session = aznew LANSession(this);
    if (!session->Initialize(si, params, carrierDesc))
    {
        delete session;
        return nullptr;
    }

    return session;
}

GridSearch* LANSessionService::StartGridSearch(const LANSearchParams& params)
{
    return aznew LANSearch(params, this);
}
