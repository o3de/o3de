/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include "UiEditorInternalBus.h"
#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <LyShine/Bus/UiSystemBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <LyShine/Bus/UiLayoutManagerBus.h>

#include <QMimeData>

// Internal helper functions
namespace Internal
{
    AZStd::string GetComponentIconPath(const AZ::SerializeContext::ClassData& componentClassData)
    {
        AZStd::string iconPath = "Icons/Components/Component_Placeholder.svg";

        auto editorElementData = componentClassData.m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
        if (auto iconAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Icon))
        {
            if (auto iconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(iconAttribute))
            {
                AZStd::string iconAttributeValue = iconAttributeData->Get(nullptr);
                if (!iconAttributeValue.empty())
                {
                    iconPath = AZStd::move(iconAttributeValue);
                }
            }
        }

        // use absolute path if possible
        bool result = false;
        AZ::Data::AssetInfo info;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, iconPath.c_str(), info, watchFolder);
        if (result)
        {
            iconPath = AZStd::string::format("%s/%s", watchFolder.c_str(), info.m_relativePath.c_str());
        }

        return iconPath;
    }

    AZ::SerializeContext* GetSerializeContext()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "We should have a valid context!");
        return serializeContext;
    }

    const char* GetFriendlyComponentName(const AZ::SerializeContext::ClassData& componentClassData)
    {
        return componentClassData.m_editData
            ? componentClassData.m_editData->m_name
            : componentClassData.m_name;
    }

    AZStd::string GetFriendlyComponentNameFromType(const AZ::TypeId& componentType)
    {
        const AZ::SerializeContext::ClassData* componentClassData = GetSerializeContext()->FindClassData(componentType);
        AZStd::string componentName(componentClassData ? GetFriendlyComponentName(*componentClassData) : "<unknown>");

        return componentName;
    }

    AzToolsFramework::Components::EditorComponentBase* GetEditorComponent(AZ::Component* component)
    {
        auto editorComponentBaseComponent = azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(component);
        return editorComponentBaseComponent;
    }

    bool AppearsInUIComponentMenu(const AZ::SerializeContext::ClassData& componentClassData, bool forCanvasEntity)
    {
        return AzToolsFramework::AppearsInAddComponentMenu(componentClassData, forCanvasEntity ? AZ_CRC_CE("CanvasUI") : AZ_CRC_CE("UI"));
    }

    bool IsAddableByUser(const AZ::SerializeContext::ClassData& componentClassData)
    {
        if (componentClassData.m_editData)
        {
            if (auto editorDataElement = componentClassData.m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                if (auto addableAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::AddableByUser))
                {
                    // skip this component (return early) if user is not allowed to add it directly
                    if (auto addableData = azdynamic_cast<AZ::Edit::AttributeData<bool>*>(addableAttribute))
                    {
                        if (!addableData->Get(nullptr))
                        {
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool IsAddableByUserAndCompatibleWithEntityType(const AZ::SerializeContext::ClassData& componentClassData, bool forCanvasEntity)
    {
        return IsAddableByUser(componentClassData) && AppearsInUIComponentMenu(componentClassData, forCanvasEntity);
    }

    bool IsComponentServiceCompatibleWithOtherServices(const AZ::TypeId& componentType,
        const AZStd::vector<AZ::TypeId>& otherComponentTypes)
    {
        // Get the componentDescriptor from the componentClassData
        AZ::ComponentDescriptor* componentDescriptor = nullptr;
        AZ::ComponentDescriptorBus::EventResult(componentDescriptor, componentType, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
        if (!componentDescriptor)
        {
            AZStd::string message = AZStd::string::format("ComponentDescriptor not found for component %s.", GetFriendlyComponentNameFromType(componentType).c_str());
            AZ_Error("UI Editor", false, message.c_str());
            return false;
        }

        // Get the incompatible, provided and required services from the descriptor
        AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
        componentDescriptor->GetIncompatibleServices(incompatibleServices, nullptr);

        AZ::ComponentDescriptor::DependencyArrayType providedServices;
        componentDescriptor->GetProvidedServices(providedServices, nullptr);

        AZ::ComponentDescriptor::DependencyArrayType requiredServices;
        componentDescriptor->GetRequiredServices(requiredServices, nullptr);

        // Check if component is compatible with other components
        AZ::ComponentDescriptor::DependencyArrayType services;
        for (const AZ::TypeId& otherComponentType : otherComponentTypes)
        {
            AZ::ComponentDescriptor* otherComponentDescriptor = nullptr;
            AZ::ComponentDescriptorBus::EventResult(
                otherComponentDescriptor, otherComponentType, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
            if (!otherComponentDescriptor)
            {
                AZStd::string message = AZStd::string::format("ComponentDescriptor not found for component %s.", GetFriendlyComponentNameFromType(otherComponentType).c_str());
                AZ_Error("UI Editor", false, message.c_str());
                return false;
            }

            services.clear();
            otherComponentDescriptor->GetProvidedServices(services, nullptr);

            // Check that none of the services currently provided by the entity are incompatible services
            // for the new component.
            // Also check that all of the required services of the new component are provided by the
            // existing components
            for (const AZ::ComponentServiceType& service : services)
            {
                for (const AZ::ComponentServiceType& incompatibleService : incompatibleServices)
                {
                    if (service == incompatibleService)
                    {
                        return false;
                    }
                }

                for (auto iter = requiredServices.begin(); iter != requiredServices.end(); ++iter)
                {
                    const AZ::ComponentServiceType& requiredService = *iter;
                    if (service == requiredService)
                    {
                        // this required service has been matched - remove from list
                        requiredServices.erase(iter);
                        break;
                    }
                }
            }

            services.clear();
            otherComponentDescriptor->GetIncompatibleServices(services, nullptr);

            // Check that none of the services provided by the component are incompatible with any
            // of the services currently provided by the entity
            for (const AZ::ComponentServiceType& service : services)
            {
                for (const AZ::ComponentServiceType& providedService : providedServices)
                {
                    if (service == providedService)
                    {
                        return false;
                    }
                }
            }
        }

        if (requiredServices.size() > 0)
        {
            // some required services were not provided by the components on the entity
            return false;
        }

        return true;
    }

    bool AreComponentServicesCompatibleWithEntity(const AZStd::vector<AZ::TypeId>& componentTypes,
        const AZ::EntityId& entityId,
        ComponentHelpers::EntityComponentPair* firstIncompatibleComponentType = nullptr)
    {
        AZ::Entity* entity = AzToolsFramework::GetEntityById(entityId);
        if (!entity)
        {
            AZ_Error("UI Editor", false, "Can't find entity by Id.");
            return false;
        }

        // Make a list of the entity's existing component types
        AZStd::vector<AZ::TypeId> entityComponentTypes;
        entityComponentTypes.reserve(entity->GetComponents().size());
        for (const AZ::Component* component : entity->GetComponents())
        {
            const AZ::Uuid& componentTypeId = AzToolsFramework::GetUnderlyingComponentType(*component);
            entityComponentTypes.push_back(componentTypeId);
        }

        if (componentTypes.size() == 1)
        {
            if (!IsComponentServiceCompatibleWithOtherServices(componentTypes[0], entityComponentTypes))
            {
                if (firstIncompatibleComponentType)
                {
                    *firstIncompatibleComponentType = AZStd::make_pair(entityId, componentTypes[0]);
                }
                return false;
            }
        }
        else
        {
            for (int i = 0; i < componentTypes.size(); i++)
            {
                // Add all but this component type
                AZStd::vector<AZ::TypeId> otherComponentTypes = entityComponentTypes;
                if (i > 0)
                {
                    otherComponentTypes.insert(otherComponentTypes.end(), componentTypes.begin(), componentTypes.begin() + i);
                }
                if (i < componentTypes.size() - 1)
                {
                    otherComponentTypes.insert(otherComponentTypes.end(), componentTypes.begin() + (i + 1), componentTypes.end());
                }

                if (!IsComponentServiceCompatibleWithOtherServices(componentTypes[i], otherComponentTypes))
                {
                    if (firstIncompatibleComponentType)
                    {
                        *firstIncompatibleComponentType = AZStd::make_pair(entityId, componentTypes[i]);
                    }
                    return false;
                }
            }
        }

        return true;
    }

    bool IsComponentServiceCompatibleWithEntity(const AZ::TypeId& componentType, const AZ::EntityId& entityId)
    {
        AZStd::vector<AZ::TypeId> componentTypes;
        componentTypes.push_back(componentType);
        return AreComponentServicesCompatibleWithEntity(componentTypes, entityId);
    }
    
    bool CanAddComponentsToEntities(const AzToolsFramework::ClassDataList& classDataForComponentsToAdd,
        const AzToolsFramework::EntityIdList& entities,
        bool isCanvasEntity,
        ComponentHelpers::EntityComponentPair* firstIncompatibleComponentType = nullptr)
    {
        if (classDataForComponentsToAdd.empty() || entities.empty())
        {
            return false;
        }

        for (auto componentClassData : classDataForComponentsToAdd)
        {
            if (!IsAddableByUserAndCompatibleWithEntityType(*componentClassData, isCanvasEntity))
            {
                return false;
            }
        }

        // Make a list of component types from component class data
        AZStd::vector<AZ::TypeId> componentTypes;
        componentTypes.reserve(classDataForComponentsToAdd.size());
        for (auto componentClassData : classDataForComponentsToAdd)
        {
            componentTypes.push_back(componentClassData->m_typeId);
        }

        for (const AZ::EntityId& entityId : entities)
        {
            if (!AreComponentServicesCompatibleWithEntity(componentTypes, entityId, firstIncompatibleComponentType))
            {
                return false;
            }
        }

        return true;
    }

    bool CanComponentServicesBeRemovedFromEntity(const AZ::Entity::ComponentArrayType& componentsToRemove, const AZ::EntityId& entityId)
    {
        AZ::Entity* entity = AzToolsFramework::GetEntityById(entityId);
        if (!entity)
        {
            AZ_Error("UI Editor", false, "Can't find entity by Id.");
            return false;
        }

        // Go through all the components on the entity (except the ones to remove) and collect all the required services
        // and all the provided services
        AZ::ComponentDescriptor::DependencyArrayType allRequiredServices;
        AZ::ComponentDescriptor::DependencyArrayType allProvidedServices;
        AZ::ComponentDescriptor::DependencyArrayType services;
        for (const AZ::Component* component : entity->GetComponents())
        {
            if (AZStd::find(componentsToRemove.begin(), componentsToRemove.end(), component) != componentsToRemove.end())
            {
                continue;
            }

            const AZ::Uuid& componentTypeId = AzToolsFramework::GetUnderlyingComponentType(*component);

            AZ::ComponentDescriptor* componentDescriptor = nullptr;
            AZ::ComponentDescriptorBus::EventResult(
                componentDescriptor, componentTypeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
            if (!componentDescriptor)
            {
                AZStd::string message = AZStd::string::format("ComponentDescriptor not found for component %s.", GetFriendlyComponentNameFromType(componentTypeId).c_str());
                AZ_Error("UI Editor", false, message.c_str());
                return false;
            }

            services.clear();
            componentDescriptor->GetRequiredServices(services, nullptr);

            for (auto requiredService : services)
            {
                allRequiredServices.push_back(requiredService);
            }

            services.clear();
            componentDescriptor->GetProvidedServices(services, nullptr);

            for (auto providedService : services)
            {
                allProvidedServices.push_back(providedService);
            }
        }

        // remove all the satisfied services from the required services list
        for (auto providedService : allProvidedServices)
        {
            for (auto iter = allRequiredServices.begin(); iter != allRequiredServices.end(); ++iter)
            {
                const AZ::ComponentServiceType& requiredService = *iter;
                if (providedService == requiredService)
                {
                    // this required service has been matched - remove from list
                    allRequiredServices.erase(iter);
                    break;
                }
            }
        }

        // if there is anything left in required services then check if the component we are removing
        // provides any of those services, if so we cannot remove it
        if (allRequiredServices.size() > 0)
        {
            for (const AZ::Component* componentToRemove : componentsToRemove)
            {
                const AZ::Uuid& componentToRemoveTypeId = AzToolsFramework::GetUnderlyingComponentType(*componentToRemove);

                AZ::ComponentDescriptor* componentDescriptor = nullptr;
                AZ::ComponentDescriptorBus::EventResult(
                    componentDescriptor, componentToRemoveTypeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
                if (!componentDescriptor)
                {
                    AZStd::string message = AZStd::string::format("ComponentDescriptor not found for component %s.", GetFriendlyComponentNameFromType(componentToRemoveTypeId).c_str());
                    AZ_Error("UI Editor", false, message.c_str());
                    return false;
                }

                // Get the services provided by the component to be deleted
                AZ::ComponentDescriptor::DependencyArrayType providedServices;
                componentDescriptor->GetProvidedServices(providedServices, nullptr);

                for (auto requiredService : allRequiredServices)
                {
                    // Check that none of the services currently still by the entity are the any of the ones we want to remove
                    for (auto providedService : providedServices)
                    {
                        if (requiredService == providedService)
                        {
                            // this required service is being provided by the component we want to remove
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    bool CanComponentServicesBeRemoved(const AZ::Entity::ComponentArrayType& componentsToRemove)
    {
        // Group components by entityId
        AZStd::unordered_map<AZ::EntityId, AZ::Entity::ComponentArrayType> m_componentsByEntityId;
        for (auto component : componentsToRemove)
        {
            m_componentsByEntityId[component->GetEntityId()].push_back(component);
        }

        for (auto componentsByEntityIdItr : m_componentsByEntityId)
        {
            const AZ::EntityId& entityId = componentsByEntityIdItr.first;
            const AZ::Entity::ComponentArrayType& componentsToRemoveFromEntity = componentsByEntityIdItr.second;
            if (!CanComponentServicesBeRemovedFromEntity(componentsToRemoveFromEntity, entityId))
            {
                return false;
            }
        }

        return true;
    }

    bool AreComponentsAddableByUser(const AZ::Entity::ComponentArrayType& components)
    {
        // Get the serialize context.
        AZ::SerializeContext* serializeContext = GetSerializeContext();

        for (auto component : components)
        {
            const AZ::Uuid& componentToAddTypeId = AzToolsFramework::GetUnderlyingComponentType(*component);
            const AZ::SerializeContext::ClassData* componentClassData = serializeContext->FindClassData(componentToAddTypeId);
            if (!componentClassData)
            {
                AZ_Error("UI Editor", false, "Can't find class data for class Id %s", componentToAddTypeId.ToString<AZStd::string>().c_str());
                return false;
            }

            if (!IsAddableByUser(*componentClassData))
            {
                return false;
            }
        }

        return true;
    }

    bool CanComponentsBeRemoved(const AZ::Entity::ComponentArrayType& componentsToRemove)
    {
        if (!AreComponentsAddableByUser(componentsToRemove))
        {
            return false;
        }

        return CanComponentServicesBeRemoved(componentsToRemove);
    }

    bool CanPasteComponentsToEntities(const AzToolsFramework::EntityIdList& entities, bool isCanvasEntity)
    {
        const QMimeData* mimeData = AzToolsFramework::ComponentMimeData::GetComponentMimeDataFromClipboard();

        // Check that there are components on the clipboard and that they can all be pasted onto the entities
        bool canPasteAll = true;
        if (entities.empty() || !mimeData)
        {
            canPasteAll = false;
        }
        else
        {
            // Create class data from mime data
            AzToolsFramework::ComponentTypeMimeData::ClassDataContainer classDataForComponentsToAdd;
            AzToolsFramework::ComponentTypeMimeData::Get(mimeData, classDataForComponentsToAdd);

            canPasteAll = CanAddComponentsToEntities(classDataForComponentsToAdd, entities, isCanvasEntity);
        }

        return canPasteAll;
    }

    AzToolsFramework::EntityIdList GetSelectedEntities(bool* isCanvasSelectedOut = nullptr)
    {
        AzToolsFramework::EntityIdList selectedEntities;
        UiEditorInternalRequestBus::BroadcastResult(selectedEntities, &UiEditorInternalRequestBus::Events::GetSelectedEntityIds);

        if (isCanvasSelectedOut)
        {
            *isCanvasSelectedOut = false;
        }
        if (selectedEntities.empty())
        {
            AZ::EntityId canvasEntityId;
            UiEditorInternalRequestBus::BroadcastResult(canvasEntityId, &UiEditorInternalRequestBus::Events::GetActiveCanvasEntityId);
            selectedEntities.push_back(canvasEntityId);
            if (isCanvasSelectedOut)
            {
                *isCanvasSelectedOut = true;
            }
        }

        return selectedEntities;
    }

    AZ::Entity::ComponentArrayType GetCopyableComponents(const AZ::Entity::ComponentArrayType& componentsToCopy)
    {
        AzToolsFramework::EntityIdList selectedEntities = GetSelectedEntities();
        AZ::EntityId firstSelectedEntity = selectedEntities.front();

        // Copyable components are the components that belong to the first selected entity
        AZ::Entity::ComponentArrayType copyableComponents;
        copyableComponents.reserve(componentsToCopy.size());
        for (auto component : componentsToCopy)
        {
            const AZ::EntityId entityId = component ? component->GetEntityId() : AZ::EntityId();
            if (entityId == firstSelectedEntity)
            {
                copyableComponents.push_back(component);
            }
        }

        return copyableComponents;
    }

    void HandleSelectedEntitiesPropertiesChanged()
    {
        UiEditorInternalNotificationBus::Broadcast(&UiEditorInternalNotificationBus::Events::OnSelectedEntitiesPropertyChanged);
    }

    void RemoveComponents(const AZ::Entity::ComponentArrayType& componentsToRemove)
    {
        // Group components by entityId
        AZStd::unordered_map<AZ::EntityId, AZ::Entity::ComponentArrayType> m_componentsByEntityId;
        for (auto component : componentsToRemove)
        {
            m_componentsByEntityId[component->GetEntityId()].push_back(component);
        }

        // Since the undo commands use the selected entities, make sure that the components being removed belong to selected entities
        AzToolsFramework::EntityIdList selectedEntities = GetSelectedEntities();
        bool foundUnselectedEntities = false;
        for (auto componentsByEntityIdItr : m_componentsByEntityId)
        {
            const AZ::EntityId& entityId = componentsByEntityIdItr.first;
            if (!entityId.IsValid() || AZStd::find(selectedEntities.begin(), selectedEntities.end(), entityId) == selectedEntities.end())
            {
                foundUnselectedEntities = true;
                break;
            }
        }
        if (foundUnselectedEntities)
        {
            AZ_Error("UI Editor", false, "Attempting to remove components from an unselected element.");
            return;
        }

        UiEditorInternalNotificationBus::Broadcast(&UiEditorInternalNotificationBus::Events::OnBeginUndoableEntitiesChange);

        for (auto componentsByEntityIdItr : m_componentsByEntityId)
        {
            const AZ::EntityId& entityId = componentsByEntityIdItr.first;
            const AZ::Entity::ComponentArrayType& componentsToRemoveFromEntity = componentsByEntityIdItr.second;

            AZ::Entity* entity = AzToolsFramework::GetEntityById(entityId);
            if (!entity)
            {
                AZ_Error("UI Editor", false, "Can't find entity by Id.");
                continue;
            }

            // We must deactivate the entity to remove components
            bool reactivate = false;
            if (entity->GetState() == AZ::Entity::State::Active)
            {
                reactivate = true;
                entity->Deactivate();
            }

            // Remove all the components requested
            for (auto componentToRemove : componentsToRemoveFromEntity)
            {
                // See if the component is on the entity
                const auto& entityComponents = entity->GetComponents();
                if (AZStd::find(entityComponents.begin(), entityComponents.end(), componentToRemove) != entityComponents.end())
                {
                    entity->RemoveComponent(componentToRemove);

                    delete componentToRemove;
                }
            }

            // Reactivate if we were previously active
            if (reactivate)
            {
                entity->Activate();
            }
        }

        UiEditorInternalNotificationBus::Broadcast(
            &UiEditorInternalNotificationBus::Events::OnEndUndoableEntitiesChange,
            componentsToRemove.size() > 1 ? "delete components" : "delete component");

        HandleSelectedEntitiesPropertiesChanged();
    }

    void CopyComponents(const AZ::Entity::ComponentArrayType& copyableComponents)
    {
        // Create the mime data object
        AZStd::unique_ptr<QMimeData> mimeData = AzToolsFramework::ComponentMimeData::Create(copyableComponents);

        // Put it on the clipboard
        AzToolsFramework::ComponentMimeData::PutComponentMimeDataOnClipboard(AZStd::move(mimeData));
    }

    void AddComponentsWithAssetToEntities(const ComponentAssetHelpers::ComponentAssetPairs& componentAssetPairs, const AzToolsFramework::EntityIdList& entities)
    {
        for (const AZ::EntityId& entityId : entities)
        {
            ComponentHelpers::AddComponentsWithAssetToEntity(componentAssetPairs, entityId);
        }
    }

    bool GetClassDataForComponentsFromType(const AZStd::vector<AZ::TypeId>& componentTypes,
        AzToolsFramework::ClassDataList& classDataForComponentsToAdd)
    {
        // Make a list of component class data from the component types
        classDataForComponentsToAdd.reserve(componentTypes.size());
        AZ::SerializeContext* serializeContext = GetSerializeContext();
        for (const AZ::TypeId& componentType : componentTypes)
        {
            const AZ::SerializeContext::ClassData* componentClassData = serializeContext->FindClassData(componentType);
            if (!componentClassData)
            {
                AZ_Error("UI Editor", false, "Can't find class data for class Id %s", componentType.ToString<AZStd::string>().c_str());
                return false;
            }
            classDataForComponentsToAdd.push_back(componentClassData);
        }

        return true;
    }
}   // namespace

namespace ComponentHelpers
{
    QList<QAction*> CreateAddComponentActions(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        QWidget* parent)
    {
        HierarchyItemRawPtrList items = SelectionHelpers::GetSelectedHierarchyItems(hierarchy,
            selectedItems);

        AzToolsFramework::EntityIdList entitiesSelected;

        bool isCanvasSelected = false;
        if (selectedItems.empty())
        {
            isCanvasSelected = true;
            entitiesSelected.push_back(hierarchy->GetEditorWindow()->GetCanvas());
        }
        else
        {
            for (auto item : items)
            {
                entitiesSelected.push_back(item->GetEntityId());
            }
        }

        using ComponentsList = AZStd::vector < const AZ::SerializeContext::ClassData* >;
        ComponentsList componentsList;

        // Get the serialize context.
        AZ::SerializeContext* serializeContext = Internal::GetSerializeContext();

        // Gather all components that match our filter and group by category.
        serializeContext->EnumerateDerived<AZ::Component>(
            [&componentsList, isCanvasSelected](const AZ::SerializeContext::ClassData* componentClassData, const AZ::Uuid& knownType) -> bool
        {
            (void)knownType;

            if (Internal::AppearsInUIComponentMenu(*componentClassData, isCanvasSelected))
            {
                if (!Internal::IsAddableByUser(*componentClassData))
                {
                    return true;
                }

                componentsList.push_back(componentClassData);
            }

            return true;
        }
        );

        // Create a component list that is in the same order that the components were registered in
        const AZStd::vector<AZ::Uuid>* componentOrderList;
        ComponentsList orderedComponentsList;
        UiSystemBus::BroadcastResult(componentOrderList, &UiSystemBus::Events::GetComponentTypesForMenuOrdering);
        for (auto& componentType : *componentOrderList)
        {
            auto iter = AZStd::find_if(componentsList.begin(), componentsList.end(),
                [componentType](const AZ::SerializeContext::ClassData* componentClassData) -> bool
            {
                return (componentClassData->m_typeId == componentType);
            }
            );

            if (iter != componentsList.end())
            {
                orderedComponentsList.push_back(*iter);
                componentsList.erase(iter);
            }
        }
        // add any remaining component desciptiors to the end of the ordered list
        // (just to catch any component types that were not registered for ordering)
        for (auto componentClass : componentsList)
        {
            orderedComponentsList.push_back(componentClass);
        }

        QList<QAction*> result;
        {
            // Add an action for each factory.
            for (auto componentClass : orderedComponentsList)
            {
                const char* typeName = componentClass->m_editData->m_name;

                AZStd::string iconPath = Internal::GetComponentIconPath(*componentClass);

                QString iconUrl = QString(iconPath.c_str());

                bool isEnabled = false;
                if (items.empty())
                {
                    AZ::EntityId canvasEntityId = hierarchy->GetEditorWindow()->GetCanvas();
                    isEnabled = Internal::IsComponentServiceCompatibleWithEntity(componentClass->m_typeId, canvasEntityId);
                }
                else
                {
                    for (auto i : items)
                    {
                        AZ::EntityId entityId = i->GetEntityId();
                        if (Internal::IsComponentServiceCompatibleWithEntity(componentClass->m_typeId, entityId))
                        {
                            isEnabled = true;

                            // Stop iterating thru the items
                            // and go to the next factory.
                            break;
                        }
                    }
                }
                QAction* action = new QAction(QIcon(iconUrl), typeName, parent);
                action->setEnabled(isEnabled);
                QObject::connect(action,
                    &QAction::triggered,
                    hierarchy,
                    [hierarchy, componentClass, items]([[maybe_unused]] bool checked)
                {
                    UiEditorInternalNotificationBus::Broadcast(&UiEditorInternalNotificationBus::Events::OnBeginUndoableEntitiesChange);

                    AzToolsFramework::EntityIdList entitiesSelected;
                    if (items.empty())
                    {
                        entitiesSelected.push_back(hierarchy->GetEditorWindow()->GetCanvas());
                    }
                    else
                    {
                        for (auto item : items)
                        {
                            entitiesSelected.push_back(item->GetEntityId());
                        }
                    }

                    for (auto entityId : entitiesSelected)
                    {
                        if (Internal::IsComponentServiceCompatibleWithEntity(componentClass->m_typeId, entityId))
                        {
                            AZ::Entity* entity = AzToolsFramework::GetEntityById(entityId);
                            if (!entity)
                            {
                                AZ_Error("UI Editor", false, "Can't find entity by Id.");
                                continue;
                            }

                            entity->Deactivate();
                            AZ::Component* component;
                            AZ::ComponentDescriptorBus::EventResult(
                                component, componentClass->m_typeId, &AZ::ComponentDescriptorBus::Events::CreateComponent);
                            entity->AddComponent(component);
                            entity->Activate();
                        }
                    }

                    UiEditorInternalNotificationBus::Broadcast(
                        &UiEditorInternalNotificationBus::Events::OnEndUndoableEntitiesChange, "add component");

                    Internal::HandleSelectedEntitiesPropertiesChanged();
                });

                result.push_back(action);
            }
        }

        return result;
    }

    QAction* CreateRemoveComponentsAction(QWidget* parent)
    {
        QAction* action = new QAction("Delete component", parent);
        action->setShortcut(QKeySequence::Delete);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            []()
        {
            AZ::Entity::ComponentArrayType componentsToRemove;
            UiEditorInternalRequestBus::BroadcastResult(componentsToRemove, &UiEditorInternalRequestBus::Events::GetSelectedComponents);

            Internal::RemoveComponents(componentsToRemove);

            Internal::HandleSelectedEntitiesPropertiesChanged();
        });

        return action;
    }

    void UpdateRemoveComponentsAction(QAction* action)
    {
        AZ::Entity::ComponentArrayType componentsToRemove;
        UiEditorInternalRequestBus::BroadcastResult(componentsToRemove, &UiEditorInternalRequestBus::Events::GetSelectedComponents);

        action->setText(componentsToRemove.size() > 1 ? "Delete components" : "Delete component");

        // Check if we can remove every component from every element
        bool canRemove = true;
        if (componentsToRemove.empty() || !Internal::CanComponentsBeRemoved(componentsToRemove))
        {
            canRemove = false;
        }

        // Disable the action if not every element can remove the component
        action->setEnabled(canRemove);
    }

    QAction* CreateCutComponentsAction(QWidget* parent)
    {
        QAction* action = new QAction("Cut component", parent);
        action->setShortcut(QKeySequence::Cut);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            []()
        {
            AZ::Entity::ComponentArrayType componentsToCut;
            UiEditorInternalRequestBus::BroadcastResult(componentsToCut, &UiEditorInternalRequestBus::Events::GetSelectedComponents);

            AZ::Entity::ComponentArrayType copyableComponents = Internal::GetCopyableComponents(componentsToCut);

            // Copy components
            Internal::CopyComponents(copyableComponents);
            // Delete components
            Internal::RemoveComponents(componentsToCut);
        });

        return action;
    }

    void UpdateCutComponentsAction(QAction* action)
    {
        AZ::Entity::ComponentArrayType componentsToCut;
        UiEditorInternalRequestBus::BroadcastResult(componentsToCut, &UiEditorInternalRequestBus::Events::GetSelectedComponents);

        AZ::Entity::ComponentArrayType copyableComponents = Internal::GetCopyableComponents(componentsToCut);

        action->setText(componentsToCut.size() > 1 ? "Cut components" : "Cut component");

        // Check that all components can be deleted and that all copyable components can be pasted
        bool canCut = true;
        if (copyableComponents.empty()
            || componentsToCut.empty()
            || !Internal::AreComponentsAddableByUser(copyableComponents)
            || !Internal::CanComponentsBeRemoved(componentsToCut))
        {
            canCut = false;
        }

        // Disable the action if not every component can be deleted or every copyable components pasted
        action->setEnabled(canCut);
    }

    QAction* CreateCopyComponentsAction(QWidget* parent)
    {
        QAction* action = new QAction("Copy component", parent);
        action->setShortcut(QKeySequence::Copy);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            []()
        {
            AZ::Entity::ComponentArrayType componentsToCopy;
            UiEditorInternalRequestBus::BroadcastResult(componentsToCopy, &UiEditorInternalRequestBus::Events::GetSelectedComponents);

            // Get the components of the first selected elements to copy onto the clipboard
            AZ::Entity::ComponentArrayType copyableComponents = Internal::GetCopyableComponents(componentsToCopy);

            Internal::CopyComponents(copyableComponents);
        });

        return action;
    }

    void UpdateCopyComponentsAction(QAction* action)
    {
        AZ::Entity::ComponentArrayType componentsToCopy;
        UiEditorInternalRequestBus::BroadcastResult(componentsToCopy, &UiEditorInternalRequestBus::Events::GetSelectedComponents);

        // Get the components of the first selected elements to copy onto the clipboard
        AZ::Entity::ComponentArrayType copyableComponents = Internal::GetCopyableComponents(componentsToCopy);

        action->setText(copyableComponents.size() > 1 ? "Copy components" : "Copy component");

        // Check that all copyable components can be added by the user
        bool canCopy = true;
        if (copyableComponents.empty() || !Internal::AreComponentsAddableByUser(copyableComponents))
        {
            canCopy = false;
        }

        // Disable the action if not all copyable components can be added to all elements
        action->setEnabled(canCopy);
    }

    QAction* CreatePasteComponentsAction(QWidget* parent)
    {
        QAction* action = new QAction("Paste component", parent);
        action->setShortcut(QKeySequence::Paste);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            []()
        {
            bool isCanvasSelected = false;
            AzToolsFramework::EntityIdList selectedEntities = Internal::GetSelectedEntities(&isCanvasSelected);

            if (Internal::CanPasteComponentsToEntities(selectedEntities, isCanvasSelected))
            {
                // Create components from mime data
                const QMimeData* mimeData = AzToolsFramework::ComponentMimeData::GetComponentMimeDataFromClipboard();
                AzToolsFramework::ComponentMimeData::ComponentDataContainer componentsToAdd;
                AzToolsFramework::ComponentMimeData::GetComponentDataFromMimeData(mimeData, componentsToAdd);

                // Create class data from mime data
                AzToolsFramework::ComponentTypeMimeData::ClassDataContainer classDataForComponentsToAdd;
                AzToolsFramework::ComponentTypeMimeData::Get(mimeData, classDataForComponentsToAdd);
                AZ_Error("UI Editor", componentsToAdd.size() == classDataForComponentsToAdd.size(), "Component mime data's components list size is different from class data list size.");
                if (componentsToAdd.size() == classDataForComponentsToAdd.size())
                {
                    UiEditorInternalNotificationBus::Broadcast(&UiEditorInternalNotificationBus::Events::OnBeginUndoableEntitiesChange);

                    // Paste to all selected entities
                    for (const AZ::EntityId& entityId : selectedEntities)
                    {
                        AZ::Entity* entity = AzToolsFramework::GetEntityById(entityId);
                        // De-serialize from mime data each time the component is pasted, otherwise the same component pointer could be added to multiple entities.
                        AzToolsFramework::ComponentMimeData::GetComponentDataFromMimeData(mimeData, componentsToAdd);
                        if (!entity)
                        {
                            AZ_Error("UI Editor", false, "Can't find entity by Id.");
                            continue;
                        }

                        // We must deactivate the entity to add components
                        bool reactivate = false;
                        if (entity->GetState() == AZ::Entity::State::Active)
                        {
                            reactivate = true;
                            entity->Deactivate();
                        }

                        // Add components
                        for (int componentIndex = 0; componentIndex < componentsToAdd.size(); ++componentIndex)
                        {
                            AZ::Component* component = componentsToAdd[componentIndex];
                            if (!component)
                            {
                                AZ_Error("UI Editor", false, "Null component provided by mime data.");
                                continue;
                            }

                            // Add the component
                            entity->AddComponent(component);
                        }

                        // Reactivate if we were previously active
                        if (reactivate)
                        {
                            entity->Activate();
                        }
                    }

                    UiEditorInternalNotificationBus::Broadcast(
                        &UiEditorInternalNotificationBus::Events::OnEndUndoableEntitiesChange, "paste component");

                    Internal::HandleSelectedEntitiesPropertiesChanged();
                }
            }
        });

        return action;
    }

    void UpdatePasteComponentsAction(QAction* action)
    {
        const QMimeData* mimeData = AzToolsFramework::ComponentMimeData::GetComponentMimeDataFromClipboard();
        AzToolsFramework::ComponentTypeMimeData::ClassDataContainer classDataForComponentsToAdd;
        AzToolsFramework::ComponentTypeMimeData::Get(mimeData, classDataForComponentsToAdd);

        action->setText(classDataForComponentsToAdd.size() > 1 ? "Paste components" : "Paste component");

        bool isCanvasSelected = false;
        AzToolsFramework::EntityIdList selectedEntities = Internal::GetSelectedEntities(&isCanvasSelected);

        // Check that there are components on the clipboard and that they can all be pasted onto the selected elements
        bool canPasteAll = Internal::CanPasteComponentsToEntities(selectedEntities, isCanvasSelected);

        // Disable the action if not every component can be pasted onto every element
        action->setEnabled(canPasteAll);
    }

    bool CanAddComponentsToSelectedEntities(const AZStd::vector<AZ::TypeId>& componentTypes, EntityComponentPair* firstIncompatibleComponentType)
    {
        bool isCanvasSelected = false;
        AzToolsFramework::EntityIdList selectedEntities = Internal::GetSelectedEntities(&isCanvasSelected);
        if (selectedEntities.empty())
        {
            return false;
        }

        // Make a list of component class data for all the components to add
        AzToolsFramework::ClassDataList classDataForComponentsToAdd;
        if (!Internal::GetClassDataForComponentsFromType(componentTypes, classDataForComponentsToAdd))
        {
            return false;
        }

        return Internal::CanAddComponentsToEntities(classDataForComponentsToAdd, selectedEntities, isCanvasSelected, firstIncompatibleComponentType);
    }

    void AddComponentsWithAssetToSelectedEntities(const ComponentAssetHelpers::ComponentAssetPairs& componentAssetPairs)
    {
        UiEditorInternalNotificationBus::Broadcast(&UiEditorInternalNotificationBus::Events::OnBeginUndoableEntitiesChange);

        AzToolsFramework::EntityIdList selectedEntities = Internal::GetSelectedEntities();
        Internal::AddComponentsWithAssetToEntities(componentAssetPairs, selectedEntities);

        UiEditorInternalNotificationBus::Broadcast(&UiEditorInternalNotificationBus::Events::OnEndUndoableEntitiesChange, "add component");

        Internal::HandleSelectedEntitiesPropertiesChanged();
    }

    bool CanAddComponentsToEntity(const AZStd::vector<AZ::TypeId>& componentTypes,
        const AZ::EntityId& entityId,
        EntityComponentPair* firstIncompatibleComponentType)
    {
        // Make a list of component class data for all the components to add
        AzToolsFramework::ClassDataList classDataForComponentsToAdd;
        if (!Internal::GetClassDataForComponentsFromType(componentTypes, classDataForComponentsToAdd))
        {
            return false;
        }

        AzToolsFramework::EntityIdList entities;
        entities.push_back(entityId);
        bool isCanvasEntity = (UiCanvasBus::FindFirstHandler(entityId));
        return Internal::CanAddComponentsToEntities(classDataForComponentsToAdd, entities, isCanvasEntity, firstIncompatibleComponentType);
    }

    void AddComponentsWithAssetToEntity(const ComponentAssetHelpers::ComponentAssetPairs& componentAssetPairs, const AZ::EntityId& entityId)
    {
        if (!entityId.IsValid())
        {
            AZ_Error("UI Editor", false, "Attempting to add components to an invalid entityId.");
            return;
        }

        if (componentAssetPairs.empty())
        {
            AZ_Error("UI Editor", false, "Attempting to add an empty list of components to and entity.");
            return;
        }

        AZ::Entity* entity = AzToolsFramework::GetEntityById(entityId);
        if (!entity)
        {
            AZ_Error("UI Editor", false, "Can't find entity by Id.");
            return;
        }

        // We must deactivate the entity to add components
        bool reactivate = false;
        if (entity->GetState() == AZ::Entity::State::Active)
        {
            reactivate = true;
            entity->Deactivate();
        }

        // Add all components and remember the assets that will be assigned to them after the element is reactivated
        AZStd::vector<AZStd::pair<AZ::Component*, AZ::Data::AssetId>> newComponentAssetPairs;
        for (const ComponentAssetHelpers::ComponentAssetPair& pair : componentAssetPairs)
        {
            const AZ::TypeId& componentType = pair.first;
            const AZ::Data::AssetId& assetId = pair.second;
            
            AZ::Component* component;
            AZ::ComponentDescriptorBus::EventResult(component, componentType, &AZ::ComponentDescriptorBus::Events::CreateComponent);
            entity->AddComponent(component);

            newComponentAssetPairs.push_back(AZStd::make_pair(component, assetId));
        }

        // Reactivate if we were previously active
        if (reactivate)
        {
            entity->Activate();
        }

        // Assign assets to components after the entity has been reactivated
        for (AZStd::pair<AZ::Component*, AZ::Data::AssetId>& pair : newComponentAssetPairs)
        {
            AZ::Component* component = pair.first;
            const AZ::Data::AssetId& assetId = pair.second;

            auto editorComponent = Internal::GetEditorComponent(component);
            if (editorComponent)
            {
                editorComponent->SetPrimaryAsset(assetId);
            }
        }
    }

    AZStd::vector<ComponentTypeData> GetAllComponentTypesThatCanAppearInAddComponentMenu()
    {
        using ComponentsList = AZStd::vector<ComponentTypeData>;
        ComponentsList componentsList;

        // Get the serialize context.
        AZ::SerializeContext* serializeContext = Internal::GetSerializeContext();

        const AZStd::list<AZ::ComponentDescriptor*>* lyShineComponentDescriptors;
        UiSystemBus::BroadcastResult(lyShineComponentDescriptors, &UiSystemBus::Events::GetLyShineComponentDescriptors);

        // Gather all components that match our filter and group by category.
        serializeContext->EnumerateDerived<AZ::Component>(
            [ &componentsList, lyShineComponentDescriptors ](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& knownType) -> bool
            {
                (void)knownType;

                if (Internal::AppearsInUIComponentMenu(*classData, false))
                {
                    if (!Internal::IsAddableByUser(*classData))
                    {
                        // skip this component (return early) if user is not allowed to add it directly
                        return true;
                    }

                    bool isLyShineComponent = false;
                    for (auto descriptor : * lyShineComponentDescriptors)
                    {
                        if (descriptor->GetUuid() == classData->m_typeId)
                        {
                            isLyShineComponent = true;
                            break;
                        }
                    }
                    ComponentTypeData data;
                    data.classData = classData;
                    data.isLyShineComponent = isLyShineComponent;
                    componentsList.push_back(data);
                }

                return true;
            }
            );

        return componentsList;
    }
}   // namespace ComponentHelpers
