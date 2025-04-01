/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/** @file
 * Header file for buses that dispatch notification events concerning the AZ::Entity class. 
 * Buses enable entities and components to communicate with each other and with external 
 * systems.
 */

#ifndef AZCORE_ENTITY_BUS_H
#define AZCORE_ENTITY_BUS_H

#include <AzCore/std/string/string.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

namespace AZ
{
    /**
     * Interface for the AZ::EntitySystemBus, which is the EBus that dispatches   
     * notification events about every entity in the system.
     */
    class EntitySystemEvents
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the instance of the class.
         */
        virtual ~EntitySystemEvents() {}

        /**
         * Global entity initialization notification.
         * @param id The ID of the initialized entity.
         */
        virtual void OnEntityInitialized(const AZ::EntityId&) {}

        /**
         * Signals that an initialized entity is about to be deleted.
         */
        virtual void OnEntityDestruction(const AZ::EntityId&) {}

        /**
         * Signals that an initialized entity has been deleted.
         */
        virtual void OnEntityDestroyed(const AZ::EntityId&) {}

        /**
         * Signals that an entity was activated.
         * This event is dispatched after the activation of the entity is complete. 
         * @param id The ID of the activated entity.
         */
        virtual void OnEntityActivated(const AZ::EntityId&) {}
        
        /**
         * Signals that an entity is being deactivated.
         * This event is dispatched immediately before the entity is deactivated.
         * @param id The ID of the deactivated entity.
         */
        virtual void OnEntityDeactivated(const AZ::EntityId&) {}
        
        /**
         * Signals that the name of an entity changed.
         * @param id The ID of the entity. 
         * @param name The new name of the entity.
         */
        virtual void OnEntityNameChanged(const AZ::EntityId&, const AZStd::string& /*name*/) {}

        /**
         * Signals that the start status of an entity changed.
         * @param EntityId The ID of the entity that has had the status changed.
         */
        virtual void OnEntityStartStatusChanged(const AZ::EntityId&) {}

    };


    /**
     * The EBus for systemwide entity notification events. 
     * The events are defined in the AZ::EntitySystemEvents class.
     */
    typedef AZ::EBus<EntitySystemEvents> EntitySystemBus;

    /**
     * Interface for the AZ::EntityBus, which is the EBus for notification 
     * events dispatched by a specific entity.
     */
    class EntityEvents
        : public ComponentBus
    {
    private:
        template<class Bus>
        struct EntityEventsConnectionPolicy
            : public EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
            {
                EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
                if (entity)
                {
                    const AZ::Entity::State entityState = entity->GetState();
                    if (entityState >= Entity::State::Init)
                    {
                        handler->OnEntityExists(id);
                    }
                    
                    if (entityState == Entity::State::Active)
                    {
                        handler->OnEntityActivated(id);
                    }
                }
            }
        };
        
    public:

        /**
         * With this connection policy, AZ::EntityEvents::OnEntityExists and
         * AZ::EntityEvents::OnEntityActivated events may be immediately
         * dispatched when a handler connects to the bus.
         */
        template<class Bus>
        using ConnectionPolicy = EntityEventsConnectionPolicy<Bus>;

        /**
         * Destroys the instance of the class.
         */
        virtual ~EntityEvents() {}

        /**
         * Signals that an entity has come into existence.
         * This event is dispatched after initialization of the entity.
         * It is also dispatched to handlers immediately upon connecting
         * to the bus if the entity has already been initialized.
         * Note that in this case the entity may or may not be activated.
         * @param id The ID of the entity.
         */
        virtual void OnEntityExists(const AZ::EntityId&) {}

        /**
         * Signals that an initialized entity is about to be deleted.
         */
        virtual void OnEntityDestruction(const AZ::EntityId&) {}

        /**
         * Signals that an initialized entity has been deleted.
         */
        virtual void OnEntityDestroyed(const AZ::EntityId&) {}

        /**
         * Signals that an entity was activated. 
         * This event is dispatched after the activation of the entity is complete.  
         * It is also dispatched immediately if the entity is already active 
         * when a handler connects to the bus.
         * @param EntityId The ID of the entity that was activated.
         */
        virtual void OnEntityActivated(const AZ::EntityId&)              {}

        /**
         * Signals that an entity is being deactivated.
         * This event is dispatched immediately before the entity is deactivated.
         * @param EntityId The ID of the entity that is being deactivated.
         */
        virtual void OnEntityDeactivated(const AZ::EntityId&)             {}

        /**
         * Signals that the name of an entity changed.
         * @param name The new name of the entity.
         */
        virtual void OnEntityNameChanged(const AZStd::string& name) { (void)name; }
    };

    /**
     * The EBus for notification events dispatched by a specific entity.
     * The events are defined in the AZ::EntityEvents class.
     */
    typedef AZ::EBus<EntityEvents>  EntityBus;
}

#endif // AZCORE_ENTITY_BUS_H
#pragma once

DECLARE_EBUS_EXTERN_DLL_SINGLE_ADDRESS(EntityEvents);
