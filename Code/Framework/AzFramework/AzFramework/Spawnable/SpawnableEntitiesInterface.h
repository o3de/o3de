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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/functional.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace AZ
{
    class Entity;
}

namespace AzFramework
{
    class SpawnableEntityContainerView
    {
    public:
        SpawnableEntityContainerView(AZ::Entity** begin, size_t length);
        SpawnableEntityContainerView(AZ::Entity** begin, AZ::Entity** end);

        AZ::Entity** begin();
        AZ::Entity** end();
        const AZ::Entity* const* cbegin();
        const AZ::Entity* const* cend();
        size_t size();

    private:
        AZ::Entity** m_begin;
        AZ::Entity** m_end;
    };

    class SpawnableConstEntityContainerView
    {
    public:
        SpawnableConstEntityContainerView(AZ::Entity** begin, size_t length);
        SpawnableConstEntityContainerView(AZ::Entity** begin, AZ::Entity** end);

        const AZ::Entity* const* begin();
        const AZ::Entity* const* end();
        const AZ::Entity* const* cbegin();
        const AZ::Entity* const* cend();
        size_t size();

    private:
        AZ::Entity** m_begin;
        AZ::Entity** m_end;
    };

    //! Requests to the SpawnableEntitiesInterface require a ticket with a valid spawnable that be used as a template. A ticket can
    //! be reused for multiple calls on the same spawnable and is safe to use by multiple threads at the same time. Entities created
    //! from the spawnable may be tracked by the ticket and so using the same ticket is needed to despawn the exact entities created
    //! by a call so spawn entities. The life cycle of the spawned entities is tied to the ticket and all entities spawned using a
    //! ticket will be despawned when it's deleted.
    class EntitySpawnTicket
    {
    public:
        friend class SpawnableEntitiesDefinition;

        EntitySpawnTicket() = default;
        EntitySpawnTicket(const EntitySpawnTicket&) = delete;
        EntitySpawnTicket(EntitySpawnTicket&& rhs);
        explicit EntitySpawnTicket(AZ::Data::Asset<Spawnable> spawnable);
        ~EntitySpawnTicket();

        EntitySpawnTicket& operator=(const EntitySpawnTicket&) = delete;
        EntitySpawnTicket& operator=(EntitySpawnTicket&& rhs);

        bool IsValid() const;

    private:
        void* m_payload{ nullptr };
    };

    using EntitySpawnCallback = AZStd::function<void(EntitySpawnTicket&, SpawnableConstEntityContainerView)>;
    using EntityDespawnCallback = AZStd::function<void(EntitySpawnTicket&)>;
    using ReloadSpawnableCallback = AZStd::function<void(EntitySpawnTicket&, SpawnableConstEntityContainerView)>;
    using ListEntitiesCallback = AZStd::function<void(EntitySpawnTicket&, SpawnableConstEntityContainerView)>;
    using ClaimEntitiesCallback = AZStd::function<void(EntitySpawnTicket&, SpawnableEntityContainerView)>;
    using BarrierCallback = AZStd::function<void(EntitySpawnTicket&)>;

    //! Interface definition to (de)spawn entities from a spawnable into the game world.
    //! While the callbacks of the individual calls are being processed they will block processing any other request. Callbacks can be
    //! issued from threads other than the one that issued the call, including the main thread.
    //! Calls on the same ticket are guaranteed to be executed in the order they are issued. Note that when issuing requests from
    //! multiple threads on the same ticket the order in which the requests are assigned to the ticket is not guaranteed.
    class SpawnableEntitiesDefinition
    {
    public:
        AZ_RTTI(AzFramework::SpawnableEntitiesDefinition, "{A9ED3F1F-4D69-4182-B0CD-EB561EEA7068}");

        friend class EntitySpawnTicket;

        virtual ~SpawnableEntitiesDefinition() = default;

        //! Spawn instances of all entities in the spawnable.
        //! @param spawnable The Spawnable asset that will be used to create entity instances from.
        //! @param ticket Stores the results of the call. Use this ticket to spawn additional entities or to despawn them.
        //! @param completionCallback Optional callback that's called when spawning entities has completed. This can be called from
        //!     a different thread than the one that made the function call. The returned list of entities contains all the newly
        //!     created entities.
        virtual void SpawnAllEntities(EntitySpawnTicket& ticket, EntitySpawnCallback completionCallback = {}) = 0;
        //! Spawn instances of some entities in the spawnable.
        //! @param ticket Stores the results of the call. Use this ticket to spawn additional entities or to despawn them.
        //! @param entityIndices The indices into the template entities stored in the spawnable that will be used to spawn entities from.
        //! @param completionCallback Optional callback that's called when spawning entities has completed. This can be called from
        //!     a different thread than the one that made this function call. The returned list of entities contains all the newly
        //!     created entities.
        virtual void SpawnEntities(EntitySpawnTicket& ticket, AZStd::vector<size_t> entityIndices,
            EntitySpawnCallback completionCallback = {}) = 0;
        //! Removes all entities in the provided list from the environment.
        //! @param ticket The ticket previously used to spawn entities with.
        //! @param completionCallback Optional callback that's called when despawning entities has completed. This can be called from
        //!     a different thread than the one that made this function call.
        virtual void DespawnAllEntities(EntitySpawnTicket& ticket, EntityDespawnCallback completionCallback = {}) = 0;

        //! Removes all entities in the provided list from the environment and reconstructs the entities from the provided spawnable.
        //! @param ticket Stores the results of the call. Use this ticket to spawn additional entities or to despawn them.
        //! @param spawnable The spawnable that will replace the existing spawnable. Both need to have the same asset id.
        //! @param completionCallback Optional callback that's called when the entities have been reloaded. This can be called from
        //!     a different thread than the one that made this function call. The returned list of entities contains all the replacement
        //!     entities.
        virtual void ReloadSpawnable(EntitySpawnTicket& ticket, AZ::Data::Asset<Spawnable> spawnable,
            ReloadSpawnableCallback completionCallback = {}) = 0;

        //! List all entities that are spawned using this ticket.
        //! @param ticket Only the entities associated with this ticket will be listed.
        //! @param listCallback Required callback that will be called to list the entities on.
        virtual void ListEntities(EntitySpawnTicket& ticket, ListEntitiesCallback listCallback) = 0;
        //! Claim all entities that are spawned using this ticket. Ownership of the entities is transferred from the ticket to the
        //!     caller through the callback. After this call the ticket will have no entities associated with it. The caller of
        //!     this function will need to manage the entities after this call.
        //! @param ticket Only the entities associated with this ticket will be released.
        //! @param listCallback Required callback that will be called to transfer the entities through.
        virtual void ClaimEntities(EntitySpawnTicket& ticket, ClaimEntitiesCallback listCallback) = 0;

        //! Blocks until all operations made on the provided ticket before the barrier call have completed.
        virtual void Barrier(EntitySpawnTicket& ticket, BarrierCallback completionCallback) = 0;

        //! Register a handler for OnSpawned events.
        virtual void AddOnSpawnedHandler(AZ::Event<AZ::Data::Asset<Spawnable>>::Handler& handler) = 0;

        //! Register a handler for OnDespawned events.
        virtual void AddOnDespawnedHandler(AZ::Event<AZ::Data::Asset<Spawnable>>::Handler& handler) = 0;

    protected:
        [[nodiscard]] virtual void* CreateTicket(AZ::Data::Asset<Spawnable>&& spawnable) = 0;
        virtual void DestroyTicket(void* ticket) = 0;

        template<typename T>
        static T& GetTicketPayload(EntitySpawnTicket& ticket)
        {
            return *reinterpret_cast<T*>(ticket.m_payload);
        }

        template<typename T>
        static const T& GetTicketPayload(const EntitySpawnTicket& ticket)
        {
            return *reinterpret_cast<const T*>(ticket.m_payload);
        }

        template<typename T>
        static T* GetTicketPayload(EntitySpawnTicket* ticket)
        {
            return reinterpret_cast<T*>(ticket->m_payload);
        }

        template<typename T>
        static const T* GetTicketPayload(const EntitySpawnTicket* ticket)
        {
            return reinterpret_cast<const T*>(ticket->m_payload);
        }
    };

    using SpawnableEntitiesInterface = AZ::Interface<SpawnableEntitiesDefinition>;
} // namespace AzFramework
