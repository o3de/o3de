/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/** @file
 * Header file for the component bus class, which most AZ::Components use as the base 
 * class for their buses. Buses enable components to communicate with each other and 
 * with external systems.
 */

#ifndef AZCORE_COMPONENT_BUS_H
#define AZCORE_COMPONENT_BUS_H

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

namespace AZ
{
    class ReflectContext;

    typedef AZ::u64     ComponentId;

    static const ComponentId InvalidComponentId = 0;

    /**
     * Base class for message buses.
     * Most components that derive from AZ::Component use this class to implement 
     * their buses, and then override the default AZ::EBusTraits to suit their needs. 
     */
    class ComponentBus
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys a component bus.
         */
        virtual ~ComponentBus() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        /**
         * Overrides the default AZ::EBusTraits address policy so that the bus
         * has multiple addresses at which to receive messages. This bus is 
         * identified by EntityId. Messages addressed to an ID are received by 
         * handlers connected to that ID.
         */
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        /**
         * Overrides the default AZ::EBusTraits ID type so that entity IDs are 
         * used to access the addresses of the bus.
         */
        typedef EntityId BusIdType;
        //////////////////////////////////////////////////////////////////////////
    };

    /**
     * Base class for component configurations.
     * Components that accept a ComponentConfig can be configured in-game using code.
     * To author a component that is runtime configurable:
     * 1) Create a class which inherits from ComponentConfig.
     *    a) Write this class in a publicly available header file (ex: within an include/ folder).
     *    b) Add AZ_RTTI to this class.
     *    c) Put all properties that your component needs to be configured into this class.
     *    d) You might find it helpful to simply store an instance of the configuration class
     *       within your component, rather than having duplicate properties in each class.
     * 2) Implement the ReadInConfig() function for your component.
     *    Set properties in your component, based on the properties in the configuration class.
     * 3) Implement the WriteOutConfig() function for your component.
     *    Set properties in the config class, based on the properties in the component.
     * 4) Reflect your configuration class to the appropriate contexts.
     *    BehaviorContext allows the configuration to be used from scripts.
     *    If your component stores an instance of the configuration within itself,
     *    reflect to the SerializeContext and EditContext to make the properties
     *    accessible in the Editor UI.
     * @note Components are not required to support a configuration class.
     * The EditContext can expose a component's properties to the editor's UI
     * regardless of whether the component has a configuration class.
     */
    class ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ComponentConfig, SystemAllocator);
        AZ_RTTI(ComponentConfig, "{0A7929DF-2932-40EA-B2B3-79BC1C3490D0}");

        virtual ~ComponentConfig() = default;
    };

    /**
     * A pair of entity and component IDs that are used to access an address 
     * of an AZ::EntityComponentBus.
     */
    class EntityComponentIdPair 
    {
    public:

        /**
         * Specifies that this class should use AZ::SystemAllocator for memory
         * management by default.
         */
        AZ_CLASS_ALLOCATOR(EntityComponentIdPair, AZ::SystemAllocator);

        /**
         * Adds run-time type information to this class.
         */
        AZ_RTTI(EntityComponentIdPair, "{C845E5EC-5580-4E12-A9B2-9AE7E5B7826F}");

        /**
         * Creates an empty entity-component ID pair.
         * Entity-component ID pairs are used to access addresses of an
         * AZ::EntityComponentBus.
         */
        EntityComponentIdPair() {}

        /**
         * Creates an empty entity-component ID pair with the specified entity and component ID.
         * Entity-component ID pairs are used to access addresses of an AZ::EntityComponentBus.
         * @param entityId ID of an entity.
         * @param componentId ID of a component.
         */
        EntityComponentIdPair(const AZ::EntityId& entityId, const AZ::ComponentId& componentId) 
            : m_entityId(entityId)
            , m_componentId(componentId) {}

        /**
         * Destroys the entity-ID pair.
         */
        virtual ~EntityComponentIdPair() = default;

        /**
         * Gets the ID of the entity so that it can be hashed and
         * combined with the ID of the component to find which address  
         * to use on the message bus.
         * @return The ID of the specified component.
        */
        AZ::EntityId GetEntityId() const { return m_entityId; }

        /**
         * Gets the ID of the component so that it can be hashed and
         * combined with the ID of the entity to find which address 
         * to use on the message bus.
         * @return The ID of the specified component.
         */
        AZ::ComponentId GetComponentId() const { return m_componentId; }

        /**
         * Overloads the == operator so that entity-component ID pairs can
         * be checked for equality.
         * @param other An entity-component ID pair whose equality you want to check against.
         * @result Returns true if the entity-component ID pairs are equal.
         */
        bool operator==(const EntityComponentIdPair& other) const
        {
            return m_entityId == other.m_entityId && m_componentId == other.m_componentId;
        }

        /**
         * Overloads the != operator so that entity-component ID pairs can
         * be checked for difference.
         * @param other An entity-component ID pair whose equality you want to check against.
         * @result Returns true if the entity-component ID pairs are not equal.
         */
        bool operator!=(const EntityComponentIdPair& other) const
        {
            return m_entityId != other.m_entityId || m_componentId != other.m_componentId;
        }

        /**
         * Reflects this class into a variety of contexts (script, serialize, edit, and so on).
         * @param reflection A pointer to the reflection context.
         */
        static void Reflect(AZ::ReflectContext* context);

    private:
        AZ::EntityId    m_entityId;
        AZ::ComponentId m_componentId;
    };

    /// @cond EXCLUDE_DOCS
    // Base class for message buses that enable an entity to communicate
    // with a specific instance of a component. This is similar to the
    // AZ::ComponentBus base class. The difference is that this class requires
    // messages to be addressed to a specific instance of a component
    // rather than receiving messages for all components of the same type.

    class EntityComponentBus
        : public AZ::EBusTraits
    {
    public:

        /**
         * Destroys the bus that entities use to communicate with a component.
         */
        virtual ~EntityComponentBus() = default;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        /**
         * Overrides the default AZ::EBusTraits address policy so that the bus
         * has multiple addresses at which to receive messages. This bus is 
         * identified by EntityId. Messages addressed to an ID are received by 
         * handlers connected to that ID.
         */
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        /**
         * Overrides the default AZ::EBusTraits ID type so that entity IDs are 
         * used to access the addresses of the bus.
         */
        typedef EntityComponentIdPair BusIdType;
        //////////////////////////////////////////////////////////////////////////
    };
    /// @endcond

}

namespace AZStd
{
    /**
     * Implements the hash for the entity-component ID pair, because buses are identified
     * by a hash of the ID.
     */
    template <>
    struct hash < AZ::EntityComponentIdPair >
    {
        inline size_t operator()(const AZ::EntityComponentIdPair& entityComponentIdPair) const
        {
            AZStd::hash<AZ::EntityId> entityIdHasher;
            size_t retVal = entityIdHasher(entityComponentIdPair.GetEntityId());
            AZStd::hash_combine(retVal, entityComponentIdPair.GetComponentId());
            return retVal;
        }
    };
}
#endif // AZCORE_COMPONENT_BUS_H
#pragma once
