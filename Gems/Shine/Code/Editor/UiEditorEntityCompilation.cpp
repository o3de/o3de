/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiEditorEntityCompilation.h"

#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>

namespace Shine
{
    //////////////////////////////////////////////////////////////////////////
    // Internal helpers — ported from EditorInfoRemover (prefab system's slice-free path)
    //////////////////////////////////////////////////////////////////////////
    namespace Internal
    {
        static bool ReadComponentAttribute(
            AZ::Component* component,
            AZ::Edit::Attribute* attribute,
            AZStd::vector<AZ::Crc32>& attributeTags)
        {
            attributeTags.clear();
            AZ::AttributeReader reader(component, attribute);
            return reader.Read<AZStd::vector<AZ::Crc32>>(attributeTags);
        }

        //! Check platform tag attributes to determine if a component should be exported.
        static AZ::Outcome<bool, AZStd::string> ShouldExportComponent(
            AZ::Component* component,
            const AZ::PlatformTagSet& platformTags,
            AZ::SerializeContext& serializeContext)
        {
            const AZ::SerializeContext::ClassData* classData = serializeContext.FindClassData(component->RTTI_GetType());
            if (!classData || !classData->m_editData)
            {
                return AZ::Success(true);
            }

            const AZ::Edit::ElementData* editorDataElement =
                classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
            if (!editorDataElement)
            {
                return AZ::Success(true);
            }

            AZStd::vector<AZ::Crc32> attributeTags;

            // ExportIfAllPlatformTags: skip export if any required tag is missing
            AZ::Edit::Attribute* allTagsAttribute =
                editorDataElement->FindAttribute(AZ::Edit::Attributes::ExportIfAllPlatformTags);
            if (allTagsAttribute)
            {
                if (!ReadComponentAttribute(component, allTagsAttribute, attributeTags))
                {
                    return AZ::Failure(AZStd::string(
                        "'ExportIfAllPlatforms' attribute is not bound to the correct return type."));
                }
                for (AZ::Crc32 tag : attributeTags)
                {
                    if (platformTags.find(tag) == platformTags.end())
                    {
                        return AZ::Success(false);
                    }
                }
            }

            // ExportIfAnyPlatformTags: skip export if none of the specified tags are present
            AZ::Edit::Attribute* anyTagsAttribute =
                editorDataElement->FindAttribute(AZ::Edit::Attributes::ExportIfAnyPlatformTags);
            if (anyTagsAttribute)
            {
                if (!ReadComponentAttribute(component, anyTagsAttribute, attributeTags))
                {
                    return AZ::Failure(AZStd::string(
                        "'ExportIfAnyPlatforms' attribute is not bound to the correct return type."));
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
                    return AZ::Success(false);
                }
            }

            return AZ::Success(true);
        }

        //! Resolve RuntimeExportCallback if present, recursively.
        static AZ::Outcome<AZ::ExportedComponent, AZStd::string> ResolveExportedComponent(
            AZ::ExportedComponent& component,
            const AZ::PlatformTagSet& platformTags,
            AZ::SerializeContext& serializeContext)
        {
            AZ::Component* inputComponent = component.m_component;
            if (!inputComponent)
            {
                return AZ::Success(component);
            }

            auto shouldExportResult = ShouldExportComponent(inputComponent, platformTags, serializeContext);
            if (!shouldExportResult)
            {
                return AZ::Failure(shouldExportResult.TakeError());
            }
            if (!shouldExportResult.GetValue())
            {
                return AZ::Success(AZ::ExportedComponent());
            }

            const AZ::SerializeContext::ClassData* classData = serializeContext.FindClassData(inputComponent->RTTI_GetType());
            if (!classData || !classData->m_editData)
            {
                return AZ::Success(component);
            }
            const AZ::Edit::ElementData* editorDataElement =
                classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
            if (!editorDataElement)
            {
                return AZ::Success(component);
            }
            AZ::Edit::Attribute* exportCallbackAttribute =
                editorDataElement->FindAttribute(AZ::Edit::Attributes::RuntimeExportCallback);
            if (!exportCallbackAttribute)
            {
                return AZ::Success(component);
            }

            AZ::AttributeReader reader(inputComponent, exportCallbackAttribute);
            AZ::ExportedComponent exportedComponent;
            if (reader.Read<AZ::ExportedComponent>(exportedComponent, inputComponent, platformTags))
            {
                if (exportedComponent.m_componentExportHandled && (exportedComponent.m_component != inputComponent))
                {
                    return ResolveExportedComponent(exportedComponent, platformTags, serializeContext);
                }
                return AZ::Success(exportedComponent);
            }

            return AZ::Failure(AZStd::string("Bound 'RuntimeExportCallback' does not have the required return type/signature."));
        }

        //! Convert a single editor component via BuildGameEntity.
        static AZ::Outcome<void, AZStd::string> BuildGameEntity(
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

                auto* exportAsEditorComponent =
                    azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(exportComponent);
                if (exportAsEditorComponent)
                {
                    return AZ::Failure(AZStd::string::format(
                        "Entity '%s' %s - component '%s' exported an editor component from BuildGameEntity().",
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
                while (sourceEntity->FindComponent(newID))
                {
                    ++newID;
                }
            }
            return AZ::Success();
        }

        //! Export a single component: resolve callback → BuildGameEntity fallback → clone runtime.
        static AZ::Outcome<void, AZStd::string> ExportComponent(
            AZ::Component* component,
            const AZ::PlatformTagSet& platformTags,
            AZ::SerializeContext& serializeContext,
            AZ::Entity* sourceEntity,
            AZ::Entity* exportEntity)
        {
            AZ::ExportedComponent exportComponent(component, false, false);
            auto exportResult = ResolveExportedComponent(exportComponent, platformTags, serializeContext);
            if (!exportResult)
            {
                return AZ::Failure(AZStd::string::format(
                    "Entity '%s' %s - component '%s' could not be exported: %s.",
                    sourceEntity->GetName().c_str(),
                    sourceEntity->GetId().ToString().c_str(),
                    component->RTTI_GetTypeName(),
                    exportResult.GetError().c_str()));
            }

            AZ::ExportedComponent& exportedComponent = exportResult.GetValue();

            if (!exportedComponent.m_componentExportHandled)
            {
                auto* asEditorComponent =
                    azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(component);
                if (asEditorComponent)
                {
                    auto buildResult = BuildGameEntity(asEditorComponent, sourceEntity, exportEntity);
                    if (!buildResult.IsSuccess())
                    {
                        return AZ::Failure(AZStd::string::format(
                            "Entity '%s' %s - component '%s' BuildGameEntity failed: %s.",
                            sourceEntity->GetName().c_str(),
                            sourceEntity->GetId().ToString().c_str(),
                            component->RTTI_GetTypeName(),
                            buildResult.GetError().c_str()));
                    }
                    return AZ::Success();
                }
                else
                {
                    // Runtime-ready component, mark as handled for the clone path below
                    exportedComponent = AZ::ExportedComponent(component, false);
                }
            }

            if (exportedComponent.m_componentExportHandled && exportedComponent.m_component)
            {
                AZ::Component* runtimeComponent = exportedComponent.m_component;

                auto* exportAsEditorComponent =
                    azrtti_cast<AzToolsFramework::Components::EditorComponentBase*>(runtimeComponent);
                if (exportAsEditorComponent)
                {
                    return AZ::Failure(AZStd::string::format(
                        "Entity '%s' %s - component '%s' is trying to export an Editor component for runtime.",
                        sourceEntity->GetName().c_str(),
                        sourceEntity->GetId().ToString().c_str(),
                        component->RTTI_GetType().ToString<AZStd::string>().c_str()));
                }

                if (!exportedComponent.m_deleteAfterExport)
                {
                    runtimeComponent = serializeContext.CloneObject(runtimeComponent);
                }

                runtimeComponent->SetId(component->GetId());

                if (!exportEntity->AddComponent(runtimeComponent))
                {
                    return AZ::Failure(AZStd::string::format(
                        "Entity '%s' %s - component '%s' could not be added.",
                        exportEntity->GetName().c_str(),
                        exportEntity->GetId().ToString().c_str(),
                        runtimeComponent->RTTI_GetTypeName()));
                }
            }

            return AZ::Success();
        }

        //! Export a single entity: create runtime entity, export all components.
        static AZ::Outcome<AZ::Entity*, AZStd::string> ExportEntity(
            AZ::Entity* sourceEntity,
            const AZ::PlatformTagSet& platformTags,
            AZ::SerializeContext& serializeContext)
        {
            // Ensure entity is initialized (but not activated)
            if (sourceEntity->GetState() == AZ::Entity::State::Constructed)
            {
                sourceEntity->Init();
            }

            AZ::Entity* exportEntity = aznew AZ::Entity(sourceEntity->GetId(), sourceEntity->GetName().c_str());
            exportEntity->SetRuntimeActiveByDefault(sourceEntity->IsRuntimeActiveByDefault());

            const AZ::Entity::ComponentArrayType& editorComponents = sourceEntity->GetComponents();
            for (AZ::Component* component : editorComponents)
            {
                auto result = ExportComponent(component, platformTags, serializeContext, sourceEntity, exportEntity);
                if (!result)
                {
                    delete exportEntity;
                    return AZ::Failure(result.GetError());
                }
            }

            // Pre-sort component dependencies
            const auto sortResult = exportEntity->EvaluateDependenciesGetDetails();
            if (!sortResult.IsSuccess())
            {
                AZStd::string error = AZStd::string::format(
                    "Entity '%s' %s - dependency evaluation failed: %s",
                    exportEntity->GetName().c_str(),
                    exportEntity->GetId().ToString().c_str(),
                    sortResult.GetError().m_message.c_str());
                delete exportEntity;
                return AZ::Failure(AZStd::move(error));
            }

            return AZ::Success(exportEntity);
        }

    } // namespace Internal

    //////////////////////////////////////////////////////////////////////////
    // Public API
    //////////////////////////////////////////////////////////////////////////

    AZ::Outcome<AZStd::vector<AZ::Entity*>, AZStd::string> CompileEditorEntities(
        const AZStd::vector<AZ::Entity*>& sourceEntities,
        const AZ::PlatformTagSet& platformTags,
        AZ::SerializeContext& serializeContext,
        const EditorOnlyEntityHandlers& editorOnlyEntityHandlers)
    {
        using namespace AzToolsFramework::Prefab::PrefabConversionUtils;

        AZStd::vector<AZ::Entity*> exportEntities;
        exportEntities.reserve(sourceEntities.size());

        // Determine which editor-only entity handler to use (if any)
        EditorOnlyEntityHandler* editorOnlyEntityHandler = nullptr;
        AZStd::unordered_set<AZ::EntityId> editorOnlyEntityIds;

        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            for (auto* handler : editorOnlyEntityHandlers)
            {
                if (handler->IsEntityUniquelyForThisHandler(sourceEntity))
                {
                    editorOnlyEntityHandler = handler;
                    break;
                }
            }
            if (editorOnlyEntityHandler)
            {
                break;
            }
        }

        // Export each entity
        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            auto result = Internal::ExportEntity(sourceEntity, platformTags, serializeContext);
            if (!result)
            {
                // Clean up already-exported entities
                for (auto* entity : exportEntities)
                {
                    delete entity;
                }
                return AZ::Failure(result.GetError());
            }

            AZ::Entity* exportEntity = result.GetValue();
            exportEntities.push_back(exportEntity);

            // Check if entity is editor-only
            if (editorOnlyEntityHandler)
            {
                bool isEditorOnly = false;
                AzToolsFramework::EditorOnlyEntityComponentRequestBus::EventResult(
                    isEditorOnly, sourceEntity->GetId(),
                    &AzToolsFramework::EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);
                if (isEditorOnly)
                {
                    editorOnlyEntityHandler->AddEditorOnlyEntity(sourceEntity, editorOnlyEntityIds);
                }
            }
        }

        // Handle editor-only entities (remove from hierarchy, then remove from list)
        if (editorOnlyEntityHandler && !editorOnlyEntityIds.empty())
        {
            auto handlerResult = editorOnlyEntityHandler->HandleEditorOnlyEntities(
                exportEntities, editorOnlyEntityIds, serializeContext);
            if (!handlerResult)
            {
                for (auto* entity : exportEntities)
                {
                    delete entity;
                }
                return AZ::Failure(AZStd::string::format(
                    "Error handling editor-only entities: %s", handlerResult.GetError().c_str()));
            }

            // Remove editor-only entities from the export list and delete them
            AZStd::erase_if(exportEntities, [&editorOnlyEntityIds](AZ::Entity* entity)
            {
                if (editorOnlyEntityIds.find(entity->GetId()) != editorOnlyEntityIds.end())
                {
                    delete entity;
                    return true;
                }
                return false;
            });
        }

        return AZ::Success(AZStd::move(exportEntities));
    }

} // namespace Shine
