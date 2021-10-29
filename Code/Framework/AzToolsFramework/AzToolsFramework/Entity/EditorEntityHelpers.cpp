/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorEntityHelpers.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponentBus.h>

namespace AzToolsFramework
{
    namespace Internal
    {
        /// Internal helper function for CloneInstantiatedEntities. Performs the initial cloning of the given set of entities if they
        /// are slice or sub-slice entities, recursively. Populates a list of loose entities to clone as it traverses the duplication set.
        /// @param duplicationSet The set of entities to clone.
        /// @param out_allEntityClones Output parameter populated with all cloned entities.
        /// @param out_sourceToCloneEntityIdMap Output parameter populated with a map from all source entities to cloned entities.
        /// @param out_looseEntitiesToClone Output parameter populated with all not yet cloned entities that are not associated with a slice.
        ///         This will be used after slices are cloned to clone these entities.
        void CloneSliceEntitiesAndChildren(
            const EntityIdSet& duplicationSet,
            EntityList& out_allEntityClones,
            AZ::SliceComponent::EntityIdToEntityIdMap& out_sourceToCloneEntityIdMap,
            EntityIdList& out_looseEntitiesToClone);

        /// Internal helper function for CloneInstantiatedEntities. Updates entity ID references in all cloned entities based on
        /// the given entity ID map. This handles cases like entity transform parents, entity attachments, and entity ID references
        /// in scripting components. This will update all references in the pool of cloned entities to reference other cloned
        /// entities, if they were previously referencing any of the source entities.
        /// @param inout_allEntityClones The collection of entities that have been cloned and should have entity references updated
        ///         based on the given map.
        /// @param sourceToCloneEntityIdMap A map of entity IDs to update in the given clone list, any references to key will be
        ///         changed to a reference to value instead.
        void UpdateClonedEntityReferences(
            AZ::SliceComponent::InstantiatedContainer& inout_allEntityClones,
            const AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap);

        /// Internal helper function for CloneInstantiatedEntities. Selects all cloned entities, and updates the undo stack with
        /// information on all cloned entities.
        /// @param allEntityClones The collection of all entities that were cloned.
        /// @param undoBatch The undo batch used for tracking the cloning operation.
        void UpdateUndoStackAndSelectClonedEntities(
            const EntityList& allEntityClones,
            ScopedUndoBatch& undoBatch);

        /// Internal helper function for CloneInstantiatedEntities. If the given entity identified by the ancestor list is a slice root, clone it.
        /// @param ancestors The entity to clone if it is a slice root entity, tracked through its ancestry.
        /// @param out_allEntityClones Output parameter populated with all cloned entities.
        /// @param out_sourceToCloneEntityIdMap Output parameter populated with a map from all source entities to cloned entities.
        /// @return True if the passed in entity was cloned, false if not.
        bool CloneIfSliceRoot(
            const AZ::SliceComponent::EntityAncestorList& ancestors,
            EntityList& out_allEntityClones,
            AZ::SliceComponent::EntityIdToEntityIdMap& out_sourceToCloneEntityIdMap);

        /// Internal helper function for CloneInstantiatedEntities. If the given entity identified by the slice address and
        /// ancestor list is a subslice root, clone it.
        /// @param owningSliceAddress The slice address that owns this entity. This is necessary to clone the subslice.
        /// @param ancestors The entity to clone if it is a subslice root entity, tracked through its ancestry.
        /// @param out_allEntityClones Output parameter populated with all cloned entities.
        /// @param out_sourceToCloneEntityIdMap Output parameter populated with a map from all source entities to cloned entities.
        /// @return True if the passed in entity was cloned, false if not.
        bool CloneIfSubsliceRoot(
            const AZ::SliceComponent::SliceInstanceAddress& owningSliceAddress,
            const AZ::SliceComponent::EntityAncestorList& ancestors,
            EntityList& out_allEntityClones,
            AZ::SliceComponent::EntityIdToEntityIdMap& out_sourceToCloneEntityIdMap);

        /// Internal helper function for CloneInstantiatedEntities. Clones the given entity collection as loose entities.
        /// @param duplicationList The collection of entities to clone.
        /// @param out_allEntityClones Output parameter populated with all cloned entities.
        /// @param out_sourceToCloneEntityIdMap Output parameter populated with a map from all source entities to cloned entities.
        /// @param out_clonedLooseEntities Output parameter populated with all cloned loose entities.
        void CloneLooseEntities(
            const EntityIdList& duplicationList,
            EntityList& out_allEntityClones,
            AZ::SliceComponent::EntityIdToEntityIdMap& out_sourceToCloneEntityIdMap,
            EntityList& out_clonedLooseEntities);

    } // namespace Internal


    AZ::Entity* GetEntityById(const AZ::EntityId& entityId)
    {
        AZ_Assert(entityId.IsValid(), "Invalid EntityId provided.");
        if (!entityId.IsValid())
        {
            return nullptr;
        }

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
        return entity;
    }

    AZStd::string GetEntityName(const AZ::EntityId& entityId, const AZStd::string_view& nameOverride)
    {
        if (!entityId.IsValid())
        {
            return AZStd::string();
        }
        if (!nameOverride.empty())
        {
            return nameOverride;
        }

        const AZ::Entity* entity = GetEntityById(entityId);
        if (!entity)
        {
            return AZStd::string();
        }

        return entity->GetName();
    }

    EntityList EntityIdListToEntityList(const EntityIdList& inputEntityIds)
    {
        EntityList entities;
        entities.reserve(inputEntityIds.size());

        for (AZ::EntityId entityId : inputEntityIds)
        {
            if (!entityId.IsValid())
            {
                continue;
            }

            if (auto entity = GetEntityById(entityId))
            {
                entities.emplace_back(entity);
            }
        }

        return entities;
    }

    EntityList EntityIdSetToEntityList(const EntityIdSet& inputEntityIds)
    {
        EntityList entities;
        entities.reserve(inputEntityIds.size());

        for (AZ::EntityId entityId : inputEntityIds)
        {
            if (!entityId.IsValid())
            {
                continue;
            }

            if (auto entity = GetEntityById(entityId))
            {
                entities.emplace_back(entity);
            }
        }

        return entities;
    }

    void GetAllComponentsForEntity(const AZ::Entity* entity, AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        if (entity)
        {
            //build a set of all active and pending components associated with the entity
            componentsOnEntity.insert(componentsOnEntity.end(), entity->GetComponents().begin(), entity->GetComponents().end());
            EditorPendingCompositionRequestBus::Event(entity->GetId(), &EditorPendingCompositionRequests::GetPendingComponents, componentsOnEntity);
            EditorDisabledCompositionRequestBus::Event(entity->GetId(), &EditorDisabledCompositionRequests::GetDisabledComponents, componentsOnEntity);
        }
    }

    void GetAllComponentsForEntity(const AZ::EntityId& entityId, AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        GetAllComponentsForEntity(GetEntity(entityId), componentsOnEntity);
    }

    AZ::Uuid GetComponentTypeId(const AZ::Component* component)
    {
        return GetUnderlyingComponentType(*component);
    }

    const AZ::SerializeContext::ClassData* GetComponentClassData(const AZ::Component* component)
    {
        return GetComponentClassDataForType(GetComponentTypeId(component));
    }

    const AZ::SerializeContext::ClassData* GetComponentClassDataForType(const AZ::Uuid& componentTypeId)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        const AZ::SerializeContext::ClassData* componentClassData = serializeContext->FindClassData(componentTypeId);
        return componentClassData;
    }

    AZStd::string GetFriendlyComponentName(const AZ::Component* component)
    {
        auto className = component->RTTI_GetTypeName();
        auto classData = GetComponentClassData(component);
        if (!classData)
        {
            return className;
        }

        if (!classData->m_editData)
        {
            return classData->m_name;
        }

        if (auto editorData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
        {
            if (auto nameAttribute = editorData->FindAttribute(AZ::Edit::Attributes::NameLabelOverride))
            {
                AZStd::string name;
                AZ::AttributeReader nameReader(const_cast<AZ::Component*>(component), nameAttribute);
                nameReader.Read<AZStd::string>(name);
                return name;
            }
        }

        return classData->m_editData->m_name;
    }

    const char* GetFriendlyComponentDescription(const AZ::Component* component)
    {
        auto classData = GetComponentClassData(component);
        if (!classData || !classData->m_editData)
        {
            return "";
        }
        return classData->m_editData->m_description;
    }

    AZ::ComponentDescriptor* GetComponentDescriptor(const AZ::Component* component)
    {
        AZ::ComponentDescriptor* componentDescriptor = nullptr;
        AZ::ComponentDescriptorBus::EventResult(componentDescriptor, GetComponentTypeId(component), &AZ::ComponentDescriptor::GetDescriptor);
        return componentDescriptor;
    }

    Components::EditorComponentDescriptor* GetEditorComponentDescriptor(const AZ::Component* component)
    {
        Components::EditorComponentDescriptor* editorComponentDescriptor = nullptr;
        Components::EditorComponentDescriptorBus::EventResult(editorComponentDescriptor, component->RTTI_GetType(), &Components::EditorComponentDescriptor::GetEditorDescriptor);
        return editorComponentDescriptor;
    }

    Components::EditorComponentBase* GetEditorComponent(AZ::Component* component)
    {
        auto editorComponentBaseComponent = azrtti_cast<Components::EditorComponentBase*>(component);
        AZ_Assert(editorComponentBaseComponent, "Editor component does not derive from EditorComponentBase");
        return editorComponentBaseComponent;
    }

    bool OffersRequiredServices(
        const AZ::SerializeContext::ClassData* componentClass,
        const AZStd::vector<AZ::ComponentServiceType>& serviceFilter,
        const AZStd::vector<AZ::ComponentServiceType>& incompatibleServiceFilter
    )
    {
        AZ_Assert(componentClass, "Component class must not be null");

        if (!componentClass)
        {
            return false;
        }

        AZ::ComponentDescriptor* componentDescriptor = nullptr;
        AZ::ComponentDescriptorBus::EventResult(
            componentDescriptor, componentClass->m_typeId, &AZ::ComponentDescriptor::GetDescriptor);
        if (!componentDescriptor)
        {
            return false;
        }

        // If no services are provided, this function returns true
        if (serviceFilter.empty())
        {
            return true;
        }

        AZ::ComponentDescriptor::DependencyArrayType providedServices;
        componentDescriptor->GetProvidedServices(providedServices, nullptr);

        //reject this component if it does not offer any of the required services
        if (AZStd::find_first_of(
            providedServices.begin(),
            providedServices.end(),
            serviceFilter.begin(),
            serviceFilter.end()) == providedServices.end())
        {
            return false;
        }

        //reject this component if it does offer any of the incompatible services
        if (AZStd::find_first_of(
            providedServices.begin(),
            providedServices.end(),
            incompatibleServiceFilter.begin(),
            incompatibleServiceFilter.end()) != providedServices.end())
        {
            return false;
        }

        return true;
    }

    bool OffersRequiredServices(
        const AZ::SerializeContext::ClassData* componentClass,
        const AZStd::vector<AZ::ComponentServiceType>& serviceFilter
    )
    {
        const AZStd::vector<AZ::ComponentServiceType> incompatibleServices;
        return OffersRequiredServices(componentClass, serviceFilter, incompatibleServices);
    }

    bool ShouldInspectorShowComponent(const AZ::Component* component)
    {
        if (!component)
        {
            return false;
        }

        const AZ::SerializeContext::ClassData* classData = GetComponentClassData(component);

        // Don't show components without edit data
        if (!classData || !classData->m_editData)
        {
            return false;
        }

        // Don't show components that are set to invisible.
        if (const AZ::Edit::ElementData* editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
        {
            if (AZ::Edit::Attribute* visibilityAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Visibility))
            {
                PropertyAttributeReader reader(const_cast<AZ::Component*>(component), visibilityAttribute);
                AZ::u32 visibilityValue;
                if (reader.Read<AZ::u32>(visibilityValue))
                {
                    if (visibilityValue == AZ::Edit::PropertyVisibility::Hide)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    AZ::EntityId GetEntityIdForSortInfo(const AZ::EntityId parentId)
    {
        AZ::EntityId sortEntityId = parentId;
        if (!sortEntityId.IsValid())
        {
            AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
            EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequestBus::Events::GetEditorEntityContextId);
            AZ::SliceComponent* rootSliceComponent = nullptr;
            AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSliceComponent, editorEntityContextId,
                &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootSlice);
            if (rootSliceComponent)
            {
                return rootSliceComponent->GetMetadataEntity()->GetId();
            }
        }
        return sortEntityId;
    }

    void AddEntityIdToSortInfo(const AZ::EntityId parentId, const AZ::EntityId childId, bool forceAddToBack)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AZ::EntityId sortEntityId = GetEntityIdForSortInfo(parentId);

        bool success = false;
        EditorEntitySortRequestBus::EventResult(success, sortEntityId, &EditorEntitySortRequestBus::Events::AddChildEntity, childId, forceAddToBack);
        if (success && parentId != sortEntityId)
        {
            EditorEntitySortNotificationBus::Event(parentId, &EditorEntitySortNotificationBus::Events::ChildEntityOrderArrayUpdated);
        }
    }

    void AddEntityIdToSortInfo(const AZ::EntityId parentId, const AZ::EntityId childId, const AZ::EntityId beforeEntity)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AZ::EntityId sortEntityId = GetEntityIdForSortInfo(parentId);

        bool success = false;
        EditorEntitySortRequestBus::EventResult(success, sortEntityId, &EditorEntitySortRequestBus::Events::AddChildEntityAtPosition, childId, beforeEntity);
        if (success && parentId != sortEntityId)
        {
            EditorEntitySortNotificationBus::Event(parentId, &EditorEntitySortNotificationBus::Events::ChildEntityOrderArrayUpdated);
        }
    }

    bool RecoverEntitySortInfo(const AZ::EntityId parentId, const AZ::EntityId childId, AZ::u64 sortIndex)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        EntityOrderArray entityOrderArray;
        EditorEntitySortRequestBus::EventResult(entityOrderArray, GetEntityIdForSortInfo(parentId), &EditorEntitySortRequestBus::Events::GetChildEntityOrderArray);

        // Make sure the child entity isn't already in order array.
        auto sortIter = AZStd::find(entityOrderArray.begin(), entityOrderArray.end(), childId);
        if (sortIter != entityOrderArray.end())
        {
            entityOrderArray.erase(sortIter);
        }
        // Make sure we don't overwrite the bounds of our vector.
        if (sortIndex > entityOrderArray.size())
        {
            sortIndex = entityOrderArray.size();
        }
        entityOrderArray.insert(entityOrderArray.begin() + sortIndex, childId);
        // Push the final array back to the sort component
        return SetEntityChildOrder(parentId, entityOrderArray);
    }

    void RemoveEntityIdFromSortInfo(const AZ::EntityId parentId, const AZ::EntityId childId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        AZ::EntityId sortEntityId = GetEntityIdForSortInfo(parentId);

        bool success = false;
        EditorEntitySortRequestBus::EventResult(success, sortEntityId, &EditorEntitySortRequestBus::Events::RemoveChildEntity, childId);
        if (success && parentId != sortEntityId)
        {
            EditorEntitySortNotificationBus::Event(parentId, &EditorEntitySortNotificationBus::Events::ChildEntityOrderArrayUpdated);
        }
    }

    bool SetEntityChildOrder(const AZ::EntityId parentId, const EntityIdList& children)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        auto sortEntityId = GetEntityIdForSortInfo(parentId);

        bool success = false;
        EditorEntitySortRequestBus::EventResult(success, sortEntityId, &EditorEntitySortRequestBus::Events::SetChildEntityOrderArray, children);
        if (success && parentId != sortEntityId)
        {
            EditorEntitySortNotificationBus::Event(parentId, &EditorEntitySortNotificationBus::Events::ChildEntityOrderArrayUpdated);
        }
        return success;
    }

    EntityIdList GetEntityChildOrder(const AZ::EntityId parentId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        EntityIdList children;
        EditorEntityInfoRequestBus::EventResult(children, parentId, &EditorEntityInfoRequestBus::Events::GetChildren);

        EntityIdList entityChildOrder;
        AZ::EntityId sortEntityId = GetEntityIdForSortInfo(parentId);
        EditorEntitySortRequestBus::EventResult(entityChildOrder, sortEntityId, &EditorEntitySortRequestBus::Events::GetChildEntityOrderArray);

        // Prune out any entries in the child order array that aren't currently known to be children
        entityChildOrder.erase(
            AZStd::remove_if(
                entityChildOrder.begin(),
                entityChildOrder.end(),
                [&children](const AZ::EntityId& entityId)
                {
                    // Return true to remove if entity id was not in the child array
                    return AZStd::find(children.begin(), children.end(), entityId) == children.end();
                }
            ),
            entityChildOrder.end()
        );

        return entityChildOrder;
    }

    //build an address based on depth and order of entities
    void GetEntityLocationInHierarchy(const AZ::EntityId& entityId, AZStd::list<AZ::u64>& location)
    {
        if(entityId.IsValid())
        {
            AZ::EntityId parentId;
            EditorEntityInfoRequestBus::EventResult(parentId, entityId, &EditorEntityInfoRequestBus::Events::GetParent);
            AZ::u64 entityOrder = 0;
            EditorEntitySortRequestBus::EventResult(entityOrder, GetEntityIdForSortInfo(parentId), &EditorEntitySortRequestBus::Events::GetChildEntityIndex, entityId);
            location.push_front(entityOrder);
            GetEntityLocationInHierarchy(parentId, location);
        }
    }

    //sort vector of entities by how they're arranged
    void SortEntitiesByLocationInHierarchy(EntityIdList& entityIds)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        //cache locations for faster sort
        AZStd::unordered_map<AZ::EntityId, AZStd::list<AZ::u64>> locations;
        for (auto entityId : entityIds)
        {
            GetEntityLocationInHierarchy(entityId, locations[entityId]);
        }
        AZStd::sort(entityIds.begin(), entityIds.end(), [&locations](const AZ::EntityId& e1, const AZ::EntityId& e2) {
            //sort by container contents
            const auto& locationsE1 = locations[e1];
            const auto& locationsE2 = locations[e2];
            return AZStd::lexicographical_compare(locationsE1.begin(), locationsE1.end(), locationsE2.begin(), locationsE2.end());
        });
    }

    AZ::SliceComponent* GetEntityRootSlice(AZ::EntityId entityId)
    {
        if (entityId.IsValid())
        {
            AzFramework::EntityContextId contextId = AzFramework::EntityContextId::CreateNull();
            AzFramework::EntityIdContextQueryBus::EventResult(contextId, entityId, &AzFramework::EntityIdContextQueryBus::Events::GetOwningContextId);
            if (!contextId.IsNull())
            {
                AZ::SliceComponent* rootSlice = nullptr;
                AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(rootSlice, contextId,
                    &AzFramework::SliceEntityOwnershipServiceRequestBus::Events::GetRootSlice);
                return rootSlice;
            }
        }
        return nullptr;
    }

    bool ComponentArrayHasComponentOfType(const AZ::Entity::ComponentArrayType& components, AZ::Uuid componentType)
    {
        for (const AZ::Component* component : components)
        {
            if (component)
            {
                if (GetUnderlyingComponentType(*component) == componentType)
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool EntityHasComponentOfType(const AZ::EntityId& entityId, AZ::Uuid componentType, bool checkPendingComponents, bool checkDisabledComponents)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

        if (entity)
        {
            const AZ::Entity::ComponentArrayType components = entity->GetComponents();
            if (ComponentArrayHasComponentOfType(components, componentType))
            {
                return true;
            }

            if (checkPendingComponents)
            {
                AZ::Entity::ComponentArrayType pendingComponents;
                AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
                if (ComponentArrayHasComponentOfType(pendingComponents, componentType))
                {
                    return true;
                }
            }

            if (checkDisabledComponents)
            {
                // Check for disabled component
                AZ::Entity::ComponentArrayType disabledComponents;
                AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
                if (ComponentArrayHasComponentOfType(disabledComponents, componentType))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool IsComponentWithServiceRegistered(const AZ::Crc32& serviceId)
    {
        bool result = false;

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        if (serializeContext)
        {
            serializeContext->EnumerateDerived<AZ::Component>(
                [&](const AZ::SerializeContext::ClassData* componentClass, const AZ::Uuid& knownType) -> bool
            {
                (void)knownType;

                AZ::ComponentDescriptor* componentDescriptor = nullptr;
                EBUS_EVENT_ID_RESULT(componentDescriptor, componentClass->m_typeId, AZ::ComponentDescriptorBus, GetDescriptor);
                if (componentDescriptor)
                {
                    AZ::ComponentDescriptor::DependencyArrayType providedServices;
                    componentDescriptor->GetProvidedServices(providedServices, nullptr);

                    if (AZStd::find(providedServices.begin(), providedServices.end(), serviceId) != providedServices.end())
                    {
                        result = true;
                        return false;
                    }
                }

                return true;
            }
            );
        }
        return result;
    }

    void RemoveHiddenComponents(AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        componentsOnEntity.erase(
            AZStd::remove_if(
                componentsOnEntity.begin(), componentsOnEntity.end(),
                [](const AZ::Component* component)
                {
                    return !ShouldInspectorShowComponent(component);
                }),
            componentsOnEntity.end());
    }

    bool IsSelected(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        bool selected = false;
        EditorEntityInfoRequestBus::EventResult(
            selected, entityId, &EditorEntityInfoRequestBus::Events::IsSelected);
        return selected;
    }

    bool IsSelectableInViewport(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // Detect if the Entity is Visible
        bool visible = false;
        EditorEntityInfoRequestBus::EventResult(
            visible, entityId, &EditorEntityInfoRequestBus::Events::IsVisible);

        if (!visible)
        {
            return false;
        }

        // Detect if the Entity is Locked
        bool locked = false;
        EditorEntityInfoRequestBus::EventResult(locked, entityId, &EditorEntityInfoRequestBus::Events::IsLocked);

        if (locked)
        {
            return false;
        }

        // Detect if the Entity is part of the Editor Focus
        if (auto focusModeInterface = AZ::Interface<FocusModeInterface>::Get();
            !focusModeInterface->IsInFocusSubTree(entityId))
        {
            return false;
        }

        // Detect if the Entity is a descendant of a closed container
        if (auto containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get();
            containerEntityInterface->IsUnderClosedContainerEntity(entityId))
        {
            return false;
        }
        
        return true;
    }

    static void SetEntityLockStateRecursively(
        const AZ::EntityId entityId, const bool locked,
        const AZ::EntityId toggledEntityId, const bool toggledEntityWasLayer)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!entityId.IsValid())
        {
            return;
        }

        // first set lock state of the entity in the outliner we clicked on to lock
        bool notifyChildrenOfLayer = true;
        if (!toggledEntityWasLayer || toggledEntityId == entityId)
        {
            EditorLockComponentRequestBus::Event(
                entityId, &EditorLockComponentRequests::SetLocked, locked);
        }
        else
        {
            bool layerEntity = false;
            Layers::EditorLayerComponentRequestBus::EventResult(
                layerEntity, entityId, &Layers::EditorLayerComponentRequestBus::Events::HasLayer);

            bool prevLockState = false;
            EditorEntityInfoRequestBus::EventResult(
                prevLockState, entityId, &EditorEntityInfoRequestBus::Events::IsLocked);

            // if we're unlocking a layer, we do not want to notify/modify child entities
            // as this is a non-destructive change (their individual lock state is preserved)
            if (prevLockState && layerEntity && !locked)
            {
                notifyChildrenOfLayer = false;
            }

            // for all other entities, if we're unlocking and they were individually already locked,
            // keep their lock state, otherwise if we're locking, set all entities to be locked.
            // note: this notification will update the lock state in ComponentEntityObject and EditorLockComponent
            bool newLockState = locked ? true : prevLockState;
            EditorLockComponentNotificationBus::Event(
                entityId, &EditorLockComponentNotificationBus::Events::OnEntityLockChanged,
                newLockState);
        }

        if (notifyChildrenOfLayer)
        {
            EntityIdList children;
            EditorEntityInfoRequestBus::EventResult(
                children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);

            for (auto childId : children)
            {
                SetEntityLockStateRecursively(childId, locked, toggledEntityId, toggledEntityWasLayer);
            }
        }
    }

    // if a child of a layer has its lock state changed to false, change that layer to no longer be locked
    // do this for all layers in the hierarchy (this is because not all entities under these layers
    // will be locked so the layer cannot be represented as 'fully' locked)
    // note: must be called on layer entity
    static void UnlockLayer(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        EditorLockComponentRequestBus::Event(
            entityId, &EditorLockComponentRequestBus::Events::SetLocked, false);

        // recursive lambda - notify all children of the layer if the lock has changed
        const auto notifyChildrenOfLockChange = [](const AZ::EntityId entityId)
        {
            const auto notifyChildrenOfLockChangeImpl =
                [](const AZ::EntityId entityId, const auto& notifyChildrenRef) -> void
            {
                EntityIdList children;
                EditorEntityInfoRequestBus::EventResult(
                    children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);

                for (auto childId : children)
                {
                    notifyChildrenRef(childId, notifyChildrenRef);
                }

                bool locked = false;
                EditorLockComponentRequestBus::EventResult(
                    locked, entityId, &EditorLockComponentRequests::GetLocked);

                EditorEntityLockComponentNotificationBus::Event(
                    entityId, &EditorEntityLockComponentNotificationBus::Events::OnEntityLockChanged,
                    locked);
            };

            notifyChildrenOfLockChangeImpl(entityId, notifyChildrenOfLockChangeImpl);
        };

        notifyChildrenOfLockChange(entityId);
    }

    void SetEntityLockState(const AZ::EntityId entityId, const bool locked)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // when an entity is unlocked, if it was in a locked layer(s), unlock those layers
        if (!locked)
        {
            AZ::EntityId currentEntityId = entityId;
            while (currentEntityId.IsValid())
            {
                AZ::EntityId parentId;
                EditorEntityInfoRequestBus::EventResult(
                    parentId, currentEntityId, &EditorEntityInfoRequestBus::Events::GetParent);

                bool parentLayer = false;
                Layers::EditorLayerComponentRequestBus::EventResult(
                    parentLayer, parentId, &Layers::EditorLayerComponentRequestBus::Events::HasLayer);

                if (parentLayer && IsEntitySetToBeLocked(parentId))
                {
                    // if a child of a layer has its lock state changed to false, unlock
                    // that layer, do this for all layers in the hierarchy
                    UnlockLayer(parentId);
                    // even though layer lock state is saved to each layer individually, parents still
                    // need to be checked recursively so that the entity that was toggled can be unlocked
                }

                currentEntityId = parentId;
            }
        }

        bool isLayer = false;
        Layers::EditorLayerComponentRequestBus::EventResult(
            isLayer, entityId, &Layers::EditorLayerComponentRequestBus::Events::HasLayer);

        SetEntityLockStateRecursively(entityId, locked, entityId, isLayer);
    }

    void ToggleEntityLockState(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (entityId.IsValid())
        {
            bool locked = false;
            EditorLockComponentRequestBus::EventResult(
                locked, entityId, &EditorLockComponentRequests::GetLocked);

            AzToolsFramework::ScopedUndoBatch undo("Toggle Entity Lock State");

            if (IsSelected(entityId))
            {
                // handles the case where we have multiple entities selected but
                // must click one entity specifically in the outliner, this will
                // apply the lock state to all entities in the selection
                // (note: shift must be held)
                EntityIdList selectedEntityIds;
                ToolsApplicationRequestBus::BroadcastResult(
                    selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

                for (auto selectedId : selectedEntityIds)
                {
                    SetEntityLockState(selectedId, !locked);
                }
            }
            else
            {
                // just change the single clicked entity in the outliner
                // without affecting the current selection (should one exist)
                SetEntityLockState(entityId, !locked);
            }
        }
    }

    static void SetEntityVisibilityInternal(const AZ::EntityId entityId, const bool visibility)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        bool layerEntity = false;
        Layers::EditorLayerComponentRequestBus::EventResult(
            layerEntity, entityId, &Layers::EditorLayerComponentRequestBus::Events::HasLayer);

        if (layerEntity)
        {
            // update the EditorLayerComponent state to stay in sync with Entity visibility
            Layers::EditorLayerComponentRequestBus::Event(
                entityId, &Layers::EditorLayerComponentRequestBus::Events::SetLayerChildrenVisibility,
                visibility);
        }
        else
        {
            EditorVisibilityRequestBus::Event(
                entityId, &EditorVisibilityRequestBus::Events::SetVisibilityFlag, visibility);
        }
    }

    // note: must be called on layer entity
    static void ShowLayer(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        SetEntityVisibilityInternal(entityId, true);

        // recursive lambda - notify all children of the layer if visibility has changed
        const auto notifyChildrenOfVisibilityChange =
            [](const AZ::EntityId entityId)
        {
            const auto notifyChildrenOfVisibilityChangeImpl =
                [](const AZ::EntityId entityId, const auto& notifyChildrenRef) -> void
            {
                EntityIdList children;
                EditorEntityInfoRequestBus::EventResult(
                    children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);

                for (auto childId : children)
                {
                    notifyChildrenRef(childId, notifyChildrenRef);
                }

                EditorVisibilityNotificationBus::Event(
                    entityId, &EditorVisibilityNotificationBus::Events::OnEntityVisibilityChanged,
                    IsEntitySetToBeVisible(entityId));
            };

            notifyChildrenOfVisibilityChangeImpl(entityId, notifyChildrenOfVisibilityChangeImpl);
        };

        notifyChildrenOfVisibilityChange(entityId);
    }

    static void SetEntityVisibilityStateRecursively(
        const AZ::EntityId entityId, const bool visible,
        const AZ::EntityId toggledEntityId, const bool toggledEntityWasLayer)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!entityId.IsValid())
        {
            return;
        }

        // should we notify children of this entity or layer of a visibility change
        bool notifyChildrenOfLayer = true;
        if (!toggledEntityWasLayer || toggledEntityId == entityId)
        {
            SetEntityVisibilityInternal(entityId, visible);
        }
        else
        {
            bool layerEntity = false;
            Layers::EditorLayerComponentRequestBus::EventResult(
                layerEntity, entityId, &Layers::EditorLayerComponentRequestBus::Events::HasLayer);

            // if the layer in question was not visible and we're trying to make
            // entities visible, do not override the state of the layer and notify
            // child entities of a change in visibility
            bool oldVisibilityState = IsEntitySetToBeVisible(entityId);
            if (!oldVisibilityState && layerEntity && visible)
            {
                notifyChildrenOfLayer = false;
            }

            bool newVisibilityState = visible ? oldVisibilityState : false;
            EditorVisibilityNotificationBus::Event(
                entityId, &EditorVisibilityNotificationBus::Events::OnEntityVisibilityChanged,
                newVisibilityState);
        }

        if (notifyChildrenOfLayer)
        {
            EntityIdList children;
            EditorEntityInfoRequestBus::EventResult(
                children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);

            for (auto childId : children)
            {
                SetEntityVisibilityStateRecursively(childId, visible, toggledEntityId, toggledEntityWasLayer);
            }
        }
    }

    void SetEntityVisibility(const AZ::EntityId entityId, const bool visible)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // when an entity is set to visible, if it was in an invisible layer(s), make that layer visible
        if (visible)
        {
            AZ::EntityId currentEntityId = entityId;
            while (currentEntityId.IsValid())
            {
                AZ::EntityId parentId;
                EditorEntityInfoRequestBus::EventResult(
                    parentId, currentEntityId, &EditorEntityInfoRequestBus::Events::GetParent);

                bool parentLayer = false;
                Layers::EditorLayerComponentRequestBus::EventResult(
                    parentLayer, parentId, &Layers::EditorLayerComponentRequestBus::Events::HasLayer);

                if (parentLayer && !IsEntitySetToBeVisible(parentId))
                {
                    // if a child of a layer has its visibility state changed to true, change
                    // that layer to be visible, do this for all layers in the hierarchy
                    ShowLayer(parentId);
                    // even though layer visibility is saved to each layer individually, parents still
                    // need to be checked recursively so that the entity that was toggled can become visible
                }

                currentEntityId = parentId;
            }
        }

        bool isLayer = false;
        Layers::EditorLayerComponentRequestBus::EventResult(
            isLayer, entityId, &Layers::EditorLayerComponentRequestBus::Events::HasLayer);

        SetEntityVisibilityStateRecursively(entityId, visible, entityId, isLayer);
    }

    void ToggleEntityVisibility(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (entityId.IsValid())
        {
            bool visible = IsEntitySetToBeVisible(entityId);

            AzToolsFramework::ScopedUndoBatch undo("Toggle Entity Visibility");

            if (IsSelected(entityId))
            {
                // handles the case where we have multiple entities selected but
                // must click one entity specifically in the outliner, this will
                // apply the visibility change to all entities in the selection
                // (note: shift must be held)
                EntityIdList selectedEntityIds;
                ToolsApplicationRequestBus::BroadcastResult(
                    selectedEntityIds, &ToolsApplicationRequests::GetSelectedEntities);

                for (AZ::EntityId selectedId : selectedEntityIds)
                {
                    SetEntityVisibility(selectedId, !visible);
                }
            }
            else
            {
                // just change the single clicked entity in the outliner
                // without affecting the current selection (should one exist)
                SetEntityVisibility(entityId, !visible);
            }
        }
    }

    bool IsEntitySetToBeLocked(const AZ::EntityId entityId)
    {
        bool locked = false;
        EditorLockComponentRequestBus::EventResult(
            locked, entityId, &EditorLockComponentRequestBus::Events::GetLocked);

        return locked;
    }

    bool IsEntityLocked(const AZ::EntityId entityId)
    {
        bool locked = false;
        EditorEntityInfoRequestBus::EventResult(
            locked, entityId, &EditorEntityInfoRequestBus::Events::IsLocked);

        return locked;
    }

    bool IsEntitySetToBeVisible(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // Visibility state is tracked in 5 places, see OutlinerListModel::dataForLock for info on 3 of these ways.
        // Visibility's fourth state over lock is the EditorVisibilityRequestBus has two sets of
        // setting and getting functions for visibility. Get/SetVisibilityFlag is what should be used in most cases.
        // The fifth state is tracked on layers. Layers are always invisible to other systems, so the visibility flag
        // is set false there. However, layers need to be able to toggle visibility to hide/show their children, so
        // layers have a unique flag.
        bool layerEntity = false;
        Layers::EditorLayerComponentRequestBus::EventResult(
            layerEntity, entityId, &Layers::EditorLayerComponentRequestBus::Events::HasLayer);

        bool visible = true;
        if (layerEntity)
        {
            Layers::EditorLayerComponentRequestBus::EventResult(
                visible, entityId, &Layers::EditorLayerComponentRequestBus::Events::AreLayerChildrenVisible);
        }
        else
        {
            EditorVisibilityRequestBus::EventResult(
                visible, entityId, &EditorVisibilityRequestBus::Events::GetVisibilityFlag);
        }

        return visible;
    }

    bool IsEntityVisible(const AZ::EntityId entityId)
    {
        bool visible = false;
        EditorEntityInfoRequestBus::EventResult(
            visible, entityId, &EditorEntityInfoRequestBus::Events::IsVisible);

        return visible;
    }

    AZ::Vector3 GetWorldTranslation(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ::Vector3 worldTranslation = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(
            worldTranslation, entityId, &AZ::TransformBus::Events::GetWorldTranslation);

        return worldTranslation;
    }

    AZ::Vector3 GetLocalTranslation(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        AZ::Vector3 localTranslation = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(
            localTranslation, entityId, &AZ::TransformBus::Events::GetLocalTranslation);

        return localTranslation;
    }

    AZ::Transform GetWorldTransform(const AZ::EntityId entityId)
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, entityId, &AZ::TransformBus::Events::GetWorldTM);
        return transform;
    }

    void SetWorldTransform(const AZ::EntityId entityId, const AZ::Transform& transform)
    {
        AZ::TransformBus::Event(entityId, &AZ::TransformBus::Events::SetWorldTM, transform);
    }

    void SelectEntity(const AZ::EntityId entity)
    {
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities,
            AzToolsFramework::EntityIdList{entity});
    }

    void SelectEntities(const AzToolsFramework::EntityIdList& entities)
    {
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequestBus::Events::SetSelectedEntities, entities);
    }

    bool CloneInstantiatedEntities(const EntityIdSet& entitiesToClone, EntityIdSet& clonedEntities)
    {
        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnEntitiesAboutToBeCloned);
        ScopedUndoBatch undoBatch("Clone Selection");

        // Track the mapping of source to cloned entity. This both helps make sure that an entity is not accidentally
        // cloned twice, as well as provides the information needed to remap entity references.
        AZ::SliceComponent::EntityIdToEntityIdMap sourceToCloneEntityIdMap;
        // Track every entity that has been cloned in the container type that AZ::EntityUtils::ReplaceEntityRefs uses.
        AZ::SliceComponent::InstantiatedContainer allEntityClonesContainer(false);
        // Loose entities can all be cloned at once, so track each one found while looking for slice instances to clone.
        EntityIdList looseEntitiesToClone;

        // Clone the entities.
        Internal::CloneSliceEntitiesAndChildren(
            entitiesToClone,
            allEntityClonesContainer.m_entities,
            sourceToCloneEntityIdMap,
            looseEntitiesToClone);
        // All entities cloned so far are slice entities, so store those in a container to use for adding to the editor.
        EntityList clonedSliceEntities(allEntityClonesContainer.m_entities.begin(), allEntityClonesContainer.m_entities.end());
        // Capture all cloned loose entities, so they can be added to the editor.
        EntityList clonedLooseEntities;

        Internal::CloneLooseEntities(
            looseEntitiesToClone,
            allEntityClonesContainer.m_entities,
            sourceToCloneEntityIdMap,
            clonedLooseEntities);

        // Update any references cloned entities have to each other.
        Internal::UpdateClonedEntityReferences(allEntityClonesContainer, sourceToCloneEntityIdMap);

        // Add the cloned entities to the editor, which will also activate them.
        EditorEntityContextRequestBus::Broadcast(
            &EditorEntityContextRequests::AddEditorEntities,
            clonedLooseEntities);
        EditorEntityContextRequestBus::Broadcast(
            &EditorEntityContextRequests::HandleEntitiesAdded, clonedSliceEntities);

        // Make sure an undo operation will delete all of these cloned entities.
        // Also replace the selection with the entities that have been cloned.
        Internal::UpdateUndoStackAndSelectClonedEntities(allEntityClonesContainer.m_entities, undoBatch);

        EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnEntitiesCloned);

        for (const AZ::Entity* entity : allEntityClonesContainer.m_entities)
        {
            clonedEntities.insert(entity->GetId());
        }

        return !allEntityClonesContainer.m_entities.empty();
    }

    EntityIdSet GetCulledEntityHierarchy(const EntityIdList& entities)
    {
        EntityIdSet culledEntities;

        for (const AZ::EntityId& entityId : entities)
        {
            bool selectionIncludesTransformHeritage = false;
            AZ::EntityId parentEntityId = entityId;
            do
            {
                AZ::EntityId nextParentId;
                AZ::TransformBus::EventResult(
                    /*result*/ nextParentId,
                    /*address*/ parentEntityId,
                    &AZ::TransformBus::Events::GetParentId);
                parentEntityId = nextParentId;
                if (!parentEntityId.IsValid())
                {
                    break;
                }
                for (const AZ::EntityId& parentCheck : entities)
                {
                    if (parentCheck == parentEntityId)
                    {
                        selectionIncludesTransformHeritage = true;
                        break;
                    }
                }
            } while (parentEntityId.IsValid() && !selectionIncludesTransformHeritage);

            if (!selectionIncludesTransformHeritage)
            {
                culledEntities.insert(entityId);
            }
        }

        return culledEntities;
    }

    namespace Internal
    {
        void CloneSliceEntitiesAndChildren(
            const EntityIdSet& duplicationSet,
            EntityList& out_allEntityClones,
            AZ::SliceComponent::EntityIdToEntityIdMap& out_sourceToCloneEntityIdMap,
            EntityIdList& out_looseEntitiesToClone)
        {
            for (const AZ::EntityId& entityId : duplicationSet)
            {
                AZ::SliceComponent::SliceInstanceAddress owningSliceAddress;
                AzFramework::SliceEntityRequestBus::EventResult(owningSliceAddress, entityId,
                    &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);
                AZ::SliceComponent::EntityAncestorList ancestors;
                bool hasInstanceEntityAncestors = false;
                if (owningSliceAddress.IsValid())
                {
                    hasInstanceEntityAncestors = owningSliceAddress.GetReference()->GetInstanceEntityAncestry(entityId, ancestors);
                }
                AZ::SliceComponent::SliceInstanceAddress sourceSliceInstance;

                // Don't clone if this entity has already been cloned.
                if (out_sourceToCloneEntityIdMap.find(entityId) != out_sourceToCloneEntityIdMap.end())
                {
                }
                // Slice roots take first priority when cloning.
                else if (hasInstanceEntityAncestors &&
                    CloneIfSliceRoot(
                        ancestors,
                        out_allEntityClones,
                        out_sourceToCloneEntityIdMap))
                {
                }
                // Subslice roots take second priority.
                else if (hasInstanceEntityAncestors &&
                    CloneIfSubsliceRoot(
                        owningSliceAddress,
                        ancestors,
                        out_allEntityClones,
                        out_sourceToCloneEntityIdMap))
                {
                }
                else
                {
                    // If this wasn't a slice root or subslice root, clone it as a loose entity.
                    out_looseEntitiesToClone.push_back(entityId);
                }

                // Search through all the children of this entity for anything that needs to be cloned.
                // Slice instance entities that are not subslice roots will have been cloned already
                // when the slice root was cloned or the subslice root was cloned. Entities may exist
                // in the hierarchy that weren't cloned if they are entities that have a slice instance entity
                // as a parent, but the entity itself is not part of that slice instance.
                EntityIdList children;
                AZ::TransformBus::EventResult(
                    /*result*/ children,
                    /*address*/ entityId,
                    &AZ::TransformBus::Events::GetChildren);

                EntityIdSet childrenSet;
                for (const AZ::EntityId& child : children)
                {
                    childrenSet.insert(child);
                }
                CloneSliceEntitiesAndChildren(
                    childrenSet,
                    out_allEntityClones,
                    out_sourceToCloneEntityIdMap,
                    out_looseEntitiesToClone);
            }
        }

        void UpdateClonedEntityReferences(
            AZ::SliceComponent::InstantiatedContainer& inout_allEntityClones,
            const AZ::SliceComponent::EntityIdToEntityIdMap& sourceToCloneEntityIdMap)
        {
            // Update all cloned entities to reference other cloned entities, if any references were to entities that were cloned.
            // This includes parenting. If Parent and Child are two cloned entities, and Child is a transform child of Parent,
            // this will update that reference, so that Child (Clone) is now a child of Parent (Clone).
            // It will also include other entity references. If Entity A has an entity reference in Script Canvas to Entity B,
            // and both are cloned, then Entity A (Clone)'s Script Canvas entity reference will now be to Unrelated Entity B (Clone).
            // If only Entity A was cloned, then Entity B will not be in the mapping, and Entity A (Clone) will continue to reference Entity B.
            AZ::EntityUtils::ReplaceEntityRefs(&inout_allEntityClones,
                [&sourceToCloneEntityIdMap](const AZ::EntityId& originalId, bool /*isEntityId*/) -> AZ::EntityId
            {
                auto findIt = sourceToCloneEntityIdMap.find(originalId);
                if (findIt == sourceToCloneEntityIdMap.end())
                {
                    return originalId; // entityId is not being remapped
                }
                else
                {
                    return findIt->second; // return the remapped id
                }
            });
        }

        void UpdateUndoStackAndSelectClonedEntities(
            const EntityList& allEntityClones,
            ScopedUndoBatch &undoBatch)
        {
            EntityIdList selectEntities;
            selectEntities.reserve(allEntityClones.size());
            for (AZ::Entity* newEntity : allEntityClones)
            {
                AZ::EntityId entityId = newEntity->GetId();
                selectEntities.push_back(entityId);

                // Make sure all cloned entities are contained within the currently active undo batch command.
                EntityCreateCommand* command = aznew EntityCreateCommand(
                    static_cast<UndoSystem::URCommandID>(entityId));
                command->Capture(newEntity);
                command->SetParent(undoBatch.GetUndoBatch());
            }
            // Clear selection and select everything we cloned.
            ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::SetSelectedEntities, selectEntities);
        }

        bool CloneIfSliceRoot(
            const AZ::SliceComponent::EntityAncestorList& ancestors,
            EntityList& out_allEntityClones,
            AZ::SliceComponent::EntityIdToEntityIdMap& out_sourceToCloneEntityIdMap)
        {
            // This entity can't be a slice root if it has no ancestors.
            if (ancestors.size() <= 0)
            {
                return false;
            }
            // This entity is a slice root if it is the root entity of the first ancestor.
            if (!ancestors[0].m_entity || !SliceUtilities::IsRootEntity(*ancestors[0].m_entity))
            {
                return false;
            }

            AZ::SliceComponent::EntityIdToEntityIdMap sourceToCloneSliceEntityIdMap;
            AZ::SliceComponent::SliceInstanceAddress newInstance;
            SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(newInstance,
                &SliceEditorEntityOwnershipServiceRequests::CloneEditorSliceInstance,
                ancestors[0].m_sliceAddress, sourceToCloneSliceEntityIdMap);

            if (!newInstance.IsValid())
            {
                AZ_Warning(
                    "Cloning",
                    false,
                    "Unable to clone slice instance, check your duplicated entity selection and verify it contains the entities you expect to see.");
                return false;
            }

            for (AZ::Entity* clone : newInstance.GetInstance()->GetInstantiated()->m_entities)
            {
                out_allEntityClones.push_back(clone);
            }
            for (const AZStd::pair<AZ::EntityId, AZ::EntityId>& sourceIdToCloneId : sourceToCloneSliceEntityIdMap)
            {
                out_sourceToCloneEntityIdMap.insert(sourceIdToCloneId);
            }
            return true;
        }

        bool CloneIfSubsliceRoot(
            const AZ::SliceComponent::SliceInstanceAddress& owningSliceAddress,
            const AZ::SliceComponent::EntityAncestorList& ancestors,
            EntityList& out_allEntityClones,
            AZ::SliceComponent::EntityIdToEntityIdMap& out_sourceToCloneEntityIdMap)
        {
            bool result = false;
            // This entity can't be a subslice root if there was only one ancestor.
            if (ancestors.size() <= 1)
            {
                return result;
            }

            AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sourceSubSliceAncestry;


            AZ::SliceComponent::EntityAncestorList::const_iterator ancestorIter = ancestors.begin();
            // Skip the first, that would be a regular slice root and not a subslice root, which was already checked.
            ++ancestorIter;
            for (; ancestorIter != ancestors.end(); ++ancestorIter)
            {
                const AZ::SliceComponent::Ancestor& ancestor = *ancestorIter;
                if (!ancestor.m_entity || !SliceUtilities::IsRootEntity(*ancestor.m_entity))
                {
                    // This entity was not the root entity of this slice, so add the slice to the ancestor
                    // list and move on to the next ancestor.
                    sourceSubSliceAncestry.push_back(ancestor.m_sliceAddress);
                    continue;
                }

                // This entity has been verified to be a subslice root at this point, so clone the entity's subslice instance.
                AZ::SliceComponent::SliceInstanceAddress clonedAddress;
                AZ::SliceComponent::EntityIdToEntityIdMap sourceToCloneSliceEntityIdMap;
                SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(clonedAddress,
                    &SliceEditorEntityOwnershipServiceRequests::CloneSubSliceInstance, owningSliceAddress, sourceSubSliceAncestry,
                    ancestor.m_sliceAddress, &sourceToCloneSliceEntityIdMap);

                for (AZ::Entity* instanceEntity : clonedAddress.GetInstance()->GetInstantiated()->m_entities)
                {
                    out_allEntityClones.push_back(instanceEntity);
                }
                for (const AZStd::pair<AZ::EntityId, AZ::EntityId>& sourceIdToCloneId : sourceToCloneSliceEntityIdMap)
                {
                    out_sourceToCloneEntityIdMap.insert(sourceIdToCloneId);
                }

                // Only perform one clone, and prioritize the first found ancestor, which will track against
                // the rest of the ancestors automatically.
                result = true;
                break;
            }
            return result;
        }

        void CloneLooseEntities(
            const EntityIdList& duplicationList,
            EntityList& out_allEntityClones,
            AZ::SliceComponent::EntityIdToEntityIdMap& out_sourceToCloneEntityIdMap,
            EntityList& out_clonedLooseEntities)
        {
            AZ::SliceComponent::EntityIdToEntityIdMap looseSourceToCloneEntityIdMap;
            EntityList looseEntityClones;
            EditorEntityContextRequestBus::Broadcast(
                &EditorEntityContextRequests::CloneEditorEntities,
                duplicationList,
                looseEntityClones,
                looseSourceToCloneEntityIdMap);

            AZ_Error("Clone", looseEntityClones.size() == duplicationList.size(), "Cloned entity set is a different size from the source entity set.");

            out_allEntityClones.insert(out_allEntityClones.end(), looseEntityClones.begin(), looseEntityClones.end());
            out_clonedLooseEntities.insert(out_clonedLooseEntities.end(), looseEntityClones.begin(), looseEntityClones.end());

            for (int entityIndex = 0; entityIndex < looseEntityClones.size(); ++entityIndex)
            {
                EditorEntityContextNotificationBus::Broadcast(&EditorEntityContextNotification::OnEditorEntityDuplicated, duplicationList[entityIndex], looseEntityClones[entityIndex]->GetId());
                out_sourceToCloneEntityIdMap[duplicationList[entityIndex]] = looseEntityClones[entityIndex]->GetId();
            }
        }
    } // namespace Internal
} // namespace AzToolsFramework
