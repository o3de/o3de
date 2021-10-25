/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzCore/Component/ComponentApplication.h>

namespace AzToolsFramework
{
    // Entity helpers (Uses EBus calls, so safe)
    AZ::Entity* GetEntityById(const AZ::EntityId& entityId);

    inline AZ::Entity* GetEntity(const AZ::EntityId& entityId)
    {
        return GetEntityById(entityId);
    }

    inline AZ::Entity* GetEntity(AZ::Entity* entity)
    {
        return entity;
    }

    inline AZ::EntityId GetEntityId(const AZ::Entity* entity)
    {
        return entity->GetId();
    }

    inline AZ::EntityId GetEntityId(const AZ::EntityId& entityId)
    {
        return entityId;
    }

    AZStd::string GetEntityName(const AZ::EntityId& entityId, const AZStd::string_view& nameOverride = {});

    EntityList EntityIdListToEntityList(const EntityIdList& inputEntityIds);
    EntityList EntityIdSetToEntityList(const EntityIdSet& inputEntityIds);

    template <typename... ComponentTypes>
    struct AddComponents
    {
        template <typename... EntityType>
        static EntityCompositionRequests::AddComponentsOutcome ToEntities(EntityType... entities)
        {
            EntityCompositionRequests::AddComponentsOutcome outcome = AZ::Failure(AZStd::string(""));
            EntityCompositionRequestBus::BroadcastResult(outcome, &EntityCompositionRequests::AddComponentsToEntities, EntityIdList{ GetEntityId(entities)... }, AZ::ComponentTypeList{ azrtti_typeid<ComponentTypes>()... });
            return outcome;
        }
    };

    template <typename ComponentType>
    struct FindComponent
    {
        template <typename EntityType>
        static ComponentType* OnEntity(EntityType entityParam)
        {
            auto entity = GetEntity(entityParam);
            if (!entity)
            {
                return nullptr;
            }
            return entity->FindComponent<ComponentType>();
        }
    };

    template <typename... ComponentType>
    EntityCompositionRequests::RemoveComponentsOutcome RemoveComponents(ComponentType... components)
    {
        EntityCompositionRequests::RemoveComponentsOutcome outcome = AZ::Failure(AZStd::string(""));
        EntityCompositionRequestBus::BroadcastResult(outcome, &EntityCompositionRequests::RemoveComponents, AZ::Entity::ComponentArrayType{ components... });
        return outcome;
    }

    template <typename... ComponentType>
    void EnableComponents(ComponentType... components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::EnableComponents, AZ::Entity::ComponentArrayType{ components... });
    }

    template <typename... ComponentType>
    void DisableComponents(ComponentType... components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::DisableComponents, AZ::Entity::ComponentArrayType{ components... });
    }

    void GetAllComponentsForEntity(const AZ::Entity* entity, AZ::Entity::ComponentArrayType& componentsOnEntity);
    void GetAllComponentsForEntity(const AZ::EntityId& entityId, AZ::Entity::ComponentArrayType& componentsOnEntity);

    // Component helpers (Uses EBus calls, so safe)
    AZ::Uuid GetComponentTypeId(const AZ::Component* component);
    const AZ::SerializeContext::ClassData* GetComponentClassData(const AZ::Component* component);
    const AZ::SerializeContext::ClassData* GetComponentClassDataForType(const AZ::Uuid& componentTypeId);
    AZStd::string GetFriendlyComponentName(const AZ::Component* component);
    const char* GetFriendlyComponentDescription(const AZ::Component* component);
    AZ::ComponentDescriptor* GetComponentDescriptor(const AZ::Component* component);
    Components::EditorComponentDescriptor* GetEditorComponentDescriptor(const AZ::Component* component);
    Components::EditorComponentBase* GetEditorComponent(AZ::Component* component);
    // Returns true if the given component provides at least one of the services specified or no services are provided
    bool OffersRequiredServices(
        const AZ::SerializeContext::ClassData* componentClass,
        const AZStd::vector<AZ::ComponentServiceType>& serviceFilter,
        const AZStd::vector<AZ::ComponentServiceType>& incompatibleServiceFilter
    );
    bool OffersRequiredServices(
        const AZ::SerializeContext::ClassData* componentClass,
        const AZStd::vector<AZ::ComponentServiceType>& serviceFilter
    );

    /// Return true if the editor should show this component to users,
    /// false if the component should be hidden from users.
    bool ShouldInspectorShowComponent(const AZ::Component* component);

    AZ::EntityId GetEntityIdForSortInfo(const AZ::EntityId parentId);

    void AddEntityIdToSortInfo(const AZ::EntityId parentId, const AZ::EntityId childId, bool forceAddToBack = false);
    void AddEntityIdToSortInfo(const AZ::EntityId parentId, const AZ::EntityId childId, const AZ::EntityId beforeEntity);

    // returns true if the restore modified the sort order, false otherwise
    bool RecoverEntitySortInfo(const AZ::EntityId parentId, const AZ::EntityId childId, AZ::u64 sortIndex);

    void RemoveEntityIdFromSortInfo(const AZ::EntityId parentId, const AZ::EntityId childId);

    // returns true if the order was updated, false otherwise
    bool SetEntityChildOrder(const AZ::EntityId parentId, const EntityIdList& children);

    EntityIdList GetEntityChildOrder(const AZ::EntityId parentId);

    void GetEntityLocationInHierarchy(const AZ::EntityId& entityId, AZStd::list<AZ::u64>& location);

    void SortEntitiesByLocationInHierarchy(EntityIdList& entityIds);

    /// Return root slice containing this entity
    AZ::SliceComponent* GetEntityRootSlice(AZ::EntityId entityId);

    bool EntityHasComponentOfType(const AZ::EntityId& entityId, AZ::Uuid componentType, bool checkPendingComponents = false, bool checkDisabledComponents = false);
    bool IsComponentWithServiceRegistered(const AZ::Crc32& serviceId);

    /// Clones the passed in set of instantiated entities. Note that this will have unexpected results
    /// if given any entities that are not instantiated.
    /// @param entitiesToClone The container of entities to clone.
    /// @param clonedEntities An output parameter containing the IDs of all cloned entities.
    /// @return True if anything was cloned, false if no cloning occurred.
    bool CloneInstantiatedEntities(const EntityIdSet& entitiesToClone, EntityIdSet& clonedEntities);

    /// Clones the passed in set of instantiated entities. Note that this will have unexpected results
    /// if given any entities that are not instantiated.
    /// @param entitiesToClone The container of entities to clone.
    /// @return True if anything was cloned, false if no cloning occurred.
    inline bool CloneInstantiatedEntities(const EntityIdSet& entitiesToClone)
    {
        EntityIdSet unusedClonedEntities;
        return CloneInstantiatedEntities(entitiesToClone, unusedClonedEntities);
    }

    /// Remove all Components from the Entity that are not displayed in the Entity Inspector.
    void RemoveHiddenComponents(AZ::Entity::ComponentArrayType& componentsOnEntity);

    /// Is the entity currently selected.
    bool IsSelected(AZ::EntityId entityId);
    /// Is the entity currently selectable in the viewport.
    bool IsSelectableInViewport(AZ::EntityId entityId);

    /// Entity Lock interface to enable/disable selection in the viewport.
    /// Setting the editor lock state on a parent will recursively set the flag on all descendants as well. (to match visibility)
    void SetEntityLockState(AZ::EntityId entityId, bool locked);
    void ToggleEntityLockState(AZ::EntityId entityId);

    /// Entity Visibility interface to enable/disable rendering in the viewport.
    /// Setting the editor visibility on a parent will recursively set the flag on all descendants as well.
    void SetEntityVisibility(AZ::EntityId entityId, bool visible);
    void ToggleEntityVisibility(AZ::EntityId entityId);

    /// Determine if an Entity is set to visible or not in the Editor.
    /// This call looks at the visibility flag of the entity (GetVisibilityFlag)
    /// or the state of the layer (AreLayerChildrenVisible) if this entity is a layer.
    bool IsEntitySetToBeVisible(AZ::EntityId entityId);
    /// Is the entity visible based on layer state as well as individual state.
    bool IsEntityVisible(AZ::EntityId entityId);

    /// Determine if an Entity is set to locked or not in the Editor.
    /// This call looks at the lock state/flag of the entity (GetLocked).
    bool IsEntitySetToBeLocked(AZ::EntityId entityId);
    /// Is the entity locked based on layer state as well as individual state.
    bool IsEntityLocked(AZ::EntityId entityId);

    /// Wrap EBus GetWorldTranslation call.
    AZ::Vector3 GetWorldTranslation(AZ::EntityId entityId);

    /// Wrap EBus GetLocalTranslation call.
    AZ::Vector3 GetLocalTranslation(AZ::EntityId entityId);

    /// Wrap EBus GetWorldTM call. 
    AZ::Transform GetWorldTransform(AZ::EntityId entityId);

    /// Wrap EBus SetWorldTM call.
    void SetWorldTransform(AZ::EntityId entityId, const AZ::Transform& transform);

    /// Wrap EBus SetSelectedEntities call (overload for one entity).
    void SelectEntity(AZ::EntityId entity);

    /// Wrap EBus SetSelectedEntities call.
    void SelectEntities(const AzToolsFramework::EntityIdList& entities);

    /// Return a set of entities, culling any that have an ancestor in the list.
    /// e.g. This is useful for getting a concise set of entities that need to be duplicated.
    EntityIdSet GetCulledEntityHierarchy(const EntityIdList& entities);

}; // namespace AzToolsFramework
