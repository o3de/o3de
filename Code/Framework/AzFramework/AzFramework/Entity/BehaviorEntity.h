/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Entity/EntityContext.h>

namespace AzFramework
{

    /**
     * A wrapper around AZ::ComponentId, for use within the BehaviorContext.
     * This wrapper is necessary because AZ::ComponentId is just a 64bit int and
     * Lua cannot store the exact value of a 64bit int.
     *
     * BehaviorComponentId should only be used in coordination with the
     * BehaviorEntity class to access components on deactivated entities.
     * Other systems, which communicate with activated entities,
     * should use the appropriate EBus to communicate with components.
     */
    class BehaviorComponentId
    {
    public:
        AZ_TYPE_INFO(BehaviorComponentId, "{60A9A069-9C3D-465A-B7AD-0D6CC803990A}");
        AZ_CLASS_ALLOCATOR(BehaviorComponentId, AZ::ComponentAllocator);
        static void Reflect(AZ::ReflectContext* context);

        BehaviorComponentId() = default;
        BehaviorComponentId(AZ::ComponentId id);
        operator AZ::ComponentId() const;
        bool operator==(const BehaviorComponentId& rhs) const;
        bool IsValid() const;
        AZStd::string ToString() const;

    private:
        AZ::ComponentId m_id = AZ::InvalidComponentId;
    };

    /**
     * A wrapper around calls to AZ::Entity, for use within the BehaviorContext.
     * It is always safe to call functions on this class
     * even if the entity it represents has been deleted from memory.
     */
    class BehaviorEntity
    {
    public:
        AZ_RTTI(BehaviorEntity, "{41CC88A4-FE07-48E6-943D-998DE68AFF5C}");
        AZ_CLASS_ALLOCATOR(BehaviorEntity, AZ::EntityAllocator);
        static void Reflect(AZ::ReflectContext* context);

        virtual ~BehaviorEntity() = default;

        /**
         * Constructs an invalid BehaviorEntity.
         * Any methods called on this instance will have no effect.
         */
        BehaviorEntity() = default;

        /**
         * Constructs a BehaviorEntity with the given entity ID.
         * @param entityId The ID of the entity.
         */
        explicit BehaviorEntity(AZ::EntityId entityId);

        /**
         * Constructs a BehaviorEntity with the ID of the provided entity.
         * @param entity Entity that this BehaviorEntity will represent.
         * If nullptr is provided then an invalid BehaviorEntity is constructed.
         */
        explicit BehaviorEntity(AZ::Entity* entity);

        /**
         * @copydoc AZ::Entity::GetName()
         */
        AZStd::string GetName() const;

        /**
         * @copydoc AZ::Entity::SetName()
         */
        void SetName(const char* name);

        /**
         * @copydoc AZ::Entity::GetId()
         */
        AZ::EntityId GetId() const { return m_entityId; }

        /**
         * @copydoc EntityIdContextQueries::GetOwningContextId()
         */
        EntityContextId GetOwningContextId() const;

        /**
         * Check whether this instance has a valid entity ID.
         * Note that a valid entity ID does not indicate whether
         * the entity it represents currently exists in memory.
         * @return Returns true if the entity ID is valid. Otherwise, false.
         */
        bool IsValid() const { return m_entityId.IsValid(); };

        /**
         * Check whether the entity exists in memory.
         * Note that an entity which exists may or may not be activated.
         * @return true if the entity exists in memory.
         */
        bool Exists() const;

        /**
         * Check whether the entity is activated.
         * @return Returns true if the entity is activated. Otherwise, false.
         */
        bool IsActivated() const;

        /**
         * @copydoc AZ::Entity::Activate()
         */
        void Activate();

        /**
         * @copydoc AZ::Entity::Deactivate()
         */
        void Deactivate();

        /**
         * Creates a component and attaches it to the entity.
         * You cannot add a component to an entity when the entity is activated.
         * @param componentTypeId Type ID of component to create.
         * For example, pass TransformComponentTypeId to create a TransformComponent.
         * @param componentConfig (Optional) A configuration to apply to the new component.
         * The configuration class must be of the appropriate type for this component.
         * For example, use a TransformConfig with a TransformComponent.
         * @return Returns the ID of the new component.
         * If the component could not be created then AZ::InvalidEntityId is returned.
         */
        BehaviorComponentId CreateComponent(const AZ::TypeId& componentTypeId, const AZ::ComponentConfig* componentConfig = nullptr);
        
        /**
         * Removes the component from the entity and destroys it.
         * You cannot destroy a component while the entity is activated.
         * @param componentId ID of the component to destroy.
         * @return True if the component was destroyed. Otherwise, false.
         */
        bool DestroyComponent(BehaviorComponentId componentId);

        /**
        * Gets all components registered with the entity.
        * @return A vector with the IDs of all components registered with the entity.
        */
        AZStd::vector<BehaviorComponentId> GetComponents() const;

        /**
         * Finds the first component of the requested component type.
         * @param componentTypeId The type of component to find.
         * @return The ID of the first component of the requested type.
         * Returns invalid component ID if a component of the requested type cannot be found.
         */
        BehaviorComponentId FindComponentOfType(const AZ::TypeId& componentTypeId) const;

        /**
         * Gets all components of a specified type registered with the entity.
         * @param componentTypeId The type of component to find.
         * @return A vector with the IDs of all components of a specified type registered with the entity.
         */
        AZStd::vector<BehaviorComponentId> FindAllComponentsOfType(const AZ::TypeId& componentTypeId) const;

        /**
         * Get the type of a specific component on the entity.
         * @param componentId The ID of the component to query.
         * @return The type of the specified component.
         * Returns an invalid type ID if the component is not found.
         */
        AZ::TypeId GetComponentType(BehaviorComponentId componentId) const;

        /**
         * Get the name of a specific component on the entity.
         * @param componentId the ID of the component to query.
         * @return The name of the component.
         */
        AZStd::string GetComponentName(BehaviorComponentId componentId) const;

        /**
         * Set the component's configuration.
         * You cannot configure a component while the entity is activated.
         * @param componentId The ID of the component to configure.
         * @param componentConfig The component will set its properties based on this configuration.
         * The configuration class must be of the appropriate type for this component.
         * For example, use a TransformConfig with a TransformComponent.
         * @return True if the configuration was successfully copied to the component.
         * Returns false if the component was not found, or the component was not
         * compatible with the provided configuration class.
         */
        bool SetComponentConfiguration(BehaviorComponentId componentId, const AZ::ComponentConfig& componentConfig);

        /**
         * Get a component's configuration.
         * @param componentId The ID of the component to query.
         * @param outComponentConfig[out] The component will copy its properties into this configuration class.
         * The configuration class must be of the appropriate type for this component.
         * For example, use a TransformConfig with a TransformComponent.
         * @return True if the configuration was successfully copied from the component.
         * Returns false if the component was not found, or the component was not
         * compatible with the provided configuration class.
         */
        bool GetComponentConfiguration(BehaviorComponentId componentId, AZ::ComponentConfig& outComponentConfig) const;

    private:

        /**
         * Get a pointer to the entity.
         * @return Return a pointer to the entity if it exists. Otherwise, nullptr.
         */
        AZ::Entity* GetRawEntityPtr();

        /**
         * Attempts to retrieve valid entity values.
         * If anything is invalid, an error message describes the issue.
         * @param[out] outEntity (Optional) On success, the valid entity pointer.
         * @param[out] outEntityContextId (Optional) On success, the valid entity context ID.
         * @param[out] outErrorMessage (Optional) On failure, an error message describing what went wrong.
         * @return True if successful and all values are valid. Otherwise, false.
         */
        bool GetValidEntity(AZ::Entity** outEntity, EntityContextId* outContextId, AZStd::string* outErrorMessage) const;

        /**
         * Attempt to retrieve a valid component.
         * If anything is invalid, an error message describes the issue.
         * @param componentId component ID to retrieve
         * @param outComponent (Optional On success, the valid component pointer.
         * @param outErrorMessage (Optional) On failure, an error message describing what went wrong.
         * @return True if successful and component was valid. Otherwise, false.
         */
        bool GetValidComponent(BehaviorComponentId componentId, AZ::Component** outComponent, AZStd::string* outErrorMessage) const;

        AZ::EntityId m_entityId;
    };
} // namespace AzFramework
