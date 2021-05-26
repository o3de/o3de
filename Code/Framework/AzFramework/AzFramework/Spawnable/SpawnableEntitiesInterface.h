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
#include <AzCore/RTTI/TypeSafeIntegral.h>
#include <AzCore/std/functional.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace AZ
{
    class Entity;
}

namespace AzFramework
{
    AZ_TYPE_SAFE_INTEGRAL(SpawnablePriority, uint8_t);

    inline static constexpr SpawnablePriority SpawnablePriorty_Highest  { 0 };
    inline static constexpr SpawnablePriority SpawnablePriorty_High     { 32 };
    inline static constexpr SpawnablePriority SpawnablePriorty_Default  { 128 };
    inline static constexpr SpawnablePriority SpawnablePriorty_Low      { 192 };
    inline static constexpr SpawnablePriority SpawnablePriorty_Lowest   { 255 };

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

    class SpawnableIndexEntityPair
    {
    public:
        friend class SpawnableIndexEntityIterator;

        AZ::Entity* GetEntity();
        const AZ::Entity* GetEntity() const;
        size_t GetIndex() const;

    private:
        SpawnableIndexEntityPair() = default;
        SpawnableIndexEntityPair(const SpawnableIndexEntityPair&) = default;
        SpawnableIndexEntityPair(SpawnableIndexEntityPair&&) = default;
        SpawnableIndexEntityPair(AZ::Entity** entityIterator, size_t* indexIterator);

        SpawnableIndexEntityPair& operator=(const SpawnableIndexEntityPair&) = default;
        SpawnableIndexEntityPair& operator=(SpawnableIndexEntityPair&&) = default;

        AZ::Entity** m_entity { nullptr };
        size_t* m_index { nullptr };
    };

    class SpawnableIndexEntityIterator
    {
    public:
        // Limited to bidirectional iterator as there's no use case for extending it further, but can be extended if a use case is found.
        using iterator_category = AZStd::bidirectional_iterator_tag;
        using value_type = SpawnableIndexEntityPair;
        using difference_type = size_t;
        using pointer = SpawnableIndexEntityPair*;
        using reference = SpawnableIndexEntityPair&;

        SpawnableIndexEntityIterator(AZ::Entity** entityIterator, size_t* indexIterator);

        SpawnableIndexEntityIterator& operator++();
        SpawnableIndexEntityIterator operator++(int);
        SpawnableIndexEntityIterator& operator--();
        SpawnableIndexEntityIterator operator--(int);

        bool operator==(const SpawnableIndexEntityIterator& rhs);
        bool operator!=(const SpawnableIndexEntityIterator& rhs);

        SpawnableIndexEntityPair& operator*();
        const SpawnableIndexEntityPair& operator*() const;
        SpawnableIndexEntityPair* operator->();
        const SpawnableIndexEntityPair* operator->() const;

    private:
        SpawnableIndexEntityPair m_value;
    };

    class SpawnableConstIndexEntityContainerView
    {
    public:
        SpawnableConstIndexEntityContainerView(AZ::Entity** beginEntity, size_t* beginIndices, size_t length);

        const SpawnableIndexEntityIterator& begin();
        const SpawnableIndexEntityIterator& end();
        const SpawnableIndexEntityIterator& cbegin();
        const SpawnableIndexEntityIterator& cend();

    private:
        SpawnableIndexEntityIterator m_begin;
        SpawnableIndexEntityIterator m_end;
    };

    //! Requests to the SpawnableEntitiesInterface require a ticket with a valid spawnable that is used as a template. A ticket can
    //! be reused for multiple calls on the same spawnable and is safe to be used by multiple threads at the same time. Entities created
    //! from the spawnable may be tracked by the ticket and so using the same ticket is needed to despawn the exact entities created
    //! by a call to spawn entities. The life cycle of the spawned entities is tied to the ticket and all entities spawned using a
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
    using EntityPreInsertionCallback = AZStd::function<void(EntitySpawnTicket&, SpawnableEntityContainerView)>;
    using EntityDespawnCallback = AZStd::function<void(EntitySpawnTicket&)>;
    using ReloadSpawnableCallback = AZStd::function<void(EntitySpawnTicket&, SpawnableConstEntityContainerView)>;
    using ListEntitiesCallback = AZStd::function<void(EntitySpawnTicket&, SpawnableConstEntityContainerView)>;
    using ListIndicesEntitiesCallback = AZStd::function<void(EntitySpawnTicket&, SpawnableConstIndexEntityContainerView)>;
    using ClaimEntitiesCallback = AZStd::function<void(EntitySpawnTicket&, SpawnableEntityContainerView)>;
    using BarrierCallback = AZStd::function<void(EntitySpawnTicket&)>;

    //! Interface definition to (de)spawn entities from a spawnable into the game world.
    //! 
    //! While the callbacks of the individual calls are being processed they will block processing any other request. Callbacks can be
    //! issued from threads other than the one that issued the call, including the main thread.
    //!
    //! Calls on the same ticket are guaranteed to be executed in the order they are issued. Note that when issuing requests from
    //! multiple threads on the same ticket the order in which the requests are assigned to the ticket is not guaranteed.
    //!
    //! Most calls have a priority where values closer to 0 mean higher priority than values closer to 255. The implementation of this
    //! interface may choose to use priority lanes which doesn't guarantee that higher priority requests happen before lower priority
    //! requests if they don't pass the priority lane threshold. Priority lanes and their thresholds are implementation specific and may
    //! differ between platforms. Note that if a call happened on a ticket with lower priority followed by a one with a higher priority
    //! the first lower priority call will still needs to complete before the second higher priority call can be executed and the priority
    //! of the first call will not be updated.
    class SpawnableEntitiesDefinition
    {
    public:
        AZ_RTTI(AzFramework::SpawnableEntitiesDefinition, "{A9ED3F1F-4D69-4182-B0CD-EB561EEA7068}");

        friend class EntitySpawnTicket;

        virtual ~SpawnableEntitiesDefinition() = default;

        //! Spawn instances of all entities in the spawnable.
        //! @param ticket Stores the results of the call. Use this ticket to spawn additional entities or to despawn them.
        //! @param priority The priority at which this call will be executed.
        //! @param completionCallback Optional callback that's called when spawning entities has completed. This can be called from
        //!     a different thread than the one that made the function call. The returned list of entities contains all the newly
        //!     created entities.
        virtual void SpawnAllEntities(
            EntitySpawnTicket& ticket, SpawnablePriority priority, EntityPreInsertionCallback preInsertionCallback = {},
            EntitySpawnCallback completionCallback = {}) = 0;
        //! Spawn instances of some entities in the spawnable.
        //! @param ticket Stores the results of the call. Use this ticket to spawn additional entities or to despawn them.
        //! @param priority The priority at which this call will be executed.
        //! @param entityIndices The indices into the template entities stored in the spawnable that will be used to spawn entities from.
        //! @param completionCallback Optional callback that's called when spawning entities has completed. This can be called from
        //!     a different thread than the one that made this function call. The returned list of entities contains all the newly
        //!     created entities.
        virtual void SpawnEntities(
            EntitySpawnTicket& ticket, SpawnablePriority priority, AZStd::vector<size_t> entityIndices,
            EntityPreInsertionCallback preInsertionCallback = {}, EntitySpawnCallback completionCallback = {}) = 0;
        //! Removes all entities in the provided list from the environment.
        //! @param ticket The ticket previously used to spawn entities with.
        //! @param priority The priority at which this call will be executed.
        //! @param completionCallback Optional callback that's called when despawning entities has completed. This can be called from
        //!     a different thread than the one that made this function call.
        virtual void DespawnAllEntities(
            EntitySpawnTicket& ticket, SpawnablePriority priority, EntityDespawnCallback completionCallback = {}) = 0;

        //! Removes all entities in the provided list from the environment and reconstructs the entities from the provided spawnable.
        //! @param ticket Holds the information on the entities to reload.
        //! @param priority The priority at which this call will be executed.
        //! @param spawnable The spawnable that will replace the existing spawnable. Both need to have the same asset id.
        //! @param completionCallback Optional callback that's called when the entities have been reloaded. This can be called from
        //!     a different thread than the one that made this function call. The returned list of entities contains all the replacement
        //!     entities.
        virtual void ReloadSpawnable(
            EntitySpawnTicket& ticket, SpawnablePriority priority, AZ::Data::Asset<Spawnable> spawnable,
            ReloadSpawnableCallback completionCallback = {}) = 0;

        //! List all entities that are spawned using this ticket.
        //! @param ticket Only the entities associated with this ticket will be listed.
        //! @param priority The priority at which this call will be executed.
        //! @param listCallback Required callback that will be called to list the entities on.
        virtual void ListEntities(EntitySpawnTicket& ticket, SpawnablePriority priority, ListEntitiesCallback listCallback) = 0;
        //! List all entities that are spawned using this ticket with their spawnable index.
        //!     Spawnables contain a flat list of entities, which are used as templates to spawn entities from. For every spawned entity
        //!     the index of the entity in the spawnable that was used as a template is stored. This version of ListEntities will return
        //!     both the entities and this index. The index can be used with SpawnEntities to create the same entities again. Note that
        //!     the same index may appear multiple times as there are no restriction on how many instance of a specific entity can be
        //!     created.
        //! @param ticket Only the entities associated with this ticket will be listed.
        //! @param priority The priority at which this call will be executed.
        //! @param listCallback Required callback that will be called to list the entities and indices on.
        virtual void ListIndicesAndEntities(
            EntitySpawnTicket& ticket, SpawnablePriority priority, ListIndicesEntitiesCallback listCallback) = 0;
        //! Claim all entities that are spawned using this ticket. Ownership of the entities is transferred from the ticket to the
        //!     caller through the callback. After this call the ticket will have no entities associated with it. The caller of
        //!     this function will need to manage the entities after this call.
        //! @param ticket Only the entities associated with this ticket will be released.
        //! @param priority The priority at which this call will be executed.
        //! @param listCallback Required callback that will be called to transfer the entities through.
        virtual void ClaimEntities(EntitySpawnTicket& ticket, SpawnablePriority priority, ClaimEntitiesCallback listCallback) = 0;

        //! Blocks until all operations made on the provided ticket before the barrier call have completed.
        //! @param ticket The ticket to monitor.
        //! @param priority The priority at which this call will be executed.
        //! @param completionCallback Required callback that will be called as soon as the barrier has been reached.
        virtual void Barrier(EntitySpawnTicket& ticket, SpawnablePriority priority, BarrierCallback completionCallback) = 0;

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
