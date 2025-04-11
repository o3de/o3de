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
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
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

    EntityCompositionRequests::RemoveComponentsOutcome RemoveComponents(AZStd::span<AZ::Component* const> components)
    {
        EntityCompositionRequests::RemoveComponentsOutcome outcome = AZ::Failure(AZStd::string(""));
        EntityCompositionRequestBus::BroadcastResult(outcome, &EntityCompositionRequests::RemoveComponents, components);
        return outcome;
    }

    EntityCompositionRequests::RemoveComponentsOutcome RemoveComponents(AZStd::initializer_list<AZ::Component* const> components)
    {
        EntityCompositionRequests::RemoveComponentsOutcome outcome = AZ::Failure(AZStd::string(""));
        EntityCompositionRequestBus::BroadcastResult(outcome, &EntityCompositionRequests::RemoveComponents, components);
        return outcome;
    }

    void EnableComponents(AZStd::span<AZ::Component* const> components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::EnableComponents, components);
    }

    void EnableComponents(AZStd::initializer_list<AZ::Component* const> components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::EnableComponents, components);
    }

    void DisableComponents(AZStd::span<AZ::Component* const> components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::DisableComponents, components);
    }

    void DisableComponents(AZStd::initializer_list<AZ::Component* const> components)
    {
        EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::DisableComponents, components);
    }

    void GetAllComponentsForEntity(const AZ::Entity* entity, AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        if (entity)
        {
            //build a set of all active and pending components associated with the entity
            componentsOnEntity.append_range(entity->GetComponents());
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

    AZStd::string GetNameFromComponentClassData(const AZ::Component* component)
    {
        const AZ::SerializeContext::ClassData* classData = GetComponentClassData(component);

        // If the class data cannot be fetched for the underlying component type, return the typename of the actual component.
        if (!classData)
        {
            return component->RTTI_GetTypeName();
        }

        return classData->m_name;
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
        const AZStd::span<const AZ::ComponentServiceType> serviceFilter,
        const AZStd::span<const AZ::ComponentServiceType> incompatibleServiceFilter
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
        const AZStd::span<const AZ::ComponentServiceType> serviceFilter
    )
    {
        const AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
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

        // If Prefabs are enabled, don't check the order for an invalid parent, just return its children (i.e. the root container entity)
        // There will currently always be one root container entity, so there's no order to retrieve
        if (!parentId.IsValid())
        {
            return children;
        }

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
                AZ::ComponentDescriptorBus::EventResult(
                    componentDescriptor, componentClass->m_typeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
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

    
    AZStd::optional<int> GetFixedComponentListIndex(const AZ::Component* component)
    {
        if (component)
        {
            auto componentClassData = GetComponentClassData(component);
            if (componentClassData && componentClassData->m_editData)
            {
                if (auto editorDataElement = componentClassData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
                {
                    if (auto attribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::FixedComponentListIndex))
                    {
                        if (auto attributeData = azdynamic_cast<AZ::Edit::AttributeData<int>*>(attribute))
                        {
                            return { attributeData->Get(nullptr) };
                        }
                    }
                }
            }
        }
        return {};
    }
    
    bool IsComponentDraggable(const AZ::Component* component)
    {
        return !GetFixedComponentListIndex(component).has_value();
    }

    bool IsComponentRemovable(const AZ::Component* component)
    {
        // Determine if this component can be removed.
        auto componentClassData = component ? GetComponentClassData(component) : nullptr;
        if (componentClassData && componentClassData->m_editData)
        {
            if (auto editorDataElement = componentClassData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto attribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::RemoveableByUser))
                {
                    if (auto attributeData = azdynamic_cast<AZ::Edit::AttributeData<bool>*>(attribute))
                    {
                        return attributeData->Get(nullptr);
                    }
                }
            }
        }

        if (componentClassData && AppearsInAnyComponentMenu(*componentClassData))
        {
            return true;
        }

        // If this is a GenericComponentWrapper which wraps a nullptr, let the user remove it
        if (auto genericComponentWrapper = azrtti_cast<const Components::GenericComponentWrapper*>(component))
        {
            if (!genericComponentWrapper->GetTemplate())
            {
                return true;
            }
        }

        return false;
    }

    void SortComponentsByOrder(const AZ::EntityId entityId, AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        // sort by component order, shuffling anything not found in component order to the end
        ComponentOrderArray componentOrder;
        EditorInspectorComponentRequestBus::EventResult(
            componentOrder, entityId, &EditorInspectorComponentRequests::GetComponentOrderArray);

        if (componentOrder.empty())
        {
            return;
        }

        AZStd::sort(
            componentsOnEntity.begin(),
            componentsOnEntity.end(),
            [&componentOrder](const AZ::Component* component1, const AZ::Component* component2)
            {
                return
                    AZStd::find(componentOrder.begin(), componentOrder.end(), component1->GetId()) <
                    AZStd::find(componentOrder.begin(), componentOrder.end(), component2->GetId());
            });
    }

    void SortComponentsByPriority(AZ::Entity::ComponentArrayType& componentsOnEntity)
    {
        // The default order for components is sorted by priorit only
        struct OrderedSortComponentEntry
        {
            AZ::Component* m_component;
            int m_originalOrder;

            OrderedSortComponentEntry(AZ::Component* component, int originalOrder)
            {
                m_component = component;
                m_originalOrder = originalOrder;
            }
        };

        AZStd::vector< OrderedSortComponentEntry> sortedComponents;
        int index = 0;
        for (AZ::Component* component : componentsOnEntity)
        {
            sortedComponents.push_back(OrderedSortComponentEntry(component, index++));
        }

        // shuffle immovable components back to the front
        AZStd::sort(
            sortedComponents.begin(),
            sortedComponents.end(),
            [](const OrderedSortComponentEntry& component1, const OrderedSortComponentEntry& component2)
            {
                AZStd::optional<int> fixedComponentListIndex1 = GetFixedComponentListIndex(component1.m_component);
                AZStd::optional<int> fixedComponentListIndex2 = GetFixedComponentListIndex(component2.m_component);

                // If both components have fixed list indices, sort based on those indices
                if (fixedComponentListIndex1.has_value() && fixedComponentListIndex2.has_value())
                {
                    return fixedComponentListIndex1.value() < fixedComponentListIndex2.value();
                }

                // If component 1 has a fixed list index, sort it first
                if (fixedComponentListIndex1.has_value())
                {
                    return true;
                }

                // If component 2 has a fixed list index, component 1 should not be sorted before it
                if (fixedComponentListIndex2.has_value())
                {
                    return false;
                }

                if (!IsComponentRemovable(component1.m_component) && IsComponentRemovable(component2.m_component))
                {
                    return true;
                }

                if (IsComponentRemovable(component1.m_component) && !IsComponentRemovable(component2.m_component))
                {
                    return false;
                }

                //maintain original order if they don't need swapping
                return component1.m_originalOrder < component2.m_originalOrder;
            });

        //create new order array from sorted structure
        componentsOnEntity.clear();
        for (OrderedSortComponentEntry& component : sortedComponents)
        {
            componentsOnEntity.push_back(component.m_component);
        }
    }

    bool IsSelected(const AZ::EntityId entityId)
    {
        bool selected = false;
        EditorEntityInfoRequestBus::EventResult(selected, entityId, &EditorEntityInfoRequestBus::Events::IsSelected);
        return selected;
    }

    bool IsSelectableInViewport(const AZ::EntityId entityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // Detect if the Entity is Visible
        bool visible = false;
        EditorEntityInfoRequestBus::EventResult(visible, entityId, &EditorEntityInfoRequestBus::Events::IsVisible);

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
        const AZ::EntityId toggledEntityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!entityId.IsValid())
        {
            return;
        }

        EditorLockComponentRequestBus::Event(
            entityId, &EditorLockComponentRequests::SetLocked, locked);

        EntityIdList children;
        EditorEntityInfoRequestBus::EventResult(
            children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);

        for (auto childId : children)
        {
            SetEntityLockStateRecursively(childId, locked, toggledEntityId);
        }
    }

    void SetEntityLockState(const AZ::EntityId entityId, const bool locked)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        SetEntityLockStateRecursively(entityId, locked, entityId);
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

    static void SetEntityVisibilityStateRecursively(
        const AZ::EntityId entityId, const bool visible,
        const AZ::EntityId toggledEntityId)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (!entityId.IsValid())
        {
            return;
        }

        EditorVisibilityRequestBus::Event(entityId, &EditorVisibilityRequestBus::Events::SetVisibilityFlag, visible);

        EntityIdList children;
        EditorEntityInfoRequestBus::EventResult(
            children, entityId, &EditorEntityInfoRequestBus::Events::GetChildren);

        for (auto childId : children)
        {
            SetEntityVisibilityStateRecursively(childId, visible, toggledEntityId);
        }
    }

    void SetEntityVisibility(const AZ::EntityId entityId, const bool visible)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        SetEntityVisibilityStateRecursively(entityId, visible, entityId);
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

        bool visible = true;
        EditorVisibilityRequestBus::EventResult(
            visible, entityId, &EditorVisibilityRequestBus::Events::GetVisibilityFlag);
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

    bool EntitiesBelongToSamePrefab(const EntityIdList& selectedEntityIds, const AZ::EntityId newParentId)
    {
        auto prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
        AZ_Assert(prefabPublicInterface, "EditorEntityHelpers::EntitiesBelongToSamePrefab - Could not get the prefab public interface.");

        // Only allow reparenting the selected entities if they are all under the same instance.
        // We check the parent entity separately because it may be a container entity and
        // container entities consider their owning instance to be the parent instance.
        if (!prefabPublicInterface->EntitiesBelongToSameInstance(selectedEntityIds))
        {
            return false;
        }

        // Prevent parenting to a different owning prefab instance.
        AZ::EntityId selectedEntityId = selectedEntityIds.front();
        AZ::EntityId selectedContainerId = prefabPublicInterface->GetInstanceContainerEntityId(selectedEntityId);
        AZ::EntityId newParentContainerId = prefabPublicInterface->GetInstanceContainerEntityId(newParentId);

        // If the selected entity id is a container entity id, then we need to get its parent owning instance.
        // This is because a container entity is actually owned by the prefab instance itself.
        if (selectedContainerId == selectedEntityId && !prefabPublicInterface->IsLevelInstanceContainerEntity(selectedEntityId))
        {
            AZ::EntityId parentOfSelectedContainer;
            AZ::TransformBus::EventResult(parentOfSelectedContainer, selectedContainerId, &AZ::TransformBus::Events::GetParentId);
            selectedContainerId = prefabPublicInterface->GetInstanceContainerEntityId(parentOfSelectedContainer);
        }

        if (selectedContainerId != newParentContainerId)
        {
            return false;
        }

        return true;
    }

    namespace Internal
    {
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
