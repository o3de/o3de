/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
 * @file
 * Header file for buses that dispatch and receive events from an entity context. 
 * Entity contexts are collections of entities. Examples of entity contexts are 
 * the editor context, game context, a custom context, and so on.
 */

#ifndef AZFRAMEWORK_ENTITYCONTEXTBUS_H
#define AZFRAMEWORK_ENTITYCONTEXTBUS_H

#include <AzCore/Debug/Budget.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentBus.h>

AZ_DECLARE_BUDGET(AzFramework);

namespace AZ
{
    class Entity;
    class EntityId;
}

namespace AzFramework
{
    class EntityContext;

    /**
     * Unique ID for an entity context. 
     */
    using EntityContextId = AZ::Uuid;

    using EntityList = AZStd::vector<AZ::Entity*>;

    /**
     * Interface for AzFramework::EntityContextRequestBus, which is
     * the EBus that makes requests to a given entity context. 
     * If you want to make requests to a specific entity context, such  
     * as the game entity context, use the interface specific to that  
     * context. If you want to make requests to multiple types of entity   
     * contexts, use this interface.
     */
    class EntityContextRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        /**
         * Overrides the default AZ::EBusAddressPolicy so that the EBus has 
         * multiple addresses. Events that are addressed to an ID are received
         * by all handlers that are connected to that ID.
         */
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        /**
         * Specifies that events are addressed by entity context ID.
         */
        typedef EntityContextId BusIdType;
        //////////////////////////////////////////////////////////////////////////
        

        /**
         * Creates an entity and adds it to the entity context.
         * This operation does not activate the entity by default.
         * @param name A name for the entity.
         * @return A pointer to a new entity. 
         * This operation succeeds unless the system is completely out of memory.
         */
        virtual AZ::Entity* CreateEntity(const char* name) = 0;

        /**
         * Adds an entity to the entity context. 
         * This operation does not activate the entity by default.
         * Derived classes might choose to set the entity to another state.
         * @param entity A pointer to the entity to add.
         */
        virtual void AddEntity(AZ::Entity* entity) = 0;

        /**
         * Activates an entity that is owned by the entity context.
         * @param id The ID of the entity to activate.
         */
        virtual void ActivateEntity(AZ::EntityId entityId) = 0;

        /**
         * Deactivates an entity that is owned by the entity context.
         * @param id The ID of the entity to deactivate.
         */
        virtual void DeactivateEntity(AZ::EntityId entityId) = 0;

        /**
         * Removes an entity from the entity context and destroys the entity.
         * @param entity A pointer to the entity to destroy.
         * @return If the entity context does not own the entity,  
         * this returns false and does not destroy the entity.  
         */
        virtual bool DestroyEntity(AZ::Entity* entity) = 0;

        /**
         * Removes an entity from the entity context and destroys the entity.
         * @param entityId The ID of the entity to destroy.
         * @return If the entity context does not own the entity,
         * this returns false and does not destroy the entity.
         */
        virtual bool DestroyEntityById(AZ::EntityId entityId) = 0;

        /**
         * Creates a copy of the entity in the entity context.
         * The cloned copy is assigned a unique entity ID.
         * @param sourceEntity A reference to the entity to clone.
         * @return A pointer to the cloned copy of the entity. This operation 
         * can fail if serialization data fails to interpret the source entity.
         */
        virtual AZ::Entity* CloneEntity(const AZ::Entity& sourceEntity) = 0;

        /**
         * Clears the entity context by destroying all entities and prefab instances 
         * that the entity context owns.
         */
        virtual void ResetContext() = 0;
    };

    /**
     * The EBus for requests to the entity context.
     * The events are defined in the AzFramework::EntityContextRequests class.
     * If you want to make requests to a specific entity context, such
     * as the game entity context, use the bus specific to that context.
     * If you want to make requests to multiple types of entity contexts,
     * use this bus.
     */
    using EntityContextRequestBus = AZ::EBus<EntityContextRequests>;

    /**
     * Interface for the AzFramework::EntityContextEventBus, which is the EBus
     * that dispatches notification events from the global entity context.
     * If you want to receive notification events from a specific entity context, 
     * such as the game entity context, use the interface specific to that context.
     * If you want to receive notification events from multiple types of entity 
     * contexts, use this interface.
     */
    class EntityContextEvents
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~EntityContextEvents() {}

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        /**
         * Overrides the default AZ::EBusAddressPolicy to specify that the EBus
         * has multiple addresses. Events that are addressed to an ID are received
         * by all handlers connected to that ID.
         */
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        /**
         * Specifies that events are addressed by entity context ID.
         */
        typedef EntityContextId BusIdType;
        //////////////////////////////////////////////////////////////////////////

        /**
         * Signals that an entity context was loaded from a stream.
         * @param contextEntities A reference to a list of entities that 
         * are owned by the entity context that was loaded.
         */
        virtual void OnEntityContextLoadedFromStream(const EntityList& /*contextEntities*/) {}

        /**
         * Signals that the entity context was reset.
         */
        virtual void OnEntityContextReset() {}

        /**
         * Signals that the entity context created an entity.
         * @param entity A reference to the entity that was created.
         */
        virtual void OnEntityContextCreateEntity(AZ::Entity& /*entity*/) {}

        /**
         * Signals that the entity context is about to destroy an entity.
         * @param id A reference to the ID of the entity that will be destroyed.
         */
        virtual void OnEntityContextDestroyEntity(const AZ::EntityId& /*id*/) {}

    };

    /**
     * The EBus for entity context events.
     * The events are defined in the AzFramework::EntityContextEvents class.
     * If you want to receive event notifications from a specific entity context, 
     * such as the game entity context, use the bus specific to that context.
     * If you want to receive event notifications from multiple types of entity 
     * contexts, use this bus.
     */
    using EntityContextEventBus = AZ::EBus<EntityContextEvents>;

    /**
     * Interface for AzFramework::EntityIdContextQueryBus, which is
     * the EBus that queries an entity about its context.
     */
    class EntityIdContextQueries
        : public AZ::ComponentBus
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~EntityIdContextQueries() {}

        /**
         * Gets the ID of the entity context that the entity belongs to.
         * @return The ID of the entity context that the entity belongs to.
         */
        virtual EntityContextId GetOwningContextId() = 0;
    };

    /**
     * The EBus for querying an entity about its context.
     * The events are defined in the AzFramework::EntityIdContextQueries class.
     */
    using EntityIdContextQueryBus = AZ::EBus<EntityIdContextQueries>;
} // namespace AzFramework

#endif // AZFRAMEWORK_ENTITYCONTEXTBUS_H
