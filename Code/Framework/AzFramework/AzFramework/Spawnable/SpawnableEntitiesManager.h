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

        enum class CommandQueueStatus : bool
        {
            HasCommandsLeft,
            NoCommandLeft
        };

        ~SpawnableEntitiesManager() override = default;

        //
        // The following functions are thread safe
        //

        void SpawnAllEntities(EntitySpawnTicket& ticket, EntityPreInsertionCallback preInsertionCallback = {}, EntitySpawnCallback completionCallback = {}) override;
        void SpawnEntities(EntitySpawnTicket& ticket, AZStd::vector<size_t> entityIndices, EntityPreInsertionCallback preInsertionCallback = {},
            EntitySpawnCallback completionCallback = {}) override;
        void DespawnAllEntities(EntitySpawnTicket& ticket, EntityDespawnCallback completionCallback = {}) override;

        void ReloadSpawnable(EntitySpawnTicket& ticket, AZ::Data::Asset<Spawnable> spawnable,
            ReloadSpawnableCallback completionCallback = {}) override;

        void ListEntities(EntitySpawnTicket& ticket, ListEntitiesCallback listCallback) override;
        void ClaimEntities(EntitySpawnTicket& ticket, ClaimEntitiesCallback listCallback) override;

        void Barrier(EntitySpawnTicket& spawnInfo, BarrierCallback completionCallback) override;

        void AddOnSpawnedHandler(AZ::Event<AZ::Data::Asset<Spawnable>>::Handler& handler) override;
        void AddOnDespawnedHandler(AZ::Event<AZ::Data::Asset<Spawnable>>::Handler& handler) override;

        //
        // The following function is thread safe but intended to be run from the main thread.
        //

        CommandQueueStatus ProcessQueue();

    protected:
        void* CreateTicket(AZ::Data::Asset<Spawnable>&& spawnable) override;
        void DestroyTicket(void* ticket) override;

    private:
        struct Ticket
        {
            AZ_CLASS_ALLOCATOR(Ticket, AZ::ThreadPoolAllocator, 0);
            static constexpr uint32_t Processing = AZStd::numeric_limits<uint32_t>::max();

            AZStd::vector<AZ::Entity*> m_spawnedEntities;
            AZStd::vector<size_t> m_spawnedEntityIndices;
            AZ::Data::Asset<Spawnable> m_spawnable;
            uint32_t m_nextTicketId{ 0 }; //!< Next id for this ticket.
            uint32_t m_currentTicketId{ 0 }; //!< The id for the command that should be executed.
            bool m_loadAll{ true };
        };

        struct SpawnAllEntitiesCommand
        {
            EntitySpawnCallback m_completionCallback;
            EntityPreInsertionCallback m_preInsertionCallback;
            EntitySpawnTicket* m_ticket;
            uint32_t m_ticketId;
        };
        struct SpawnEntitiesCommand
        {
            AZStd::vector<size_t> m_entityIndices;
            EntitySpawnCallback m_completionCallback;
            EntityPreInsertionCallback m_preInsertionCallback;
            EntitySpawnTicket* m_ticket;
            uint32_t m_ticketId;
        };
        struct DespawnAllEntitiesCommand
        {
            EntityDespawnCallback m_completionCallback;
            EntitySpawnTicket* m_ticket;
            uint32_t m_ticketId;
        };
        struct ReloadSpawnableCommand
        {
            AZ::Data::Asset<Spawnable> m_spawnable;
            ReloadSpawnableCallback m_completionCallback;
            EntitySpawnTicket* m_ticket;
            uint32_t m_ticketId;
        };
        struct ListEntitiesCommand
        {
            ListEntitiesCallback m_listCallback;
            EntitySpawnTicket* m_ticket;
            uint32_t m_ticketId;
        };
        struct ClaimEntitiesCommand
        {
            ClaimEntitiesCallback m_listCallback;
            EntitySpawnTicket* m_ticket;
            uint32_t m_ticketId;
        };
        struct BarrierCommand
        {
            BarrierCallback m_completionCallback;
            EntitySpawnTicket* m_ticket;
            uint32_t m_ticketId;
        };
        struct DestroyTicketCommand
        {
            Ticket* m_ticket;
            uint32_t m_ticketId;
        };

        using Requests = AZStd::variant<SpawnAllEntitiesCommand, SpawnEntitiesCommand, DespawnAllEntitiesCommand, ReloadSpawnableCommand,
            ListEntitiesCommand, ClaimEntitiesCommand, BarrierCommand, DestroyTicketCommand>;

        AZ::Entity* SpawnSingleEntity(const AZ::Entity& entityTemplate, AZ::SerializeContext& serializeContext);

        bool ProcessRequest(SpawnAllEntitiesCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(SpawnEntitiesCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(DespawnAllEntitiesCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(ReloadSpawnableCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(ListEntitiesCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(ClaimEntitiesCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(BarrierCommand& request, AZ::SerializeContext& serializeContext);
        bool ProcessRequest(DestroyTicketCommand& request, AZ::SerializeContext& serializeContext);

        [[nodiscard]] static bool IsEqualTicket(const EntitySpawnTicket* lhs, const EntitySpawnTicket* rhs);
        [[nodiscard]] static bool IsEqualTicket(const Ticket* lhs, const EntitySpawnTicket* rhs);
        [[nodiscard]] static bool IsEqualTicket(const EntitySpawnTicket* lhs, const Ticket* rhs);
        [[nodiscard]] static bool IsEqualTicket(const Ticket* lhs, const Ticket* rhs);

        AZStd::deque<Requests> m_delayedQueue; //!< Requests that were processed before, but couldn't be completed.
        AZStd::queue<Requests> m_pendingRequestQueue;
        AZStd::mutex m_pendingRequestQueueMutex;

        AZ::Event<AZ::Data::Asset<Spawnable>> m_onSpawnedEvent;
        AZ::Event<AZ::Data::Asset<Spawnable>> m_onDespawnedEvent;
    };
} // namespace AzFramework
