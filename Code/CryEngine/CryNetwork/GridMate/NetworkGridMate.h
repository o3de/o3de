/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef INCLUDE_NETWORKGRIDMATE_HEADER
#define INCLUDE_NETWORKGRIDMATE_HEADER

#pragma once

#include "NetworkGridMateCommon.h"
#include "NetworkGridMateSessionEvents.h"
#include "NetworkGridMateSystemEvents.h"
#include "NetworkGridMateProfiling.h"
#include "Replicas/EntityReplicaSpawnParams.h"

namespace GridMate
{
    class SecureSocketDriver;

    /*!
     * Implementation of INetwork interface for GridMate-backed network.
     */
    class Network
        : public INetwork
        , public ILevelSystemListener
        , private NetSerialize::ILegacySerializeProvider
    {
    public:

        friend class SessionEvents;
        friend class NetworkSystemEvents;

        static Network& Get();

        Network();
        virtual ~Network();

    public:

        typedef AZStd::unordered_map<EntityId, AZStd::intrusive_ptr<EntityReplica>> EntityReplicaMap;

    public: // INetwork implementation.

        IGridMate* GetGridMate() override { return m_gridMate; }

        //! Helper for grabbing the channel id corresponding to a particular session member.
        ChannelId GetChannelIdForSessionMember(GridMate::GridMember* member) const override;

        //! Main module initialization, called by engine.
        bool Init(int ncpu);

        //! Legacy cleanup functions invoked by the engine.
        void Release() override;

        //! Main update routine invoked by the engine.
        void SyncWithGame(ENetworkGameSync syncType) override;

        //! Marks an aspect dirty. This will trigger a NetSerialize invocation, after which
        //! we'll determine if a re-send is necessary.
        void ChangedAspects(EntityId id, NetworkAspectType aspectBits) override;

        //! Retrieve the local user's channel Id.
        ChannelId GetLocalChannelId() const override;

        //! Retrieve the channel Id of the server we're connected to.
        //! If we are the server, we simply return our own channel Id.
        ChannelId GetServerChannelId() const override;

        //! Convert a local entity Id to the server side Id, since they can vary across
        //! systems.
        //! Before dispatching messages or events to the server or other clients, local
        //! Ids should be converted to server Ids so they can be properly deciphered.
        EntityId LocalEntityIdToServerEntityId(EntityId localId) const override;

        //! Convert a server entity Id to a local entity Id so we can dispatch messages
        //! to local objects.
        EntityId ServerEntityIdToLocalEntityId(EntityId serverId, bool allowForcedEstablishment = false) const override;

        //! Gets the synchronized network time as milliseconds since session creation time.
        virtual CTimeValue GetSessionTime() override;

        ////////////////////////////////////////////////////////////////
        //! Compatibility "Shim" interfaces.
        //! Invoke a GameCore actor RMI through GridMate RPCs.
        void InvokeActorRMI(EntityId entityId, uint8 actorExtensionId, ChannelId targetChannelFilter, IActorRMIRep& rep) override;
        //! Invoke a lua script RMI through GridMate RPCs.
        void InvokeScriptRMI(ISerializable* serializable, bool isServerRMI, ChannelId toChannelId = kInvalidChannelId, ChannelId avoidChannelId = kInvalidChannelId) override;
        //! Registers an actor RMI rep; required for dispatching to the game upon receipt.
        void RegisterActorRMI(IActorRMIRep* rep) override;
        void UnregisterActorRMI(IActorRMIRep* rep) override;
        //! Sets mask describing which aspects are globally delegatable.
        void SetDelegatableAspectMask(NetworkAspectType aspectBits) override;
        //! Sets mask on a given obejct describing which aspect that object has delegated to the controlling client.
        void SetObjectDelegatedAspectMask(EntityId entityId, NetworkAspectType aspects, bool set) override;
        //! Request authority for entityId be delegated to client at clientChannelId.
        void DelegateAuthorityToClient(EntityId entityId, ChannelId clientChannelId) override;
        ////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////
        //! Currently unused INetwork APIs.
        void GetMemoryStatistics(ICrySizer* pSizer) override { (void)pSizer; };
        const char* GetHostName() override { return ""; }
        ////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////
        //! Other INetwork API functions still applicable to GridMate.
        void GetBandwidthStatistics(SBandwidthStats* const pStats) override;
        void GetPerformanceStatistics(SNetworkPerformance* pSizer) override;
        void GetProfilingStatistics(SNetworkProfilingStats* const pStats) override;
        ////////////////////////////////////////////////////////////////

        bool IsInMinimalUpdate() const;

    public: // GridMate-integration-specific APIs

        //! Returns true if it's safe to spawn replicated entities at this time.
        //! An example of a time during which this is not safe would be during a local level load.
        bool AllowEntityCreation() const;

        //! Returns the active client or server session.
        GridSession* GetCurrentSession() { return m_session; }

        //! Locate the replica for a service-side entity Id.
        EntityReplica* FindEntityReplica(EntityId id) const;

        //! Retrieves the global replica registration, mapped by server-side entity Id.
        EntityReplicaMap& GetEntityReplicaMap() { return m_activeEntityReplicaMap; }

        //! Retrieves new proxy registrations, mapped by server-side entity Id.
        EntityReplicaMap& GetNewProxyEntityMap() { return m_newProxyEntities; }

        ////////////////////////////////////////////////////////////////
        //! ILevelSystemListener callbacks.
        void OnLoadingComplete(ILevel* level) override;
        void OnUnloadComplete(ILevel* level) override;
        ////////////////////////////////////////////////////////////////

        GameStatistics& GetGameStatistics();
        CarrierStatistics GetCarrierStatistics();

        //! Pumps GridMate instance.
        void UpdateGridMate(ENetworkGameSync syncType);
        //! Instantiates GridMate instance.

        //! Create replicas for newly-spawned entities (server only).
        void BindNewEntitiesToNetwork();

        //! Execute deferred tasks.
        void FlushPostFrameTasks();

        //! Bandwidth statistics and profiling.
        void UpdateNetworkStatistics();
        void ClearNetworkStatistics();
        void DumpNetworkStatistics();

        void DebugDraw();

        void SetLegacySerializeProvider(NetSerialize::ILegacySerializeProvider* provider) { m_legacySerializeProvider = provider; }
        NetSerialize::ILegacySerializeProvider* GetLegacySerializeProvider() { return m_legacySerializeProvider; }

    private:
        // ILegacySerializeProvider implementation
        void AcquireSerializer(WriteBuffer& wb, NetSerialize::AcquireSerializeCallback callback) override;
        void AcquireDeserializer(ReadBuffer& rb, NetSerialize::AcquireSerializeCallback callback) override;

        //! Instantiates GridMate
        void StartGridMate();
        //! Shuts down GridMate instance.
        void ShutdownGridMate();

        //! Sets globals and context flags appropriate for an active server hosting a session.
        void MarkAsConnectedServer();
        //! Sets globals and context flags for a client connected to a hosted session.
        void MarkAsConnectedClient();
        //! Sets globals and context flags for a single-player instance.
        void MarkAsLocalOnly();

        typedef std::map<ChannelId, CarrierStatistics> CarrierStatisticsMap;
        typedef std__hash_map<EntityId, EntitySpawnParamsStorage> NewEntitiesMap;

        //! Connection statistics for each outgoing channel.
        CarrierStatisticsMap m_statisticsPerChannel;

        //! Statistics for incoming/outgoing RMIs and aspects (global and per-entity).
        GameStatistics m_gameStatistics;

        //! The local "channel id", required by CryEngine to know which
        //! client owns which actor.
        ChannelId m_localChannelId;
        //! Pointer to GridMate instance.
        GridMate::IGridMate*            m_gridMate;
        //! Pointer to MP session
        GridMate::GridSession* m_session;

        //! Maintain a map of entity replicas per their server-side entity Id.
        EntityReplicaMap                m_activeEntityReplicaMap;
        EntityReplicaMap                m_newProxyEntities;

        //! Stores a map of entities spawned this frame, so we can instantiate replicas
        //! once it's safe to do so.
        NewEntitiesMap                  m_newServerEntities;

        //! EBus handlers for GridMate sessions.
        SessionEvents                   m_sessionEvents;

        //! EBus handlers for various system events.
        NetworkSystemEvents             m_systemEvents;

        //! Used so areas of the network can be aware that we're loading a level.
        enum LevelLoadState
        {
            LevelLoadState_None,
            LevelLoadState_Loading,
            LevelLoadState_Loaded
        };
        
        AZStd::atomic<LevelLoadState>   m_levelLoadState;

        //! Set if we're currently in a GridMate update.
        AZStd::mutex                    m_mutexUpdatingGridMate;

        //! Inherited from CryNetwork, this is sent by the NetworkStallTicker mechanism
        //! to tell us it's unsafe to process minimal network updates (loading updates).
        AZStd::atomic<bool>             m_allowMinimalUpdate;

        typedef AZStd::function<void()> Task;
        std::vector<Task>              m_postFrameTasks;

        NetSerialize::ILegacySerializeProvider* m_legacySerializeProvider;

        static Network* s_instance;

    public:

        // Profiler settings.
        static int                      s_StatsIntervalMS;
        static int                      s_DumpStatsEnabled;
        static FILE*                    s_DumpStatsFile;
    };
} // namespace GridMate

/// External systems expect the type 'CNetwork' in the global namespace.
typedef GridMate::Network CNetwork;

#endif // INCLUDE_NETWORKGRIDMATE_HEADER
