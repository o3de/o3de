/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace AZ
{
    class Entity;
    class SerializeContext;
}

namespace AzFramework
{
    class SpawnableEntitiesManager
        : public SpawnableEntitiesInterface::Registrar
    {
    public:
        AZ_RTTI(AzFramework::SpawnableEntitiesManager, "{6E14333F-128C-464C-94CA-A63B05A5E51C}");
        AZ_CLASS_ALLOCATOR(SpawnableEntitiesManager, AZ::SystemAllocator);

        using EntityIdMap = AZStd::unordered_map<AZ::EntityId, AZ::EntityId>;
        
        enum class CommandQueueStatus : bool
        {
            HasCommandsLeft,
            NoCommandsLeft
        };

        enum class CommandQueuePriority
        {
            High = 1 << 0,
            Regular = 1 << 1
        };

        SpawnableEntitiesManager();
        ~SpawnableEntitiesManager() override;

        //
        // The following functions are thread safe
        //

        void SpawnAllEntities(EntitySpawnTicket& ticket, SpawnAllEntitiesOptionalArgs optionalArgs = {}) override;
        void SpawnEntities(
            EntitySpawnTicket& ticket, AZStd::vector<uint32_t> entityIndices, SpawnEntitiesOptionalArgs optionalArgs = {}) override;
        void DespawnAllEntities(EntitySpawnTicket& ticket, DespawnAllEntitiesOptionalArgs optionalArgs = {}) override;
        void DespawnEntity(AZ::EntityId entityId, EntitySpawnTicket& ticket, DespawnEntityOptionalArgs optionalArgs = {}) override;
        void RetrieveTicket(
            EntitySpawnTicket::Id ticketId,
            RetrieveEntitySpawnTicketCallback callback,
            RetrieveTicketOptionalArgs optionalArgs = {}) override;
        void ReloadSpawnable(
            EntitySpawnTicket& ticket, AZ::Data::Asset<Spawnable> spawnable, ReloadSpawnableOptionalArgs optionalArgs = {}) override;

        void UpdateEntityAliasTypes(
            EntitySpawnTicket& ticket,
            AZStd::vector<EntityAliasTypeChange> updatedAliases,
            UpdateEntityAliasTypesOptionalArgs optionalArgs = {}) override;

        void ListEntities(
            EntitySpawnTicket& ticket, ListEntitiesCallback listCallback, ListEntitiesOptionalArgs optionalArgs = {}) override;
        void ListIndicesAndEntities(
            EntitySpawnTicket& ticket, ListIndicesEntitiesCallback listCallback, ListEntitiesOptionalArgs optionalArgs = {}) override;
        void ClaimEntities(
            EntitySpawnTicket& ticket, ClaimEntitiesCallback listCallback, ClaimEntitiesOptionalArgs optionalArgs = {}) override;

        void Barrier(EntitySpawnTicket& spawnInfo, BarrierCallback completionCallback, BarrierOptionalArgs optionalArgs = {}) override;
        void LoadBarrier(
            EntitySpawnTicket& spawnInfo, BarrierCallback completionCallback, LoadBarrierOptionalArgs optionalArgs = {}) override;

        //
        // The following function is thread safe but intended to be run from the main thread.
        //

        CommandQueueStatus ProcessQueue(CommandQueuePriority priority);

    protected:
        enum class CommandResult : bool
        {
            Executed,
            Requeue
        };

        struct Ticket final
        {
            AZ_CLASS_ALLOCATOR(Ticket, AZ::ThreadPoolAllocator);
            static constexpr uint32_t Processing = AZStd::numeric_limits<uint32_t>::max();

            AZStd::atomic_int64_t m_referenceCount{ 1 };
            //! Map of prototype entity ids to their associated instance ids.
            //! Tickets can be used to spawn the same prototype entities multiple times, in any order, across multiple calls.
            //! Since prototype entities can reference other entities, this map is used to fix up those references across calls
            //! using the following policy:
            //! - Entities referencing an entity that hasn't been spawned yet will get a reference to the id that *will* be used
            //!   the first time that entity will be spawned.  The reference will be invalid until that entity is spawned, but
            //!   will be valid if/when it gets spawned.
            //! - Entities referencing an entity that *has* been spawned will get a reference to the id that was *last* used to
            //!   spawn the entity.
            //! Note that this implies a certain level of non-determinism when spawning across calls, because the entity references
            //! will be based on the order in which the SpawnEntity calls occur, which can be affected by things like priority.
            EntityIdMap m_entityIdReferenceMap;
            //! For this to work, we also need to keep track of whether or not each entity has been spawned at least once, so we know
            //! whether or not to replace the id in the map when spawning a new instance of that entity.
            AZStd::unordered_set<AZ::EntityId> m_previouslySpawned;

            AZStd::vector<AZ::Entity*> m_spawnedEntities;
            AZStd::vector<uint32_t> m_spawnedEntityIndices;
            AZ::Data::Asset<Spawnable> m_spawnable;
            uint32_t m_nextRequestId{ 0 }; //!< Next id to be handed out to command that's using this ticket..
            uint32_t m_currentRequestId { 0 }; //!< The id for the command that should be executed.
            uint32_t m_ticketId{ 0 }; //!< The unique id that identifies this ticket.
            bool m_loadAll{ true };
        };

        struct SpawnAllEntitiesCommand final
        {
            EntitySpawnCallback m_completionCallback;
            EntityPreInsertionCallback m_preInsertionCallback;
            AZ::SerializeContext* m_serializeContext;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct SpawnEntitiesCommand final
        {
            AZStd::vector<uint32_t> m_entityIndices;
            EntitySpawnCallback m_completionCallback;
            EntityPreInsertionCallback m_preInsertionCallback;
            AZ::SerializeContext* m_serializeContext;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
            bool m_referencePreviouslySpawnedEntities;
        };
        struct DespawnAllEntitiesCommand final
        {
            EntityDespawnCallback m_completionCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct DespawnEntityCommand
        {
            EntityDespawnCallback m_completionCallback;
            Ticket* m_ticket;
            AZ::EntityId m_entityId;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct ReloadSpawnableCommand final
        {
            AZ::Data::Asset<Spawnable> m_spawnable;
            ReloadSpawnableCallback m_completionCallback;
            AZ::SerializeContext* m_serializeContext;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct UpdateEntityAliasTypesCommand final
        {
            AZStd::vector<EntityAliasTypeChange> m_entityAliases;
            UpdateEntityAliasTypesCallback m_completionCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct ListEntitiesCommand final
        {
            ListEntitiesCallback m_listCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct ListIndicesEntitiesCommand final
        {
            ListIndicesEntitiesCallback m_listCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct ClaimEntitiesCommand final
        {
            ClaimEntitiesCallback m_listCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct BarrierCommand final
        {
            BarrierCallback m_completionCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct LoadBarrierCommand final
        {
            BarrierCallback m_completionCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
            bool m_checkAliasSpawnables;
        };
        struct RetrieveTicketCommand final
        {
            EntitySpawnTicket::Id m_ticketId;
            RetrieveEntitySpawnTicketCallback m_callback;
        };
        struct RegisterTicketCommand final
        {
            Ticket* m_ticket;
            uint32_t m_requestId;
        };
        struct DestroyTicketCommand final
        {
            Ticket* m_ticket;
            uint32_t m_requestId;
        };

        using Requests = AZStd::variant<
            SpawnAllEntitiesCommand,
            SpawnEntitiesCommand,
            DespawnAllEntitiesCommand,
            DespawnEntityCommand,
            ReloadSpawnableCommand,
            UpdateEntityAliasTypesCommand,
            ListEntitiesCommand,
            ListIndicesEntitiesCommand,
            ClaimEntitiesCommand,
            BarrierCommand,
            LoadBarrierCommand,
            RetrieveTicketCommand,
            RegisterTicketCommand,
            DestroyTicketCommand>;

        struct Queue
        {
            AZStd::deque<Requests> m_delayed; //!< Requests that were processed before, but couldn't be completed.
            AZStd::queue<Requests> m_pendingRequest; //!< Requests waiting to be processed for the first time.
            AZStd::mutex m_pendingRequestMutex;
        };

        template<typename T>
        void QueueRequest(EntitySpawnTicket& ticket, SpawnablePriority priority, T&& request);

        void* CreateTicket(AZ::Data::Asset<Spawnable>&& spawnable) override;
        void IncrementTicketReference(void* ticket) override;
        void DecrementTicketReference(void* ticket) override;
        EntitySpawnTicket::Id GetTicketId(void* ticket) override;
        const AZ::Data::Asset<Spawnable>& GetSpawnableOnTicket(void* ticket) override;
        
        CommandQueueStatus ProcessQueue(Queue& queue);

        AZ::Entity* CloneSingleEntity(
            const AZ::Entity& entityPrototype, EntityIdMap& prototypeToCloneMap, AZ::SerializeContext& serializeContext);
        AZ::Entity* CloneSingleAliasedEntity(
            const AZ::Entity& entityPrototype,
            const Spawnable::EntityAlias& alias,
            EntityIdMap& prototypeToCloneMap,
            AZ::Entity* previouslySpawnedEntity,
            AZ::SerializeContext& serializeContext);
        void AppendComponents(
            AZ::Entity& target,
            const AZ::Entity::ComponentArrayType& componentPrototypes,
            EntityIdMap& prototypeToCloneMap,
            AZ::SerializeContext& serializeContext);
        
        CommandResult ProcessRequest(SpawnAllEntitiesCommand& request);
        CommandResult ProcessRequest(SpawnEntitiesCommand& request);
        CommandResult ProcessRequest(DespawnAllEntitiesCommand& request);
        CommandResult ProcessRequest(DespawnEntityCommand& request);
        CommandResult ProcessRequest(ReloadSpawnableCommand& request);
        CommandResult ProcessRequest(UpdateEntityAliasTypesCommand& request);
        CommandResult ProcessRequest(ListEntitiesCommand& request);
        CommandResult ProcessRequest(ListIndicesEntitiesCommand& request);
        CommandResult ProcessRequest(ClaimEntitiesCommand& request);
        CommandResult ProcessRequest(BarrierCommand& request);
        CommandResult ProcessRequest(LoadBarrierCommand& request);
        CommandResult ProcessRequest(RetrieveTicketCommand& request);
        CommandResult ProcessRequest(RegisterTicketCommand& request);
        CommandResult ProcessRequest(DestroyTicketCommand& request);

        //! Generate a base set of original-to-new entity ID mappings to use during spawning.
        //! Since Entity references get fixed up on an entity-by-entity basis while spawning, it's important to have the complete
        //! set of new IDs available right at the start.  This way, entities that refer to other entities that haven't spawned yet
        //! will still get their references remapped correctly.
        void InitializeEntityIdMappings(
            const Spawnable::EntityList& entities, EntityIdMap& idMap, AZStd::unordered_set<AZ::EntityId>& previouslySpawned);
        void RefreshEntityIdMapping(
            const AZ::EntityId& entityId, EntityIdMap& idMap, AZStd::unordered_set<AZ::EntityId>& previouslySpawned);

        Queue m_highPriorityQueue;
        Queue m_regularPriorityQueue;

        AZ::SerializeContext* m_defaultSerializeContext { nullptr };
        //! The threshold used to determine if a request goes in the regular (if bigger than the value) or high priority queue (if smaller
        //! or equal to this value). The starting value of 64 is chosen as it's between default values SpawnablePriority_High and
        //! SpawnablePriority_Default which gives users a bit of room to fine tune the priorities as this value can be configured
        //! through the Settings Registry under the key "/O3DE/AzFramework/Spawnables/HighPriorityThreshold".
        SpawnablePriority m_highPriorityThreshold { 64 };

        AZStd::unordered_map<EntitySpawnTicket::Id, Ticket*> m_entitySpawnTicketMap;
        AZStd::atomic_int m_totalTickets{ 0 };
        AZStd::atomic_int m_ticketsPendingRegistration{ 0 };
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AzFramework::SpawnableEntitiesManager::CommandQueuePriority);
} // namespace AzFramework
