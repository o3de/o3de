/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/** 
 * @file
 * Header file for buses that dispatch and receive events 
 * from the game entity context. 
 * The game entity context holds gameplay entities, as opposed 
 * to system entities, editor entities, and so on.
 */

#ifndef AZFRAMEWORK_GAMEENTITYCONTEXTBUS_H
#define AZFRAMEWORK_GAMEENTITYCONTEXTBUS_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Entity/BehaviorEntity.h>

namespace AZ
{
    class Entity;
}

namespace AzFramework
{
    /**
     * Interface for AzFramework::GameEntityContextRequestBus, which is  
     * the EBus that makes requests to the game entity context. 
     * The game entity context holds gameplay entities, as opposed
     * to system entities, editor entities, and so on.
     */
    class GameEntityContextRequests
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~GameEntityContextRequests() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        /**
         * Overrides the default AZ::EBusTraits handler policy so that this 
         * EBus supports a single handler at each address. This EBus has only  
         * one handler because it uses the default AZ::EBusTraits address 
         * policy, and that policy specifies that the EBus has only one address.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /**
         * Gets the ID of the game entity context.
         * @return The ID of the game entity context.
         */
        virtual EntityContextId GetGameEntityContextId() = 0;

        virtual EntityContext* GetGameEntityContextInstance() = 0;

        /**
         * Creates an entity in the game context.
         * @param name A name for the new entity.
         * @return A pointer to a new entity.
         */
        virtual AZ::Entity* CreateGameEntity(const char* /*name*/) = 0;

        /**
         * Creates an entity in the game context.
         * @param name A name for the new entity.
         * @return An entity wrapper for use within the BehaviorContext.
         */
        virtual BehaviorEntity CreateGameEntityForBehaviorContext(const char* /*name*/) = 0;

        /**
         * Adds an existing entity to the game context.
         * @param entity A pointer to the entity to add to the game context.
         */
        virtual void AddGameEntity(AZ::Entity* /*entity*/) = 0;

        /**
         * Destroys an entity. 
         * The entity is immediately deactivated and will be destroyed on the next tick.
         * @param id The ID of the entity to destroy.
         */
        virtual void DestroyGameEntity(const AZ::EntityId& /*id*/) = 0;

        /**
         * Destroys an entity and all of its descendants. 
         * The entity and its descendants are immediately deactivated and will be 
         * destroyed on the next tick.
         * @param id The ID of the entity to destroy.
         */
        virtual void DestroyGameEntityAndDescendants(const AZ::EntityId& /*id*/) = 0;

        /**
         * Activates the game entity.
         * @param id The ID of the entity to activate.
         */
        virtual void ActivateGameEntity(const AZ::EntityId& /*id*/) = 0;
        
        /**
         * Deactivates the game entity.
         * @param id The ID of the entity to deactivate.
         */
        virtual void DeactivateGameEntity(const AZ::EntityId& /*id*/) = 0;

        /**
         * Loads game entities from a stream.
         * @param stream The stream to load the entities from.
         * @param remapIds Use true to remap the entity IDs after the stream is loaded.
         * @return True if the stream successfully loaded. Otherwise, false. This operation  
         * can fail if the source file is corrupt or the data could not be up-converted.
         */
        virtual bool LoadFromStream(AZ::IO::GenericStream& /*stream*/, bool /*remapIds*/) = 0;

        /**
         * Completely resets the game context. 
         * This includes deleting all prefabs and entities.
         */
        virtual void ResetGameContext() = 0;

        /**
         * Returns the entity's name.
         * @param id The ID of the entity.
         * @return The name of the entity. Returns an empty string if the entity 
         * cannot be found.
         */
        virtual AZStd::string GetEntityName(const AZ::EntityId&) = 0;
    };

    /**
     * The EBus for requests to the game entity context.
     * The events are defined in the AzFramework::GameEntityContextRequests class.
     */
    using GameEntityContextRequestBus = AZ::EBus<GameEntityContextRequests>;

     /**
      * Interface for the AzFramework::GameEntityContextEventBus, which is the EBus 
      * that dispatches notification events from the game entity context. 
      * The game entity context holds gameplay entities, as opposed
      * to system entities, editor entities, and so on.
      */
    class GameEntityContextEvents
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~GameEntityContextEvents() = default;

        /**
         * Signals that the game entity context is about to be loaded and activated, which happens at the
         * start of a level. If the concept of levels is eradicated, this event will be removed.
         */
        virtual void OnPreGameEntitiesStarted() {}

        /**
         * Signals that the game entity context is loaded and activated, which happens at the  
         * start of a level. If the concept of levels is eradicated, this event will be removed.
         */
        virtual void OnGameEntitiesStarted() {}

        /**
         * Signals that the game entity context is shut down or reset. 
         * This is equivalent to the end of a level. 
         * This event will be valid even if the concept of levels is eradicated. 
         * In that case, its meaning will vary depending on how and if the game 
         * uses the game entity context.
         */
        virtual void OnGameEntitiesReset() {}
    };

    /**
     * The EBus for game entity context events.
     * The events are defined in the AzFramework::GameEntityContextEvents class.
     */
    using GameEntityContextEventBus = AZ::EBus<GameEntityContextEvents>;

} // namespace AzFramework

#endif // AZFRAMEWORK_GAMEENTITYCONTEXTBUS_H
