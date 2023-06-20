/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/InGameUI/UiFrameworkBus.h>

#include <AzToolsFramework/Slice/SliceCompilation.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AzToolsFramework
{
    namespace Internal
    {
        using ShouldExportResult = AZ::Outcome<bool, AZStd::string>;                            // Outcome describing whether a component should be exported based on
                                                                                                // user EditContext attributes. Error string provided in failure case.
        using ExportedComponentResult = AZ::Outcome<AZ::ExportedComponent, AZStd::string>;      // Outcome describing final resolved component for export.
                                                                                                // Error string provided in error case.

        /**
         * Checks EditContext attributes to determine if the input component should be exported based on the current platform tags.
         */
        ShouldExportResult ShouldExportComponent(AZ::Component* component, const AZ::PlatformTagSet& platformTags, AZ::SerializeContext& serializeContext)
        {
            const AZ::SerializeContext::ClassData* classData = serializeContext.FindClassData(component->RTTI_GetType());
            if (!classData || !classData->m_editData)
            {
                return AZ::Success(true);
            }

            const AZ::Edit::ElementData* editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
            if (!editorDataElement)
            {
                return AZ::Success(true);
            }

            AZ::Edit::Attribute* allTagsAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::ExportIfAllPlatformTags);
            AZ::Edit::Attribute* anyTagsAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::ExportIfAnyPlatformTags);

            AZStd::vector<AZ::Crc32> attributeTags;

            // If the component has declared the 'ExportIfAllPlatforms' attribute, skip export if any of the flags are not present.
            if (allTagsAttribute)
            {
                attributeTags.clear();
                PropertyAttributeReader reader(component, allTagsAttribute);
                if (!reader.Read<decltype(attributeTags)>(attributeTags))
                {
                    return AZ::Failure(AZStd::string("'ExportIfAllPlatforms' attribute is not bound to the correct return type. Expects AZStd::vector<AZ::Crc32>."));
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
            if (anyTagsAttribute)
            {
                attributeTags.clear();
                PropertyAttributeReader reader(component, anyTagsAttribute);
                if (!reader.Read<decltype(attributeTags)>(attributeTags))
                {
                    return AZ::Failure(AZStd::string("'ExportIfAnyPlatforms' attribute is not bound to the correct return type. Expects AZStd::vector<AZ::Crc32>."));
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

        /**
         * Recursively resolves to the component that should be exported to the runtime slice per the current platform flags
         * and any custom user export callbacks.
         * This is recursive to allow deep exports, such an editor component exporting a runtime component, which in turn
         * exports a custom version of itself depending on platform.
         */
        ExportedComponentResult ResolveExportedComponent(AZ::ExportedComponent component,
            const AZ::PlatformTagSet& platformTags,
            AZ::SerializeContext& serializeContext)
        {
            AZ::Component* inputComponent = component.m_component;

            if (!inputComponent)
            {
                return AZ::Success(AZ::ExportedComponent(component));
            }

            // Don't export the component if it has unmet platform tag requirements.
            ShouldExportResult shouldExportResult = ShouldExportComponent(inputComponent, platformTags, serializeContext);
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
            const AZ::SerializeContext::ClassData* classData = serializeContext.FindClassData(inputComponent->RTTI_GetType());
            if (classData && classData->m_editData)
            {
                const AZ::Edit::ElementData* editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                if (editorDataElement)
                {
                    AZ::Edit::Attribute* exportCallbackAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::RuntimeExportCallback);
                    if (exportCallbackAttribute)
                    {
                        PropertyAttributeReader reader(inputComponent, exportCallbackAttribute);
                        AZ::ExportedComponent exportedComponent;

                        if (reader.Read<AZ::ExportedComponent>(exportedComponent, inputComponent, platformTags))
                        {
                            // If the callback handled the export and provided a different component instance, continue to resolve recursively.
                            if (exportedComponent.m_componentExportHandled && (exportedComponent.m_component != inputComponent))
                            {
                                return ResolveExportedComponent(exportedComponent, platformTags, serializeContext);
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
                }
            }

            // If there's no custom export callback, just return what we were given.
            return AZ::Success(component);
        }

        //! Iterates through the list of entities for each handler provided and returns the first handler
        //! that can handle at least one of the entities in the list.
        //!
        //! We currently don't have support the concept of using multiple handlers for a given list of
        //! entities. So once a handler is found, we assume that it can handle all of the entities in
        //! the list.
        //!
        //! This may not always be true if the list contains world entities and UI element entities, for
        //! example - so this may need udpating eventually.
        EditorOnlyEntityHandler* FindHandlerForEntities(const AZ::SliceComponent::EntityList& entities, const EditorOnlyEntityHandlers& editorOnlyEntityHandlers)
        {
            EditorOnlyEntityHandler* editorOnlyEntityHandler = nullptr;
            for (auto handlerCandidate : editorOnlyEntityHandlers)
            {
                const bool handlerInvalid = handlerCandidate == nullptr;
                if (handlerInvalid)
                {
                    continue;
                }

                // See if this handler can handle at least one of the entities.
                for (AZ::Entity* entity : entities)
                {
                    if (handlerCandidate->IsEntityUniquelyForThisHandler(entity))
                    {
                        editorOnlyEntityHandler = handlerCandidate;
                        break;
                    }
                }

                const bool editorOnlyHandlerValid = editorOnlyEntityHandler != nullptr;
                if (editorOnlyHandlerValid)
                {
                    break;
                }
            }

            return editorOnlyEntityHandler;
        }

        /**
        * Identify and remove any entities marked as editor-only.
        * If any are discovered, adjust descendants' transforms to retain spatial relationships.
        * Note we cannot use EBuses for this purpose, since we're crunching data, and can't assume any entities are active.
        */
        EditorOnlyEntityHandler::Result 
        AdjustForEditorOnlyEntities(AZ::SliceComponent* slice, const AZStd::unordered_set<AZ::EntityId>& editorOnlyEntities, AZ::SerializeContext& serializeContext, EditorOnlyEntityHandler* customHandler)
        {
            AzToolsFramework::EntityList entities;
            slice->GetEntities(entities);

            // Invoke custom handler if provided, so callers can process the slice to account for editor-only entities
            // that are about to be removed.
            if (customHandler)
            {
                const EditorOnlyEntityHandler::Result handlerResult = customHandler->HandleEditorOnlyEntities(entities, editorOnlyEntities, serializeContext);
                if (!handlerResult)
                {
                    return handlerResult;
                }
            }

            // Remove editor-only entities from the slice's final entity list.
            for (auto entityIter = entities.begin(); entityIter != entities.end(); )
            {
                AZ::Entity* entity = (*entityIter);

                if (editorOnlyEntities.find(entity->GetId()) != editorOnlyEntities.end())
                {
                    entityIter = entities.erase(entityIter);
                    slice->RemoveEntity(entity);
                }
                else
                {
                    ++entityIter;
                }
            }

            return AZ::Success();
        }

        AZ::TransformInterface* FindTransformInterfaceComponent(AZ::Entity& entity)
        {
            for (AZ::Component* component : entity.GetComponents())
            {
                if (auto transformInterface = azrtti_cast<AZ::TransformInterface*>(component))
                {
                    return transformInterface;
                }
            }
            return nullptr;
        }

    } // namespace Internal

    /**
     * Compiles the provided source slice into a runtime slice.
     * Components are validated and exported considering platform tags and EditContext-driven user validation and export customizations.
     */
    SliceCompilationResult CompileEditorSlice(const AZ::Data::Asset<AZ::SliceAsset>& sourceSliceAsset, const AZ::PlatformTagSet& platformTags, AZ::SerializeContext& serializeContext, const EditorOnlyEntityHandlers& editorOnlyEntityHandlers)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);
        if (!sourceSliceAsset)
        {
            return AZ::Failure(AZStd::string("Source slice is invalid."));
        }

        AZ::SliceComponent::EntityList sourceEntities;
        sourceSliceAsset.Get()->GetComponent()->GetEntities(sourceEntities);

        // Create a new target slice asset to which we'll export entities & components.
        AZ::Entity* exportSliceEntity = aznew AZ::Entity(sourceSliceAsset.Get()->GetEntity()->GetId());
        AZ::SliceComponent* exportSliceData = exportSliceEntity->CreateComponent<AZ::SliceComponent>();
        AZ::Data::Asset<AZ::SliceAsset> exportSliceAsset;
        exportSliceAsset.Create(AZ::Data::AssetId(AZ::Uuid::CreateRandom()));
        exportSliceAsset.Get()->SetData(exportSliceEntity, exportSliceData);

        // For export, components can assume they're initialized, but not activated.
        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            if (sourceEntity->GetState() == AZ::Entity::State::Constructed)
            {
                sourceEntity->Init();
            }
        }

        // Prepare source entity container for validation callbacks.
        AZ::ImmutableEntityVector immutableSourceEntities;
        immutableSourceEntities.reserve(sourceEntities.size());
        for (AZ::Entity* entity : sourceEntities)
        {
            immutableSourceEntities.push_back(entity);
        }

        // Pick the correct handler to use
        EditorOnlyEntityHandler* editorOnlyEntityHandler = Internal::FindHandlerForEntities(sourceEntities, editorOnlyEntityHandlers);
        const bool editorOnlyHandlerValid = editorOnlyEntityHandler != nullptr;

        EntityIdSet editorOnlyEntities;

        // Prepare entities for export. This involves invoking BuildGameEntity on source
        // entity's components, targeting a separate entity for export.
        for (AZ::Entity* sourceEntity : sourceEntities)
        {
            auto exportEntity = AZStd::make_unique<AZ::Entity>(sourceEntity->GetId(), sourceEntity->GetName().c_str());
            exportEntity->SetRuntimeActiveByDefault(sourceEntity->IsRuntimeActiveByDefault());

            bool isEditorOnly = false;
            EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, sourceEntity->GetId(), &EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);
            if (isEditorOnly && editorOnlyHandlerValid)
            {
                editorOnlyEntityHandler->AddEditorOnlyEntity(sourceEntity, editorOnlyEntities);
            }

            const AZ::Entity::ComponentArrayType& editorComponents = sourceEntity->GetComponents();
            for (AZ::Component* component : editorComponents)
            {
                auto* asEditorComponent =
                    azrtti_cast<Components::EditorComponentBase*>(component);

                // Call validation callback on source component.  
                // (Later, we'll call the validation callback on the final exported component as well.)
                AZ::ComponentValidationResult result = component->ValidateComponentRequirements(immutableSourceEntities, platformTags);
                if (!result.IsSuccess())
                {
                    // Try to cast to GenericComponentWrapper, and if we can, get the internal template.
                    const char* componentName = component->RTTI_GetTypeName();
                    Components::GenericComponentWrapper* wrapper = azrtti_cast<Components::GenericComponentWrapper*>(asEditorComponent);
                    if (wrapper && wrapper->GetTemplate())
                    {
                        componentName = wrapper->GetTemplate()->RTTI_GetTypeName();
                    }

                    return AZ::Failure(AZStd::string::format("[Entity] \"%s\" [Entity Id] 0x%llu, [Editor Component] \"%s\" could not pass validation for [Slice] \"%s\" [Error] %s",
                        sourceEntity->GetName().c_str(),
                        static_cast<AZ::u64>(sourceEntity->GetId()),
                        componentName,
                        sourceSliceAsset.GetHint().c_str(),
                        result.GetError().c_str()));
                }

                // Whether or not this is an editor component, the source component might have a custom user export callback,
                // so try to call it.
                const Internal::ExportedComponentResult exportResult = Internal::ResolveExportedComponent(
                    AZ::ExportedComponent(component, false, false), 
                    platformTags, 
                    serializeContext);

                if (!exportResult)
                {
                    return AZ::Failure(AZStd::string::format("Source component \"%s\" could not be exported for Entity \"%s\" [0x%llu] due to export attributes: %s.",
                        component->RTTI_GetTypeName(),
                        exportEntity->GetName().c_str(),
                        static_cast<AZ::u64>(exportEntity->GetId()),
                        exportResult.GetError().c_str()));
                }

                AZ::ExportedComponent exportedComponent = exportResult.GetValue();

                // If ResolveExportedComponent didn't handle the component export, then we'll do the following:
                // - For editor components, fall back on the legacy BuildGameEntity() path for handling component exports.
                // - For runtime components, provide a default behavior of "clone / add" to export the component.
                if (!exportedComponent.m_componentExportHandled)
                {
                    // Editor components:  Try to use BuildGameEntity()
                    if (asEditorComponent) // BEGIN BuildGameEntity compatibility path for editor components not using the newer RuntimeExportCallback functionality.
                    {
                        const size_t oldComponentCount = exportEntity->GetComponents().size();
                        asEditorComponent->BuildGameEntity(exportEntity.get());
                        AZ::ComponentId newID = asEditorComponent->GetId();
                        for (auto i = oldComponentCount ; i < exportEntity->GetComponents().size(); ++i)
                        {
                            AZ::Component* exportComponent = exportEntity->GetComponents()[i];

                            // Verify that the result of BuildGameEntity() wasn't an editor component.
                            auto* exportAsEditorComponent = azrtti_cast<Components::EditorComponentBase*>(exportComponent);
                            if (exportAsEditorComponent)
                            {
                                return AZ::Failure(AZStd::string::format("Entity \"%s\" [0x%llu], component \"%s\" exported an editor component from BuildGameEntity() for runtime use.",
                                    sourceEntity->GetName().c_str(),
                                    static_cast<AZ::u64>(sourceEntity->GetId()),
                                    asEditorComponent->RTTI_GetType().ToString<AZStd::string>().c_str()));
                            }

                            if (asEditorComponent->GetId() == AZ::InvalidComponentId)
                            {
                                return AZ::Failure(AZStd::string::format("Entity \"%s\" [0x%llu], component \"%s\" doesn't have a valid component Id.",
                                    sourceEntity->GetName().c_str(),
                                    static_cast<AZ::u64>(sourceEntity->GetId()),
                                    asEditorComponent->RTTI_GetType().ToString<AZStd::string>().c_str()));
                            }

                            exportComponent->SetId(newID++);
                            // The first time round set the new componet the same as the ditors one. This will change in a seperate ticket
                            // when 8 bit runtime Ids are implemented.
                            // Make sure the newID isn't already on the source Entity. If it is increment the ID and try again.
                            while (sourceEntity->FindComponent(newID))
                            {
                                ++newID;
                            }
                        }

                        // Since this is an editor component, we very specifically do *not* want to clone and add it as a runtime
                        // component by default, so regardless of whether or not the BuildGameEntity() call did anything, 
                        // null out the editor component and mark it handled.
                        exportedComponent = AZ::ExportedComponent();

                    } // END BuildGameEntity compatibility path for editor components not using the newer RuntimeExportCallback functionality.
                    else
                    {
                        // Nothing else has handled the component export, so fall back on the default behavior 
                        // for runtime components:  clone and add the runtime component that already exists.
                        exportedComponent = AZ::ExportedComponent(component, false, true);
                    }
                }

                // At this point, either ResolveExportedComponent or the default logic above should have set the component export 
                // as being handled.  If not, there is likely a new code path that requires a default export behavior.
                AZ_Assert(exportedComponent.m_componentExportHandled, "Component \"%s\" had no export handlers and could not be added to Entity \"%s\" [0x%llu].",
                    component->RTTI_GetTypeName(),
                    exportEntity->GetName().c_str(),
                    static_cast<AZ::u64>(exportEntity->GetId()));

                // If we have an exported component, we add it to the exported entity.  
                // If we don't (m_component == nullptr), this component chose not to be exported, so we skip it.
                if (exportedComponent.m_componentExportHandled && exportedComponent.m_component)
                {
                    AZ::Component* runtimeComponent = exportedComponent.m_component;

                    // Verify that we aren't trying to export an editor component.
                    auto* exportAsEditorComponent = azrtti_cast<Components::EditorComponentBase*>(runtimeComponent);
                    if (exportAsEditorComponent)
                    {
                        return AZ::Failure(AZStd::string::format("Entity \"%s\" [0x%llu], component \"%s\" is trying to export an Editor component for runtime use.",
                            sourceEntity->GetName().c_str(),
                            static_cast<AZ::u64>(sourceEntity->GetId()),
                            asEditorComponent->RTTI_GetType().ToString<AZStd::string>().c_str()));
                    }

                    // If the final component is not owned by us, make our own copy.
                    if (!exportedComponent.m_deleteAfterExport)
                    {
                        runtimeComponent = serializeContext.CloneObject(runtimeComponent);
                    }

                    // Synchronize to source component Id, and add to the export entity.
                    runtimeComponent->SetId(component->GetId());

                    if (!exportEntity->AddComponent(runtimeComponent))
                    {
                        return AZ::Failure(AZStd::string::format("Component \"%s\" could not be added to Entity \"%s\" [0x%llu].",
                            runtimeComponent->RTTI_GetTypeName(),
                            exportEntity->GetName().c_str(),
                            static_cast<AZ::u64>(exportEntity->GetId())));
                    }
                }
            }

            // Pre-sort prior to exporting so it isn't required at instantiation time.
            const AZ::Entity::DependencySortOutcome sortOutcome = exportEntity->EvaluateDependenciesGetDetails();
            // :CBR_TODO: verify AZ::Entity::DependencySortResult::HasIncompatibleServices and AZ::Entity::DependencySortResult::DescriptorNotRegistered are still covered here
            if (!sortOutcome.IsSuccess())
            {
                return AZ::Failure(AZStd::string::format("Entity \"%s\" %s dependency evaluation failed. %s",
                    exportEntity->GetName().c_str(),
                    exportEntity->GetId().ToString().c_str(),
                    sortOutcome.GetError().m_message.c_str()));
            }

            exportSliceData->AddEntity(exportEntity.release());
        }

        {
            AZ::SliceComponent::EntityList exportEntities;
            exportSliceData->GetEntities(exportEntities);

            if (exportEntities.size() != sourceEntities.size())
            {
                return AZ::Failure(AZStd::string("Entity export list size must match that of the import list."));
            }
        }

        // Notify user callback, and then strip out any editor-only entities.
        // This operation can generate a failure if a the callback failed validation.
        if (!editorOnlyEntities.empty())
        {
            EditorOnlyEntityHandler::Result result = Internal::AdjustForEditorOnlyEntities(exportSliceData, editorOnlyEntities, serializeContext, editorOnlyEntityHandler);
            if (!result)
            {
                return AZ::Failure(result.TakeError());
            }
        }

        // Sort entities by transform hierarchy, so parents will activate before children
        {
            AZStd::vector<AZ::Entity*> sortedEntities;
            exportSliceData->GetEntities(sortedEntities);
            SortTransformParentsBeforeChildren(sortedEntities);

            // Sort the entities in the slice by removing them, and putting them back in sorted order
            exportSliceData->RemoveAllEntities(/*deleteEntities*/ false, /*removeEmptyInstances*/ false);
            for (AZ::Entity* entity : sortedEntities)
            {
                exportSliceData->AddEntity(entity);
            }
        }

        // Call validation callbacks on final runtime components.
        AZ::SliceComponent::EntityList exportEntities;
        exportSliceData->GetEntities(exportEntities);

        AZ::ImmutableEntityVector immutableExportEntities;
        immutableExportEntities.reserve(exportEntities.size());
        for (AZ::Entity* entity : exportEntities)
        {
            immutableExportEntities.push_back(entity);
        }

        for (AZ::Entity* exportEntity : exportEntities)
        {
            const AZ::Entity::ComponentArrayType& gameComponents = exportEntity->GetComponents();
            for (AZ::Component* component : gameComponents)
            {
                AZ::ComponentValidationResult result = component->ValidateComponentRequirements(immutableExportEntities, platformTags);
                if (!result.IsSuccess())
                {
                    // Try to cast to GenericComponentWrapper, and if we can, get the internal template.
                    const char* componentName = component->RTTI_GetTypeName();
                    return AZ::Failure(AZStd::string::format("[Entity] \"%s\" [Entity Id] 0x%llu, [Exported Component] \"%s\" could not pass validation for [Slice] \"%s\" [Error] %s",
                        exportEntity->GetName().c_str(), 
                        static_cast<AZ::u64>(exportEntity->GetId()),
                        componentName,
                        sourceSliceAsset.GetHint().c_str(),
                        result.GetError().c_str()));
                }
            }
        }
    
        return AZ::Success(exportSliceAsset);
    }

    bool WorldEditorOnlyEntityHandler::IsEntityUniquelyForThisHandler(AZ::Entity* entity)
    {
        return Internal::FindTransformInterfaceComponent(*entity) != nullptr;
    }
    
    EditorOnlyEntityHandler::Result WorldEditorOnlyEntityHandler::HandleEditorOnlyEntities(const AzToolsFramework::EntityList& entities, const AzToolsFramework::EntityIdSet& editorOnlyEntityIds, AZ::SerializeContext& serializeContext)
    {
        FixTransformRelationships(entities, editorOnlyEntityIds);

        return ValidateReferences(entities, editorOnlyEntityIds, serializeContext);
    }

    void WorldEditorOnlyEntityHandler::FixTransformRelationships(const AzToolsFramework::EntityList& entities, const AzToolsFramework::EntityIdSet& editorOnlyEntityIds)
    {
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::Entity*>> parentToChildren;

        // Build a map of entity Ids to their parent Ids, for faster lookup during processing.
        for (AZ::Entity* entity : entities)
        {
            AZ::TransformInterface* transformComponent = AZ::EntityUtils::FindFirstDerivedComponent<AZ::TransformInterface>(entity);
            if (transformComponent)
            {
                const AZ::EntityId parentId = transformComponent->GetParentId();
                if (parentId.IsValid())
                {
                    parentToChildren[parentId].push_back(entity);
                }
            }
        }

        // Identify any editor-only entities. If we encounter one, adjust transform relationships
        // for all of its children to ensure relative transforms are maintained and respected at
        // runtime.
        // This works regardless of entity ordering in the slice because we add reassigned children to 
        // parentToChildren cache during the operation.
        for (AZ::Entity* entity : entities)
        {
            if (editorOnlyEntityIds.end() == editorOnlyEntityIds.find(entity->GetId()))
            {
                continue; // This is not an editor-only entity.
            }

            AZ::TransformInterface* transformComponent = AZ::EntityUtils::FindFirstDerivedComponent<AZ::TransformInterface>(entity);
            if (transformComponent)
            {
                const AZ::Transform& parentLocalTm = transformComponent->GetLocalTM();

                // Identify all transform children and adjust them to be children of the removed entity's parent.
                for (AZ::Entity* childEntity : parentToChildren[entity->GetId()])
                {
                    AZ::TransformInterface* childTransformComponent = AZ::EntityUtils::FindFirstDerivedComponent<AZ::TransformInterface>(childEntity);

                    if (childTransformComponent && childTransformComponent->GetParentId() == entity->GetId())
                    {
                        const AZ::Transform localTm = childTransformComponent->GetLocalTM();
                        childTransformComponent->SetParent(transformComponent->GetParentId());
                        childTransformComponent->SetLocalTM(parentLocalTm * localTm);

                        parentToChildren[transformComponent->GetParentId()].push_back(childEntity);
                    }
                }
            }
        }
    }

    EditorOnlyEntityHandler::Result EditorOnlyEntityHandler::ValidateReferences(const AzToolsFramework::EntityList& entities, const AzToolsFramework::EntityIdSet& editorOnlyEntityIds, AZ::SerializeContext& serializeContext)
    {
        EditorOnlyEntityHandler::Result result = AZ::Success();

        // Inspect all runtime entities via the serialize context and identify any references to editor-only entity Ids.
        for (AZ::Entity* runtimeEntity : entities)
        {
            if (editorOnlyEntityIds.end() != editorOnlyEntityIds.find(runtimeEntity->GetId()))
            {
                continue; // This is not a runtime entity, so no need to validate its references as it's going away.
            }

            AZ::EntityUtils::EnumerateEntityIds<AZ::Entity>(runtimeEntity, 
                [&editorOnlyEntityIds, &result, runtimeEntity](const AZ::EntityId& id, bool /*isEntityId*/, const AZ::SerializeContext::ClassElement* /*elementData*/)
                {
                    if (editorOnlyEntityIds.end() != editorOnlyEntityIds.find(id))
                    {
                        result = AZ::Failure(AZStd::string::format("A runtime entity (%s) contains references to an entity marked as editor-only.", runtimeEntity->GetName().c_str()));
                        return false;
                    }

                    return true;
                }
                , &serializeContext);

            if (!result)
            {
                break;
            }
        }

        return result;
    }

    // perform breadth-first topological sort, placing parents before their children.
    // tolerate ALL possible input errors (looping parents, invalid IDs, etc).
    void SortTransformParentsBeforeChildren(AZStd::vector<AZ::Entity*>& entities)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // IDs of those present in 'entities'. Does not include parent ID if parent not found in 'entities'
        AZStd::unordered_set<AZ::EntityId> existingEntityIds;

        // map children by their parent ID (even if parent not found in 'entities')
        AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZ::Entity*>> parentIdToChildrenPtrs;

        // store any entities with bad setups here, we'll put them last in the final sort
        AZStd::vector<AZ::Entity*> badEntities;

        // gather data about the entities...
        for (AZ::Entity* entity : entities)
        {
            if (!entity)
            {
                badEntities.push_back(entity);
                continue;
            }

            AZ::EntityId entityId = entity->GetId();

            if (!entityId.IsValid())
            {
                AZ_Warning("Entity", false, "Hierarchy sort found entity '%s' with invalid ID", entity->GetName().c_str());

                badEntities.push_back(entity);
                continue;
            }

            bool entityIdIsUnique = existingEntityIds.insert(entityId).second;
            if (!entityIdIsUnique)
            {
                AZ_Warning("Entity", false, "Hierarchy sort found multiple entities using same ID as entity '%s' %s",
                    entity->GetName().c_str(),
                    entityId.ToString().c_str());

                badEntities.push_back(entity);
                continue;
            }

            // search for any component that implements the TransformInterface.
            // don't use EBus because we support sorting entities that haven't been initialized or activated.
            // entities with no transform component will be treated like entities with no parent.
            AZ::EntityId parentId;
            if (AZ::TransformInterface* transformInterface = AZ::EntityUtils::FindFirstDerivedComponent<AZ::TransformInterface>(entity))
            {
                parentId = transformInterface->GetParentId();

                // if entity is parented to itself, sort it as if it had no parent
                if (parentId == entityId)
                {
                    AZ_Warning("Entity", false, "Hierarchy sort found entity parented to itself '%s' %s",
                        entity->GetName().c_str(),
                        entityId.ToString().c_str());

                    parentId.SetInvalid();
                }
            }

            parentIdToChildrenPtrs[parentId].push_back(entity);
        }

        // clear 'entities', we'll refill it in sorted order...
        const size_t originalEntityCount = entities.size();
        entities.clear();

        // use 'candidateIds' to track the parent IDs we're going to process next.
        // the first candidates should be the parents of the roots.
        AZStd::vector<AZ::EntityId> candidateIds;
        candidateIds.reserve(originalEntityCount + 1);
        for (auto& parentChildrenPair : parentIdToChildrenPtrs)
        {
            const AZ::EntityId& parentId = parentChildrenPair.first;

            // we found a root if parent ID doesn't correspond to any entity in the list
            if (existingEntityIds.find(parentId) == existingEntityIds.end())
            {
                candidateIds.push_back(parentId);
            }
        }

        // process candidates until everything is sorted:
        // - add candidate's children to the final sorted order
        // - add candidate's children to list of candidates, so we can process *their* children in a future loop
        // - erase parent/children entry from parentToChildrenIds
        // - continue until nothing is left in parentToChildrenIds
        for (size_t candidateIndex = 0; !parentIdToChildrenPtrs.empty(); ++candidateIndex)
        {
            // if there are no more candidates, but there are still unsorted children, then we have an infinite loop.
            // pick an arbitrary parent from the loop to be the next candidate.
            if (candidateIndex == candidateIds.size())
            {
                const AZ::EntityId& parentFromLoopId = parentIdToChildrenPtrs.begin()->first;

#ifdef AZ_ENABLE_TRACING
                // Find name to use in warning message
                AZStd::string parentFromLoopName;
                for (auto& parentIdChildrenPtrPair : parentIdToChildrenPtrs)
                {
                    for (AZ::Entity* entity : parentIdChildrenPtrPair.second)
                    {
                        if (entity->GetId() == parentFromLoopId)
                        {
                            parentFromLoopName = entity->GetName();
                            break;
                        }
                        if (!parentFromLoopName.empty())
                        {
                            break;
                        }
                    }
                }

                AZ_Warning("Entity", false, "Hierarchy sort found parenting loop involving entity '%s' %s",
                    parentFromLoopName.c_str(),
                    parentFromLoopId.ToString().c_str());
#endif // AZ_ENABLE_TRACING

                candidateIds.push_back(parentFromLoopId);
            }

            const AZ::EntityId& parentId = candidateIds[candidateIndex];

            auto foundChildren = parentIdToChildrenPtrs.find(parentId);
            if (foundChildren != parentIdToChildrenPtrs.end())
            {
                for (AZ::Entity* child : foundChildren->second)
                {
                    entities.push_back(child);
                    candidateIds.push_back(child->GetId());
                }

                parentIdToChildrenPtrs.erase(foundChildren);
            }
        }

        // put bad entities at the end of the sorted list
        entities.insert(entities.end(), badEntities.begin(), badEntities.end());

        AZ_Assert(entities.size() == originalEntityCount, "Wrong number of entities after sort! This algorithm is busted.");
    }
    
    bool UiEditorOnlyEntityHandler::IsEntityUniquelyForThisHandler(AZ::Entity* entity)
    {
        // Assume that an entity is a UI element if it has a UI element component.
        bool uniqueForThisHandler = false;
        UiFrameworkBus::BroadcastResult(uniqueForThisHandler, &UiFrameworkInterface::HasUiElementComponent, entity);
        return uniqueForThisHandler;
    }

    void UiEditorOnlyEntityHandler::AddEditorOnlyEntity(AZ::Entity* editorOnlyEntity, EntityIdSet& editorOnlyEntities)
    {
        UiFrameworkBus::Broadcast(&UiFrameworkInterface::AddEditorOnlyEntity, editorOnlyEntity, editorOnlyEntities);
    }

    EditorOnlyEntityHandler::Result UiEditorOnlyEntityHandler::HandleEditorOnlyEntities(const AzToolsFramework::EntityList& exportSliceEntities, const AzToolsFramework::EntityIdSet& editorOnlyEntityIds, AZ::SerializeContext& serializeContext)
    {
        UiFrameworkBus::Broadcast(&UiFrameworkInterface::HandleEditorOnlyEntities, exportSliceEntities, editorOnlyEntityIds);

        // Perform a final check to verify that all editor-only entities have been removed
        auto result = ValidateReferences(exportSliceEntities, editorOnlyEntityIds, serializeContext);

        return result;
    }

} // AzToolsFramework
