/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiEditorEntityContext.h"

#include "Commands/CommandHierarchyItemDelete.h"
#include "Commands/CommandHierarchyItemCreateFromData.h"
#include "Helpers/EntityHelpers.h"
#include "Helpers/HierarchyHelpers.h"
#include "Helpers/SelectionHelpers.h"
#include "Widgets/HierarchyWidget/HierarchyWidget.h"
#include "Widgets/HierarchyWidget/HierarchyItem.h"
#include "Widgets/HierarchyWidget/TreeWidgetItemList.h"
#include "Windows/EditorWindow/EditorWindow.h"
#include "Undo/UndoStack.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponent.h>
#include <AzToolsFramework/Prefab/Spawnable/EditorOnlyEntityHandler/UiEditorOnlyEntityHandler.h>

#include "UiEditorEntityCompilation.h"

#include <Shine/Bus/UiElementBus.h>
#include <Shine/Bus/UiTransformBus.h>
#include <Shine/Bus/UiCanvasBus.h>
#include <Shine/Bus/Tools/UiSystemToolsBus.h>
#include <Shine/UiComponentTypes.h>

namespace Internal
{
    void RemoveIncompatibleComponents(AZ::Entity* entity)
    {
        const AZ::Entity::ComponentArrayType components = entity->GetComponents();
        AZ::Entity::ComponentArrayType validComponents;
        AZ::Entity::ComponentArrayType incompatibleComponents;
        AZ::ComponentDescriptor::DependencyArrayType incompatibleServices;
        AZ::ComponentDescriptor::DependencyArrayType providedServices;
        AZStd::string incompatibleNames;
        for (auto component : components)
        {
            AZ::ComponentDescriptor* testComponentDesc = nullptr;
            AZ::ComponentDescriptorBus::EventResult(testComponentDesc, azrtti_typeid(component), &AZ::ComponentDescriptorBus::Events::GetDescriptor);
            providedServices.clear();
            testComponentDesc->GetProvidedServices(providedServices, component);

            bool isIncompatible = false;
            for (auto validComponent : validComponents)
            {
                AZ::ComponentDescriptor* validComponentDesc = nullptr;
                AZ::ComponentDescriptorBus::EventResult(validComponentDesc, azrtti_typeid(validComponent), &AZ::ComponentDescriptorBus::Events::GetDescriptor);

                incompatibleServices.clear();
                validComponentDesc->GetIncompatibleServices(incompatibleServices, validComponent);

                auto foundItr = AZStd::find_first_of(incompatibleServices.begin(), incompatibleServices.end(), providedServices.begin(), providedServices.end());
                if (foundItr != incompatibleServices.end())
                {
                    isIncompatible = true;
                    break;
                }
            }

            if (isIncompatible)
            {
                incompatibleComponents.push_back(component);

                incompatibleNames.append(testComponentDesc->GetName());
                incompatibleNames += '\n';
            }
            else
            {
                validComponents.push_back(component);
            }
        }

        // Should be safe to remove components, because the entity hasn't been activated.
        for (auto componentToRemove : incompatibleComponents)
        {
            entity->RemoveComponent(componentToRemove);
        }

        AZ_Error("UiCanvas", incompatibleComponents.empty(), "The following incompatible component(s) are removed from the entity %s:\n%s", entity->GetName().c_str(), incompatibleNames.c_str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiEditorEntityContext::UiEditorEntityContext(EditorWindow* editorWindow)
    : m_editorWindow(editorWindow)
    , m_requiredEditorComponentTypes
    ({
        azrtti_typeid<AzToolsFramework::Components::EditorOnlyEntityComponent>()
    })
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UiEditorEntityContext::~UiEditorEntityContext()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::HandleLoadedEntities(const AZStd::vector<AZ::Entity*>& entities, bool remapIds,
    AZStd::unordered_map<AZ::EntityId, AZ::EntityId>* idRemapTable)
{
    AZ_Assert(m_entityOwnershipService->IsInitialized(), "The context has not been initialized.");

    auto* ownershipService = static_cast<UiSimpleEntityOwnershipService*>(m_entityOwnershipService.get());
    ownershipService->LoadEntities(entities, remapIds, idRemapTable);

    AzFramework::EntityList allEntities;
    m_entityOwnershipService->GetAllEntities(allEntities);

    InitializeEntities(allEntities);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::InitUiContext()
{
    m_entityOwnershipService = AZStd::make_unique<UiSimpleEntityOwnershipService>(GetContextId(), GetSerializeContext());
    InitContext();

    m_entityOwnershipService->InstantiateAllPrefabs();

    UiEntityContextRequestBus::Handler::BusConnect(GetContextId());
    UiEditorEntityContextRequestBus::Handler::BusConnect(GetContextId());
    AzToolsFramework::EditorEntityContextPickingRequestBus::Handler::BusConnect(GetContextId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::DestroyUiContext()
{
    UiEditorEntityContextRequestBus::Handler::BusDisconnect();
    UiEntityContextRequestBus::Handler::BusDisconnect();
    AzToolsFramework::EditorEntityContextPickingRequestBus::Handler::BusDisconnect();

    DestroyContext();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::SaveToStreamForGame(AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType)
{
    AzFramework::EntityList sourceEntities;
    m_entityOwnershipService->GetAllEntities(sourceEntities);

    // Emulate client flags.
    AZ::PlatformTagSet platformTags = { AZ_CRC_CE("renderer") };

    // Compile editor entities to runtime entities.
    AzToolsFramework::Prefab::PrefabConversionUtils::UiEditorOnlyEntityHandler uiEditorOnlyEntityHandler;
    Shine::EditorOnlyEntityHandlers handlers = { &uiEditorOnlyEntityHandler };
    auto compilationResult = Shine::CompileEditorEntities(sourceEntities, platformTags, *m_serializeContext, handlers);

    if (!compilationResult)
    {
        m_errorMessage = compilationResult.GetError();
        return false;
    }

    AZStd::vector<AZ::Entity*> exportEntities = compilationResult.TakeValue();

    if (exportEntities.empty())
    {
        AZ_Error("Save Runtime Stream", false, "No entities were exported for runtime.");
        return false;
    }

    // Serialize the entities directly as a vector
    bool result = AZ::Utils::SaveObjectToStream<AZStd::vector<AZ::Entity*>>(stream, streamType, &exportEntities);

    // Clean up exported entities -- they are clones
    for (auto* entity : exportEntities)
    {
        delete entity;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::SaveCanvasEntityToStreamForGame(AZ::Entity* canvasEntity, AZ::IO::GenericStream& stream, AZ::DataStream::StreamType streamType)
{
    AZ::Entity* sourceCanvasEntity = canvasEntity;
    AZ::Entity* exportCanvasEntity = aznew AZ::Entity(sourceCanvasEntity->GetName().c_str());
    exportCanvasEntity->SetId(sourceCanvasEntity->GetId());
    AZ_Assert(exportCanvasEntity, "Failed to create target entity \"%s\" for export.",
        sourceCanvasEntity->GetName().c_str());

    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::PreExportEntity, *sourceCanvasEntity, *exportCanvasEntity);

    // Export entity representing the canvas, which has only runtime components.
    AZ::Utils::SaveObjectToStream<AZ::Entity>(stream, streamType, exportCanvasEntity);

    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::PostExportEntity, *sourceCanvasEntity, *exportCanvasEntity);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AZ::Entity* UiEditorEntityContext::CreateUiEntity(const char* name)
{
    AZ::Entity* entity = CreateEntity(name);
    return entity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::AddUiEntity(AZ::Entity* entity)
{
    AZ_Assert(entity, "Supplied entity is invalid.");
    AddEntity(entity);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::AddUiEntities(const AzFramework::EntityList& entities)
{
    for (AZ::Entity* entity : entities)
    {
        AZ_Assert(!AzFramework::EntityIdContextQueryBus::MultiHandler::BusIsConnectedId(entity->GetId()), "Entity already in context.");
    }

    m_entityOwnershipService->AddEntities(entities);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::CloneUiEntities(const AZStd::vector<AZ::EntityId>& sourceEntities, AzFramework::EntityList& resultEntities)
{
    resultEntities.clear();

    AzFramework::EntityList sourceEntityList;
    for (const AZ::EntityId& id : sourceEntities)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
        if (entity)
        {
            sourceEntityList.push_back(entity);
        }
    }

    AZStd::unordered_map<AZ::EntityId, AZ::EntityId> idMap;
    AzFramework::EntityList* clonedEntities =
        AZ::IdUtils::Remapper<AZ::EntityId>::CloneObjectAndGenerateNewIdsAndFixRefs(&sourceEntityList, idMap);
    if (!clonedEntities)
    {
        AZ_Error("UiEntityContext", false, "Failed to clone source entities.");
        return false;
    }

    resultEntities = *clonedEntities;

    AddUiEntities(resultEntities);

    delete clonedEntities;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::DestroyUiEntity(AZ::EntityId entityId)
{
    return DestroyEntityById(entityId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::SupportsViewportEntityIdPicking()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::DeleteElements(AzToolsFramework::EntityIdList elements)
{
    // Deletes the specified elements using an undoable command
    if (elements.size() > 0)
    {
        HierarchyWidget* hierarchy = m_editorWindow->GetHierarchy();

        // Get the list of currently selected entities so that we can attempt to restore that
        // after the delete (the undoable command currently only works on selected entities)
        QTreeWidgetItemRawPtrQList selection = hierarchy->selectedItems();
        EntityHelpers::EntityIdList selectedEntities = SelectionHelpers::GetSelectedElementIds(hierarchy, selection, false);

        // Make sure elements still exist.
        elements.erase(
            AZStd::remove_if(
                elements.begin(), elements.end(),
                [](AZ::EntityId entityId)
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
                    return !entity;
                }),
            elements.end());

        if (elements.empty())
        {
            return;
        }

        // Use an undoable command to delete the entities
        HierarchyHelpers::SetSelectedItems(hierarchy, &elements);
        CommandHierarchyItemDelete::Push(m_editorWindow->GetActiveStack(),
            hierarchy,
            hierarchy->selectedItems());

        // Attempt to set the selection back to what it was but first remove any items from the selected
        // list that no longer exist
        selectedEntities.erase(
            AZStd::remove_if(
                selectedEntities.begin(), selectedEntities.end(),
                [](AZ::EntityId entityId)
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
                    return !entity;
                }),
            selectedEntities.end());

        HierarchyHelpers::SetSelectedItems(hierarchy, &selectedEntities);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::HasPendingRequests()
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::ResetContext()
{
    // First deactivate all the entities, before calling the base class ResetContext which will
    // delete them all.
    AzFramework::EntityList entities;
    m_entityOwnershipService->GetAllEntities(entities);
    for (AZ::Entity* entity : entities)
    {
        if (entity->GetState() == AZ::Entity::State::Active)
        {
            entity->Deactivate();
        }
    }

    // Now reset the context which will destroy all the entities
    EntityContext::ResetContext();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnAssetReady([[maybe_unused]] AZ::Data::Asset<AZ::Data::AssetData> asset)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
{
    bool isActive = false;
    if (m_editorWindow->GetEntityContext() && m_editorWindow->GetEntityContext()->GetContextId() == GetContextId())
    {
        isActive = true;
    }

    HierarchyWidget* hierarchy = nullptr;
    EntityHelpers::EntityIdList selectedEntities;
    if (isActive)
    {
        hierarchy = m_editorWindow->GetHierarchy();
        const QTreeWidgetItemRawPtrQList& selection = hierarchy->selectedItems();
        selectedEntities = SelectionHelpers::GetSelectedElementIds(hierarchy, selection, false);

        // This ensures there's no "current item".
        hierarchy->SetUniqueSelectionHighlight((QTreeWidgetItem*)nullptr);

        // IMPORTANT: This is necessary to indirectly trigger detach()
        // in the PropertiesWidget.
        hierarchy->SetUserSelection(nullptr);
    }

    m_entityOwnershipService->OnAssetReloaded(asset);

    UiCanvasBus::Event(m_editorWindow->GetCanvasForEntityContext(GetContextId()), &UiCanvasBus::Events::ReinitializeElements);

    if (isActive)
    {
        // Ensure selection set is preserved after loading entities.
        selectedEntities.erase(
            AZStd::remove_if(
                selectedEntities.begin(), selectedEntities.end(),
                [](AZ::EntityId entityId)
                {
                    AZ::Entity* entity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
                    return !entity;
                }),
            selectedEntities.end());

        // Refresh the Hierarchy pane
        Shine::EntityArray childElements;
        UiCanvasBus::EventResult(childElements, m_editorWindow->GetCanvas(), &UiCanvasBus::Events::GetChildElements);
        hierarchy->RecreateItems(childElements);

        HierarchyHelpers::SetSelectedItems(hierarchy, &selectedEntities);
    }

    m_editorWindow->UpdateChangedStatusOnAssetChange(GetContextId(), asset);
}

//////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::OnContextEntitiesAdded(const AzFramework::EntityList& entities)
{
    EntityContext::OnContextEntitiesAdded(entities);
    InitializeEntities(entities);
}

//////////////////////////////////////////////////////////////////////////
bool UiEditorEntityContext::ValidateEntitiesAreValidForContext(const AzFramework::EntityList& entities)
{
    // All entities in this context should have the UiElementComponent on them.
    for (AZ::Entity* entity : entities)
    {
        if (!entity->FindComponent(Shine::UiElementComponentUuid))
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::SetupUiEntity(AZ::Entity* entity)
{
    InitializeEntities({ entity });
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UiEditorEntityContext::InitializeEntities(const AzFramework::EntityList& entities)
{
    for (AZ::Entity* entity : entities)
    {
        if (entity->GetState() == AZ::Entity::State::Constructed)
        {
            entity->Init();
        }
    }

    // Add required editor components to entities
    for (AZ::Entity* entity : entities)
    {
        for (const auto& componentType : m_requiredEditorComponentTypes)
        {
            if (!entity->FindComponent(componentType))
            {
                entity->CreateComponent(componentType);
            }
        }
    }

    for (AZ::Entity* entity : entities)
    {
        if (entity->GetState() == AZ::Entity::State::Init)
        {
            entity->InvalidateDependencies();

            Internal::RemoveIncompatibleComponents(entity);
            entity->Activate();
        }
    }
}
