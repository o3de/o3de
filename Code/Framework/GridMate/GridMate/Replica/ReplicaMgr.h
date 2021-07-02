/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICAMGR_H
#define GM_REPLICAMGR_H

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/SystemReplicas.h>
#include <GridMate/Replica/ReplicaDefs.h>
#include <GridMate/Types.h>
#include <GridMate/Containers/unordered_map.h>
#include <GridMate/Containers/list.h>
#include <GridMate/MathUtils.h>
#include <GridMate/Replica/Tasks/ReplicaTaskManager.h>
#include <GridMate/Replica/Tasks/ReplicaPriorityPolicy.h>
#include <GridMate/Replica/Tasks/ReplicaProcessPolicy.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/intrusive_set.h>
#include <AzCore/std/containers/map.h>

namespace UnitTest
{
    template <typename ComponentType>
    class NetContextMarshalFixture;
}

namespace GridMate
{
    class Carrier;
    namespace ReplicaInternal
    {
        class SessionInfo;
        class SessionInfoDesc;
        class MigrationSequence;
        class PeerReplica;
        class MigrationSequence;
    }

    struct ReplicaObject
        : public AZStd::intrusive_multiset_node<ReplicaObject>
    {
        AZ_FORCE_INLINE bool operator<(const ReplicaObject& right) const { return m_replica->GetCreateTime() < right.m_replica->GetCreateTime(); }
        ReplicaPtr m_replica;
    };

    typedef unordered_map<ReplicaId, ReplicaObject> ReplicaMap;
    typedef AZStd::intrusive_multiset<ReplicaObject, AZStd::intrusive_multiset_base_hook<ReplicaObject> > ReplicaTimeSet; // Set sorted on time
    typedef list<ReplicaPeer*> ReplicaPeerList;

    //-----------------------------------------------------------------------------
    // ReplicaPeer
    //-----------------------------------------------------------------------------
    enum RemotePeerMode : AZ::u8
    {
        Mode_Undefined,
        Mode_Peer,      // All authoritative objects (owned + clients) will be replicated
        Mode_Client,    // All objects (authoritative + non-authoritative) will be replicated
    };

    class PeerAckCallbacks final : public CarrierACKCallback
    {
        CallbackBuffer m_callbackTargets;
    public:
        GM_CLASS_ALLOCATOR(PeerAckCallbacks);
        /**
         *  Initializes by capturing the callback buffer
         */
        explicit PeerAckCallbacks(CallbackBuffer &callbacks)
            : m_callbackTargets(AZStd::move(callbacks))
        {
        }
        void Run() override
        {
            for( auto& cb : m_callbackTargets)
            {
                auto ptr = cb.lock();
                if (ptr)
                {
                    (*ptr)();
                }
            }
        }
    };

    class ReplicaPeer
    {
        friend class ReplicaInternal::SessionInfo;
        friend class ReplicaManager;
        friend class Replica;
        friend class ReplicaInternal::MigrationSequence;
        friend class ReplicaUpdateTaskBase;
        friend class ReplicaTarget;
        friend class SendLimitProcessPolicy;
        friend class ReplicaMarshalTaskBase;
        friend class ReplicaMarshalTask;
        template <typename ComponentType>
        friend class UnitTest::NetContextMarshalFixture;

        AZ::u32 m_flags;
        PeerId m_peerId;
        ConnectionID m_connId;
        RemotePeerMode m_mode;
        ReplicaMap m_objectsMap;
        ReplicaTimeSet m_objectsTimeSort;
        PeerTargetList m_targets;
        WriteBufferDynamic m_reliableOutBuffer;
        WriteBufferDynamic m_unreliableOutBuffer;
        CallbackBuffer m_reliableCallbacks;
        CallbackBuffer m_unreliableCallbacks;
        WriteBuffer::Marker<AZ::u32> m_reliableTimestamp;
        WriteBuffer::Marker<AZ::u32> m_unreliableTimestamp;
        WriteBuffer::Marker<AZ::Crc32> m_reliableMsgCrc;
        WriteBuffer::Marker<AZ::Crc32> m_unreliableMsgCrc;
        ZoneMask m_zoneMask;
        ReplicaManager* m_rm;

        // orphan resolution
        list<PeerId> m_pendingReports;

        int m_lastReceiveTicks; // Debug

        // Bandwidth throttling
        RollingSum<unsigned int, 10> m_dataSentLastSecond; // rolling send rate for last second
        float m_avgSendRateBurst; // send rate averaged for >=1 seconds used for burst control
        int m_sentBytes; // number of bytes of replica data current sent
        int m_sendBytesAllowed; // number of bytes allowed to be sent current frame
        ////

        void SetNew(bool b)
        {
            if (b)
            {
                m_flags |= PeerFlags::Peer_New;
            }
            else
            {
                m_flags &= ~PeerFlags::Peer_New;
            }
        }
        void MakeSyncHost(bool b) { m_flags = b ? m_flags | PeerFlags::Peer_SyncHost : m_flags & ~PeerFlags::Peer_SyncHost; }
        void Add(Replica* pObj);
        void Remove(Replica* pObj);

    public:
        GM_CLASS_ALLOCATOR(ReplicaPeer);

        ReplicaPeer(ReplicaManager* manager, ConnectionID connId = InvalidConnectionID, RemotePeerMode mode = Mode_Undefined);

        void Accept();
        PeerId GetId() const { return m_peerId; }
        ConnectionID GetConnectionId() const { return m_connId; }
        RemotePeerMode GetMode() const { return m_mode; }
        bool IsNew() const { return !!(m_flags & PeerFlags::Peer_New); }
        bool IsSyncHost() const { return !!(m_flags & PeerFlags::Peer_SyncHost); }
        bool IsOrphan() const { return GetConnectionId() == InvalidConnectionID; }

        void SetEndianType(EndianType endianType);

        bool CanAcceptData(ReplicaPeer* from) const;
        ZoneMask GetZoneMask() const { return m_zoneMask; }

        WriteBuffer& GetReliableOutBuffer() { return m_reliableOutBuffer; }
        WriteBuffer& GetUnreliableOutBuffer() { return m_unreliableOutBuffer; }
        void SendBuffer(Carrier* carrier, unsigned char commChannel, const AZ::u32 replicaManagerTimer);
        void ResetBuffer();
        CallbackBuffer& GetReliableCallbackBuffer() { return m_reliableCallbacks; }
        CallbackBuffer& GetUnreliableCallbackBuffer() { return m_unreliableCallbacks; }

        ReplicaPtr GetReplica(ReplicaId repId);

    private:
        //---------------------------------------------------------------------
        // DEBUG and Test Interface. Do not use in production code.
        //---------------------------------------------------------------------
        void Debug_Add(Replica* pObj) { Add(pObj); }
        void Debug_Remove(Replica* pObj) { Remove(pObj); }
    };
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // ReplicaMgrDesc
    //-----------------------------------------------------------------------------
    struct ReplicaMgrDesc
    {
        // Single-primary roles that replica managers can have
        enum Roles
        {
            Role_SyncHost = 1 << 0,
        };

        // default value for m_targetFixedTimeStepsPerSecond, used to indicate fixed time step is disabled
        static const AZ::s16 k_fixedTimeStepDisabled = -1;

        // id for the local peer
        AZ::Crc32 m_myPeerId;

        // pointer to underlying carrier
        Carrier* m_carrier;

        // carrier comm channel to use
        unsigned char m_commChannel;

        // roles for this replica manager
        AZ::u32 m_roles;

        // target milliseconds between sends
        unsigned int m_targetSendTimeMS;

        // incoming bandwidth limit per peer in bytes per second (0 - unlimited)
        unsigned int m_targetSendLimitBytesPerSec;

        // burst in bandwidth will be allowed for the given amount of time maximum. burst will only be allowed if bandwidth is not capped at the time of burst
        float m_targetSendLimitBurst;

        // -1 (default) means use real time (time from Carrier) when adding timestamp to send buffer read in Unmarshal and propagated to datasets and replicas
        // as m_lastUpdateTime, otherwise specify a value that indicates the target server frame rate and the server will send a fixed time step in packets.
        // This should match your intended target frame rate.
        // This feature would really only be useful if you are running a server, since clients should be time stamping with their local time. The idea would be
        // that the application should read a config file or cvar to know when to set this value.
        AZ::s16 m_targetFixedTimeStepsPerSecond;

        ReplicaMgrDesc(const AZ::Crc32& myPeerId = AZ::Crc32()
            , Carrier* carrier = NULL
            , unsigned char commChannel = 0
            , AZ::u32 roles = 0
            , unsigned int targetSendTimeMS = 0
            , unsigned int targetSendLimitBytesPerSec = 0)
            : m_myPeerId(myPeerId)
            , m_carrier(carrier)
            , m_commChannel(commChannel)
            , m_roles(roles)
            , m_targetSendTimeMS(targetSendTimeMS)
            , m_targetSendLimitBytesPerSec(targetSendLimitBytesPerSec)
            , m_targetSendLimitBurst(10.f)
            , m_targetFixedTimeStepsPerSecond(k_fixedTimeStepDisabled)
        {
        }
    };
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // ReplicationSecurityOptions
    //-----------------------------------------------------------------------------
    struct ReplicationSecurityOptions
    {
        /**
        * If turned on, only requests from verifiable authority are allowed.
        * For RPCs with source peer id forwarding, only the host is allowed to specify the source peer id.
        * Breaks object migration, including host migration.
        */
        bool m_enableStrictSourceValidation = false;
    };

    //-----------------------------------------------------------------------------
    // FixedTimeStep
    //-----------------------------------------------------------------------------
    class FixedTimeStep
    {
    public:
        static const AZ::u16 k_millisecondsPerSecond = 1000;

        FixedTimeStep()
            : m_updateCount(0)
            , m_updateCountTargetPerSecond(0)
            , m_currentTime(0)
            , m_seconds(0)
        {
        }

        void UpdateFixedTimeStep()
        {
            m_updateCount++;

            // every second update the seconds count
            if (m_updateCount % m_updateCountTargetPerSecond == 0)
            {
                m_seconds += 1;
            }

            // generate a ratio of the progress through the current second, this solves rounding issues created by trying to accumulate repeating decimal values (16.66666 for example)
            const AZ::u64 oneSecondRatio = (k_millisecondsPerSecond * (m_updateCount % m_updateCountTargetPerSecond)) / m_updateCountTargetPerSecond;

            // update the time to be the second count plus the ratio of our progress through the current second
            m_currentTime = (m_seconds * k_millisecondsPerSecond) + oneSecondRatio;
        }

        void SetTargetUpdateRate(AZ::u32 updateCountTargetPerSecond)
        {
            AZ_Warning("GridMate", (updateCountTargetPerSecond == 0), "Calling SetTargetUpdateRate() while the system is updating will lead to inconsistencies in timing, this value should be set ONCE!\n");

            if (updateCountTargetPerSecond > k_millisecondsPerSecond)
            {
                AZ_Warning("GridMate", false, "SetTargetUpdateRate() is clamping rate from requested [%u] to max value of [%u]!\n", updateCountTargetPerSecond, k_millisecondsPerSecond);
                updateCountTargetPerSecond = k_millisecondsPerSecond;
            }
            m_updateCountTargetPerSecond = updateCountTargetPerSecond;

            // this could allow for changing on the fly but it would need to ensure that if it were in the middle of a second, that the new rate would result in landing on the
        }

        AZ::u64 GetCurrentTime() const
        {
            return m_currentTime;
        }

    private:
        AZ::u64 m_updateCount;
        AZ::u32 m_updateCountTargetPerSecond;

        AZ::u64 m_currentTime;     // the local time which we will time stamp outgoing changes with in calls to SendBuffer(), updated once a frame and guaranteed to be consistent between sends that occur on the same frame.
        AZ::u64 m_seconds;         // an accumulation of seconds used when calculating m_fixedTimeStepCurrentTime
    };

    //-----------------------------------------------------------------------------
    // ReplicaManager
    //-----------------------------------------------------------------------------
    class ReplicaManager : public CarrierEventBus::Handler
    {
        friend class Replica;
        friend class ReplicaInternal::SessionInfo;
        friend class ReplicaInternal::SessionInfoDesc;
        friend class ReplicaInternal::MigrationSequence;
        friend class ReplicaInternal::PeerReplica;
        friend class ReplicaMarshalTask;
        friend class ReplicaUpdateTaskBase;
        friend class ReplicaDestroyPeerTask;
        friend class SendLimitProcessPolicy;
        friend class InterestManager;

        typedef unordered_map<int, void*> UserContextMapType;
        typedef unordered_map<ReplicaId, ReplicaPtr> ReplicaMap;

        //---------------------------------------------------------------------
        // RepIdMgrClient
        // Responsible for dispensing replica ids on the client side
        //---------------------------------------------------------------------
        class RepIdMgrClient
        {
            typedef unordered_map<RepIdSeed, ReplicaId> RepIdContainerType;

            RepIdContainerType m_idBlocks;
            size_t m_nAvailableIds;

        public:
            RepIdMgrClient()
                : m_nAvailableIds(0) {}

            void AddBlock(RepIdSeed seed);
            void RemoveBlock(RepIdSeed seed);
            ReplicaId Alloc();
            void Dealloc(ReplicaId id);
            size_t Available() const { return m_nAvailableIds; }
        };

        enum
        {
            // Status flags
            Rm_Initialized = 1 << 0,
            Rm_Processing  = 1 << 2,
            Rm_Terminating = 1 << 3,
        };

        AZ::u32 m_flags;
        ReplicaMgrDesc m_cfg;
        UserContextMapType m_userContexts;
        AZStd::chrono::system_clock::time_point m_lastCheckTime; // last time we tried to send
        AZStd::chrono::system_clock::time_point m_nextSendTime; // next expected send slot

        FixedTimeStep m_fixedTimeStep;

        ReplicaPeer m_self; // the local peer
        AZStd::recursive_mutex  m_mutexRemotePeers; // mutex for remote peers
        ReplicaPeerList         m_remotePeers;      // remote peers
        unordered_map<PeerId, ReplicaInternal::PeerReplica::Ptr> m_peerReplicas;

        vector<char> m_receiveBuffer;
        ReplicaInternal::SessionInfo::Ptr m_sessionInfo;

        ReplicaMap m_replicas;
        RepIdMgrClient m_localIdBlocks; // used by every peer to track their own id assignments

        typedef unordered_map<ReplicaId, ReplicaInternal::MigrationSequence*> MigrationsContainer;
        MigrationsContainer m_activeMigrations;

        typedef unordered_map<ReplicaId, unsigned int> TombstoneRecords;
        TombstoneRecords m_tombstones;

        typedef AZStd::intrusive_list<Replica, AZStd::list_member_hook<Replica, & Replica::m_dirtyHook> > DirtyReplicas;
        DirtyReplicas m_dirtyReplicas;
        AZ::PoolAllocator m_tasksAllocator;
        ReplicaTaskManager<SendLimitProcessPolicy, SendPriorityPolicy> m_marshalingTasks;
        ReplicaTaskManager<NullProcessPolicy, NullPriorityPolicy> m_updateTasks;
        ReplicaTaskManager<NullProcessPolicy, NullPriorityPolicy> m_peerUpdateTasks;

        TimeContext m_currentFrameTime;
        AZ::u32 m_latchedCarrierTime;   // timer that is constant across a frame

        ReplicationSecurityOptions m_securityOptions;
        bool m_autoBroadcast; ///< should replicas be automatically broadcast to every session member?

        // forbidding replica manager copying
        ReplicaManager(const ReplicaManager&) = delete;
        ReplicaManager& operator=(const ReplicaManager&) = delete;

        bool AcceptPeer(ReplicaPeer* peer);
        void DiscardOrphans(PeerId orphanId);
        ReplicaPeer* FindPeer(PeerId peerId);

        void OnPeerReplicaActivated(ReplicaInternal::PeerReplica::Ptr peerReplica);
        void OnPeerReplicaDeactivated(ReplicaInternal::PeerReplica::Ptr peerReplica);

        RepIdSeed ReserveIdBlock(PeerId requestor);
        size_t ReleaseIdBlock(PeerId requestor);

        void _Unmarshal(ReadBuffer& rb, ReplicaPeer* from);
        void RegisterReplica(const ReplicaPtr& pReplica, bool isPrimary, ReplicaContext& rc);
        void UnregisterReplica(const ReplicaPtr& replica, const ReplicaContext& rc);
        void RemoveReplicaFromDownstream(const ReplicaPtr& replica, const ReplicaContext& rc);
        void MigrateReplica(ReplicaPtr replica, PeerId newOwnerId);
        void AnnounceReplicaMigrated(ReplicaId replicaId, PeerId newOwnerId);
        void OnReplicaMigrated(ReplicaPtr replica, bool isOwner, const ReplicaContext& rc);
        void ChangeReplicaOwnership(ReplicaPtr replica, const ReplicaContext& rc, bool isPrimary);
        void AckUpstreamSuspended(ReplicaId replicaId, PeerId sendTo, AZ::u32 requestTime);
        void OnAckUpstreamSuspended(ReplicaId replicaId, PeerId from, AZ::u32 requestTime);
        void AckDownstream(ReplicaId replicaId, PeerId sendTo, AZ::u32 requestTime);
        void OnAckDownstream(ReplicaId replicaId, PeerId from, AZ::u32 requestTime);
        void SendGreetings(ReplicaPeer* peer);

        virtual bool Destroy(Replica* requestor);
        virtual void GetReplicaContext(const Replica* requestor, ReplicaContext& rc);

        bool ShouldBroadcastReplica(Replica* replica) const;
    protected:

        /***
        * RateConnectionPair wrapper for priority queue sorting and searching by connection rate
        */
        struct RateConnectionPair
        {
            AZ::u32            m_rate;
            ConnectionID       m_connection;

            RateConnectionPair(AZ::u32 value1, ConnectionID value2) : m_rate(value1), m_connection(value2) { }

            ///Searches for the connection
            bool operator==(const ConnectionID right) const
            {
                return (m_connection == right);
            }

            ///Compares the stored rates of two pairs
            bool operator<(const RateConnectionPair& right) const
            {
                return (m_rate < right.m_rate);
            }
        };
        static bool                                 k_enableBackPressure;
        AZStd::priority_queue<RateConnectionPair>   m_connByCongestionState;      ///< Connections priority queue sorted by congestion window
        /***
        * Updates connection's rate in priority and updates send limit
        *
        *
        * \param rate connections's new rate
        * \param conn connection being updated
        */
        void UpdateConnectionRate(AZ::u32 rate, ConnectionID id);

        //////////////////////////////////////////////////////////////////////////
        // CarrierEventBus
        void OnConnectionEstablished(Carrier* carrier, ConnectionID id) override
        {
            if (m_cfg.m_carrier != carrier)
            {
                return; //Not our carrier
            }
            AZ_Assert(carrier, "NULL carrier!");

            m_connByCongestionState.emplace(RateConnectionPair(AZ::u32(1500), id)); //default to 1500Bps (ex 1 Ethernet frame/second minimum)
        }
        void OnDisconnect(Carrier* carrier, ConnectionID id, CarrierDisconnectReason reason) override
        {
            (void)reason;
            if (m_cfg.m_carrier != carrier)
            {
                return; //Not our carrier
            }
            AZ_Assert(carrier, "NULL carrier!");

            auto connIt = AZStd::find(m_connByCongestionState.get_container().begin(), m_connByCongestionState.get_container().end(), id);
            if (connIt != m_connByCongestionState.get_container().end())
            {
                //Since we are using a weakly sorted heap, we need to re-generate when the top is removed
                bool remake = (connIt == m_connByCongestionState.get_container().begin());

                m_connByCongestionState.get_container().erase(connIt);

                if (remake)
                {
                    AZStd::make_heap(m_connByCongestionState.get_container().begin(), m_connByCongestionState.get_container().end());
                }
            }
        }
        void OnRateChange(Carrier* carrier, ConnectionID id, AZ::u32 sendLimitBytesPerSec) override
        {
            if (!k_enableBackPressure)
            {
                return;
            }
            if (m_cfg.m_carrier != carrier)
            {
                return; //Not our carrier
            }

            AZ_Assert(carrier, "NULL carrier!");

            UpdateConnectionRate(sendLimitBytesPerSec, id);
        };
        //End CarrierEventBus
        //////////////////////////////////////////////////////////////////////////

    public:
        GM_CLASS_ALLOCATOR(ReplicaManager);

        ReplicaManager();
        virtual ~ReplicaManager() {}

        /*
         * Init/Shutdown
         */
        void Init(const ReplicaMgrDesc& desc);
        void Shutdown();

        /// Access to the owning gridmate instance.
        IGridMate* GetGridMate() const;

        /*
         * Query functions
         */
        bool IsInitialized() const { return !!(m_flags & Rm_Initialized); }
        bool IsReady() const { return m_sessionInfo && m_sessionInfo->GetReplica() && m_sessionInfo->GetReplica()->IsActive(); }
        bool IsSyncHost() const { return m_self.IsSyncHost(); }
        bool HasValidHost() const { return m_sessionInfo->GetReplica() && m_sessionInfo->GetReplica()->IsActive() && m_sessionInfo->m_pHostPeer && !m_sessionInfo->m_pHostPeer->IsOrphan(); }
        PeerId GetLocalPeerId() const { return m_cfg.m_myPeerId; }
        TimeContext GetTime() const;

        AZ::u32 GetTimeForNetworkTimestamp() const;
        void UpdateFixedTimeStep();
        bool IsUsingFixedTimeStep() const { return m_cfg.m_targetFixedTimeStepsPerSecond != ReplicaMgrDesc::k_fixedTimeStepDisabled; }

        void SetSendTimeInterval(unsigned int sendTimeMs); // Set time interval between sends (in milliseconds), 0 will bound sends to GridMate tick rate
        unsigned int GetSendTimeInterval() const; // Returns time interval between sends (in milliseconds)

        void SetSendLimit(unsigned int sendLimitBytesPerSec); // Sets outgoing bandwidth limit per peer per second
        unsigned int GetSendLimit() const; // Returns outgoing bandwidth limit per peer per second

        void SetSendLimitBurstRange(float rangeSec); // Sets burst range for bandwidth limiter, burst in bandwidth will be allowed for the given amount of time in seconds
        float GetSendLimitBurstRange() const; // Returns burst range for bandwidth limiter

        void SetAutoBroadcast(bool isEnabled);

        void SetLocalLagAmt(unsigned int ms);

        /*
         * Custom user-contexts
         * These will be passed to the replicas during frame ticks.
         */
        void RegisterUserContext(int key, void* data);
        void UnregisterUserContext(int key);
        void* GetUserContext(int key);

        /*
         * Operations
         */
        void UpdateFromReplicas(); // Updates local states from replica information
        void UpdateReplicas(); // Updates replicas with local information
        void Marshal(); // Send updates
        void Unmarshal(); // Receive updates
        void Promote(); // Promote this manager to host

        /*
         * Replica Peers
         */
        void AddPeer(ConnectionID connId, RemotePeerMode peerMode);
        void RemovePeer(ConnectionID connId);

        /*
         * Replicas
         */
        virtual ReplicaPtr FindReplica(ReplicaId replicaId);
        ReplicaId AddPrimary(const ReplicaPtr& pPrimary);

        /*
        * Tasks
        */
        void EnqueueUpdateTask(ReplicaPtr replica);
        void UpdateReplicaTargets(ReplicaPtr replica);
        void OnPeerAccepted(ReplicaPeer* peer);
        void OnPeerReadyToRemove(ReplicaPeer* peer);
        void OnReplicaChanged(ReplicaPtr replica);
        void OnRPCQueued(ReplicaPtr replica);
        void OnReplicaUnmarshaled(ReplicaPtr replica);
        void RemoveFromDirtyList(Replica& replica);
        void CancelTasks(ReplicaPtr replica);
        void OnDestroyProxy(ReplicaId repId);
        void OnPendingReportsReceived(PeerId peerId);
        void OnMigratePeer(ReplicaPeer* peer);

        void OnReplicaPriorityUpdated(Replica* replica);

        void SetSecurityOptions(const ReplicationSecurityOptions& options);
        ReplicationSecurityOptions GetSecurityOptions() const;
    };
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // ReplicaMgrCallbackBus
    // Systems interested in receiving notification events from the replica manager
    // should listen on this bus
    //-----------------------------------------------------------------------------
    class ReplicaMgrCallbacks
        : public GridMateEBusTraits
    {
    public:
        virtual ~ReplicaMgrCallbacks() {}

        // sent when host migration has completed
        virtual void OnNewHost(bool /*isHost*/, ReplicaManager* /*pMgr*/) {}
        // Sent when a replica is unregistered from the system
        virtual void OnDeactivateReplica(ReplicaId /*replicaId*/, ReplicaManager* /*pMgr*/) {}
        // Sent when a new peer is discovered
        virtual void OnNewPeer(PeerId /*peerId*/, ReplicaManager* /*pMgr*/) {}
        // Sent when a peer is removed
        virtual void OnPeerRemoved(PeerId /*peerId*/, ReplicaManager* /*pMgr*/) {}
    };
    //-----------------------------------------------------------------------------
    typedef AZ::EBus<ReplicaMgrCallbacks> ReplicaMgrCallbackBus;
} // namespace Gridmate

#endif // GM_REPLICAMGR_H
