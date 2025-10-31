/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Component/EditorComponentAPIComponent.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>

#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/Entity/EditorEntityActionComponent.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>


namespace AzToolsFramework
{
    namespace Components
    {
        //! Helper class for scripting.
        //! Use it to provide the right enum value when
        //! calling EditorComponentAPIRequests::BuildComponentTypeNameListByEntityType or EditorComponentAPIRequests::FindComponentTypeIdsByEntityType
        //! example of Python code:
        //! #-------------------------------------------------------------------
        //! import azlmbr.bus as bus
        //! import azlmbr.editor as editor
        //! from azlmbr.entity import EntityType
        //! levelComponentsList = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentTypeNameListByEntityType', EntityType().Level)
        //! gameComponentsList = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentTypeNameListByEntityType', EntityType().Game)
        //! #-------------------------------------------------------------------
        class EditorEntityType final
        {
        public:
            AZ_CLASS_ALLOCATOR(EditorEntityType, AZ::SystemAllocator);
            AZ_RTTI(EditorEntityType, "{9761CD58-D86E-4EA1-AE67-5302AECD54A4}");

            EditorEntityType() = default;
            ~EditorEntityType() = default;

            static void ReflectContext(AZ::ReflectContext* context)
            {
                if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
                {
                    behaviorContext->Class<EditorEntityType>("EntityType")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Category, "Components")
                        ->Attribute(AZ::Script::Attributes::Module, "entity")
                        ->Constructor()
                        ->Constant("Game", &EditorEntityType::Game)
                        ->Constant("System", &EditorEntityType::System)
                        ->Constant("Layer", &EditorEntityType::Layer)
                        ->Constant("Level", &EditorEntityType::Level)
                        ;
                }
            }

            EditorComponentAPIRequests::EntityType Game() { return EditorComponentAPIRequests::EntityType::Game; }
            EditorComponentAPIRequests::EntityType System() { return EditorComponentAPIRequests::EntityType::System; }
            EditorComponentAPIRequests::EntityType Layer() { return EditorComponentAPIRequests::EntityType::Layer; }
            EditorComponentAPIRequests::EntityType Level() { return EditorComponentAPIRequests::EntityType::Level; }
        };

        void EditorComponentAPIComponent::Reflect(AZ::ReflectContext* context)
        {
            EditorEntityType::ReflectContext(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorComponentAPIComponent, AZ::Component>();

                serializeContext->RegisterGenericType<AZStd::vector<AZ::EntityComponentIdPair>>();
                serializeContext->RegisterGenericType<AZStd::vector<AZ::ComponentServiceType>>();
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<AZ::EntityComponentIdPair>("EntityComponentIdPair")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Components")
                    ->Attribute(AZ::Script::Attributes::Module, "entity")
                    ->Method("GetEntityId", &AZ::EntityComponentIdPair::GetEntityId)
                        ->Attribute(AZ::Script::Attributes::Alias, "get_entity_id")
                    ->Method("Equal", &AZ::EntityComponentIdPair::operator==)
                        ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                    ->Method("ToString", [](const AZ::EntityComponentIdPair* self) {
                        return AZStd::string::format("[ %s - %s ]", self->GetEntityId().ToString().c_str(), AZStd::to_string(self->GetComponentId()).c_str());
                    })
                        ->Attribute(AZ::Script::Attributes::Alias, "to_string")
                    ;

                behaviorContext->EBus<EditorComponentAPIBus>("EditorComponentAPIBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Components")
                    ->Attribute(AZ::Script::Attributes::Module, "editor")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Event("FindComponentTypeIdsByEntityType", &EditorComponentAPIRequests::FindComponentTypeIdsByEntityType)
                    ->Event("FindComponentTypeIdsByService", &EditorComponentAPIRequests::FindComponentTypeIdsByService)
                    ->Event("FindComponentTypeNames", &EditorComponentAPIRequests::FindComponentTypeNames)
                    ->Event("BuildComponentTypeNameListByEntityType", &EditorComponentAPIRequests::BuildComponentTypeNameListByEntityType)
                    ->Event("AddComponentsOfType", &EditorComponentAPIRequests::AddComponentsOfType)
                    ->Event("AddComponentOfType", &EditorComponentAPIRequests::AddComponentOfType)
                    ->Event("HasComponentOfType", &EditorComponentAPIRequests::HasComponentOfType)
                    ->Event("CountComponentsOfType", &EditorComponentAPIRequests::CountComponentsOfType)
                    ->Event("GetComponentOfType", &EditorComponentAPIRequests::GetComponentOfType)
                    ->Event("GetComponentsOfType", &EditorComponentAPIRequests::GetComponentsOfType)
                    ->Event("IsValid", &EditorComponentAPIRequests::IsValid)
                    ->Event("EnableComponents", &EditorComponentAPIRequests::EnableComponents)
                    ->Event("IsComponentEnabled", &EditorComponentAPIRequests::IsComponentEnabled)
                    ->Event("DisableComponents", &EditorComponentAPIRequests::DisableComponents)
                    ->Event("RemoveComponents", &EditorComponentAPIRequests::RemoveComponents)
                    ->Event("BuildComponentPropertyTreeEditor", &EditorComponentAPIRequests::BuildComponentPropertyTreeEditor)
                    ->Event("GetComponentProperty", &EditorComponentAPIRequests::GetComponentProperty)
                    ->Event("SetComponentProperty", &EditorComponentAPIRequests::SetComponentProperty)
                    ->Event("CompareComponentProperty", &EditorComponentAPIRequests::CompareComponentProperty)
                    ->Event("BuildComponentPropertyList", &EditorComponentAPIRequests::BuildComponentPropertyList)
                    ->Event("SetVisibleEnforcement", &EditorComponentAPIRequests::SetVisibleEnforcement)
                    ;
            }
        }

        void EditorComponentAPIComponent::Activate()
        {
            EditorComponentAPIBus::Handler::BusConnect();

            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Error("Editor", m_serializeContext, "Serialize context not available");
        }

        void EditorComponentAPIComponent::Deactivate()
        {
            EditorComponentAPIBus::Handler::BusDisconnect();
        }

        void EditorComponentAPIComponent::SetVisibleEnforcement(bool enforceVisiblity)
        {
            m_usePropertyVisibility = enforceVisiblity;
        }

        AZStd::vector<AZ::Uuid> EditorComponentAPIComponent::FindComponentTypeIdsByEntityType(const AZStd::vector<AZStd::string>& componentTypeNames, EditorComponentAPIRequests::EntityType entityType)
        {
            AZStd::vector<AZ::Uuid> foundTypeIds;

            size_t typesCount = componentTypeNames.size();
            size_t counter = 0;

            foundTypeIds.resize(typesCount, AZ::Uuid::CreateNull());

            m_serializeContext->EnumerateDerived<AZ::Component>(
                [&counter, typesCount, componentTypeNames, &foundTypeIds, entityType](const AZ::SerializeContext::ClassData* componentClass, const AZ::Uuid& knownType) -> bool
                {
                    (void)knownType;

                    if (componentClass->m_editData)
                    {
                        switch (entityType)
                        {
                        case EditorComponentAPIRequests::EntityType::Game:
                            if (!AzToolsFramework::AppearsInGameComponentMenu(*componentClass))
                            {
                                return true;
                            }
                            break;
                        case EditorComponentAPIRequests::EntityType::Level:
                            if (!AzToolsFramework::AppearsInLevelComponentMenu(*componentClass))
                            {
                                return true;
                            }
                            break;
                        case EditorComponentAPIRequests::EntityType::Layer:
                            if (!AzToolsFramework::AppearsInLayerComponentMenu(*componentClass))
                            {
                                return true;
                            }
                            break;
                        default:
                            break;
                        }
                        for (int i = 0; i < typesCount; ++i)
                        {
                            if (componentClass->m_editData->m_name == componentTypeNames[i])
                            {
                                // Although it is rare, it can happen that two (or more) components can have the same name.
                                // We should only count the first occurrence, so that none of the names in componentTypeNames
                                // get skipped, but whichever component type that is encountered last will be the one
                                // that is captured in order to preserve the pre-existing behavior.
                                if (foundTypeIds[i].IsNull())
                                {
                                    ++counter;
                                }

                                foundTypeIds[i] = componentClass->m_typeId;

                                return true;
                            }
                        }

                        if (counter >= typesCount)
                        {
                            return false;
                        }
                    }

                    return true;
                });

            AZ_Warning("EditorComponentAPI", (counter >= typesCount), "FindComponentTypeIds - Not all Type Names provided could be converted to Type Ids.");

            return foundTypeIds;
        }

        AZStd::vector<AZ::Uuid> EditorComponentAPIComponent::FindComponentTypeIdsByService(AZStd::span<const AZ::ComponentServiceType> serviceFilter, const AZStd::vector<AZ::ComponentServiceType>& incompatibleServiceFilter)
        {
            AZStd::vector<AZ::Uuid> foundTypeIds;

            m_serializeContext->EnumerateDerived<AZ::Component>(
                [&foundTypeIds, serviceFilter, incompatibleServiceFilter](const AZ::SerializeContext::ClassData* componentClass, const AZ::Uuid& knownType) -> bool
            {
                AZ_UNUSED(knownType);

                if (componentClass->m_editData)
                {
                    // If none of the required services are offered by this component, or the component
                    // can not be added by the user, skip to the next component
                    if (!OffersRequiredServices(componentClass, serviceFilter, incompatibleServiceFilter))
                    {
                        return true;
                    }

                    foundTypeIds.push_back(componentClass->m_typeId);
                }

                return true;
            });

            return foundTypeIds;
        }

        AZStd::vector<AZStd::string> EditorComponentAPIComponent::FindComponentTypeNames(const AZ::ComponentTypeList& componentTypeIds)
        {
            AZStd::vector<AZStd::string> foundTypeNames;

            size_t typesCount = componentTypeIds.size();
            size_t counter = 0;

            foundTypeNames.resize(typesCount);

            m_serializeContext->EnumerateDerived<AZ::Component>(
                [&counter, typesCount, componentTypeIds, &foundTypeNames](const AZ::SerializeContext::ClassData* componentClass, const AZ::Uuid& knownType) -> bool
            {
                (void)knownType;

                if (componentClass->m_editData)
                {
                    for (int i = 0; i < typesCount; ++i)
                    {
                        if (componentClass->m_typeId == componentTypeIds[i])
                        {
                            foundTypeNames[i] = componentClass->m_editData->m_name;
                            ++counter;
                        }
                    }

                    if (counter >= typesCount)
                    {
                        return false;
                    }
                }

                return true;
            });

            AZ_Warning("EditorComponentAPI", (counter >= typesCount), "FindComponentTypeNames - Not all Type Ids provided could be converted to Type Names.");

            return foundTypeNames;
        }

        AZStd::vector<AZStd::string> EditorComponentAPIComponent::BuildComponentTypeNameListByEntityType(EditorComponentAPIRequests::EntityType entityType)
        {
            AZStd::vector<AZStd::string> typeNameList;

            m_serializeContext->EnumerateDerived<AZ::Component>(
                [&typeNameList, entityType](const AZ::SerializeContext::ClassData* componentClass, const AZ::Uuid& knownType) -> bool
                {
                    AZ_UNUSED(knownType);

                        if (!componentClass->m_editData)
                        {
                            return true;
                        }

                    switch (entityType)
                    {
                    case EditorComponentAPIRequests::EntityType::Game:
                        if (AzToolsFramework::AppearsInGameComponentMenu(*componentClass))
                        {
                            typeNameList.push_back(componentClass->m_editData->m_name);
                        }
                        break;
                    case EditorComponentAPIRequests::EntityType::Level:
                        if (AzToolsFramework::AppearsInLevelComponentMenu(*componentClass))
                        {
                            typeNameList.push_back(componentClass->m_editData->m_name);
                        }
                        break;
                    case EditorComponentAPIRequests::EntityType::Layer:
                        if (AzToolsFramework::AppearsInLayerComponentMenu(*componentClass))
                        {
                            typeNameList.push_back(componentClass->m_editData->m_name);
                        }
                        break;
                    default:
                        break;
                    }

                    return true;
                });

            return typeNameList;
        }

        // Returns an Outcome object with the Component Id if successful, and the cause of the failure otherwise
        EditorComponentAPIRequests::AddComponentsOutcome EditorComponentAPIComponent::AddComponentsOfType(AZ::EntityId entityId, const AZ::ComponentTypeList& componentTypeIds)
        {
            EditorEntityActionComponent::AddComponentsOutcome outcome;
            EntityCompositionRequestBus::BroadcastResult(outcome, &EntityCompositionRequests::AddComponentsToEntities, EntityIdList{ entityId }, componentTypeIds);

            AZ_Warning("EditorComponentAPI", outcome.IsSuccess(), "AddComponentsOfType - AddComponentsToEntities failed (%s).", outcome.GetError().c_str());

            if (!outcome.IsSuccess())
            {
                return AddComponentsOutcome(AZStd::unexpect, AZStd::string("AddComponentsOfType - AddComponentsToEntities failed (") + outcome.GetError().c_str() + ").");
            }

            auto entityToComponentMap = outcome.GetValue();

            if (entityToComponentMap.find(entityId) == entityToComponentMap.end() || entityToComponentMap[entityId].m_componentsAdded.size() == 0)
            {
                AZ_Warning("EditorComponentAPI", false, "Malformed result from AddComponentsToEntities.");
                return AddComponentsOutcome(AZStd::unexpect, AZStd::string("Malformed result from AddComponentsToEntities."));
            }

            AZStd::vector<AZ::EntityComponentIdPair> componentIds;
            for (AZ::Component* component : entityToComponentMap[entityId].m_componentsAdded)
            {
                if (!component)
                {
                    AZ_Warning("EditorComponentAPI", false, "Invalid component returned in AddComponentsToEntities.");
                    return AddComponentsOutcome(AZStd::unexpect, AZStd::string("Invalid component returned in AddComponentsToEntities."));
                }
                else
                {
                    componentIds.push_back(AZ::EntityComponentIdPair(entityId, component->GetId()));
                }
            }

            return AddComponentsOutcome( componentIds );
        }

        EditorComponentAPIRequests::AddComponentsOutcome EditorComponentAPIComponent::AddComponentOfType(AZ::EntityId entityId, const AZ::Uuid& componentTypeId)
        {
            return AddComponentsOfType(entityId, { componentTypeId });
        }

        bool EditorComponentAPIComponent::HasComponentOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            GetComponentOutcome outcome = GetComponentOfType(entityId, componentTypeId);
            return outcome.IsSuccess() && outcome.GetValue().GetComponentId() != AZ::InvalidComponentId;
        }

        size_t EditorComponentAPIComponent::CountComponentsOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            AZ::Entity::ComponentArrayType components = FindComponents(entityId, componentTypeId);
            return components.size();
        }

        EditorComponentAPIRequests::GetComponentOutcome EditorComponentAPIComponent::GetComponentOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            AZ::Component* component = FindComponent(entityId, componentTypeId);
            if (component)
            {
                return GetComponentOutcome(AZ::EntityComponentIdPair(entityId, component->GetId()));
            }
            else
            {
                return GetComponentOutcome(AZStd::unexpect, AZStd::string("GetComponentOfType - Component type of id ") + componentTypeId.ToString<AZStd::string>() + " not found on Entity");
            }
        }

        EditorComponentAPIRequests::GetComponentsOutcome EditorComponentAPIComponent::GetComponentsOfType(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            AZ::Entity::ComponentArrayType components = FindComponents(entityId, componentTypeId);

            if (components.empty())
            {
                return GetComponentsOutcome(AZStd::unexpect, AZStd::string("GetComponentOfType - Component type not found on Entity"));
            }

            AZStd::vector<AZ::EntityComponentIdPair> componentIds;
            componentIds.reserve(components.size());

            for (AZ::Component* component : components)
            {
                componentIds.push_back(AZ::EntityComponentIdPair(entityId, component->GetId()));
            }

            return { componentIds };
        }

        bool EditorComponentAPIComponent::IsValid(AZ::EntityComponentIdPair componentInstance)
        {
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
            return component != nullptr;
        }

        bool EditorComponentAPIComponent::EnableComponents(const AZStd::vector<AZ::EntityComponentIdPair>& componentInstances)
        {
            AZ::Entity::ComponentArrayType components;
            for (const AZ::EntityComponentIdPair& componentInstance : componentInstances)
            {
                AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
                if (component)
                {
                    components.push_back(component);
                }
                else
                {
                    AZ_Warning("EditorComponentAPI", false, "EnableComponent failed - could not find Component from the given entityId and componentId.");
                    return false;
                }
            }

            EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::EnableComponents, components);

            for (const AZ::EntityComponentIdPair& componentInstance : componentInstances)
            {
                if (!IsComponentEnabled(componentInstance))
                {
                    return false;
                }
            }

            return true;
        }

        bool EditorComponentAPIComponent::IsComponentEnabled(const AZ::EntityComponentIdPair& componentInstance)
        {
            // Get AZ::Entity*
            AZ::Entity* entityPtr = FindEntity(componentInstance.GetEntityId());

            if (!entityPtr)
            {
                AZ_Warning("EditorComponentAPI", false, "IsComponentEnabled failed - could not find Entity from the given entityId");
                return false;
            }

            // Get Component*
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());

            if (!component)
            {
                AZ_Warning("EditorComponentAPI", false, "IsComponentEnabled failed - could not find Component from the given entityId and componentId.");
                return false;
            }

            const auto& entityComponents = entityPtr->GetComponents();
            if (AZStd::find(entityComponents.begin(), entityComponents.end(), component) != entityComponents.end())
            {
                return true;
            }

            return false;
        }

        bool EditorComponentAPIComponent::DisableComponents(const AZStd::vector<AZ::EntityComponentIdPair>& componentInstances)
        {
            AZ::Entity::ComponentArrayType components;
            for (const AZ::EntityComponentIdPair& componentInstance : componentInstances)
            {
                AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
                if (component)
                {
                    components.push_back(component);
                }
                else
                {
                    AZ_Warning("EditorComponentAPI", false, "DisableComponent failed - could not find Component from the given entityId and componentId.");
                    return false;
                }
            }

            EntityCompositionRequestBus::Broadcast(&EntityCompositionRequests::DisableComponents, components);

            for (const AZ::EntityComponentIdPair& componentInstance : componentInstances)
            {
                if (IsComponentEnabled(componentInstance))
                {
                    return false;
                }
            }

            return true;
        }

        bool EditorComponentAPIComponent::RemoveComponents(const AZStd::vector<AZ::EntityComponentIdPair>& componentInstances)
        {
            bool cumulativeSuccess = true;

            AZ::Entity::ComponentArrayType components;
            for (const AZ::EntityComponentIdPair& componentInstance : componentInstances)
            {
                AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
                if (component)
                {
                    components.push_back(component);
                }
                else
                {
                    AZ_Warning("EditorComponentAPI", false, "RemoveComponents - a component could not be found.");
                    cumulativeSuccess = false;
                }
            }

            EditorEntityActionComponent::RemoveComponentsOutcome outcome;
            EntityCompositionRequestBus::BroadcastResult(outcome, &EntityCompositionRequests::RemoveComponents, components);

            if (!outcome.IsSuccess())
            {
                AZ_Warning("EditorComponentAPI", false, "RemoveComponents failed - components could not be removed from entity.");
                return false;
            }

            return cumulativeSuccess;
        }

        EditorComponentAPIRequests::PropertyTreeOutcome EditorComponentAPIComponent::BuildComponentPropertyTreeEditor(const AZ::EntityComponentIdPair& componentInstance)
        {
            // Verify the Component Instance still exists
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
            if (!component)
            {
                AZ_Error("EditorComponentAPIComponent", false, "BuildComponentPropertyTreeEditor - Component Instance is Invalid.");
                return EditorComponentAPIRequests::PropertyTreeOutcome{ AZStd::unexpect, PropertyTreeOutcome::ErrorType("BuildComponentPropertyTreeEditor - Component Instance is Invalid.") };
            }

            return { PropertyTreeOutcome::ValueType(reinterpret_cast<void*>(component), component->GetUnderlyingComponentType()) };
        }

        EditorComponentAPIRequests::PropertyOutcome EditorComponentAPIComponent::GetComponentProperty(const AZ::EntityComponentIdPair& componentInstance, const AZStd::string_view propertyPath)
        {
            // Verify the Component Instance still exists
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
            if (!component)
            {
                AZ_Error("EditorComponentAPIComponent", false, "GetComponentProperty - Component Instance is Invalid.");
                return EditorComponentAPIRequests::PropertyOutcome{ AZStd::unexpect, PropertyOutcome::ErrorType("GetComponentProperty - Component Instance is Invalid.") };
            }

            PropertyTreeEditor pte = PropertyTreeEditor(reinterpret_cast<void*>(component), component->GetUnderlyingComponentType());

            if (m_usePropertyVisibility)
            {
                pte.SetVisibleEnforcement(true);
            }

            return pte.GetProperty(propertyPath);
        }

        EditorComponentAPIRequests::PropertyOutcome EditorComponentAPIComponent::SetComponentProperty(const AZ::EntityComponentIdPair& componentInstance, const AZStd::string_view propertyPath, const AZStd::any& value)
        {
            // Verify the Component Instance still exists
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
            if (!component)
            {
                AZ_Error("EditorComponentAPIComponent", false, "SetComponentProperty - Component Instance is Invalid.");
                return EditorComponentAPIRequests::PropertyOutcome{ AZStd::unexpect, PropertyOutcome::ErrorType("SetComponentProperty - Component Instance is Invalid.") };
            }

            PropertyTreeEditor pte = PropertyTreeEditor(reinterpret_cast<void*>(component), component->GetUnderlyingComponentType());

            if (m_usePropertyVisibility)
            {
                pte.SetVisibleEnforcement(true);
            }

            ScopedUndoBatch undo("Modify Entity Property");
            PropertyOutcome result = pte.SetProperty(propertyPath, value);
            if (result.IsSuccess())
            {
                PropertyEditorEntityChangeNotificationBus::Event(componentInstance.GetEntityId(), &PropertyEditorEntityChangeNotifications::OnEntityComponentPropertyChanged, componentInstance.GetComponentId());
            }
            undo.MarkEntityDirty(componentInstance.GetEntityId());

            return result;
        }

        bool EditorComponentAPIComponent::CompareComponentProperty(const AZ::EntityComponentIdPair& componentInstance, const AZStd::string_view propertyPath, const AZStd::any& value)
        {
            // Verify the Component Instance still exists
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
            if (!component)
            {
                AZ_Error("EditorComponentAPIComponent", false, "CompareComponentProperty - Component Instance is Invalid.");
                return false;
            }

            PropertyTreeEditor pte = PropertyTreeEditor(reinterpret_cast<void*>(component), component->GetUnderlyingComponentType());

            if (m_usePropertyVisibility)
            {
                pte.SetVisibleEnforcement(true);
            }

            return pte.CompareProperty(propertyPath, value);
        }

        const AZStd::vector<AZStd::string> EditorComponentAPIComponent::BuildComponentPropertyList(const AZ::EntityComponentIdPair& componentInstance)
        {
            // Verify the Component Instance still exists
            AZ::Component* component = FindComponent(componentInstance.GetEntityId(), componentInstance.GetComponentId());
            if (!component)
            {
                AZ_Error("EditorComponentAPIComponent", false, "BuildComponentPropertyList - Component Instance is Invalid.");
                return { AZStd::string("BuildComponentPropertyList - Component Instance is Invalid.") };
            }

            PropertyTreeEditor pte = PropertyTreeEditor(reinterpret_cast<void*>(component), component->GetUnderlyingComponentType());

            if (m_usePropertyVisibility)
            {
                pte.SetVisibleEnforcement(true);
            }

            return pte.BuildPathsList();
        }

        AZ::Entity* EditorComponentAPIComponent::FindEntity(AZ::EntityId entityId)
        {
            AZ_Assert(entityId.IsValid(), "EditorComponentAPIComponent::FindEntity - Invalid EntityId provided.");
            if (!entityId.IsValid())
            {
                return nullptr;
            }

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            return entity;
        }

        AZ::Component* EditorComponentAPIComponent::FindComponent(AZ::EntityId entityId, AZ::ComponentId componentId)
        {
            // Get AZ::Entity*
            AZ::Entity* entityPtr = FindEntity(entityId);

            if (!entityPtr)
            {
                AZ_Warning("EditorComponentAPI", false, "FindComponent failed - could not find entity pointer from entityId provided.");
                return nullptr;
            }

            // See if the component is on the entity proper (Active)
            const auto& entityComponents = entityPtr->GetComponents();
            for (AZ::Component* component : entityComponents)
            {
                if (component->GetId() == componentId)
                {
                    return component;
                }
            }

            // Check for pending components
            AZ::Entity::ComponentArrayType pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(entityPtr->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
            for (AZ::Component* component : pendingComponents)
            {
                if (component->GetId() == componentId)
                {
                    return component;
                }
            }

            // Check for disabled components
            AZ::Entity::ComponentArrayType disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entityPtr->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
            for (AZ::Component* component : disabledComponents)
            {
                if (component->GetId() == componentId)
                {
                    return component;
                }
            }

            return nullptr;
        }

        AZ::Component* EditorComponentAPIComponent::FindComponent(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            // Get AZ::Entity*
            AZ::Entity* entityPtr = FindEntity(entityId);

            if (!entityPtr)
            {
                AZ_Warning("EditorComponentAPI", false, "FindComponent failed - could not find entity pointer from entityId provided.");
                return nullptr;
            }

            // See if the component is on the entity proper (Active)
            const auto& entityComponents = entityPtr->GetComponents();
            for (AZ::Component* component : entityComponents)
            {
                if (component->GetUnderlyingComponentType() == componentTypeId)
                {
                    return component;
                }
            }

            // Check for pending components
            AZ::Entity::ComponentArrayType pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(entityPtr->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
            for (AZ::Component* component : pendingComponents)
            {
                if (component->GetUnderlyingComponentType() == componentTypeId)
                {
                    return component;
                }
            }

            // Check for disabled components
            AZ::Entity::ComponentArrayType disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entityPtr->GetId(), &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
            for (AZ::Component* component : disabledComponents)
            {
                if (component->GetUnderlyingComponentType() == componentTypeId)
                {
                    return component;
                }
            }

            return nullptr;
        }

        AZ::Entity::ComponentArrayType EditorComponentAPIComponent::FindComponents(AZ::EntityId entityId, AZ::Uuid componentTypeId)
        {
            AZ::Entity::ComponentArrayType components;

            // Get AZ::Entity*
            AZ::Entity* entityPtr = FindEntity(entityId);

            if (!entityPtr)
            {
                AZ_Warning("EditorComponentAPI", false, "FindComponents failed - could not find entity pointer from entityId provided.");
                return components;
            }

            // See if the component is on the entity proper (Active)
            const auto& entityComponents = entityPtr->GetComponents();
            for (AZ::Component* component : entityComponents)
            {
                if (component->GetUnderlyingComponentType() == componentTypeId)
                {
                    components.push_back(component);
                }
            }

            // Check for pending components
            AZ::Entity::ComponentArrayType pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(entityId, &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
            for (AZ::Component* component : pendingComponents)
            {
                if (component->GetUnderlyingComponentType() == componentTypeId)
                {
                    components.push_back(component);
                }
            }

            // Check for disabled components
            AZ::Entity::ComponentArrayType disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entityId, &AzToolsFramework::EditorDisabledCompositionRequests::GetDisabledComponents, disabledComponents);
            for (AZ::Component* component : disabledComponents)
            {
                if (component->GetUnderlyingComponentType() == componentTypeId)
                {
                    components.push_back(component);
                }
            }

            return components;
        }

    } // Components
} // AzToolsFramework
