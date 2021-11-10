/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_SESSION_H
#define GM_SESSION_H

#include <GridMate/GridMate.h>
#include <GridMate/Carrier/Handshake.h>

#include <GridMate/Replica/ReplicaMgr.h>

#include <AzCore/PlatformId/PlatformId.h>
#include <AzCore/State/HSM.h>
#include <GridMate/Containers/queue.h>
#include <GridMate/Containers/unordered_set.h>
#include <GridMate/GridMateService.h>

#include <GridMate/Serialize/UtilityMarshal.h>

namespace GridMate
{
    class ReplicaManager;
    struct PlayerId;

    extern const EndianType kSessionEndian;

    namespace Debug {
        class SessionDriller;
    }

    typedef AZ::u32 MemberIDCompact;
    /**
     * MemberID interface class.
     */
    struct MemberID
    {
        virtual ~MemberID() {}
        virtual AZStd::string ToString() const = 0;
        virtual AZStd::string ToAddress() const = 0;
        virtual MemberIDCompact Compact() const = 0;
        virtual bool IsValid() const = 0;

        AZ_FORCE_INLINE bool operator==(const MemberID& rhs) const { return ToString() == rhs.ToString(); }
        AZ_FORCE_INLINE bool operator!=(const MemberID& rhs) const { return ToString() != rhs.ToString(); }
        AZ_FORCE_INLINE bool operator==(const MemberIDCompact& rhs) const { return Compact() == rhs; }
        AZ_FORCE_INLINE bool operator!=(const MemberIDCompact& rhs) const { return Compact() != rhs; }
    };

    typedef AZStd::string SessionID;

    struct SearchInfo;

    struct  StatisticsData {};

    struct GridSessionParam
    {
        enum ValueType
        {
            VT_INT32,
            VT_INT64,
            VT_FLOAT,
            VT_DOUBLE,
            VT_STRING,
            VT_INT32_ARRAY,
            VT_INT64_ARRAY,
            VT_FLOAT_ARRAY,
            VT_DOUBLE_ARRAY,
        };

        GridSessionParam()
            : m_type(VT_STRING) {}

        // Helper function to convert basic types to string and set the type.
        void SetValue(AZ::s32 value);
        void SetValue(AZ::s64 value);
        void SetValue(float value);
        void SetValue(double value);
        void SetValue(const char* value) { m_type = VT_STRING; m_value = value; }
        template<class Allocator>
        void SetValue(const AZStd::basic_string<char, AZStd::char_traits<char>, Allocator>& value) { m_type = VT_STRING; m_value = value; }
        void SetValue(AZ::s32* values, size_t numElements);
        void SetValue(AZ::s64* values, size_t numElements);
        void SetValue(float* values, size_t numElements);
        void SetValue(double* values, size_t numElements);

        AZStd::string m_id;
        AZStd::string m_value;
        AZ::u8 m_type;

        AZ_FORCE_INLINE bool operator==(const GridSessionParam& rhs) const { return m_type == rhs.m_type && m_id == rhs.m_id && m_value == rhs.m_value; }
    };

    enum SessionTopology : AZ::u8
    {
        /// Invalid topology, waiting for the host to deliver it's topology data.
        ST_INVALID,
        /// Standard client server. When a user joins he will be connected to the server only.
        ST_CLIENT_SERVER,
        /// Standard peer to peer. When a user joins he will connect to all other users.
        ST_PEER_TO_PEER,
        /// Client server mode, where client can have the replica/data ownership.
        ST_CLIENT_SERVER_DISTRIBUTED,
    };

    struct SessionParams
    {
        enum Flags
        {
            SF_HOST_MIGRATION = (1 << 0),                     ///< Enable/disable host migration for this session. (default: Enabled). It will always migrate the session even if you loose all connections.
            SF_HOST_MIGRATION_NO_EMPTY_SESSIONS = (1 << 1),   ///< Same as \ref SF_HOST_MIGRATION except it will NOT create sessions with 1 member, it will just leave the session.
        };
        SessionParams()
            : m_topology(ST_PEER_TO_PEER)
            , m_peerToPeerTimeout(10000)
            , m_hostMigrationTimeout(10000)
            , m_hostMigrationVotingTime(2000)
            , m_numPublicSlots(0)
            , m_numPrivateSlots(0)
            , m_flags(SF_HOST_MIGRATION)
            , m_numParams(0) {}

        SessionTopology     m_topology;
        unsigned int        m_peerToPeerTimeout;        ///< Peer to peer connectivity timeout in milliseconds. Recommended 2x the handshake time.
        unsigned int        m_hostMigrationTimeout;     ///< Timeout for a host migration procedure in milliseconds. If it takes more time we will leave the session.
        unsigned int        m_hostMigrationVotingTime;  ///< Minimum time that will spend voting (unless everybody voted - we have all the results) before we check the winner. IMPORTANT: value is clamped to less than 1/2 of m_hostMigrationTimeout.
        unsigned int        m_numPublicSlots;           ///< Number of slots for players this session will have.

        unsigned int        m_numPrivateSlots;
        unsigned char       m_flags;
        unsigned int        m_numParams;

        static const unsigned int k_maxNumParams = 32;
        GridSessionParam    m_params[k_maxNumParams];   ///< An array of session params.
    };

    struct JoinParams
    {
        JoinParams()
            : m_desiredPeerMode(Mode_Undefined)
        {}

        RemotePeerMode m_desiredPeerMode;
    };

    enum class GridSessionSearchOperators : AZ::u8
    {
        SSO_OPERATOR_EQUAL = 1,         ///< (==)
        SSO_OPERATOR_NOT_EQUAL,         ///< (!=)
        SSO_OPERATOR_LESS_THAN,         ///< (<)
        SSO_OPERATOR_LESS_EQUAL_THAN,   ///< (<=)
        SSO_OPERATOR_GREATER_THAN,      ///< (>)
        SSO_OPERATOR_GREATER_EQUAL_THAN,///< (>=)
    };

    struct GridSessionSearchParam
        : public GridSessionParam
    {
        GridSessionSearchOperators m_op;    ///< Operator \ref GridSessionSearchOperators
    };

    struct SearchParams
    {
        SearchParams()
            : m_maxSessions(s_defaultMaxSessions)
            , m_timeOutMs(2000)
            , m_numParams(0)
            , m_version(1)
        {}

        static const unsigned int s_defaultMaxSessions = 8; ///< default limit for returned session entries, user can override this by setting m_maxSessions

        unsigned int m_maxSessions; ///< maximum number of session entries to return from search
        unsigned int m_timeOutMs;
        AZ::u32 m_numParams; ///< number of parameters set in m_params
        GridSessionSearchParam m_params[SessionParams::k_maxNumParams]; ///< A list of params for a search (matchmaking).
        VersionType m_version;
    };

    struct SearchInfo
    {
        SearchInfo()
            : m_numFreePublicSlots(0)
            , m_numFreePrivateSlots(0)
            , m_numUsedPublicSlots(0)
            , m_numUsedPrivateSlots(0)
            , m_numPlayers(0)
            , m_numParams(0)
            , m_port(0)
        { }
        SessionID m_sessionId;
        AZ::u32 m_numFreePublicSlots;
        AZ::u32 m_numFreePrivateSlots;
        AZ::u32 m_numUsedPublicSlots;
        AZ::u32 m_numUsedPrivateSlots;
        AZ::u32 m_numPlayers;           ///< Number of players in the session.
        AZ::u32 m_numParams;
        AZ::u32 m_port;
        GridSessionParam m_params[SessionParams::k_maxNumParams];
    };

    struct SessionIdInfo
    {
        SessionID        m_sessionId;
    };

    class GridMember;
    class GridSession;
    class GridSearch;
    class IGridMate;

    /**
    * Grid Session Callbacks
    */
    class GridSessionCallbacks
        : public GridMateEBusTraits
    {
    public:
        virtual ~GridSessionCallbacks() {}
        /// Callback that is called when the Session service is ready to process sessions.
        virtual void OnSessionServiceReady() {}

        //virtual OnCommucationChanged() = 0  Callback that notifies the title when a member's communication settings change.

        /// Callback when we start a grid search.
        virtual void OnGridSearchStart(GridSearch* gridSearch) { (void)gridSearch; }
        /// Callback that notifies the title when a game search query have completed.
        virtual void OnGridSearchComplete(GridSearch* gridSearch) { (void)gridSearch; }
        /// Callback when we release (delete) a grid search. It's not safe to hold the grid pointer after this.
        virtual void OnGridSearchRelease(GridSearch* gridSearch) { (void)gridSearch; }

        /// Callback that notifies the title when a new member joins the game session.
        virtual void OnMemberJoined(GridSession* session, GridMember* member) { (void)session; (void)member; }
        /// Callback that notifies the title that a member is leaving the game session. member pointer is NOT valid after the callback returns.
        virtual void OnMemberLeaving(GridSession* session, GridMember* member) { (void)session; (void)member; }
        // \todo a better way will be (after we solve migration) is to supply a reason to OnMemberLeaving... like the member was kicked.
        // this will require that we actually remove the replica at the same moment.
        /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
        virtual void OnMemberKicked(GridSession* session, GridMember* member, AZ::u8 reason) { (void)session; (void)member; (void)reason; }
        /// Called when new session is created. Client session might not sync yet at this point.
        virtual void OnSessionCreated(GridSession* session) { (void)session; }
        /// After this callback it is safe to access session features. Host session is fully operational
        virtual void OnSessionHosted(GridSession* session) { (void)session; }
        /// After this callback it is safe to access session features. Client session is fully operational
        virtual void OnSessionJoined(GridSession* session) { (void)session; }
        /// Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
        virtual void OnSessionDelete(GridSession* session) { (void)session; }
        /// Called when a session error occurs.
        virtual void OnSessionError(GridSession* session, const AZStd::string& errorMsg) { (void)session; (void)errorMsg; }
        /// Called when the actual game(match) starts
        virtual void OnSessionStart(GridSession* session) { (void)session; }
        /// Called when the actual game(match) ends
        virtual void OnSessionEnd(GridSession* session) { (void)session; }
        /// Called when we start a host migration.
        virtual void OnMigrationStart(GridSession* session) { (void)session; }
        /// Called so the user can select a member that should be the new Host. Value will be ignored if NULL, current host or the member has invalid connection id.
        virtual void OnMigrationElectHost(GridSession* session, GridMember*& newHost) { (void)session; (void)newHost; }
        /// Called when the host migration has completed.
        virtual void OnMigrationEnd(GridSession* session, GridMember* newHost) { (void)session; (void)newHost; }
        /// Called when we have our last chance to write statistics data for member in the session.
        virtual void OnWriteStatistics(GridSession* session, GridMember* member, StatisticsData& data) { (void)session; (void)member; (void)data; }
    };

    typedef AZ::EBus<GridSessionCallbacks> SessionEventBus;

    /**
    * SessionServiceDesc descriptor.
    * This struct is derived from for the different platforms.
    */
    struct SessionServiceDesc
    {
    };

    namespace Internal
    {
        class GridSessionReplica;
        class GridSessionHandshake;
        class GridMemberStateReplica;
    }

    /**
     * Grid member interface class.
     */
    class GridMember
        : public ReplicaChunk
    {
        friend class GridSession;
        friend class SessionService;
        friend class Internal::GridMemberStateReplica;
    public:
        GM_CLASS_ALLOCATOR(GridMember);

        virtual ~GridMember() {}

        /// return an abstracted member id. (member ID is world unique but unrelated to player ID it's related to the session).
        virtual const MemberID& GetId() const = 0;
        /// return a compact version of the member id.
        const MemberIDCompact& GetIdCompact() const { return m_memberIdCompact; }
        /// Returns a player ID that's unique in the world and it's not session related. If NULL not player ID is supported.
        virtual const PlayerId* GetPlayerId() const = 0;

        NatType GetNatType() const;
        AZStd::string GetName() const;

        GridSession* GetSession() const { return m_session; }

        bool IsHost() const { return m_isHost.Get(); }
        bool IsLocal() const;
        bool IsInvited() const { return m_isInvited.Get(); }
        const RemotePeerMode GetPeerMode() const { return m_peerMode.Get(); }
        /**
         * Returns true if all member related data is present. Even if IsReady is false you can still access
         * all functions! Unless explicitly specified otherwise.
         */
        bool IsReady() const { return m_clientState; }

        ///< Mutes (audio/video send or received) from this member. Can be called on your local members.
        void Mute(GridMember* member)
        {
            if (member && member != this)
            {
                Mute(member->GetIdCompact());
            }
        }
        void Mute(const MemberIDCompact& id);
        ///< Unmute (audio/video is send and received) from this member. Can be called on your local members.
        void Unmute(GridMember* member)
        {
            if (member && member != this)
            {
                Unmute(member->GetIdCompact());
            }
        }
        void Unmute(const MemberIDCompact& id);
        ///< Check if a member is muted by this member.
        bool IsMuted(GridMember* member) const { return (member && member != this) ? IsMuted(member->GetIdCompact()) : true; }
        bool IsMuted(const MemberIDCompact& id) const;
        ///< Return true if the member is talking on the microphone.
        bool IsTalking() const;
        ///< Refresh talking state for given member, member will be marked as talking
        void UpdateTalking();

        /// @{ Binary data exchange - \ref Carrier for more detailed info about send and receive. Send can fail if current connectionId is invalid!
        bool SendBinary(const void* data, unsigned int dataSize, Carrier::DataReliability reliability = Carrier::SEND_RELIABLE, Carrier::DataPriority priority = Carrier::PRIORITY_NORMAL);
        Carrier::ReceiveResult ReceiveBinary(char* data, unsigned int maxDataSize);
        /// @}

        ConnectionID GetConnectionId() const { return m_connectionId; }

        //@{ Platform information
        AZ::PlatformID GetPlatformId() const;
        AZ::u32 GetProcessId() const;
        AZStd::string GetMachineName() const;
        //@}

    protected:
        GridMember(const MemberIDCompact& memberIdCompact);
        //////////////////////////////////////////////////////////////////////////
        // Replica
        bool IsReplicaMigratable() override;
        bool IsBroadcast() override { return true; }
        void OnReplicaActivate(const ReplicaContext& rc) override;
        void OnReplicaDeactivate(const ReplicaContext& rc) override;
        void OnReplicaChangeOwnership(const ReplicaContext& rc) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // RPC
        bool OnKick(AZ::u8 reason, const RpcContext& rc);
        //////////////////////////////////////////////////////////////////////////

        void SetHost(bool isHost);
        void SetInvited(bool isInvited);

        ReplicaPtr m_clientStateReplica; ///< The state is a replica owned by the actual member, while the GridMember is managed by the session!
        AZStd::intrusive_ptr<Internal::GridMemberStateReplica> m_clientState;   ///< The state is a replica owned by the actual member, while the GridMember is managed by the session!
        ConnectionID m_connectionId;
        GridSession* m_session;
        int m_slotType; ///< Local slot type returned from GridSession::ReserveSlot.
        TimeStamp m_voiceDataProcessed; ///< Time stamp when we last processed input/output voice data for this member.

        MemberIDCompact m_memberIdCompact; ///< Compact ID version (faster transfer etc.) if the full \ref MemberID Member ID

        DataSet<bool> m_isHost;
        DataSet<bool> m_isInvited;
        DataSet<RemotePeerMode> m_peerMode; // topology used by the client
        Rpc<RpcArg<AZ::u8>>::BindInterface<GridMember, &GridMember::OnKick> KickRpc;
    };

    /**
     * Session interface class.
     */
    class GridSession
        : public CarrierEventBus::Handler
        , public ReplicaMgrCallbackBus::Handler
    {
        typedef unsigned int SessionTime;

        friend class GridMember;
        friend class Internal::GridSessionReplica;
        friend class Internal::GridMemberStateReplica;
        friend class SessionService;
        friend class Debug::SessionDriller;
    public:
        enum CarrierChannels
        {
            CC_REPLICA_DATA,
            CC_VOICE_DATA,
            CC_USER_DATA,
        };
        enum Result
        {
            GS_OK = 0,
            GS_ERROR
        };

        virtual void Update();

        /// Displays send invite dialog and binds current session data to it.
        virtual bool SendInviteDlg() { return false; }

        const SessionID& GetId() const { return m_sessionId; }
        unsigned int GetNumberOfMembers() const { return static_cast<unsigned int>(m_members.size()); }
        GridMember* GetMemberByIndex(unsigned int index) const
        {
            if(m_members.size() <= index)
            {
                return nullptr;
            }
            return m_members[index];
        }
        GridMember* GetMemberById(const MemberID& id) const;
        GridMember* GetMemberById(const MemberIDCompact& id) const;
        GridMember* GetHost() const;
        GridMember* GetMyMember() const { return m_myMember; }

        Result KickMember(GridMember* member, AZ::u8 reason = 0);
        Result BanMember(GridMember* member, AZ::u8 reason = 0);

        // not supported yet
        virtual Result LockSession() { return GS_ERROR; }
        virtual Result UnlockSession() { return GS_ERROR; }

        //??? Migration

        /**
        * Leaves the session. If the local system is the host, isMigrateHost will
        * determine the host migration. If the local system is NOT the host, it will just leave the session.
        * \note When you call this you need to make sure you session supports host migration. The code is allowed to assert
        * if such operation is NOT supported.
        * IMPORTANT: You can't use the session pointer after this call.
        */
        void Leave(bool isMigrateHost);

        //virtual bool IsGameSession() const = 0;
        bool IsHost() const { return m_myMember ? m_myMember->IsHost() : false; }
        bool IsReady() const { return m_sm.IsInState(SS_IN_SESSION); }
        /// Return true if we are in host migration state, false otherwise.
        bool IsMigratingHost() const { return m_hostMigrationInProcess; }

        /// returns time in milliseconds since the session started.
        unsigned int GetTime() const;

        SessionTopology GetTopology() const;
        unsigned char GetNumUsedPublicSlots() const;
        unsigned char GetNumUsedPrivateSlots() const;
        unsigned char GetNumFreePublicSlots() const;
        unsigned char GetNumFreePrivateSlots() const;
        unsigned char GetFlags() const;
        /// Returns timeout in milliseconds for the host to tolerate 2 peers with different number of connections. The peer with less connections will be kicked out after that timeout expires.
        unsigned int GetPeerToPeerTimeout() const;
        /// Return host migration max time in milliseconds. If timeout expires the used will leave the current session.
        unsigned int GetHostMigrationTimeout() const;
        /// Return host migration min voting time. Value between 0 and GetHostMigrationTimeout() / 2 for voting time, before we go to elect the winner.
        unsigned int GetHostMigrationVotingTime() const;

        // Returns the number of session parameters currently set
        unsigned int GetNumParams() const;
        // Returns session parameter by index
        const GridSessionParam& GetParam(unsigned int index) const;
        // Adds/updates a parameter. Returns false if parameter can not be added.
        bool SetParam(const GridSessionParam& param);
        // Removes a parameter by id. Returns false if parameter can not be removed.
        bool RemoveParam(const AZStd::string& paramId);
        // Removes a parameter by index. Returns false if parameter can not be removed.
        bool RemoveParam(unsigned int index);

        /// Returns the instance of the replica manager. Replica manager may not be initialized if the session is NOT in ready (created) state.
        ReplicaManager* GetReplicaMgr() { return m_replicaMgr; }
        /// Returns the pointer to the transport layer. The pointer can be NULL if the session is NOT in ready (created) state.
        Carrier* GetCarrier() { return m_carrier; }
        /// Returns pointer to the descriptor which was used to create the transport layer (carrier)
        const CarrierDesc& GetCarrierDesc() const { return m_carrierDesc; }
        /// Return owner gridmate
        IGridMate* GetGridMate() { return m_gridMate; }

        /// @{ Debug: Change the disconnect detection state of all members in the session
        void DebugEnableDisconnectDetection(bool isEnable);
        bool DebugIsEnableDisconnectDetection() const;
        /// @}

    protected:
        // Currently we support ONLY one carrier per session, since when we received messages from the carrier we don't prefix them with session ID
        // we we do so, we can share a carrier between sessions. TODO: If you enable it make sure to add function to check if you own that carrier or NOT
        // and assert if the user call GetCarrierDesc() since the returned descriptor will NOT be the one we use to create the carrier.
        explicit GridSession(SessionService* service);
        virtual ~GridSession();

        /// Base initialization, must be called before all other operations on session
        bool Initialize(const CarrierDesc& carrierDesc);

        /// Called by the system to shutdown all session resources, before it's get deleted
        virtual void Shutdown();

        /**
         * Sets the GridSessionHandshake user data, which will be delivered with each connection request
         * and passed as a ReadBuffer into CreateRemoteMember.
         */
        void SetHandshakeUserData(const void* data, size_t size);

        //////////////////////////////////////////////////////////////////////////
        // CarrierEventBus
        void OnIncomingConnection(Carrier* carrier, ConnectionID id) override;
        void OnFailedToConnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason) override;
        void OnConnectionEstablished(Carrier* carrier, ConnectionID id) override;
        void OnDisconnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason) override;
        void OnDriverError(Carrier* carrier, ConnectionID id, const DriverError& error) override;
        void OnSecurityError(Carrier* carrier, ConnectionID id, const SecurityError& error) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Replica Manager
        void OnNewHost(bool isHost, ReplicaManager* manager) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////

        /// Return slot type 1 for public slot, 2 private slot and 0 for no slot.
        int ReserveSlot(bool isInvited);
        /// Frees a slot based on a slot type.
        void FreeSlot(int slotType);
        /// Creates remote player, when he wants to join.
        virtual GridMember* CreateRemoteMember(const AZStd::string& address, ReadBuffer& data, RemotePeerMode peerMode, ConnectionID connId = InvalidConnectionID) = 0;
        /// Returns true if this address belongs to a member in the list, otherwise false.
        virtual bool IsAddressInMemberList(const AZStd::string& address);
        virtual bool IsConnectionIdInMemberList(const ConnectionID& connId);
        /// Adds a created member to the session. Return false if no free slow was found!
        virtual bool AddMember(GridMember* member);
        /// Remove and delete current member from the session.
        virtual bool RemoveMember(const MemberID& id);
        /// Called by the state machine to elect a new host.
        virtual void ElectNewHost();
        /// Called by default from ElectNewHost to cast the new host vote.
        void CastNewHostVote(GridMember* newHost);
        /// Called when a session parameter is added/changed.
        virtual void OnSessionParamChanged(const GridSessionParam& param) = 0;
        /// Called when a session parameter is deleted.
        virtual void OnSessionParamRemoved(const AZStd::string& paramId) = 0;
        //////////////////////////////////////////////////////////////////////////

        SessionID m_sessionId; ///< Session id. Content of the string will vary based on session types and platforms.
        CarrierDesc m_carrierDesc;
        Carrier* m_carrier;
        ReplicaManager* m_replicaMgr;
        Internal::GridSessionHandshake* m_handshake;
        typedef unordered_set<ConnectionID> ConnectionIDSet;
        ConnectionIDSet m_connections;
        AZStd::string m_hostAddress;
        bool m_isShutdown;

        GridMember* m_myMember; ///< Created with the session and bound when the server replica arrives.

        AZStd::intrusive_ptr<Internal::GridSessionReplica> m_state; ///< Pointer to HOST owned session state.

        SessionService* m_service;  ///< Pointer to session service.
        IGridMate* m_gridMate; ///< Pointer to the owner GridMate interface.

        typedef vector<GridMember*> MemberArrayType;
        MemberArrayType m_members; ///< List of valid members.

        unordered_set<Internal::GridMemberStateReplica*> m_unboundMemberStates; // Holds member state replicas until the member replica arrives.
                                                                                // They should not be in here for very long.

        TimeStamp m_hostMigrationStart; ///< Time when the host migration started.
        unsigned int m_hostMigrationTimeOut; ///< Host migration time out in milliseconds
        unsigned int m_hostMigrationVotingTime; ///< Minimum time that we will spend in SS_HOST_MIGRATE_ELECTION state (before we check for majority). It should be < than m_hostMigrationTimeOut/2
        /** \note We use a bool instead of m_sm.IsInState(SS_HOST_MIGRATE_ELECTION)||m_sm.IsInState(SS_HOST_MIGRATE_SESSION)
         * because if the migration failles or timeout it will switch states. This will cause inconsistency with the user callbacks.
         */
        bool m_hostMigrationInProcess; ///< True if we are in host migration, otherwise false.
        bool m_hostMigrationSessionMigrated;
        bool m_hostMigrationReplicaMigrated;

        /**
         * In peer to peer network the host controls the connection mesh.
         * If a member has less connections than a host means that he can not connect to certain peers.
         * In peer to peer mode this is not allowed, this member will be added to "not fully connected" list
         * and observed if he corrects (connects) it's state. If not after a certain time limit he will be kicked out.
         * \note Host is the only one that can control and use that list.
         */
        struct NotFullyConnectedMember
        {
            NotFullyConnectedMember(GridMember* m)
                : member(m) {}
            bool operator==(const NotFullyConnectedMember& rhs) { return member == rhs.member; }
            TimeStamp time; ///< Time since the not connected state started.
            GridMember* member;
        };
        typedef vector<NotFullyConnectedMember> NotConnectedArrayType;
        NotConnectedArrayType m_membersNotFullyConnected;
        /*SessionTime*/TimeStamp m_lastConnectivityUpdate;

        //////////////////////////////////////////////////////////////////////////
        // State machine
        // valid session states
        enum BaseStates
        {
            SS_TOP,
            SS_NO_SESSION,
            SS_START_UP,
            SS_CREATE,
            SS_IN_SESSION,
            SS_JOIN,
            SS_IDLE,
            SS_START_GAME,
            SS_IN_GAME,
            SS_END_GAME,
            SS_DELETE,
            SS_HOST_MIGRATE_ELECTION,
            SS_HOST_MIGRATE_SESSION,
            SS_LAST, /// Must be last
        };
        enum BaseStateEvents
        {
            SE_UPDATE,                      ///< Called every frame to update a state.
            SE_HOST,
            SE_JOIN,
            SE_JOIN_INVITE,
            SE_JOINED,
            SE_CREATED,
            SE_START_GAME,
            SE_LEAVE_GAME,
            SE_END_GAME,
            SE_DELETE,
            SE_DELETED,
            SE_CONNECTION_LOST,

            // Host Migration
            SE_HM_SESSION_MIGRATED,         ///< Event executed on the host, when the session is migrated.
            SE_HM_MIGRATE_CLIENT,           ///< Event executed on the client, when he must join a migrated session.
            SE_HM_CLIENT_SESSION_MIGRATED,  ///< Event executes on the client, when the client joined the migrated session.
            SE_HM_REPLICAS_MIGRATED,        ///< Called when all replicas have been successfully migrated

            SE_LAST, /// Must be last
        };

        virtual void SetUpStateMachine();

        virtual bool OnStateNoSession(AZ::HSM& sm, const AZ::HSM::Event& e);
        virtual bool OnStateStartup(AZ::HSM& sm, const AZ::HSM::Event& e);
        virtual bool OnStateCreate(AZ::HSM& sm, const AZ::HSM::Event& e);
        virtual bool OnStateInSession(AZ::HSM& sm, const AZ::HSM::Event& e);
        virtual bool OnStateJoin(AZ::HSM& sm, const AZ::HSM::Event& e);
        virtual bool OnStateIdle(AZ::HSM& sm, const AZ::HSM::Event& e);
        virtual bool OnStateStart(AZ::HSM& sm, const AZ::HSM::Event& e);
        virtual bool OnStateInGame(AZ::HSM& sm, const AZ::HSM::Event& e);
        virtual bool OnStateEnd(AZ::HSM& sm, const AZ::HSM::Event& e);
        virtual bool OnStateDelete(AZ::HSM& sm, const AZ::HSM::Event& e);
        virtual bool OnStateHostMigrateElection(AZ::HSM& sm, const AZ::HSM::Event& e);
        virtual bool OnStateHostMigrateSession(AZ::HSM& sm, const AZ::HSM::Event& e);

        struct EventCommand
        {
            typedef void* (* DataCloner)(const void* /*sourceData*/, unsigned int /*sourceDataSize*/);
            typedef void (* DataDeleter)(void* /*data*/, unsigned int /*dataSize*/);

            AZ::HSM::Event event;
            DataDeleter m_dataDeleter;
            unsigned int m_userDataSize;
            bool m_isProcessRequired;
        };

        /**
         * Request a state machine event. You might override and queue the events if you have async operations.
         * if isProcessRequired is set to true, the event dispatch will verify that a state function returned true (processed the event)
         * otherwise it will trigger an assert and dispatch(SE_DELETE) event.
         * \note we can add optional completion callbacks if needed.
         * \note Important function function is to be used with POD data by default, provide DataCloner and DataDestructor for non POD.
         */
        virtual void RequestEvent(int id, const void* userData, unsigned int userDataSize, bool isProcessRequired, EventCommand::DataCloner cloner = nullptr, EventCommand::DataDeleter deleter = nullptr);
        /// If you queue events, you can override this function to process them when appropriate.
        virtual void ProcessEvents();

        template<class T>
        static void* UserDataCopier(const void* sourceData, unsigned int sourceDataSize)
        {
            (void)sourceDataSize;
            AZ_Assert(sizeof(T) == sourceDataSize, "Data size %d doesn't match the type size %d", sourceDataSize, sizeof(T));
            return azcreate(T, (*static_cast<const T*>(sourceData)), GridMateAllocatorMP, "UserDataCopier");
        }
        template<class T>
        static void UserDataDeleter(void* pointer, unsigned int dataSize)
        {
            (void)dataSize;
            AZ_Assert(sizeof(T) == dataSize, "Data size %d doesn't match the type size %d", dataSize, sizeof(T));
            T* data = static_cast<T*>(pointer);
            azdestroy(data, GridMateAllocatorMP, T);
        }
        /// T must be copy constructible.
        template<class T>
        inline void RequestEventData(int id, const T& userData, bool isProcessRequired = false)
        {
            RequestEvent(id, &userData, sizeof(T), isProcessRequired, &UserDataCopier<T>, &UserDataDeleter<T>);
        }
        inline void RequestEventParam(int id, size_t param, bool isProcessRequired = false)
        {
            RequestEvent(id, reinterpret_cast<const void*>(param), 0, isProcessRequired, nullptr, nullptr);
        }
        inline void RequestEvent(int id, bool isProcessRequired = false)
        {
            RequestEvent(id, nullptr, 0, isProcessRequired, nullptr, nullptr);
        }

        void EventToQueue(const AZ::HSM::Event& event, unsigned int userDataSize, bool isProcessRequired, EventCommand::DataCloner cloner, EventCommand::DataDeleter deleter);
        void ProcessEventOnQueue();

        queue<EventCommand> m_eventQueue; ///< Queue with events for the state machine to process.

        AZ::HSM m_sm; ///< Hierarchical state machine for the session management;
        //////////////////////////////////////////////////////////////////////////
        AZStd::chrono::milliseconds     m_disconnectKickedPlayersDelay; ///< number of milliseconds before forcing kicked player to disconnect
        GridMate::vector<AZStd::pair<TimeStamp, MemberIDCompact>> m_futureKickedPlayers;
    };

    /**
     * Session service interface class.
     */
    class SessionService
        : public GridMateService
    {
        friend class GridSession;
        friend class Debug::SessionDriller;
        friend class GridSearch;
    public:
        typedef vector<GridSession*> SessionArrayType;
        typedef vector<GridSearch*> SearchArrayType;

        virtual ~SessionService();

        virtual void Update();

        virtual bool IsReady() const = 0;

        const SessionArrayType& GetSessions() const { return m_sessions; }

        IGridMate* GetGridMate() { return m_gridMate; }

    protected:
        SessionService(const SessionServiceDesc& desc);

        // GridMate service
        void OnServiceRegistered(IGridMate* gridMate) override;
        void OnServiceUnregistered(IGridMate* gridMate) override;
        void OnGridMateUpdate(IGridMate* gridMate) override;

        /// Called when we create a session (from Session constructor)
        void AddSession(GridSession* session);
        /// Called once we destroy a session
        void RemoveSession(GridSession* session);

        /// Called when we start a grid search (from Session constructor)
        void AddGridSeach(GridSearch* search);
        /// Called when we release a grid search
        void ReleaseGridSearch(GridSearch* search);

        SessionArrayType    m_sessions;
        SearchArrayType     m_activeSearches;
        SearchArrayType     m_completedSearches;
        IGridMate*          m_gridMate;
    };

    /**
    * Interface for a session search.
    *
    * \note you need to make sure you release the search after you are done using it.
    */
    class GridSearch
    {
        friend class SessionService;
    public:
        GridSearch(SessionService* service)
            : m_sessionService(service)
            , m_isDone(false)
        {
            AZ_Assert(m_sessionService, "Invalid session service");
            m_sessionService->AddGridSeach(this);
        }
        virtual ~GridSearch() {}

        /// Return true if the search has finished, otherwise false.
        bool IsDone() const { return m_isDone; }

        virtual unsigned int GetNumResults() const = 0;
        virtual const SearchInfo* GetResult(unsigned int index) const = 0;
        virtual void AbortSearch() = 0;
        void Release() { m_sessionService->ReleaseGridSearch(this); }
        IGridMate* GetGridMate() const { return m_sessionService->GetGridMate(); }
    protected:
        virtual void Update() {}

        SessionService* m_sessionService;
        bool m_isDone;
    };

    namespace Internal
    {
        class GridSessionReplica
            : public ReplicaChunk
        {
            typedef DataSet<AZ::u8> BasicUChar;
            typedef DataSet<AZ::u32> BasicUInt;

            friend class GridMate::GridSession;
        public:
            GM_CLASS_ALLOCATOR(GridSessionReplica);

            GridSessionReplica(GridSession* session = nullptr)
                : m_numUsedPrivateSlots("NumUsedPrivateSlots")
                , m_numUsedPublicSlots("NumUsedPublicSlots")
                , m_numFreePrivateSlots("NumFreePrivateSlots")
                , m_numFreePublicSlots("NumFreePublicSlots")
                , m_peerToPeerTimeout("PeerToPeerTimeout", 10000)
                , m_hostMigrationTimeout("HostMigrationTimeout", 10000)
                , m_hostMigrationVotingTime("HostMigrationVotingTime")
                , m_flags("Flags")
                , m_topology("Topology", ST_INVALID)
                , m_params("Params")
                , m_isDisconnectDetection("DisconnectDetection", true)
                , m_session(session)
            {
                if (m_session)
                {
                    m_isDisconnectDetection.Set(m_session->GetCarrierDesc().m_enableDisconnectDetection);
                }
                SetPriority(k_replicaPriorityRealTime);
            }

            bool IsReplicaMigratable() override
            {
                return true;
            }

            bool IsBroadcast() override { return true; }

            typedef AZStd::fixed_vector<GridSessionParam, SessionParams::k_maxNumParams> ParamContainer;

            BasicUChar m_numUsedPrivateSlots;
            BasicUChar m_numUsedPublicSlots;
            BasicUChar m_numFreePrivateSlots;
            BasicUChar m_numFreePublicSlots;
            BasicUInt m_peerToPeerTimeout;
            BasicUInt m_hostMigrationTimeout;
            BasicUInt m_hostMigrationVotingTime;
            BasicUChar m_flags;
            DataSet<SessionTopology> m_topology;

            class ParamMarshaler
            {
            public:
                AZ_FORCE_INLINE void Marshal(WriteBuffer& wb, const GridSessionParam& param) const
                {
                    wb.Write(param.m_id);
                    wb.Write(param.m_value);
                    wb.Write(param.m_type);
                }

                AZ_FORCE_INLINE void Unmarshal(GridSessionParam& param, ReadBuffer& rb) const
                {
                    rb.Read(param.m_id);
                    rb.Read(param.m_value);
                    rb.Read(param.m_type);
                }
            };
            DataSet<ParamContainer, ContainerMarshaler<ParamContainer, ParamMarshaler> > m_params; ///< Session params.
            DataSet<bool> m_isDisconnectDetection; ///< Allows to control disconnect detection states in the entire session.

        protected:
            GridSession* m_session;
        };

        /**
         *
         */
        class GridMemberStateReplica
            : public ReplicaChunk
        {
            friend class GridMate::GridMember;
            friend class GridMate::GridSession;

        public:
            GM_CLASS_ALLOCATOR(GridMemberStateReplica);

            GridMemberStateReplica(GridMember* member = nullptr);

            void OnReplicaActivate(const ReplicaContext& rc) override;
            void OnReplicaDeactivate(const ReplicaContext& rc) override;

            /// Called during host migration from the new Host. It will indicate which session you need to join.
            bool OnNewHost(const SessionID& sessionId, const RpcContext&);

            bool IsReplicaMigratable() override { return false; }
            bool IsBroadcast() override { return true; }

            GridMember* m_member;

            typedef vector<MemberIDCompact> MuteListType;
            typedef DataSet<MuteListType, ContainerMarshaler<MuteListType> > MuteDataSetType;

            Rpc<RpcArg<const SessionID&> >::BindInterface<GridMemberStateReplica, &GridMemberStateReplica::OnNewHost, RpcAuthoritativeTraits> OnNewHostRpc;

            DataSet<AZ::u8> m_numConnections;
            DataSet<NatType> m_natType;
            DataSet<AZStd::string> m_name;
            DataSet<MemberIDCompact> m_memberId;
            DataSet<MemberIDCompact> m_newHostVote; ///< Used when in host migration, to cast machine's vote.
            MuteDataSetType m_muteList; ///< List of all players we have muted.

            // Platform and application informational data
            DataSet<AZ::PlatformID, ConversionMarshaler<AZ::u8, AZ::PlatformID> > m_platformId;
            DataSet<AZStd::string> m_machineName;
            DataSet<AZ::u32> m_processId;
            DataSet<bool> m_isInvited;
        };
    }

    namespace Debug
    {
        /**
         * Session driller events,
         * this events are in addition to the session event bus
         */
        class SessionDrillerEvents
            : public AZ::Debug::DrillerEBusTraits
        {
        public:
            virtual ~SessionDrillerEvents() {}

            /// Callback that is called when the Session service is ready to process sessions.
            virtual void OnSessionServiceReady() {}

            //virtual OnCommucationChanged() = 0  Callback that notifies the title when a member's communication settings change.

            /// Callback when we start a grid search.
            virtual void OnGridSearchStart(GridSearch* gridSearch) { (void)gridSearch; }
            /// Callback that notifies the title when a game search query have completed.
            virtual void OnGridSearchComplete(GridSearch* gridSearch) { (void)gridSearch; }
            /// Callback when we release (delete) a grid search. It's not safe to hold the grid pointer after this.
            virtual void OnGridSearchRelease(GridSearch* gridSearch) { (void)gridSearch; }

            /// Callback that notifies the title when a new member joins the game session.
            virtual void OnMemberJoined(GridSession* session, GridMember* member) { (void)session; (void)member; }
            /// Callback that notifies the title that a member is leaving the game session. member pointer is NOT valid after the callback returns.
            virtual void OnMemberLeaving(GridSession* session, GridMember* member) { (void)session; (void)member; }
            // \todo a better way will be (after we solve migration) is to supply a reason to OnMemberLeaving... like the member was kicked.
            // this will require that we actually remove the replica at the same moment.
            /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
            virtual void OnMemberKicked(GridSession* session, GridMember* member) { (void)session; (void)member; }
            /// After this callback it is safe to access session features. If host session is fully operational if client wait for OnSessionJoined.
            virtual void OnSessionCreated(GridSession* session) { (void)session; }
            /// Called on client machines to indicate that we join successfully.
            virtual void OnSessionJoined(GridSession* session) { (void)session; }
            /// Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
            virtual void OnSessionDelete(GridSession* session) { (void)session; }
            /// Called when a session error occurs.
            virtual void OnSessionError(GridSession* session, const AZStd::string& errorMsg) { (void)session; (void)errorMsg; }
            /// Called when the actual game(match) starts
            virtual void OnSessionStart(GridSession* session) { (void)session; }
            /// Called when the actual game(match) ends
            virtual void OnSessionEnd(GridSession* session) { (void)session; }
            /// Called when we start a host migration.
            virtual void OnMigrationStart(GridSession* session) { (void)session; }
            /// Called so the user can select a member that should be the new Host. Value will be ignored if NULL, current host or the member has invalid connection id.
            virtual void OnMigrationElectHost(GridSession* session, GridMember*& newHost) { (void)session; (void)newHost; }
            /// Called when the host migration has completed.
            virtual void OnMigrationEnd(GridSession* session, GridMember* newHost) { (void)session; (void)newHost; }
            /// Called when we have our last chance to write statistics data for member in the session.
            virtual void OnWriteStatistics(GridSession* session, GridMember* member, StatisticsData& data) { (void)session; (void)member; (void)data; }
        };

        typedef AZ::EBus<SessionDrillerEvents> SessionDrillerBus;
    }
}   // namespace GridMate

#endif  // GM_SESSION_H

