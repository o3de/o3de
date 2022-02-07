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
        AZ_CLASS_ALLOCATOR(SpawnableEntitiesManager, AZ::SystemAllocator, 0);

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
        ~SpawnableEntitiesManager() override = default;

        //
        // The following functions are thread safe
        //

        void SpawnAllEntities(EntitySpawnTicket& ticket, SpawnAllEntitiesOptionalArgs optionalArgs = {}) override;
        void SpawnEntities(
            EntitySpawnTicket& ticket, AZStd::vector<size_t> entityIndices, SpawnEntitiesOptionalArgs optionalArgs = {}) override;
        void DespawnAllEntities(EntitySpawnTicket& ticket, DespawnAllEntitiesOptionalArgs optionalArgs = {}) override;
        void DespawnEntity(AZ::EntityId entityId, EntitySpawnTicket& ticket, DespawnEntityOptionalArgs optionalArgs = {}) override;
        void RetrieveEntitySpawnTicket(EntitySpawnTicket::Id entitySpawnTicketId, RetrieveEntitySpawnTicketCallback callback) override;
        void ReloadSpawnable(
            EntitySpawnTicket& ticket, AZ::Data::Asset<Spawnable> spawnable, ReloadSpawnableOptionalArgs optionalArgs = {}) override;

        void ListEntities(
            EntitySpawnTicket& ticket, ListEntitiesCallback listCallback, ListEntitiesOptionalArgs optionalArgs = {}) override;
        void ListIndicesAndEntities(
            EntitySpawnTicket& ticket, ListIndicesEntitiesCallback listCallback, ListEntitiesOptionalArgs optionalArgs = {}) override;
        void ClaimEntities(
            EntitySpawnTicket& ticket, ClaimEntitiesCallback listCallback, ClaimEntitiesOptionalArgs optionalArgs = {}) override;

        void Barrier(EntitySpawnTicket& spawnInfo, BarrierCallback completionCallback, BarrierOptionalArgs optionalArgs = {}) override;

        //
        // The following function is thread safe but intended to be run from the main thread.
        //

        CommandQueueStatus ProcessQueue(CommandQueuePriority priority);

    protected:
        struct Ticket
        {
            AZ_CLASS_ALLOCATOR(Ticket, AZ::ThreadPoolAllocator, 0);
            static constexpr uint32_t Processing = AZStd::numeric_limits<uint32_t>::max();

            //! Map of template entity ids to their associated instance ids.
            //! Tickets can be used to spawn the same template entities multiple times, in any order, across multiple calls.
            //! Since template entities can reference other entities, this map is used to fix up those references across calls
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
            AZStd::vector<size_t> m_spawnedEntityIndices;
            AZ::Data::Asset<Spawnable> m_spawnable;
            uint32_t m_nextRequestId{ 0 }; //!< Next id for this ticket.
            uint32_t m_currentRequestId { 0 }; //!< The id for the command that should be executed.
            bool m_loadAll{ true };
        };

        struct SpawnAllEntitiesCommand
        {
            EntitySpawnCallback m_completionCallback;
            EntityPreInsertionCallback m_preInsertionCallback;
            AZ::SerializeContext* m_serializeContext;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct SpawnEntitiesCommand
        {
            AZStd::vector<size_t> m_entityIndices;
            EntitySpawnCallback m_completionCallback;
            EntityPreInsertionCallback m_preInsertionCallback;
            AZ::SerializeContext* m_serializeContext;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
            bool m_referencePreviouslySpawnedEntities;
        };
        struct DespawnAllEntitiesCommand
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
        struct ReloadSpawnableCommand
        {
            AZ::Data::Asset<Spawnable> m_spawnable;
            ReloadSpawnableCallback m_completionCallback;
            AZ::SerializeContext* m_serializeContext;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct ListEntitiesCommand
        {
            ListEntitiesCallback m_listCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct ListIndicesEntitiesCommand
        {
            ListIndicesEntitiesCallback m_listCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct ClaimEntitiesCommand
        {
            ClaimEntitiesCallback m_listCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct BarrierCommand
        {
            BarrierCallback m_completionCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct DestroyTicketCommand
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
            ListEntitiesCommand,
            ListIndicesEntitiesCommand,
            ClaimEntitiesCommand,
            BarrierCommand,
            DestroyTicketCommand>;

        struct Queue
        {
            AZStd::deque<Requests> m_delayed; //!< Requests that were processed before, but couldn't be completed.
            AZStd::queue<Requests> m_pendingRequest; //!< Requests waiting to be processed for the first time.
            AZStd::mutex m_pendingRequestMutex;
        };

        template<typename T>
        void QueueRequest(EntitySpawnTicket& ticket, SpawnablePriority priority, T&& request);
        AZStd::pair<EntitySpawnTicket::Id, void*> CreateTicket(AZ::Data::Asset<Spawnable>&& spawnable) override;
        void DestroyTicket(void* ticket) override;

        CommandQueueStatus ProcessQueue(Queue& queue);

        AZ::Entity* CloneSingleEntity(
            const AZ::Entity& entityTemplate, EntityIdMap& templateToCloneMap, AZ::SerializeContext& serializeContext);
        
        bool ProcessRequest(SpawnAllEntitiesCommand& request);
        bool ProcessRequest(SpawnEntitiesCommand& request);
        bool ProcessRequest(DespawnAllEntitiesCommand& request);
        bool ProcessRequest(DespawnEntityCommand& request);
        bool ProcessRequest(ReloadSpawnableCommand& request);
        bool ProcessRequest(ListEntitiesCommand& request);
        bool ProcessRequest(ListIndicesEntitiesCommand& request);
        bool ProcessRequest(ClaimEntitiesCommand& request);
        bool ProcessRequest(BarrierCommand& request);
        bool ProcessRequest(DestroyTicketCommand& request);

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
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AzFramework::SpawnableEntitiesManager::CommandQueuePriority);
} // namespace AzFramework
