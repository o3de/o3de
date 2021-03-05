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
#pragma once

#include <AzFramework/Network/NetBindingSystemBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Slice/SliceInstantiationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <GridMate/Serialize/CompressionMarshal.h>

namespace AzFramework
{
    /**
     * \brief Represents a request to bind a particular replica to an entity
     */
    class BindRequest
    {
    public:
        BindRequest()
            : m_bindTo(GridMate::InvalidReplicaId)
            , m_state(State::None)
        {
        }

        GridMate::ReplicaId m_bindTo;
        AZ::EntityId m_desiredRuntimeEntityId;
        AZ::EntityId m_actualRuntimeEntityId;
        AZStd::chrono::system_clock::time_point m_requestTime;

        /**
         * \brief Represents the state of this bind request and it's relation to the slice instantiation process
         */
        enum class State : AZ::u8
        {
            None,
            /**
             * \brief This is the first request that led to instantiating a slice
             */
            FirstBindInSlice,
            /**
             * \brief The request is a placeholder in case a real bind request arrives later.
             * Some part of the slice may never be bound (e.g. if a replica is omitted by Interest Manager)
             */
            PlaceholderBind,
            /**
             * \brief The real request did arrive to replace a placeholder request.
             */
            LateBind,
        };

        State m_state;
    };

    typedef AZStd::unordered_map<AZ::EntityId, BindRequest> BindRequestContainerType;

    /**
     * \brief Represents a slice instance being instantiated and bound to replicas
     * \note It's possible that only some of the entities are activated and bound to replicas.
     */
    class NetBindingSliceInstantiationHandler
        : public SliceInstantiationResultBus::Handler
    {
    public:
        ~NetBindingSliceInstantiationHandler() override;

        void InstantiateEntities();
        bool IsInstantiated() const;
        bool IsANewSliceRequest() const;
        bool IsBindingComplete() const;

        /**
         * \note Returns false if there are no entities in the slice or the slice instance isn't ready yet.
         * \return true if any of the entities from the slice are active
         */
        bool HasActiveEntities() const;

        //////////////////////////////////////////////////////////////////////////
        // SliceInstantiationResultBus
        void OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& sliceAddress) override;
        void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId) override;
        //////////////////////////////////////////////////////////////////////////

        void InstantiationFailureCleanup();
        void UseCacheFor(BindRequest& request, const AZ::EntityId& staticEntityId);
        void CloseEntityMap(const AZ::SliceComponent::EntityIdToEntityIdMap& staticToRuntimeMap);

        AZ::Data::AssetId m_sliceAssetId;
        BindRequestContainerType m_bindingQueue;
        SliceInstantiationTicket m_ticket;

        /**
         * \breif a cache of entities that might be networked at some point
         * \note they might be bound and unbound if their replicas leave and come back in the view
         */
        AZStd::vector<AZ::Entity*> m_boundEntities;

        /**
         * \brief identifies which slice instance the instantiation will be performed for
         */
        AZ::SliceComponent::SliceInstanceId m_sliceInstanceId;
        /**
         * \brief when was the request to spawn a slice and bind it made
         */
        AZStd::chrono::system_clock::time_point m_bindTime;

        AZ::SliceComponent::EntityIdToEntityIdMap m_staticToRuntimeEntityMap;

        /**
         * \brief The state of the slice instance.
         */
        enum class State
        {
            /**
             * \brief Has not started instantiating the slice instance.
             */
            NewRequest,
            /**
             * \brief Waiting on the slice to spawn.
             */
            Spawning,
            /**
             * \brief Successfully spawned the slice assets.
             */
            Spawned,
            /**
             * \brief Failed to spawn the slice.
             */
            Failed
        };

        State m_state = State::NewRequest;
    };

    /**
    * NetBindingSystemImpl works in conjunction with NetBindingComponent and
    * NetBindingComponentChunk to perform network binding for game entities.
    *
    * It is responsible for adding entity replicas to the network on the master side
    * and servicing entity spawn requests from the network on the proxy side, as
    * well as detecting network availability and triggering network binding/unbinding.
    *
    * The system is first activated on the host side when OnNetworkSessionActivated event
    * is received, and NetBindingSystemContextData is created.
    * The system becomes fully operational when the NetBindingSystemContextData is activated
    * and bound to the system, and remains operational as long as the NetBindingSystemContextData
    * remains valid.
    *
    * Level switching is tracked by a monotonically increasing context sequence number controlled
    * by the host. Spawn and bind operations are deferred until the correct sequence number
    * is reached. Spawning is always performed from the game thread.
    */
    class NetBindingSystemImpl
        : public NetBindingSystemBus::Handler
        , public NetBindingSystemEventsBus::Handler
        , public EntityContextEventBus::Handler
        , public AZ::TickBus::Handler
    {
        friend class NetBindingSystemContextData;

    public:
        NetBindingSystemImpl();
        ~NetBindingSystemImpl() override;

        static void Reflect(AZ::ReflectContext* context);

        virtual void Init();
        virtual void Shutdown();

        static const AZStd::chrono::milliseconds s_sliceBindingTimeout;

        //////////////////////////////////////////////////////////////////////////
        // NetBindingSystemBus
        bool ShouldBindToNetwork() override;
        NetBindingContextSequence GetCurrentContextSequence() override;
        void AddReplicaMaster(AZ::Entity* entity, GridMate::ReplicaPtr replica) override;
        AZ::EntityId GetStaticIdFromEntityId(AZ::EntityId entity) override;
        AZ::EntityId GetEntityIdFromStaticId(AZ::EntityId staticEntityId) override;
        void SpawnEntityFromSlice(GridMate::ReplicaId bindTo, const NetBindingSliceContext& bindToContext) override;
        void SpawnEntityFromStream(AZ::IO::GenericStream& spawnData, AZ::EntityId useEntityId, GridMate::ReplicaId bindTo, NetBindingContextSequence addToContext) override;
        void OnNetworkSessionActivated(GridMate::GridSession* session) override;
        void OnNetworkSessionDeactivated(GridMate::GridSession* session) override;
        void UnbindGameEntity(AZ::EntityId entity, const AZ::SliceComponent::SliceInstanceId& sliceInstanceId) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // EntityContextEventBus::Handler
        void OnEntityContextReset() override;
        void OnEntityContextLoadedFromStream(const AZ::SliceComponent::EntityList& contextEntities) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        //! Called by the NetBindingContext chunk when it is activated
        void OnContextDataActivated(GridMate::ReplicaChunkPtr contextData);

        //! Called by the NetBindingContext chunk when it is deactivated
        void OnContextDataDeactivated(GridMate::ReplicaChunkPtr contextData);

        //! Update the current binding context sequence
        virtual void UpdateContextSequence();

        //! Process pending spawn requests
        virtual void ProcessSpawnRequests();

        //! Process pending bind requests
        virtual void ProcessBindRequests();

        //! Performs final stage of entity spawning process
        virtual bool BindAndActivate(AZ::Entity* entity, GridMate::ReplicaId replicaId, bool addToContext, const AZ::SliceComponent::SliceInstanceId& sliceInstanceId);

        //! Called on the host to spawn the net binding system replica
        virtual GridMate::Replica* CreateSystemReplica();

        AZ_FORCE_INLINE bool ReadyToAddReplica() const;

        class SpawnRequest
        {
        public:
            GridMate::ReplicaId m_bindTo;
            AZ::EntityId m_useEntityId;
            AZStd::vector<AZ::u8> m_spawnDataBuffer;
        };

        typedef AZStd::list<SpawnRequest> SpawnRequestContainerType;
        typedef AZStd::map<NetBindingContextSequence, SpawnRequestContainerType> SpawnRequestContextContainerType;

        typedef AZStd::unordered_map<AZ::SliceComponent::SliceInstanceId, NetBindingSliceInstantiationHandler> SliceRequestContainerType;
        typedef AZStd::map<NetBindingContextSequence, SliceRequestContainerType> BindRequestContextContainerType;

        GridMate::GridSession* m_bindingSession;
        GridMate::ReplicaChunkPtr m_contextData;
        NetBindingContextSequence m_currentBindingContextSequence;
        SpawnRequestContextContainerType m_spawnRequests;
        BindRequestContextContainerType m_bindRequests;
        AZStd::list<AZStd::pair<AZ::EntityId, GridMate::ReplicaPtr>> m_addMasterRequests;

        /**
         * \brief override how root slice entities' replicas should be loaded
         *
         * We occasionally get GameContextBridge replica (that tells us what level to load) before we get
         * a replica that tells us that we are connecting to a network sessions, thus we may not figure out in time if we
         * need to load the root slice entities with NetBindingComponent as master replicas or proxy replicas.
         * This is a fix until proper order is established.
         *
         * \param isAuthoritative true if root slice entities with NetBindingComponents to be loaded authoritatively
         */
        void OverrideRootSliceLoadMode(bool isAuthoritative)
        {
            m_isAuthoritativeRootSliceLoad = isAuthoritative;
            m_overrideRootSliceLoadAuthoritative = true;
        }

    private:
        /**
         * \brief True if the root slice is to be loaded authoritatively
         */
        bool m_isAuthoritativeRootSliceLoad;
        /**
         * \brief True if root slice loading mode was overriden, otherwise the mode would be determined via m_bindingSession
         */
        bool m_overrideRootSliceLoadAuthoritative;
        /**
         * \brief A helper method to figure the mode of loading root slice entities' replicas
         * \return True if the root slice entities is to be loaded authoritatively
         */
        bool IsAuthoritateLoad() const;

        void UpdateClock(float deltaTime);
        AZStd::chrono::system_clock::time_point Now() const;

        AZStd::chrono::system_clock::time_point m_currentTime;
    };

    class NetBindingSystemContextData
        : public GridMate::ReplicaChunk
    {
    public:
        AZ_CLASS_ALLOCATOR(NetBindingSystemContextData, AZ::SystemAllocator, 0);

        static const char* GetChunkName() { return "NetBindingSystemContextData"; }

        NetBindingSystemContextData();

        bool IsReplicaMigratable() override { return true; }
        bool IsBroadcast() override { return true; }

        void OnReplicaActivate(const GridMate::ReplicaContext& rc) override;

        void OnReplicaDeactivate(const GridMate::ReplicaContext& rc) override;

        GridMate::DataSet<AZ::u32, GridMate::VlqU32Marshaler> m_bindingContextSequence;
    };
} // namespace AzFramework

