/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorEntityActionComponent.h"

#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Profiler.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzCore/std/containers/map.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include "EditorEntityHelpers.h"

#include <QMimeData>
#include <QMessageBox>

namespace AzToolsFramework
{
    namespace Components
    {
        namespace
        {
            bool DoesComponentProvideService(const AZ::Component* component, AZ::ComponentServiceType service)
            {
                auto componentDescriptor = GetComponentDescriptor(component);

                AZ::ComponentDescriptor::DependencyArrayType providedServices;
                componentDescriptor->GetProvidedServices(providedServices, component);

                return AZStd::find(providedServices.begin(), providedServices.end(), service) != providedServices.end();
            }

            bool IsComponentIncompatibleWithService(const AZ::Component* component, AZ::ComponentServiceType service)
            {
                auto componentDescriptor = GetComponentDescriptor(component);

                AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
                componentDescriptor->GetIncompatibleServices(incompatibleServices, component);

                return AZStd::find(incompatibleServices.begin(), incompatibleServices.end(), service) != incompatibleServices.end();
            }

            bool CanComponentBeRemoved(const AZ::Component* component)
            {
                auto componentClassData = component ? GetComponentClassData(component) : nullptr;
                // Currently, the only time a component is considered fixed to an entity is if it's not a valid candidate for being added
                return componentClassData && (AppearsInGameComponentMenu(*componentClassData) || AppearsInLayerComponentMenu(*componentClassData) || AppearsInLevelComponentMenu(*componentClassData));
            }

            // Check if existing components provide all services required by component
            bool AreComponentRequiredServicesMet(const AZ::Component* component, const AZ::Entity::ComponentArrayType& existingComponents)
            {
                auto componentDescriptor = GetComponentDescriptor(component);
                if (!componentDescriptor)
                {
                    AZ_Error("Editor", false, "failed to get ComponentDescriptor for component %s.", component->RTTI_GetTypeName());
                    return false;
                }

                AZ::ComponentDescriptor::DependencyArrayType requiredServices;
                componentDescriptor->GetRequiredServices(requiredServices, component);

                // Check it's required dependencies are already met
                for (auto& requiredService : requiredServices)
                {
                    bool serviceMet = false;
                    for (auto existingComponent : existingComponents)
                    {
                        if (DoesComponentProvideService(existingComponent, requiredService))
                        {
                            serviceMet = true;
                            break;
                        }
                    }

                    // Once any service isn't met, we're done
                    if (!serviceMet)
                    {
                        return false;
                    }
                }

                return true;
            }

            // Check if existing components are incompatible with services provided by component
            bool AreExistingComponentsIncompatibleWithComponent(const AZ::Component* component, const AZ::Entity::ComponentArrayType& existingComponents)
            {
                auto componentDescriptor = GetComponentDescriptor(component);

                AZ::ComponentDescriptor::DependencyArrayType providedServices;
                componentDescriptor->GetProvidedServices(providedServices, component);

                for (AZ::ComponentDescriptor::DependencyArrayType::iterator providedService = providedServices.begin();
                    providedService != providedServices.end(); ++providedService)
                {
                    AZ::EntityUtils::RemoveDuplicateServicesOfAndAfterIterator(
                        providedService,
                        providedServices,
                        component ? component->GetEntity() : nullptr);
                    for (auto existingComponent : existingComponents)
                    {
                        if (IsComponentIncompatibleWithService(existingComponent, *providedService))
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

            // Check if component is incompatible with any service provided by existing components
            bool IsComponentIncompatibleWithExistingComponents(const AZ::Component* component, const AZ::Entity::ComponentArrayType& existingComponents)
            {
                auto componentDescriptor = GetComponentDescriptor(component);

                AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
                componentDescriptor->GetIncompatibleServices(incompatibleServices, component);

                for (auto& incompatibleService : incompatibleServices)
                {
                    for (auto existingComponent : existingComponents)
                    {
                        if (DoesComponentProvideService(existingComponent, incompatibleService))
                        {
                            return true;
                        }
                    }
                }

                return false;
            }

            bool ObtainComponentWarnings(const AZ::Component* component, AZ::ComponentDescriptor::StringWarningArray& warnings)
            {
                const AZ::ComponentDescriptor * componentDescriptor = GetComponentDescriptor(component);
                componentDescriptor->GetWarnings(warnings, component);
                return !warnings.empty();
            }

            bool AddAnyValidComponentsToList(AZ::Entity::ComponentArrayType& validComponents, AZ::Entity::ComponentArrayType& componentsToAdd, AZ::Entity::ComponentArrayType* componentsAdded = nullptr)
            {
                bool addedAnyComponentsToList = false;

                for (auto componentToAddIterator = componentsToAdd.begin(); componentToAddIterator != componentsToAdd.end();)
                {
                    // Skip components that don't have requirements met
                    if (!AreComponentRequiredServicesMet(*componentToAddIterator, validComponents))
                    {
                        ++componentToAddIterator;
                        continue;
                    }

                    // Skip components that are incompatible with valid ones
                    if (IsComponentIncompatibleWithExistingComponents(*componentToAddIterator, validComponents))
                    {
                        ++componentToAddIterator;
                        continue;
                    }

                    // Skip components that provide services incompatible with valid ones
                    if (AreExistingComponentsIncompatibleWithComponent(*componentToAddIterator, validComponents))
                    {
                        ++componentToAddIterator;
                        continue;
                    }

                    // Component is now valid, push onto the list and remove from add list
                    addedAnyComponentsToList = true;
                    validComponents.push_back(*componentToAddIterator);
                    if (componentsAdded)
                    {
                        componentsAdded->push_back(*componentToAddIterator);
                    }
                    componentToAddIterator = componentsToAdd.erase(componentToAddIterator);
                }

                return addedAnyComponentsToList;
            }

            AZ::ComponentDescriptor::DependencyArrayType GetUnsatisfiedRequiredDependencies(const AZ::Component* component, AZ::Entity::ComponentArrayType providerComponents)
            {
                AZ::ComponentDescriptor* componentDescriptor = GetComponentDescriptor(component);

                AZ::ComponentDescriptor::DependencyArrayType requiredServices;
                componentDescriptor->GetRequiredServices(requiredServices, component);

                AZ::ComponentDescriptor::DependencyArrayType unsatisfiedRequiredDependencies;
                for (auto requiredService : requiredServices)
                {
                    bool serviceIsSatisfied = false;
                    for (auto providerComponent : providerComponents)
                    {
                        if (DoesComponentProvideService(providerComponent, requiredService))
                        {
                            serviceIsSatisfied = true;
                            break;
                        }
                    }

                    if (!serviceIsSatisfied)
                    {
                        unsatisfiedRequiredDependencies.push_back(requiredService);
                    }
                }
                return unsatisfiedRequiredDependencies;
            }

            bool ComponentsAreIncompatible(const AZ::Component* componentA, AZ::Component* componentB)
            {
                return IsComponentIncompatibleWithExistingComponents(componentA, { componentB }) || AreExistingComponentsIncompatibleWithComponent(componentA, { componentB });
            }

            // During component loading and validation, we have to deal with entities that haven't been initialized yet,
            // and in these cases we take the unorthodox step of communicating directly
            // with the component rather than using the EBus.
            EditorPendingCompositionRequests* GetPendingCompositionHandler(const AZ::Entity& entity)
            {
                EditorPendingCompositionRequests* pendingCompositionHandler = EditorPendingCompositionRequestBus::FindFirstHandler(entity.GetId());
                if (!pendingCompositionHandler)
                {
                    pendingCompositionHandler = entity.FindComponent<EditorPendingCompositionComponent>();
                }
                return pendingCompositionHandler;
            }
        } // namespace

        void EditorEntityActionComponent::Init()
        {
        }

        void EditorEntityActionComponent::Activate()
        {
            EntityCompositionRequestBus::Handler::BusConnect();
        }

        void EditorEntityActionComponent::Deactivate()
        {
            EntityCompositionRequestBus::Handler::BusDisconnect();
        }


        void EditorEntityActionComponent::CutComponents(AZStd::span<AZ::Component* const> components)
        {
            // Create the mime data object which will have a serialized copy of the components
            AZStd::unique_ptr<QMimeData> mimeData = ComponentMimeData::Create(components);
            if (!mimeData)
            {
                return;
            }

            // Try to remove the components
            auto removalOutcome = RemoveComponents(components);

            // Put it on the clipboard if succeeded
            if (removalOutcome)
            {
                ComponentMimeData::PutComponentMimeDataOnClipboard(AZStd::move(mimeData));
            }
        }

        void EditorEntityActionComponent::CopyComponents(AZStd::span<AZ::Component* const> components)
        {
            // Create the mime data object
            AZStd::unique_ptr<QMimeData> mimeData = ComponentMimeData::Create(components);

            // Put it on the clipboard
            ComponentMimeData::PutComponentMimeDataOnClipboard(AZStd::move(mimeData));
        }

        void EditorEntityActionComponent::PasteComponentsToEntity(AZ::EntityId entityId)
        {
            auto entity = GetEntityById(entityId);
            if (!entity)
            {
                return;
            }

            // Grab component data from clipboard, if exists
            const QMimeData* mimeData = ComponentMimeData::GetComponentMimeDataFromClipboard();

            if (!mimeData)
            {
                AZ_Error("Editor", false, "No component data was found on the clipboard to paste.");
                return;
            }

            // Create components from mime data
            AZ::Entity::ComponentArrayType components;
            ComponentMimeData::GetComponentDataFromMimeData(mimeData, components);

            auto addComponentsOutcome = AddExistingComponentsToEntityById(entityId, components);
            if (!addComponentsOutcome)
            {
                AZ_Error("Editor", false, "Pasting components to entity failed to add");
                return;
            }

            auto componentsAdded = addComponentsOutcome.GetValue().m_componentsAdded;
            AZ_Assert(components.size() == componentsAdded.size(), "Amount of components added is not the same amount of components requested");
            for (int componentIndex = 0; componentIndex < components.size(); ++componentIndex)
            {
                auto component = components[componentIndex];
                auto componentAdded = componentsAdded[componentIndex];
                // Skip empty entries, which were pasted onto existing components
                if (!componentAdded)
                {
                    continue;
                }
                AZ_Assert(GetComponentClassData(componentAdded) == GetComponentClassData(component), "Component added is not the same type as requested");
                componentAdded = AZStd::move(component);
            }
        }

        bool EditorEntityActionComponent::HasComponentsToPaste()
        {
            return ComponentMimeData::GetComponentMimeDataFromClipboard() != nullptr;
        }

        void EditorEntityActionComponent::EnableComponents(AZStd::span<AZ::Component* const> components)
        {
            ScopedUndoBatch undoBatch("Enable Component(s)");

            // Enable all the components requested
            for (auto component : components)
            {
                AZ::Entity* entity = component->GetEntity();

                bool isEntityEditable = false;
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isEntityEditable,
                    &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsEntityEditable, entity->GetId());
                if (!isEntityEditable)
                {
                    continue;
                }

                undoBatch.MarkEntityDirty(entity->GetId());

                bool reactivate = false;
                // We must deactivate entities to remove components
                if (entity->GetState() == AZ::Entity::State::Active)
                {
                    reactivate = true;
                    entity->Deactivate();
                }

                //calling SetEntity changes the component id so we save id to restore when component is moved
                auto componentId = component->GetId();

                //force component to be removed from entity or related containers
                RemoveComponentFromEntityAndContainers(entity, component);

                GetEditorComponent(component)->SetEntity(entity);

                component->SetId(componentId);

                //move the component to the pending container
                AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::AddPendingComponent, component);

                //update entity state after component shuffling
                ScrubEntity(entity);

                // Attempt to re-activate if we were previously active
                if (reactivate)
                {
                    // If previously active, attempt to re-activate entity
                    entity->Activate();

                    // If entity is not active now, something failed hardcore
                    AZ_Assert(entity->GetState() == AZ::Entity::State::Active, "Failed to reactivate entity even after scrubbing on component removal");
                }

                EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityComponentEnabled, entity->GetId(), componentId);
            }
        }

        void EditorEntityActionComponent::DisableComponents(AZStd::span<AZ::Component* const> components)
        {
            ScopedUndoBatch undoBatch("Disable Component(s)");

            // Disable all the components requested
            for (auto component : components)
            {
                AZ::Entity* entity = component->GetEntity();

                bool isEntityEditable = false;
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isEntityEditable,
                    &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsEntityEditable, entity->GetId());
                if (!isEntityEditable)
                {
                    continue;
                }

                undoBatch.MarkEntityDirty(entity->GetId());

                bool reactivate = false;
                // We must deactivate entities to remove components
                if (entity->GetState() == AZ::Entity::State::Active)
                {
                    reactivate = true;
                    entity->Deactivate();
                }

                //calling SetEntity changes the component id so we save id to restore when component is moved
                auto componentId = component->GetId();

                //force component to be removed from entity or related containers
                RemoveComponentFromEntityAndContainers(entity, component);

                GetEditorComponent(component)->SetEntity(entity);

                component->SetId(componentId);

                //move the component to the disabled container
                AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::AddDisabledComponent, component);

                //update entity state after component shuffling
                ScrubEntity(entity);

                // Attempt to re-activate if we were previously active
                if (reactivate)
                {
                    // If previously active, attempt to re-activate entity
                    entity->Activate();

                    // If entity is not active now, something failed hardcore
                    AZ_Assert(entity->GetState() == AZ::Entity::State::Active, "Failed to reactivate entity even after scrubbing on component removal");
                }

                EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityComponentDisabled, entity->GetId(), componentId);
            }
        }

        EditorEntityActionComponent::RemoveComponentsOutcome EditorEntityActionComponent::RemoveComponents(AZStd::span<AZ::Component* const> componentsToRemove)
        {
            EntityToRemoveComponentsResultMap resultMap;
            {
                ScopedUndoBatch undoBatch("Remove Component(s)");

                // Only remove, do not delete components until we know it was successful
                AZ::Entity::ComponentArrayType removedComponents;
                AZStd::vector<AZ::EntityId> entityIds;
                for (auto component : componentsToRemove)
                {
                    if (component->GetEntity())
                    {
                        entityIds.push_back(component->GetEntity()->GetId());
                    }
                }

                EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityCompositionChanging, entityIds);

                // Remove all the components requested
                for (auto componentToRemove : componentsToRemove)
                {
                    AZ::Entity* entity = componentToRemove->GetEntity();

                    bool isEntityEditable = false;
                    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(isEntityEditable,
                        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::IsEntityEditable, entity->GetId());
                    if (!isEntityEditable)
                    {
                        continue;
                    }

                    undoBatch.MarkEntityDirty(entity->GetId());

                    bool reactivate = false;
                    // We must deactivate entities to remove components
                    if (entity->GetState() == AZ::Entity::State::Active)
                    {
                        reactivate = true;
                        entity->Deactivate();
                    }

                    AZ::ComponentId removedComponentId = componentToRemove->GetId();

                    if (RemoveComponentFromEntityAndContainers(entity, componentToRemove))
                    {
                        removedComponents.push_back(componentToRemove);
                    }

                    // Run the scrubber and store the result
                    resultMap.emplace(entity->GetId(), AZStd::move(ScrubEntity(entity)));

                    // Attempt to re-activate if we were previously active
                    if (reactivate)
                    {
                        // If previously active, attempt to re-activate entity
                        entity->Activate();

                        // If entity is not active now, something failed hardcore
                        AZ_Assert(entity->GetState() == AZ::Entity::State::Active, "Failed to reactivate entity even after scrubbing on component removal");
                    }

                    EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityComponentRemoved, entity->GetId(), removedComponentId);
                }

                for (auto removedComponent : removedComponents)
                {
                    delete removedComponent;
                }

                EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityCompositionChanged, entityIds);
            }

            return AZ::Success(AZStd::move(resultMap));
        }


        bool EditorEntityActionComponent::RemoveComponentFromEntityAndContainers(AZ::Entity* entity, AZ::Component* componentToRemove)
        {
            // See if the component is on the entity proper
            const auto& entityComponents = entity->GetComponents();
            if (AZStd::find(entityComponents.begin(), entityComponents.end(), componentToRemove) != entityComponents.end())
            {
                entity->RemoveComponent(componentToRemove);
                return true;
            }

            // Check for pending component
            AZ::Entity::ComponentArrayType pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
            if (AZStd::find(pendingComponents.begin(), pendingComponents.end(), componentToRemove) != pendingComponents.end())
            {
                AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::RemovePendingComponent, componentToRemove);

                // Remove the entity* in this case because it'll try to call RemoveComponent if it isn't null
                GetEditorComponent(componentToRemove)->SetEntity(nullptr);
                return true;
            }

            // Check for disabled component
            AZ::Entity::ComponentArrayType disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
            if (AZStd::find(disabledComponents.begin(), disabledComponents.end(), componentToRemove) != disabledComponents.end())
            {
                AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::RemoveDisabledComponent, componentToRemove);

                // Remove the entity* in this case because it'll try to call RemoveComponent if it isn't null
                GetEditorComponent(componentToRemove)->SetEntity(nullptr);
                return true;
            }

            AZ_Assert(false, "Component being removed was not on the entity, pending, or disabled lists.");
            return false;
        }

        EditorEntityActionComponent::AddComponentsOutcome EditorEntityActionComponent::AddComponentsToEntities(const EntityIdList& entityIds, const AZ::ComponentTypeList& componentsToAdd)
        {
            if (entityIds.empty() || componentsToAdd.empty())
            {
                return AZ::Success(AddComponentsOutcome::ValueType());
            }

            EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityCompositionChanging, entityIds);

            // Validate componentsToAdd uuids and convert into class data
            AzToolsFramework::ClassDataList componentsToAddClassData;
            for (auto componentToAddUuid : componentsToAdd)
            {
                if (componentToAddUuid.IsNull())
                {
                    auto uuidStr = componentToAddUuid.ToString<AZStd::string>();
                    return AZ::Failure(AZStd::string::format("Invalid component uuid (%s) provided to AddComponentsToEntities, no components have been added", uuidStr.c_str()));
                }
                auto componentClassData = GetComponentClassDataForType(componentToAddUuid);
                if (!componentClassData)
                {
                    auto uuidStr = componentToAddUuid.ToString<AZStd::string>();
                    return AZ::Failure(AZStd::string::format("Invalid class data from uuid (%s) provided to AddComponentsToEntities, no components have been added", uuidStr.c_str()));
                }
                componentsToAddClassData.push_back(componentClassData);
            }

            ScopedUndoBatch undo("Add Component(s) to Entity");
            EntityToAddedComponentsMap entityToAddedComponentsMap;
            {
                for (auto& entityId : entityIds)
                {
                    auto entity = GetEntityById(entityId);
                    if (!entity)
                    {
                        continue;
                    }

                    auto& entityComponentsResult = entityToAddedComponentsMap[entityId];

                    AZ::Entity::ComponentArrayType componentsToAddToEntity;
                    for (auto& componentClassData : componentsToAddClassData)
                    {
                        AZ_Assert(componentClassData, "null component class data provided to AddComponentsToEntities");
                        if (!componentClassData)
                        {
                            continue;
                        }

                        // Create component.
                        // If it's not an "editor component" then wrap it in a GenericComponentWrapper.
                        AZ::Component* component = nullptr;
                        bool isEditorComponent = componentClassData->m_azRtti && componentClassData->m_azRtti->IsTypeOf(Components::EditorComponentBase::RTTI_Type());
                        if (isEditorComponent)
                        {
                            AZ::ComponentDescriptorBus::EventResult(
                                component, componentClassData->m_typeId, &AZ::ComponentDescriptorBus::Events::CreateComponent);
                        }
                        else
                        {
                            component = aznew Components::GenericComponentWrapper(componentClassData);
                        }

                        componentsToAddToEntity.push_back(component);
                    }

                    auto addExistingComponentsResult = AddExistingComponentsToEntityById(entityId, componentsToAddToEntity);
                    // This should never fail since we check the preconditions already (entity is non-null and it ignores null components)
                    AZ_Assert(addExistingComponentsResult, "Adding the components created to an entity failed");
                    if (addExistingComponentsResult)
                    {
                        // Repackage the single-entity result into the overall result
                        entityComponentsResult = addExistingComponentsResult.GetValue();
                    }

                }
            }

            EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityCompositionChanged, entityIds);

            return AZ::Success(AZStd::move(entityToAddedComponentsMap));
        }

        EditorEntityActionComponent::AddExistingComponentsOutcome EditorEntityActionComponent::AddExistingComponentsToEntityById(const AZ::EntityId& entityId, AZStd::span<AZ::Component* const> componentsToAdd)
        {
            AZ::Entity* entity = GetEntityById(entityId);
            if (!entity)
            {
                return AZ::Failure(AZStd::string("Null entity provided to AddExistingComponentsToEntity"));
            }

            ScopedUndoBatch undo("Add Existing Component(s) to Entity");

            AddComponentsResults addComponentsResults;

            EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityCompositionChanging, AZStd::vector<AZ::EntityId>{ entityId });

            // Add all components to the pending list
            for (auto component : componentsToAdd)
            {
                AZ_Assert(component, "Null component provided to AddExistingComponentsToEntity");
                if (!component)
                {
                    // Just skip null components
                    continue;
                }

                bool skipped = false;
                if (!CanComponentBeRemoved(component))
                {
                    auto existingComponent = entity->FindComponent(GetComponentTypeId(component));
                    if (existingComponent)
                    {
                        auto componentEditorDescriptor = GetEditorComponentDescriptor(component);
                        // Attempt to replace the previous version of components that may not be removed via paste over operation
                        if (componentEditorDescriptor && componentEditorDescriptor->SupportsPasteOver())
                        {
                            componentEditorDescriptor->PasteOverComponent(component, existingComponent);
                        }
                        else
                        {
                            AZ_Assert(false, "Attempting to add component that cannot be removed and cannot be pasted over");
                        }
                        skipped = true;
                    }
                }

                if (!skipped)
                {
                    // If it's not an "editor component" then wrap it in a GenericComponentWrapper.
                    if (!azrtti_istypeof<Components::EditorComponentBase>(component))
                    {
                        component = aznew Components::GenericComponentWrapper(component);
                    }

                    // Obliterate any existing component id to allow the entity to set the id
                    component->SetId(AZ::InvalidComponentId);

                    // Set the entity on the component (but do not add yet) so that existing systems such as UI will work properly and understand who this component belongs to.
                    GetEditorComponent(component)->SetEntity(entity);

                    // Needed to set up the serialized identifier for the pending component.
                    GetEditorComponent(component)->OnAfterEntitySet();

                    // Add component to pending for entity
                    AzToolsFramework::EditorPendingCompositionRequestBus::Event(entityId, &AzToolsFramework::EditorPendingCompositionRequests::AddPendingComponent, component);

                    addComponentsResults.m_componentsAdded.push_back(component);
                }
                else
                {
                        // Report that we didn't add a new component for this index
                        addComponentsResults.m_componentsAdded.push_back(nullptr);
                }

                undo.MarkEntityDirty(entityId);

                EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityComponentAdded, entityId, component->GetId());
            }

            // Run the scrubber!
            auto scrubEntityResult = ScrubEntity(entity);
            // Adding components to pending should NEVER invalidate existing components
            AZ_Assert(!scrubEntityResult.m_invalidatedComponents.size(), "Adding pending components invalidated existing components!");
            // Loop over all the components we added, if they were validated during the scrub, put them in the valid result
            // Otherwise, they are still pending
            for (auto componentAddedToEntity : addComponentsResults.m_componentsAdded)
            {
                auto iterValidComponent = AZStd::find(scrubEntityResult.m_validatedComponents.begin(), scrubEntityResult.m_validatedComponents.end(), componentAddedToEntity);
                if (iterValidComponent != scrubEntityResult.m_validatedComponents.end())
                {
                    addComponentsResults.m_addedValidComponents.push_back(componentAddedToEntity);
                    scrubEntityResult.m_validatedComponents.erase(iterValidComponent);
                }
                else
                {
                    addComponentsResults.m_addedPendingComponents.push_back(componentAddedToEntity);
                }
            }

            // Any left over validated components are other components that happened to get validated because of our change and return those, but separately
            addComponentsResults.m_additionalValidatedComponents.swap(scrubEntityResult.m_validatedComponents);

            EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityCompositionChanged, AZStd::vector<AZ::EntityId>{ entityId });

            return AZ::Success(AZStd::move(addComponentsResults));
        }

        EntityCompositionRequests::ScrubEntitiesOutcome EditorEntityActionComponent::ScrubEntities(const EntityList& entities)
        {
            // Optimization Note: We broadcast the entity's ID even if the scrubber will make no changes.
            // We could avoid doing this by breaking the scrubbing algorithm into
            // multiple steps, so we detect all entities that need scrubbing
            // before actually making the changes.

            EntityToScrubEntityResultsMap results;

            // This function is uncommon in that it may need to handle uninitialized entities.
            // Determine if entities are initialized or not.
            EntityIdList initializedEntityIds;
            initializedEntityIds.reserve(entities.size());
            for (AZ::Entity* entity : entities)
            {
                if (entity && entity->GetState() >= AZ::Entity::State::Init)
                {
                    initializedEntityIds.push_back(entity->GetId());
                }
            }

            // We only create undo actions and broadcast change-notifications if the entities are initialized.
            AZStd::unique_ptr<ScopedUndoBatch> undo;
            if (!initializedEntityIds.empty())
            {
                undo.reset(aznew ScopedUndoBatch("Scrubbing entities"));

                EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityCompositionChanging, initializedEntityIds);
            }

            // scrub the entities
            for (AZ::Entity* entity : entities)
            {
                if (entity)
                {
                    results.emplace(entity->GetId(), ScrubEntity(entity));
                }
            }

            if (!initializedEntityIds.empty())
            {
                EntityCompositionNotificationBus::Broadcast(&EntityCompositionNotificationBus::Events::OnEntityCompositionChanged, initializedEntityIds);
            }

            return AZ::Success(AZStd::move(results));
        }

        EntityCompositionRequests::ScrubEntityResults EditorEntityActionComponent::ScrubEntity(AZ::Entity* entity)
        {
            // This function is uncommon in that it may need to handle uninitialized entities.
            // We should not create undo actions for uninitialized entities,
            // and we cannot communicate with their components via EBus.

            ScrubEntityResults result;

            // This function is uncommon in that it may need to handle uninitialized entities.
            bool entityWasIntialized = entity->GetState() >= AZ::Entity::State::Init;

            // Cannot undo changes to an entity that hasn't been initialized yet.
            AZStd::unique_ptr<ScopedUndoBatch> undo;
            if (entityWasIntialized)
            {
                undo.reset(aznew ScopedUndoBatch("Scrub entity"));
            }

            bool entityWasActive = entity->GetState() == AZ::Entity::State::Active;
            if (entityWasActive)
            {
                entity->Deactivate();
            }

            // Communication with the PendingComposition handler is required.
            // Create the component if necessary.
            EditorPendingCompositionRequests* pendingCompositionHandler = GetPendingCompositionHandler(*entity);
            if (!pendingCompositionHandler)
            {
                pendingCompositionHandler = entity->CreateComponent<EditorPendingCompositionComponent>();
                if (undo)
                {
                    undo->MarkEntityDirty(entity->GetId());
                }
            }

            // If the entity was not activated, then it could possibly have some invalid components added on it that we need to check for
            // Otherwise we would have already gone through ScrubEntity and asserted/failed. So skip this if we know we were valid on entry.
            if (!entityWasActive)
            {
                AZ::Entity::ComponentArrayType validComponents;
                AZ::Entity::ComponentArrayType invalidComponents = entity->GetComponents();

                // Keep looping to add any component that has it's dependencies met
                // This will keep checking invalid components against more and more valid components
                // Until we cannot add any further components, which terminates the loop
                while (AddAnyValidComponentsToList(validComponents, invalidComponents))
                {
                    // this is an intentional empty loop, as we keep going until there are no more valid components and the above
                    // function returns false.
                }

                // Move currently invalid components from the entity to the pending queue
                if (!invalidComponents.empty())
                {
                    for (auto invalidComponent : invalidComponents)
                    {
                        if (ShouldInspectorShowComponent(invalidComponent))
                        {
                            result.m_invalidatedComponents.push_back(invalidComponent);

                            // Save the component ID since RemoveComponent will reset it
                            auto componentId = invalidComponent->GetId();

                            entity->RemoveComponent(invalidComponent);

                            // Restore the component ID and entity*
                            invalidComponent->SetId(componentId);

                            GetEditorComponent(invalidComponent)->SetEntity(entity);

                            // Add the component to the pending list
                            pendingCompositionHandler->AddPendingComponent(invalidComponent);
                        }
                        else
                        {
                            // Delete hidden components.
                            // Since they're hidden, it's not clear what users could do to resolve the problem.
                            AZ_Warning("Editor", false,
                                "Built-in component '%s' from entity '%s' %s was removed during the load/reload/push process.\n"
                                "This is generally benign, and often results from upgrades of old data that contains duplicate or deprecated components",
                                GetComponentName(invalidComponent).c_str(), entity->GetName().c_str(), entity->GetId().ToString().c_str());
                            entity->RemoveComponent(invalidComponent);
                            delete invalidComponent;
                        }
                    }

                    if (undo)
                    {
                        undo->MarkEntityDirty(entity->GetId());
                    }
                }
            }

            // Attempt to add pending components
            auto addPendingOutcome = AddPendingComponentsToEntity(entity);
            if (addPendingOutcome)
            {
                result.m_validatedComponents.swap(addPendingOutcome.GetValue());
            }

            // See if any of the previously invalid components on the entity are still invalid.
            // Since we're only checking for an entities' currently owned components that have been invalidated,
            // we do not mind components that were in the pending set that failed to add.
            if (!entityWasActive)
            {
                for (auto invalidComponentsIter = result.m_invalidatedComponents.begin(); invalidComponentsIter != result.m_invalidatedComponents.end(); )
                {
                    auto foundIter = AZStd::find(result.m_validatedComponents.begin(), result.m_validatedComponents.end(), *invalidComponentsIter);
                    if (foundIter != result.m_validatedComponents.end())
                    {
                        invalidComponentsIter = result.m_invalidatedComponents.erase(invalidComponentsIter);
                    }
                    else
                    {
                        ++invalidComponentsIter;
                    }
                }
            }

            if (entityWasActive)
            {
                entity->Activate();
                // This should never occur as AddPendingComponentsToEntity guarantees that the operation will succeed.
                // Any components that would cause this activate to fail should remain in the pending list
                AZ_Assert(entity->GetState() == AZ::Entity::State::Active, "Failed to re-activate entity during ScrubEntity.", entity->GetName().c_str());
            }
            return result;
        }

        EditorEntityActionComponent::AddPendingComponentsOutcome EditorEntityActionComponent::AddPendingComponentsToEntity(AZ::Entity* entity)
        {
            // Note that this function may process entities before they're initialized.
            // We should not create undo actions for uninitialized entities,
            // and we cannot communicate with their components via EBus.

            if (!entity)
            {
                return AZ::Failure(AZStd::string("Null passed to AddPendingComponentsToEntity, no components have been enabled"));
            }

            if (entity->GetState() == AZ::Entity::State::Active)
            {
                return AZ::Failure(AZStd::string::format("AddPendingComponentsToEntity cannot run on activated entity '%s' %s. Calling function must handle deactivation/reactivation",
                    entity->GetName().c_str(), entity->GetId().ToString().c_str()));
            }

            EditorPendingCompositionRequests* pendingCompositionHandler = GetPendingCompositionHandler(*entity);
            if (!pendingCompositionHandler)
            {
                return AZ::Failure(AZStd::string::format("AddPendingComponentsToEntity cannot run on entity '%s' %s due to it missing the EditorPendingCompositionComponent.",
                    entity->GetName().c_str(), entity->GetId().ToString().c_str()));
            }

            bool entityWasIntialized = entity->GetState() >= AZ::Entity::State::Init;

            // Don't create undo events for uninitialized entities
            AZStd::unique_ptr<ScopedUndoBatch> undo;
            if (entityWasIntialized)
            {
                undo.reset(aznew ScopedUndoBatch("Added pending components to entity"));
            }

            // Same looping algorithm as the scrubber, but we'll also get the list of added components so we can clean up the pending list if we were successful
            AZ::Entity::ComponentArrayType pendingComponents;
            pendingCompositionHandler->GetPendingComponents(pendingComponents);

            AZ::Entity::ComponentArrayType addedPendingComponents;
            AZ::Entity::ComponentArrayType currentComponents = entity->GetComponents();
            while (AddAnyValidComponentsToList(currentComponents, pendingComponents, &addedPendingComponents)); // <- Intentional semicolon

            if (!addedPendingComponents.empty())
            {
                for (auto addedPendingComponent : addedPendingComponents)
                {
                    // Save off the component id, reset the entity pointer since it will be checked in AddComponent otherwise
                    auto componentId = addedPendingComponent->GetId();
                    GetEditorComponent(addedPendingComponent)->SetEntity(nullptr);
                    // Restore the component id, in case it got changed
                    addedPendingComponent->SetId(componentId);

                    if (entity->AddComponent(addedPendingComponent))
                    {
                        pendingCompositionHandler->RemovePendingComponent(addedPendingComponent);
                    }
                }

                if (undo)
                {
                    undo->MarkEntityDirty(entity->GetId());
                }
            }

            return AZ::Success(addedPendingComponents);
        }

        EditorEntityActionComponent::PendingComponentInfo EditorEntityActionComponent::GetPendingComponentInfo(const AZ::Component* component)
        {
            EditorEntityActionComponent::PendingComponentInfo pendingComponentInfo;

            if (!component || !component->GetEntity())
            {
                return pendingComponentInfo;
            }

            ObtainComponentWarnings(component, pendingComponentInfo.m_warnings);
            AZ::Entity::ComponentArrayType pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(component->GetEntityId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
            auto iterPendingComponent = AZStd::find(pendingComponents.begin(), pendingComponents.end(), component);

            //removing assert so we can query pending info for non-pending components without calling
            //GetPendingComponents twice and just returning
            //AZ_Assert(iterPendingComponent != pendingComponents.end(), "Component being queried was not in the pending list");
            if (iterPendingComponent == pendingComponents.end())
            {
                return pendingComponentInfo;
            }

            const auto& entityComponents = component->GetEntity()->GetComponents();
            // See what is not already provided by valid components
            auto servicesNotSatisfiedByValidComponents = GetUnsatisfiedRequiredDependencies(component, entityComponents);

            for (auto serviceNotSatisfiedByValidComponent : servicesNotSatisfiedByValidComponents)
            {
                // Check if pending provides and then list them under waiting on list
                bool serviceSatisfiedByPendingComponent = false;
                for (auto pendingComponent : pendingComponents)
                {
                    if (DoesComponentProvideService(pendingComponent, serviceNotSatisfiedByValidComponent))
                    {
                        // Ensure if pending component provides a service it isn't incompatible with the component
                        if (ComponentsAreIncompatible(component, pendingComponent))
                        {
                            continue;
                        }
                        serviceSatisfiedByPendingComponent = true;
                        pendingComponentInfo.m_pendingComponentsWithRequiredServices.push_back(pendingComponent);
                        break;
                    }
                }

                // If pending does not provide, add to required services list
                if (!serviceSatisfiedByPendingComponent)
                {
                    pendingComponentInfo.m_missingRequiredServices.push_back(serviceNotSatisfiedByValidComponent);
                }
            }

            //find incompatible services that we need to avoid
            auto componentDescriptor = GetComponentDescriptor(component);
            componentDescriptor->GetIncompatibleServices(pendingComponentInfo.m_incompatibleServices, component);

            // Check for incompatibility with only the entity components, we don't care about other pending components
            for (auto entityComponent : entityComponents)
            {
                // Either we are incompatible with them or they would be incompatible with us
                if (ComponentsAreIncompatible(component, entityComponent))
                {
                    pendingComponentInfo.m_validComponentsThatAreIncompatible.push_back(entityComponent);
                }
            }

            return pendingComponentInfo;
        }

        AZStd::string EditorEntityActionComponent::GetComponentName(const AZ::Component* component)
        {
            return GetFriendlyComponentName(component);
        }

        void EditorEntityActionComponent::Reflect(AZ::ReflectContext* context)
        {
            ComponentMimeData::Reflect(context);
            AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
            if (serialize)
            {
                serialize->Class<EditorEntityActionComponent, AZ::Component>();
            }
        }

        void EditorEntityActionComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("EntityCompositionRequests"));
        }

        void EditorEntityActionComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("EntityCompositionRequests"));
        }

        void EditorEntityActionComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& /*required*/)
        {
        }

    } // namespace Components
} // namespace AzToolsFramework
