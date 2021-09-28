/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    class SerializeContext;
}

namespace AzFramework
{
    AZ_TYPE_SAFE_INTEGRAL(SpawnablePriority, uint8_t);

    inline static constexpr SpawnablePriority SpawnablePriority_Highest  { 0 };
    inline static constexpr SpawnablePriority SpawnablePriority_High     { 32 };
    inline static constexpr SpawnablePriority SpawnablePriority_Default  { 128 };
    inline static constexpr SpawnablePriority SpawnablePriority_Low      { 192 };
    inline static constexpr SpawnablePriority SpawnablePriority_Lowest   { 255 };

    class SpawnableEntityContainerView
    {
    public:
        SpawnableEntityContainerView(AZ::Entity** begin, size_t length);
        SpawnableEntityContainerView(AZ::Entity** begin, AZ::Entity** end);

        [[nodiscard]] AZ::Entity** begin();
        [[nodiscard]] AZ::Entity** end();
        [[nodiscard]] const AZ::Entity* const* begin() const;
        [[nodiscard]] const AZ::Entity* const* end() const;
        [[nodiscard]] const AZ::Entity* const* cbegin();
        [[nodiscard]] const AZ::Entity* const* cend();

        [[nodiscard]] AZ::Entity* operator[](size_t n);
        [[nodiscard]] const AZ::Entity* operator[](size_t n) const;

        [[nodiscard]] size_t size() const;
        [[nodiscard]] bool empty() const;

    private:
        AZ::Entity** m_begin;
        AZ::Entity** m_end;
    };

    class SpawnableConstEntityContainerView
    {
    public:
        SpawnableConstEntityContainerView(AZ::Entity** begin, size_t length);
        SpawnableConstEntityContainerView(AZ::Entity** begin, AZ::Entity** end);

        [[nodiscard]] const AZ::Entity* const* begin();
        [[nodiscard]] const AZ::Entity* const* end();
        [[nodiscard]] const AZ::Entity* const* begin() const;
        [[nodiscard]] const AZ::Entity* const* end() const;
        [[nodiscard]] const AZ::Entity* const* cbegin();
        [[nodiscard]] const AZ::Entity* const* cend();

        [[nodiscard]] const AZ::Entity* operator[](size_t n);
        [[nodiscard]] const AZ::Entity* operator[](size_t n) const;

        [[nodiscard]] size_t size() const;
        [[nodiscard]] bool empty() const;

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

        using Id = uint32_t;

        EntitySpawnTicket() = default;
        EntitySpawnTicket(const EntitySpawnTicket&) = delete;
        EntitySpawnTicket(EntitySpawnTicket&& rhs);
        explicit EntitySpawnTicket(AZ::Data::Asset<Spawnable> spawnable);
        ~EntitySpawnTicket();

        EntitySpawnTicket& operator=(const EntitySpawnTicket&) = delete;
        EntitySpawnTicket& operator=(EntitySpawnTicket&& rhs);

        Id GetId() const;
        bool IsValid() const;

    private:
        void* m_payload{ nullptr };
        Id m_id { 0 }; //!< An id that uniquely identifies a ticket.
    };

    using EntitySpawnCallback = AZStd::function<void(EntitySpawnTicket::Id, SpawnableConstEntityContainerView)>;
    using EntityPreInsertionCallback = AZStd::function<void(EntitySpawnTicket::Id, SpawnableEntityContainerView)>;
    using EntityDespawnCallback = AZStd::function<void(EntitySpawnTicket::Id)>;
    using RetrieveEntitySpawnTicketCallback = AZStd::function<void(EntitySpawnTicket*)>;
    using ReloadSpawnableCallback = AZStd::function<void(EntitySpawnTicket::Id, SpawnableConstEntityContainerView)>;
    using ListEntitiesCallback = AZStd::function<void(EntitySpawnTicket::Id, SpawnableConstEntityContainerView)>;
    using ListIndicesEntitiesCallback = AZStd::function<void(EntitySpawnTicket::Id, SpawnableConstIndexEntityContainerView)>;
    using ClaimEntitiesCallback = AZStd::function<void(EntitySpawnTicket::Id, SpawnableEntityContainerView)>;
    using BarrierCallback = AZStd::function<void(EntitySpawnTicket::Id)>;

    struct SpawnAllEntitiesOptionalArgs final
    {
        //! Callback that's called after instances of entities have been created, but before they're spawned into the world. This
        //!     gives the opportunity to modify the entities if needed such as injecting additional components or modifying components.
        EntityPreInsertionCallback m_preInsertionCallback;
        //! Callback that's called when spawning entities has completed. This can be triggered from a different thread than the one that
        //!     made the function call to spawn. The returned list of entities contains all the newly created entities.
        EntitySpawnCallback m_completionCallback;
        //! The Serialize Context used to clone entities with. If this is not provided the global Serialize Contetx will be used.
        AZ::SerializeContext* m_serializeContext { nullptr };
        //! The priority at which this call will be executed.
        SpawnablePriority m_priority { SpawnablePriority_Default };
    };

    struct SpawnEntitiesOptionalArgs final
    {
        //! Callback that's called after instances of entities have been created, but before they're spawned into the world. This
        //!     gives the opportunity to modify the entities if needed such as injecting additional components or modifying components.
        EntityPreInsertionCallback m_preInsertionCallback;
        //! Callback that's called when spawning entities has completed. This can be triggered from a different thread than the one that
        //!     made the function call to spawn. The returned list of entities contains all the newly created entities.
        EntitySpawnCallback m_completionCallback;
        //! The Serialize Context used to clone entities with. If this is not provided the global Serialize Contetx will be used.
        AZ::SerializeContext* m_serializeContext{ nullptr };
        //! The priority at which this call will be executed.
        SpawnablePriority m_priority{ SpawnablePriority_Default };
        //! Entity references are resolved by referring to the most recent entity spawned from a template entity in the spawnable.
        //! If the entity referred to hasn't been spawned yet, the reference will be resolved to the first one that *will* be spawned.
        //! If this flag is set to "true", the id mappings will persist across SpawnEntites calls, and the entity references will resolve
        //! correctly across them.  
        //! When "false", the entity id mappings will be reset on this call, so entity references will only work within this call, or
        //! potentially with any subsequent SpawnEntities call where the flag is true once again.
        bool m_referencePreviouslySpawnedEntities{ true };
    };

    struct DespawnAllEntitiesOptionalArgs final
    {
        //! Callback that's called when despawning entities has completed. This can be triggered from a different thread than the one that
        //! made the function call to despawn.
        EntityDespawnCallback m_completionCallback;
        //! The priority at which this call will be executed.
        SpawnablePriority m_priority { SpawnablePriority_Default };
    };

    struct DespawnEntityOptionalArgs final
    {
        //! Callback that's called when despawning entity has completed. This can be triggered from a different thread than the one that
        //! made the function call to despawn.
        EntityDespawnCallback m_completionCallback;
        //! The priority at which this call will be executed.
        SpawnablePriority m_priority{ SpawnablePriority_Default };
    };

    struct ReloadSpawnableOptionalArgs final
    {
        //! Callback that's called when respawning entities has completed. This can be triggered from a different thread than the one that
        //!     made the function call to respawn. The returned list of entities contains all the newly created entities.
        ReloadSpawnableCallback m_completionCallback;
        //! The Serialize Context used to clone entities with. If this is not provided the global Serialize Context will be used.
        AZ::SerializeContext* m_serializeContext { nullptr };
        //! The priority at which this call will be executed.
        SpawnablePriority m_priority { SpawnablePriority_Default };
    };

    struct ListEntitiesOptionalArgs final
    {
        //! The priority at which this call will be executed.
        SpawnablePriority m_priority{ SpawnablePriority_Default };
    };

    struct ClaimEntitiesOptionalArgs final
    {
        //! The priority at which this call will be executed.
        SpawnablePriority m_priority{ SpawnablePriority_Default };
    };

    struct BarrierOptionalArgs final
    {
        //! The priority at which this call will be executed.
        SpawnablePriority m_priority{ SpawnablePriority_Default };
    };

    //! Interface definition to (de)spawn entities from a spawnable into the game world.
    //! 
    //! While the callbacks of the individual calls are being processed they will block processing any other request. Callbacks can be
    //! issued from threads other than the one that issued the call, including the main thread.
    //!
    //! Calls on the same ticket are guaranteed to be executed in the order they are issued. Note that when issuing requests from
    //! multiple threads on the same ticket the order in which the requests are assigned to the ticket is not guaranteed.
    //!
    //! Most calls have a priority with values that range from 0 (highest priority) to 255 (lowest priority). The implementation of this
    //! interface may choose to use priority lanes which doesn't guarantee that higher priority requests happen before lower priority
    //! requests if they don't pass the priority lane threshold. Priority lanes and their thresholds are implementation specific and may
    //! differ between platforms. Note that if a call happened on a ticket with lower priority followed by a one with a higher priority
    //! the first lower priority call will still need to complete before the second higher priority call can be executed and the priority
    //! of the first call will not be updated.
    class SpawnableEntitiesDefinition
    {
    public:
        AZ_RTTI(AzFramework::SpawnableEntitiesDefinition, "{A9ED3F1F-4D69-4182-B0CD-EB561EEA7068}");

        friend class EntitySpawnTicket;

        virtual ~SpawnableEntitiesDefinition() = default;

        //! Spawn instances of all entities in the spawnable.
        //! @param ticket Stores the results of the call. Use this ticket to spawn additional entities or to despawn them.
        //! @param optionalArgs Optional additional arguments, see SpawnAllEntitiesOptionalArgs.
        virtual void SpawnAllEntities(EntitySpawnTicket& ticket, SpawnAllEntitiesOptionalArgs optionalArgs = {}) = 0;
        //! Spawn instances of some entities in the spawnable.
        //! @param ticket Stores the results of the call. Use this ticket to spawn additional entities or to despawn them.
        //! @param priority The priority at which this call will be executed.
        //! @param entityIndices The indices into the template entities stored in the spawnable that will be used to spawn entities from.
        //! @param optionalArgs Optional additional arguments, see SpawnEntitiesOptionalArgs.
        virtual void SpawnEntities(
            EntitySpawnTicket& ticket, AZStd::vector<size_t> entityIndices, SpawnEntitiesOptionalArgs optionalArgs = {}) = 0;
        //! Removes all entities in the provided list from the environment.
        //! @param ticket The ticket previously used to spawn entities with.
        //! @param optionalArgs Optional additional arguments, see DespawnAllEntitiesOptionalArgs.
        virtual void DespawnAllEntities(EntitySpawnTicket& ticket, DespawnAllEntitiesOptionalArgs optionalArgs = {}) = 0;
        //! Removes the entity with the provided id from the spawned list of entities.
        //! @param entityId the id of entity to despawn.
        //! @param ticket The ticket previously used to spawn entities with.
        //! @param optionalArgs Optional additional arguments, see DespawnEntityOptionalArgs.
        virtual void DespawnEntity(AZ::EntityId entityId, EntitySpawnTicket& ticket, DespawnEntityOptionalArgs optionalArgs = {}) = 0;
        //! Gets the EntitySpawnTicket associated with the entitySpawnTicketId.
        //! @param entitySpawnTicketId the id of EntitySpawnTicket to get.
        //! @param callback The callback to execute upon retrieving the ticket.
        virtual void RetrieveEntitySpawnTicket(EntitySpawnTicket::Id entitySpawnTicketId, RetrieveEntitySpawnTicketCallback callback) = 0;
        //! Removes all entities in the provided list from the environment and reconstructs the entities from the provided spawnable.
        //! @param ticket Holds the information on the entities to reload.
        //! @param priority The priority at which this call will be executed.
        //! @param spawnable The spawnable that will replace the existing spawnable. Both need to have the same asset id.
        //! @param optionalArgs Optional additional arguments, see ReloadSpawnableOptionalArgs.
        virtual void ReloadSpawnable(
            EntitySpawnTicket& ticket, AZ::Data::Asset<Spawnable> spawnable, ReloadSpawnableOptionalArgs optionalArgs = {}) = 0;

        //! List all entities that are spawned using this ticket.
        //! @param ticket Only the entities associated with this ticket will be listed.
        //! @param listCallback Required callback that will be called to list the entities on.
        //! @param optionalArgs Optional additional arguments, see ListEntitiesOptionalArgs.
        virtual void ListEntities(
            EntitySpawnTicket& ticket, ListEntitiesCallback listCallback, ListEntitiesOptionalArgs optionalArgs = {}) = 0;
        //! List all entities that are spawned using this ticket with their spawnable index.
        //!     Spawnables contain a flat list of entities, which are used as templates to spawn entities from. For every spawned entity
        //!     the index of the entity in the spawnable that was used as a template is stored. This version of ListEntities will return
        //!     both the entities and this index. The index can be used with SpawnEntities to create the same entities again. Note that
        //!     the same index may appear multiple times as there are no restriction on how many instance of a specific entity can be
        //!     created.
        //! @param ticket Only the entities associated with this ticket will be listed.
        //! @param listCallback Required callback that will be called to list the entities and indices on.
        //! @param optionalArgs Optional additional arguments, see ListEntitiesOptionalArgs.
        virtual void ListIndicesAndEntities(
            EntitySpawnTicket& ticket, ListIndicesEntitiesCallback listCallback, ListEntitiesOptionalArgs optionalArgs = {}) = 0;
        //! Claim all entities that are spawned using this ticket. Ownership of the entities is transferred from the ticket to the
        //!     caller through the callback. After this call the ticket will have no entities associated with it. The caller of
        //!     this function will need to manage the entities after this call.
        //! @param ticket Only the entities associated with this ticket will be released.
        //! @param listCallback Required callback that will be called to transfer the entities through.
        //! @param optionalArgs Optional additional arguments, see ClaimEntitiesOptionalArgs.
        virtual void ClaimEntities(
            EntitySpawnTicket& ticket, ClaimEntitiesCallback listCallback, ClaimEntitiesOptionalArgs optionalArgs = {}) = 0;

        //! Blocks until all operations made on the provided ticket before the barrier call have completed.
        //! @param ticket The ticket to monitor.
        //! @param completionCallback Required callback that will be called as soon as the barrier has been reached.
        //! @param optionalArgs Optional additional arguments, see BarrierOptionalArgs.
        virtual void Barrier(EntitySpawnTicket& ticket, BarrierCallback completionCallback, BarrierOptionalArgs optionalArgs = {}) = 0;

    protected:
        [[nodiscard]] virtual AZStd::pair<EntitySpawnTicket::Id, void*> CreateTicket(AZ::Data::Asset<Spawnable>&& spawnable) = 0;
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

        AZStd::unordered_map<EntitySpawnTicket::Id, EntitySpawnTicket*> m_entitySpawnTicketMap;
        AZStd::recursive_mutex m_entitySpawnTicketMapMutex;
    };

    using SpawnableEntitiesInterface = AZ::Interface<SpawnableEntitiesDefinition>;
} // namespace AzFramework
