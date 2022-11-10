/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentExport.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string_view.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Spawnable/EditorInfoRemover.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    EditorInfoRemover::~EditorInfoRemover()
    {
        for (auto* handler : m_editorOnlyEntityHandlerCandidates)
        {
            delete handler;
        }
    }

    void EditorInfoRemover::Process(PrefabProcessorContext& prefabProcessorContext)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Assert(serializeContext, "Failed to retrieve serialize context.");
            return;
        }

        prefabProcessorContext.ListPrefabs(
            [this, &serializeContext, &prefabProcessorContext](PrefabDocument& prefab)
            {
                auto result = RemoveEditorInfo(prefab, serializeContext, prefabProcessorContext);
                if (!result)
                {
                    AZ_Error(
                        "Prefab", false, "Converting to runtime Prefab '%s' failed, Error: %s .", prefab.GetName().c_str(),
                        result.GetError().c_str());
                    return;
                } 
            });
    }

    void EditorInfoRemover::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<EditorInfoRemover, PrefabProcessor>()->Version(1);
        }
    }

    void EditorInfoRemover::GetEntitiesFromInstance(AzToolsFramework::Prefab::Instance& instance, EntityList& hierarchyEntities)
    {
        instance.GetAllEntitiesInHierarchy(
            [&hierarchyEntities](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                hierarchyEntities.emplace_back(entity.get());
                return true;
            }
        );
    }

    void EditorInfoRemover::SetEditorOnlyEntityHandlerFromCandidates(const EntityList& entities)
    {
        ClearEditorOnlyEntityIds();
        m_editorOnlyEntityHandler = nullptr;
        for (auto& handlerCandidate : m_editorOnlyEntityHandlerCandidates)
        {
            // See if this handler can handle at least one of the entities.
            for (auto entity : entities)
            {
                if (handlerCandidate->IsEntityUniquelyForThisHandler(entity))
                {
                    m_editorOnlyEntityHandler = handlerCandidate;
                    break;
                }
            }

            if (HasValidEditorOnlyHandler())
            {
                break;
            }
        }
    }

    bool EditorInfoRemover::HasValidEditorOnlyHandler() const
    {
        return m_editorOnlyEntityHandler != nullptr;
    }

    void EditorInfoRemover::ClearEditorOnlyEntityIds()
    {
        m_editorOnlyEntityIds.clear();
    }

    void EditorInfoRemover::AddEntityIdIfEditorOnly(AZ::Entity* entity)
    {
        bool isEditorOnly = false;
        EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, entity->GetId(), &EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);
        if (isEditorOnly && HasValidEditorOnlyHandler())
        {
            m_editorOnlyEntityHandler->AddEditorOnlyEntity(entity, m_editorOnlyEntityIds);
        }
    }

    /**
    * Identify and remove any entities marked as editor-only.
    * If any are discovered, adjust descendants' transforms to retain spatial relationships.
    * Note we cannot use EBuses for this purpose, since we're crunching data, and can't assume any entities are active.
    */
    EditorInfoRemover::RemoveEditorOnlyEntitiesResult EditorInfoRemover::RemoveEditorOnlyEntities(EntityList& entities)
    {
        if (HasValidEditorOnlyHandler())
        {
            const auto handlerResult =
                m_editorOnlyEntityHandler->HandleEditorOnlyEntities(entities, m_editorOnlyEntityIds, *m_serializeContext);
            if (!handlerResult)
            {
                return AZ::Failure(AZStd::string::format(
                    "Error occurred when handle editor-only entities. Error: %s",
                    handlerResult.GetError().c_str())
                );
            }
        }

        // Remove editor-only entities from the given entity list.
        AZStd::erase_if(
            entities,
            [this](auto entity)
            {
                return m_editorOnlyEntityIds.find(entity->GetId()) != m_editorOnlyEntityIds.end();
            }
        );

        return AZ::Success();
    }

    EditorInfoRemover::ExportEntityResult EditorInfoRemover::ExportEntity(AZ::Entity* sourceEntity, PrefabProcessorContext& context)
    {
        // For export, components can assume they're initialized, but not activated.
        if (sourceEntity->GetState() == AZ::Entity::State::Constructed)
        {
            sourceEntity->Init();
        }

        AZ::Entity* exportEntity = aznew AZ::Entity(sourceEntity->GetId(), sourceEntity->GetName().c_str());
        exportEntity->SetRuntimeActiveByDefault(sourceEntity->IsRuntimeActiveByDefault());

        AddEntityIdIfEditorOnly(sourceEntity);

        const AZ::Entity::ComponentArrayType& editorComponents = sourceEntity->GetComponents();
        EntityList exportedEntities;
        for (AZ::Component* component : editorComponents)
        {
            auto result = ExportComponent(component, context, sourceEntity, exportEntity);
            if (!result)
            {
                return AZ::Failure(AZStd::string::format(
                    "Entity '%s' %s - export component '%s' failed. Error: %s",
                    exportEntity->GetName().c_str(),
                    exportEntity->GetId().ToString().c_str(),
                    component->RTTI_GetTypeName(),
                    result.GetError().c_str())
                );
            }
        }

        // Pre-sort prior to exporting so it isn't required at instantiation time.
        const auto sortResult = exportEntity->EvaluateDependenciesGetDetails();
        /* :CBR_TODO: verify AZ::Entity::DependencySortResult::HasIncompatibleServices and
         AZ::Entity::DependencySortResult::DescriptorNotRegistered are still covered here*/
        if (!sortResult.IsSuccess())
        {
            return AZ::Failure(AZStd::string::format(
                "Entity '%s' %s - dependency evaluation failed. Error: %s",
                exportEntity->GetName().c_str(),
                exportEntity->GetId().ToString().c_str(),
                sortResult.GetError().m_message.c_str()));
        }

        return AZ::Success(exportEntity);
    }

    bool EditorInfoRemover::ReadComponentAttribute(
        AZ::Component* component,
        AZ::Edit::Attribute* attribute,
        AZStd::vector<AZ::Crc32>& attributeTags)
    {
        attributeTags.clear();
        PropertyAttributeReader reader(component, attribute);
        return reader.Read<AZStd::vector<AZ::Crc32>>(attributeTags);
    }

    EditorInfoRemover::ShouldExportResult EditorInfoRemover::ShouldExportComponent(
        AZ::Component* component,
        PrefabProcessorContext& context) const
    {
        const AZ::SerializeContext::ClassData* classData = m_serializeContext->FindClassData(component->RTTI_GetType());
        if (!classData || !classData->m_editData)
        {
            return AZ::Success(true);
        }

        const AZ::Edit::ElementData* editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
        if (!editorDataElement)
        {
            return AZ::Success(true);
        }

        AZStd::vector<AZ::Crc32> attributeTags;
        const auto& platformTags = context.GetPlatformTags();

        // If the component has declared the 'ExportIfAllPlatforms' attribute, skip export if any of the flags are not present.
        AZ::Edit::Attribute* allTagsAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::ExportIfAllPlatformTags);
        if (allTagsAttribute)
        {
            if (!ReadComponentAttribute(component, allTagsAttribute, attributeTags))
            {
                return AZ::Failure(
                    AZStd::string("'ExportIfAllPlatforms' attribute is not bound to the correct return type. Expects AZStd::vector<AZ::Crc32>.")
                );
            }

            for (AZ::Crc32 tag : attributeTags)
            {
                if (platformTags.find(tag) == platformTags.end())
                {
                    // Export platform tags does not contain all tags specified in 'ExportIfAllPlatforms' attribute.
                    return AZ::Success(false);
                }
            }
        }

        // If the component has declared the 'ExportIfAnyPlatforms' attribute, skip export if none of the flags are present.
        AZ::Edit::Attribute* anyTagsAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::ExportIfAnyPlatformTags);
        if (anyTagsAttribute)
        {
            if (!ReadComponentAttribute(component, anyTagsAttribute, attributeTags))
            {
                return AZ::Failure(
                    AZStd::string("'ExportIfAnyPlatforms' attribute is not bound to the correct return type. Expects AZStd::vector<AZ::Crc32>.")
                );
            }

            bool anyFlagSet = false;
            for (AZ::Crc32 tag : attributeTags)
            {
                if (platformTags.find(tag) != platformTags.end())
                {
                    anyFlagSet = true;
                    break;
                }
            }
            if (!anyFlagSet)
            {
                // None of the flags in 'ExportIfAnyPlatforms' was present in the export platform tags.
                return AZ::Success(false);
            }
        }

        return AZ::Success(true);
    }

    EditorInfoRemover::ResolveExportedComponentResult EditorInfoRemover::ResolveExportedComponent(
        AZ::ExportedComponent& component,
        PrefabProcessorContext& prefabProcessorContext)
    {
        AZ::Component* inputComponent = component.m_component;
        if (!inputComponent)
        {
            return AZ::Success(component);
        }

        // Don't export the component if it has unmet platform tag requirements.
        ShouldExportResult shouldExportResult = ShouldExportComponent(inputComponent, prefabProcessorContext);
        if (!shouldExportResult)
        {
            return AZ::Failure(shouldExportResult.TakeError());
        }

        if (!shouldExportResult.GetValue())
        {
            // If the platform tag requirements aren't met, return a null component that's been flagged as exported,
            // so that we know not to try and process it any further.
            return AZ::Success(AZ::ExportedComponent());
        }

        // Determine if the component has a custom export callback, and invoke it if so.
        // If there's no custom export callback, just return what we were given.
        const AZ::SerializeContext::ClassData* classData = m_serializeContext->FindClassData(inputComponent->RTTI_GetType());
        if (!classData || !classData->m_editData)
        {
            return AZ::Success(component);
        }
        const AZ::Edit::ElementData* editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
        if (!editorDataElement)
        {
            return AZ::Success(component);
        }
        AZ::Edit::Attribute* exportCallbackAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::RuntimeExportCallback);
        if (!exportCallbackAttribute)
        {
            return AZ::Success(component);
        }

        PropertyAttributeReader reader(inputComponent, exportCallbackAttribute);
        AZ::ExportedComponent exportedComponent;
        if (reader.Read<AZ::ExportedComponent>(exportedComponent, inputComponent, prefabProcessorContext.GetPlatformTags()))
        {
            // If the callback handled the export and provided a different component instance, continue to resolve recursively.
            if (exportedComponent.m_componentExportHandled && (exportedComponent.m_component != inputComponent))
            {
                return ResolveExportedComponent(exportedComponent, prefabProcessorContext);
            }
            else
            {
                // It provided the *same* component back (or didn't handle the export at all), so we're done.
                return AZ::Success(exportedComponent);
            }
        }
        else
        {
            return AZ::Failure(AZStd::string("Bound 'CustomExportCallback' does not have the required return type/signature."));
        }
    }

    EditorInfoRemover::BuildGameEntityResult EditorInfoRemover::BuildGameEntity(
        AzToolsFramework::Components::EditorComponentBase* editorComponent,
        AZ::Entity* sourceEntity,
        AZ::Entity* exportEntity)
    {
        const size_t oldComponentCount = exportEntity->GetComponents().size();
        editorComponent->BuildGameEntity(exportEntity);
        AZ::ComponentId newID = editorComponent->GetId();
        for (auto i = oldComponentCount; i < exportEntity->GetComponents().size(); ++i)
        {
            AZ::Component* exportComponent = exportEntity->GetComponents()[i];

            // Verify that the result of BuildGameEntity() wasn't an editor component.
            auto* exportAsEditorComponent = azrtti_cast<Components::EditorComponentBase*>(exportComponent);
            if (exportAsEditorComponent)
            {
                return AZ::Failure(AZStd::string::format(
                    "Entity '%s' %s - component '%s' exported an editor component from BuildGameEntity() for runtime use.",
                    sourceEntity->GetName().c_str(),
                    sourceEntity->GetId().ToString().c_str(),
                    editorComponent->RTTI_GetType().ToString<AZStd::string>().c_str()));
            }
            else if (editorComponent->GetId() == AZ::InvalidComponentId)
            {
                return AZ::Failure(AZStd::string::format(
                    "Entity '%s' %s - component '%s' doesn't have a valid component Id.",
                    sourceEntity->GetName().c_str(),
                    sourceEntity->GetId().ToString().c_str(),
                    editorComponent->RTTI_GetType().ToString<AZStd::string>().c_str()));
            }

            exportComponent->SetId(newID++);
            // The first time round set the new component the same as the editor one. This will change in a separate ticket
            // when 8 bit runtime Ids are implemented.
            // Make sure the newID isn't already on the source Entity. If it is increment the ID and try again.
            while (sourceEntity->FindComponent(newID))
            {
                ++newID;
            }
        }

        return AZ::Success();
    }

    EditorInfoRemover::ExportComponentResult EditorInfoRemover::ExportComponent(
        AZ::Component* component,
        PrefabProcessorContext& prefabProcessorContext,
        AZ::Entity* sourceEntity,
        AZ::Entity* exportEntity)
    {
        auto validationResult = m_componentRequirementsValidator.Validate(component);
        if (!validationResult.IsSuccess())
        {
            return AZ::Failure(AZStd::string::format(
                "Entity '%s' %s - validation of component '%s' failed: %s",
                sourceEntity->GetName().c_str(),
                sourceEntity->GetId().ToString().c_str(),
                component->RTTI_GetTypeName(),
                validationResult.GetError().c_str())
            );

        }

        AZ::ExportedComponent exportComponent(component, false, false);
        auto exportResult = ResolveExportedComponent(
exportComponent, prefabProcessorContext);
        if (!exportResult)
        {
            return AZ::Failure(AZStd::string::format(
                "Entity '%s' %s - component '%s' could not be exported due to export attributes: %s.",
                sourceEntity->GetName().c_str(),
                sourceEntity->GetId().ToString().c_str(),
                component->RTTI_GetTypeName(),
                exportResult.GetError().c_str()));
        }

        AZ::ExportedComponent& exportedComponent = exportResult.GetValue();
        
        // If ResolveExportedComponent didn't handle the component export, then we'll do the following:
        // - For editor components, fall back on the legacy BuildGameEntity() path for handling component exports.
        // - For runtime components, provide a default behavior of "clone / add" to export the component.
        if (!exportedComponent.m_componentExportHandled)
        {
            auto* asEditorComponent = azrtti_cast<Components::EditorComponentBase*>(component);
            // Editor components:  Try to use BuildGameEntity()
            if (asEditorComponent) // BEGIN BuildGameEntity compatibility path for editor components not using the newer RuntimeExportCallback functionality.
            {
                auto buildGameEntityResult = BuildGameEntity(asEditorComponent, sourceEntity, exportEntity);
                if (!buildGameEntityResult.IsSuccess())
                {
                    return AZ::Failure(AZStd::string::format(
                        "Entity '%s' %s - component '%s' to build game entity failed. Error: %s.",
                        sourceEntity->GetName().c_str(),
                        sourceEntity->GetId().ToString().c_str(),
                        component->RTTI_GetTypeName(),
                        buildGameEntityResult.GetError().c_str()));
                }

                // Since this is an editor component, we very specifically do *not* want to clone and add it as a runtime
                // component by default, so regardless of whether or not the BuildGameEntity() call did anything, 
                // null out the editor component and mark it handled.
                return AZ::Success();

            } // END BuildGameEntity compatibility path for editor components not using the newer RuntimeExportCallback functionality.
            else
            {
                // Nothing else has handled the component export, so fall back on the default behavior 
                // for runtime components:  clone and add the runtime component that already exists.
                exportedComponent = AZ::ExportedComponent(component, false);
            }
        }

        // At this point, either ResolveExportedComponent or the default logic above should have set the component export 
        // as being handled.  If not, there is likely a new code path that requires a default export behavior.
        AZ_Assert(exportedComponent.m_componentExportHandled,
            "Entity '%s' %s - component '%s' had no export handlers and could not be added to the entity.",
            exportEntity->GetName().c_str(),
            exportEntity->GetId().ToString().c_str(),
            component->RTTI_GetTypeName());

        // If we have an exported component, we add it to the exported entity.  
        // If we don't (m_component == nullptr), this component chose not to be exported, so we skip it.
        if (exportedComponent.m_componentExportHandled && exportedComponent.m_component)
        {
            AZ::Component* runtimeComponent = exportedComponent.m_component;

            // Verify that we aren't trying to export an editor component.
            auto* exportAsEditorComponent = azrtti_cast<Components::EditorComponentBase*>(runtimeComponent);
            if (exportAsEditorComponent)
            {
                auto* asEditorComponent =
                    azrtti_cast<Components::EditorComponentBase*>(component);

                return AZ::Failure(AZStd::string::format(
                    "Entity '%s' %s - component '%s' is trying to export an Editor component for runtime use.",
                    sourceEntity->GetName().c_str(),
                    sourceEntity->GetId().ToString().c_str(),
                    asEditorComponent->RTTI_GetType().ToString<AZStd::string>().c_str()));
            }

            // If the final component is not owned by us, make our own copy.
            if (!exportedComponent.m_deleteAfterExport)
            {
                runtimeComponent = m_serializeContext->CloneObject(runtimeComponent);
            }

            // Synchronize to source component Id, and add to the export entity.
            runtimeComponent->SetId(component->GetId());

            if (!exportEntity->AddComponent(runtimeComponent))
            {
                return AZ::Failure(AZStd::string::format(
                    "Entity '%s' %s - component '%s' could not be added to this entity.",
                    exportEntity->GetName().c_str(),
                    exportEntity->GetId().ToString().c_str(),
                    runtimeComponent->RTTI_GetTypeName())
                );
            }
        }

        return AZ::Success();
    }

    EditorInfoRemover::RemoveEditorInfoResult EditorInfoRemover::RemoveEditorInfo(
        PrefabDocument& prefab,
        AZ::SerializeContext* serializeContext,
        PrefabProcessorContext& prefabProcessorContext)
    {
        if (!serializeContext)
        {
            return AZ::Failure(AZStd::string("Invalid Serialize Context used."));
        }
        m_serializeContext = serializeContext;

        m_componentRequirementsValidator.SetPlatformTags(prefabProcessorContext.GetPlatformTags());

        // grab all nested entities from the Instance as source entities.
        AzToolsFramework::Prefab::Instance& sourceInstance = prefab.GetInstance();
        EntityList sourceEntities;
        GetEntitiesFromInstance(sourceInstance, sourceEntities);

        EntityList exportEntities;

        // prepare for validation of component requirements.
        m_componentRequirementsValidator.SetEntities(sourceEntities);

        // find valid editor-only entity handler for removing editor-only entities later.
        SetEditorOnlyEntityHandlerFromCandidates(sourceEntities);

        // export entities.
        for (AZ::Entity* entity : sourceEntities)
        {
            const auto result = ExportEntity(entity, prefabProcessorContext);
            if (!result)
            {
                return AZ::Failure(AZStd::string::format(
                    "Entity '%s' %s - export entity failed. Error: %s",
                    entity->GetName().c_str(),
                    entity->GetId().ToString().c_str(),
                    result.GetError().c_str())
                );
            }

            exportEntities.emplace_back(result.GetValue());
        }

        // remove editor-only entities with valid editor-only entity handler.
        const auto removeEditorOnlyEntitiesResult = RemoveEditorOnlyEntities(exportEntities);
        if (!removeEditorOnlyEntitiesResult)
        {
            return AZ::Failure(AZStd::string::format(
                "Remove Editor-Only Entities failed. Error: '%s'",
                removeEditorOnlyEntitiesResult.GetError().c_str())
            );
        }

        // validate component requirements for exported entities.
        m_componentRequirementsValidator.SetEntities(exportEntities);
        for (AZ::Entity* exportEntity : exportEntities)
        {
            const AZ::Entity::ComponentArrayType& gameComponents = exportEntity->GetComponents();
            for (const AZ::Component* component : gameComponents)
            {
                const auto result = m_componentRequirementsValidator.Validate(component);
                if (!result)
                {
                    return AZ::Failure(AZStd::string::format(
                        "Entity '%s' %s - validation of export component '%s' failed: %s",
                        exportEntity->GetName().c_str(),
                        exportEntity->GetId().ToString().c_str(),
                        component->RTTI_GetTypeName(),
                        result.GetError().c_str())
                    );
                }
            }
        }

        // remove editor-only entities from instance.
        AZStd::unordered_map<AZ::EntityId, AZ::Entity*> exportEntitiesMap;
        AZStd::for_each(exportEntities.begin(), exportEntities.end(),
            [&exportEntitiesMap](auto& entity)
            {
                exportEntitiesMap.emplace(entity->GetId(), entity);
            }
        );
        sourceInstance.RemoveEntitiesInHierarchy(
            [&exportEntitiesMap](const AZStd::unique_ptr<AZ::Entity>& entity)
            {
                return exportEntitiesMap.find(entity->GetId()) == exportEntitiesMap.end();
            }
        );

        // replace entities of instance with exported ones.
        sourceInstance.GetAllEntitiesInHierarchy(
            [&exportEntitiesMap](AZStd::unique_ptr<AZ::Entity>& entity)
            {
                auto entityId = entity->GetId();
                entity.reset(exportEntitiesMap[entityId]);
                return true;
            }
        );

        return AZ::Success();
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
