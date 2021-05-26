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

        static constexpr SpawnablePriority HighPriorityThreshold = SpawnablePriority { 64 };

        ~SpawnableEntitiesManager() override = default;

        //
        // The following functions are thread safe
        //

        void SpawnAllEntities(
            EntitySpawnTicket& ticket, SpawnablePriority priority, EntityPreInsertionCallback preInsertionCallback = {},
            EntitySpawnCallback completionCallback = {}) override;
        void SpawnEntities(
            EntitySpawnTicket& ticket, SpawnablePriority priority, AZStd::vector<size_t> entityIndices,
            EntityPreInsertionCallback preInsertionCallback = {},
            EntitySpawnCallback completionCallback = {}) override;
        void DespawnAllEntities(
            EntitySpawnTicket& ticket, SpawnablePriority priority, EntityDespawnCallback completionCallback = {}) override;

        void ReloadSpawnable(
            EntitySpawnTicket& ticket, SpawnablePriority priority, AZ::Data::Asset<Spawnable> spawnable,
            ReloadSpawnableCallback completionCallback = {}) override;

        void ListEntities(EntitySpawnTicket& ticket, SpawnablePriority priority, ListEntitiesCallback listCallback) override;
        void ListIndicesAndEntities(
            EntitySpawnTicket& ticket, SpawnablePriority priority, ListIndicesEntitiesCallback listCallback) override;
        void ClaimEntities(EntitySpawnTicket& ticket, SpawnablePriority priority, ClaimEntitiesCallback listCallback) override;

        void Barrier(EntitySpawnTicket& spawnInfo, SpawnablePriority priority, BarrierCallback completionCallback) override;

        void AddOnSpawnedHandler(AZ::Event<AZ::Data::Asset<Spawnable>>::Handler& handler) override;
        void AddOnDespawnedHandler(AZ::Event<AZ::Data::Asset<Spawnable>>::Handler& handler) override;

        //
        // The following function is thread safe but intended to be run from the main thread.
        //

        CommandQueueStatus ProcessQueue(CommandQueuePriority priority);

    protected:
        struct Ticket
        {
            AZ_CLASS_ALLOCATOR(Ticket, AZ::ThreadPoolAllocator, 0);
            static constexpr uint32_t Processing = AZStd::numeric_limits<uint32_t>::max();

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
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct SpawnEntitiesCommand
        {
            AZStd::vector<size_t> m_entityIndices;
            EntitySpawnCallback m_completionCallback;
            EntityPreInsertionCallback m_preInsertionCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct DespawnAllEntitiesCommand
        {
            EntityDespawnCallback m_completionCallback;
            Ticket* m_ticket;
            EntitySpawnTicket::Id m_ticketId;
            uint32_t m_requestId;
        };
        struct ReloadSpawnableCommand
        {
            AZ::Data::Asset<Spawnable> m_spawnable;
            ReloadSpawnableCallback m_completionCallback;
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
            SpawnAllEntitiesCommand, SpawnEntitiesCommand, DespawnAllEntitiesCommand, ReloadSpawnableCommand, ListEntitiesCommand,
            ListIndicesEntitiesCommand, ClaimEntitiesCommand, BarrierCommand, DestroyTicketCommand>;

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

        AZ::Entity* SpawnSingleEntity(const AZ::Entity& entityTemplate,
            AZ::SerializeContext& serializeContext);

        AZ::Entity* CloneSingleEntity(const AZ::Entity& entityTemplate,
            EntityIdMap& templateToCloneEntityIdMap, AZ::SerializeContext& serializeContext);

        bool ProcessRequest(SpawnAllEntitiesCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(SpawnEntitiesCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(DespawnAllEntitiesCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(ReloadSpawnableCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(ListEntitiesCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(ListIndicesEntitiesCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(ClaimEntitiesCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(BarrierCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(DestroyTicketCommand& request, AZ::SerializeContext& serializeContext);

        [[nodiscard]] static bool IsEqualTicket(const EntitySpawnTicket* lhs, const EntitySpawnTicket* rhs);
        [[nodiscard]] static bool IsEqualTicket(const Ticket* lhs, const EntitySpawnTicket* rhs);
        [[nodiscard]] static bool IsEqualTicket(const EntitySpawnTicket* lhs, const Ticket* rhs);
        [[nodiscard]] static bool IsEqualTicket(const Ticket* lhs, const Ticket* rhs);

        Queue m_highPriorityQueue;
        Queue m_regularPriorityQueue;

        AZ::Event<AZ::Data::Asset<Spawnable>> m_onSpawnedEvent;
        AZ::Event<AZ::Data::Asset<Spawnable>> m_onDespawnedEvent;
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AzFramework::SpawnableEntitiesManager::CommandQueuePriority);
} // namespace AzFramework
