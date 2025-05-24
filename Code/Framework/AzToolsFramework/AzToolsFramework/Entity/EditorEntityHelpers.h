/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/optional.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

namespace AzToolsFramework
{
    // Entity helpers (Uses EBus calls, so safe)
    AZTF_API AZ::Entity* GetEntityById(const AZ::EntityId& entityId);

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

    AZTF_API AZStd::string GetEntityName(const AZ::EntityId& entityId, const AZStd::string_view& nameOverride = {});

    AZTF_API EntityList EntityIdListToEntityList(const EntityIdList& inputEntityIds);
    AZTF_API EntityList EntityIdSetToEntityList(const EntityIdSet& inputEntityIds);

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
            return entity->template FindComponent<ComponentType>();
        }
    };

    AZTF_API EntityCompositionRequests::RemoveComponentsOutcome RemoveComponents(AZStd::span<AZ::Component* const> components);
    AZTF_API EntityCompositionRequests::RemoveComponentsOutcome RemoveComponents(AZStd::initializer_list<AZ::Component* const> components);

    AZTF_API void EnableComponents(AZStd::span<AZ::Component* const> components);

    AZTF_API void EnableComponents(AZStd::initializer_list<AZ::Component* const> components);
    AZTF_API void DisableComponents(AZStd::span<AZ::Component* const> components);
    AZTF_API void DisableComponents(AZStd::initializer_list<AZ::Component* const> components);

    AZTF_API void GetAllComponentsForEntity(const AZ::Entity* entity, AZ::Entity::ComponentArrayType& componentsOnEntity);
    AZTF_API void GetAllComponentsForEntity(const AZ::EntityId& entityId, AZ::Entity::ComponentArrayType& componentsOnEntity);

    // Component helpers (Uses EBus calls, so safe)
    AZTF_API AZ::Uuid GetComponentTypeId(const AZ::Component* component);
    AZTF_API const AZ::SerializeContext::ClassData* GetComponentClassData(const AZ::Component* component);
    AZTF_API const AZ::SerializeContext::ClassData* GetComponentClassDataForType(const AZ::Uuid& componentTypeId);
    AZTF_API AZStd::string GetNameFromComponentClassData(const AZ::Component* component);
    AZTF_API AZStd::string GetFriendlyComponentName(const AZ::Component* component);
    AZTF_API const char* GetFriendlyComponentDescription(const AZ::Component* component);
    AZTF_API AZ::ComponentDescriptor* GetComponentDescriptor(const AZ::Component* component);
    AZTF_API Components::EditorComponentDescriptor* GetEditorComponentDescriptor(const AZ::Component* component);
    AZTF_API Components::EditorComponentBase* GetEditorComponent(AZ::Component* component);
    // Returns true if the given component provides at least one of the services specified or no services are provided
    AZTF_API bool OffersRequiredServices(
        const AZ::SerializeContext::ClassData* componentClass,
        AZStd::span<const AZ::ComponentServiceType> serviceFilter,
        AZStd::span<const AZ::ComponentServiceType> incompatibleServiceFilter
    );
    AZTF_API bool OffersRequiredServices(
        const AZ::SerializeContext::ClassData* componentClass,
        AZStd::span<const AZ::ComponentServiceType> serviceFilter
    );

    /// Return true if the editor should show this component to users,
    /// false if the component should be hidden from users.
    AZTF_API bool ShouldInspectorShowComponent(const AZ::Component* component);

    //! Components can set an attribute (@ref AZ::Edit::Attributes::FixedComponentListIndex) to specify
    //! that they should appear at a fixed position in the property editor and should not be draggable.
    //! This helper function will return that index if it is set, or an empty optional if it is not.
    AZTF_API AZStd::optional<int> GetFixedComponentListIndex(const AZ::Component* component);

    //! Returns true if the component can be removed by the entity inspector.
    AZTF_API bool IsComponentRemovable(const AZ::Component* component);

    //! Returns true if the given component is draggable in the entity inspector.
    AZTF_API bool IsComponentDraggable(const AZ::Component* component);

    //! Given a ComponentArrayType, sort them into the order they would by default be sorted
    //! based on various attributes such as whether they can be dragged, deleted, whether they are
    //! visible, etc, but does not take into account the user modified order from dragging and dropping
    //! Note that this sort will try its best to keep things in the order they were in the original array
    //! and will attempt to only move elements around if necessary, so they can be sorted in another way first,
    //! then sorted by this function to ensure any additional constraints are met.
    AZTF_API void SortComponentsByPriority(AZ::Entity::ComponentArrayType& componentsOnEntity);

    //! Sorts the components on the entity based on the order specified in the componentOrder array on the entity.
    //! The order will shuffle around since this is not a stable sort, but it can be sorted by priority afterwards to stabilize.
    //! Helper function, same as above, but sorts components by order by getting the order from the given entityId.
    AZTF_API void SortComponentsByOrder(const AZ::EntityId entityId, AZ::Entity::ComponentArrayType& componentsOnEntity);

    AZTF_API AZ::EntityId GetEntityIdForSortInfo(const AZ::EntityId parentId);

    AZTF_API void AddEntityIdToSortInfo(const AZ::EntityId parentId, const AZ::EntityId childId, bool forceAddToBack = false);
    AZTF_API void AddEntityIdToSortInfo(const AZ::EntityId parentId, const AZ::EntityId childId, const AZ::EntityId beforeEntity);

    // returns true if the restore modified the sort order, false otherwise
    AZTF_API bool RecoverEntitySortInfo(const AZ::EntityId parentId, const AZ::EntityId childId, AZ::u64 sortIndex);

    AZTF_API void RemoveEntityIdFromSortInfo(const AZ::EntityId parentId, const AZ::EntityId childId);

    // returns true if the order was updated, false otherwise
    AZTF_API bool SetEntityChildOrder(const AZ::EntityId parentId, const EntityIdList& children);

    AZTF_API EntityIdList GetEntityChildOrder(const AZ::EntityId parentId);

    AZTF_API void GetEntityLocationInHierarchy(const AZ::EntityId& entityId, AZStd::list<AZ::u64>& location);

    AZTF_API void SortEntitiesByLocationInHierarchy(EntityIdList& entityIds);

    /// Return root slice containing this entity
    AZTF_API AZ::SliceComponent* GetEntityRootSlice(AZ::EntityId entityId);

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
    AZTF_API void RemoveHiddenComponents(AZ::Entity::ComponentArrayType& componentsOnEntity);

    /// Is the entity currently selected.
    AZTF_API bool IsSelected(AZ::EntityId entityId);
    /// Is the entity currently selectable in the viewport.
    AZTF_API bool IsSelectableInViewport(AZ::EntityId entityId);

    /// Entity Lock interface to enable/disable selection in the viewport.
    /// Setting the editor lock state on a parent will recursively set the flag on all descendants as well. (to match visibility)
    AZTF_API void SetEntityLockState(AZ::EntityId entityId, bool locked);
    AZTF_API void ToggleEntityLockState(AZ::EntityId entityId);

    /// Entity Visibility interface to enable/disable rendering in the viewport.
    /// Setting the editor visibility on a parent will recursively set the flag on all descendants as well.
    AZTF_API void SetEntityVisibility(AZ::EntityId entityId, bool visible);
    AZTF_API void ToggleEntityVisibility(AZ::EntityId entityId);

    /// Determine if an Entity is set to visible or not in the Editor.
    /// This call looks at the visibility flag of the entity (GetVisibilityFlag)
    /// or the state of the layer (AreLayerChildrenVisible) if this entity is a layer.
    AZTF_API bool IsEntitySetToBeVisible(AZ::EntityId entityId);
    /// Is the entity visible based on layer state as well as individual state.
    AZTF_API bool IsEntityVisible(AZ::EntityId entityId);

    /// Determine if an Entity is set to locked or not in the Editor.
    /// This call looks at the lock state/flag of the entity (GetLocked).
    AZTF_API bool IsEntitySetToBeLocked(AZ::EntityId entityId);
    /// Is the entity locked based on layer state as well as individual state.
    AZTF_API bool IsEntityLocked(AZ::EntityId entityId);

    /// Wrap EBus GetWorldTranslation call.
    AZTF_API AZ::Vector3 GetWorldTranslation(AZ::EntityId entityId);

    /// Wrap EBus GetLocalTranslation call.
    AZTF_API AZ::Vector3 GetLocalTranslation(AZ::EntityId entityId);

    /// Wrap EBus GetWorldTM call. 
    AZTF_API AZ::Transform GetWorldTransform(AZ::EntityId entityId);

    /// Wrap EBus SetWorldTM call.
    AZTF_API void SetWorldTransform(AZ::EntityId entityId, const AZ::Transform& transform);

    /// Wrap EBus SetSelectedEntities call (overload for one entity).
    AZTF_API void SelectEntity(AZ::EntityId entity);

    /// Wrap EBus SetSelectedEntities call.
    AZTF_API void SelectEntities(const AzToolsFramework::EntityIdList& entities);

    /// Return a set of entities, culling any that have an ancestor in the list.
    /// e.g. This is useful for getting a concise set of entities that need to be duplicated.
    AZTF_API EntityIdSet GetCulledEntityHierarchy(const EntityIdList& entities);

    /// Checks if the selected entities and the new parent entity belong to the same prefab.
    AZTF_API bool EntitiesBelongToSamePrefab(const EntityIdList& selectedEntities, const AZ::EntityId newParentId);

}; // namespace AzToolsFramework
